/*
 * undo.c — Undo via Relation graph (v0.3)
 *
 * Per ADR-0002: Relation's necessity is proven by undo-via-graph.
 *
 * When a Closure triggers, we snapshot only the Estimates reachable
 * from that Closure via PX_REL_TRIGGERS edges. This is the key
 * differentiator from Solid.js:
 *
 *   - Solid tracks dependencies PER-EFFECT (each effect knows its
 *     own sources). There is no global graph query "which effects
 *     depend on this signal?".
 *   - Planex's Relation is a globally queryable graph. We can ask
 *     "which Estimates does this Closure trigger?" in O(edges).
 *
 * This means Planex can do MINIMAL undo — only snapshot the Estimates
 * that will actually change. Solid must snapshot everything (or
 * maintain a separate dependency index).
 *
 * Implementation:
 *   - Undo stack is a linked list of snapshots
 *   - Each snapshot is an array of (estimate*, value, confidence)
 *   - px_undo_record queries the graph for TRIGGERS edges from
 *     the Closure, snapshots those Estimates
 *   - px_undo restores the last snapshot
 *
 * SAFETY (2026-10 audit fix): Relation edges carry raw void* with no
 * type information, so a TRIGGERS target is not guaranteed to be an
 * Estimate. Every target is validated against the live-estimate
 * registry (px__estimate_is_live, estimate.c) — pointer comparison
 * only, the candidate is never dereferenced — before being cast and
 * snapshotted. Non-Estimate and freed targets are skipped; a
 * snapshot that filtered down to zero entries is not pushed.
 *
 * The undo stack is GLOBAL (not per-graph). This matches the
 * Perception registry pattern — Planex is single-app.
 *
 * Inspired by:
 *   - Redux undo (but Planex uses Relation graph, not action log)
 *   - Sketchpad constraint rollback
 *   - Emacs undo (linear, simple)
 *
 * THREAD SAFETY: This module uses global state (g_undo_stack,
 * g_undo_count, g_undo_enabled). Not thread-safe. Planex is
 * designed as single-threaded single-app. If multi-threading is
 * needed, add a mutex around undo operations.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

/* Internal cross-TU seam (house pattern: px__perception_clear_cache in
 * feedback.c). Owned by estimate.c; answers by pointer comparison and
 * never dereferences the candidate. */
extern bool px__estimate_is_live(const void* p);

/* ============================================================
 * Snapshot structure
 * ============================================================ */

typedef struct {
    px_estimate* est;
    double       value;
    double       confidence;
} px_undo_entry;

typedef struct px_undo_snapshot {
    px_undo_entry*            entries;
    int                        count;
    struct px_undo_snapshot*  next;  /* stack: next older snapshot */
} px_undo_snapshot;

/* ============================================================
 * Global undo state
 * ============================================================ */

static px_undo_snapshot* g_undo_stack = NULL;
static int               g_undo_count = 0;
static bool              g_undo_enabled = false;  /* default: off */

/* ============================================================
 * API
 * ============================================================ */

void px_undo_set_enabled(bool enabled) {
    g_undo_enabled = enabled;
}

bool px_undo_is_enabled(void) {
    return g_undo_enabled;
}

int px_undo_count(void) {
    return g_undo_count;
}

void px_undo_clear(void) {
    while (g_undo_stack) {
        px_undo_snapshot* s = g_undo_stack;
        g_undo_stack = s->next;
        free(s->entries);
        free(s);
    }
    g_undo_count = 0;
}

/* Snapshot Estimates affected by Closure c (via TRIGGERS relation).
 * Queries graph for "c TRIGGERS ?" and snapshots each Estimate found.
 *
 * Note: TRIGGERS edges point from Closure to Estimate (or to anything
 * the Closure affects). We snapshot only px_estimate* targets — other
 * targets (windows, app structs) are skipped.
 *
 * Returns count snapshotted, or -1 on error. */
int px_undo_record(px_graph* g, px_closure* c) {
    if (!g || !c) return -1;

    /* Query: what does c trigger? */
    px_node_list triggered = px_query(g, c, PX_REL_TRIGGERS);
    if (triggered.count == 0) {
        px_node_list_free(&triggered);
        return 0;  /* nothing to snapshot */
    }

    /* Allocate snapshot */
    px_undo_snapshot* snap = (px_undo_snapshot*)calloc(1, sizeof(px_undo_snapshot));
    if (!snap) {
        px_node_list_free(&triggered);
        return -1;
    }

    snap->entries = (px_undo_entry*)calloc(triggered.count, sizeof(px_undo_entry));
    if (!snap->entries) {
        free(snap);
        px_node_list_free(&triggered);
        return -1;
    }

    /* For each triggered node, snapshot it ONLY if it is a live
     * Estimate (registry-checked, undo-safety fix). Type-confused
     * edges (any non-Estimate target) and freed Estimates — the
     * documented dangling-endpoint hazard in relation.c — are
     * skipped rather than blind-cast into UB. NULL entries likewise.
     *
     * Residual limit: an estimate freed and its address immediately
     * reused by a new estimate IS live — snapshotting the new one is
     * memory-safe, see the registry comment in estimate.c. */
    int n = 0;
    for (int i = 0; i < triggered.count; i++) {
        px_estimate* est = (px_estimate*)triggered.items[i];
        if (!est || !px__estimate_is_live(est)) continue;
        snap->entries[n].est        = est;
        snap->entries[n].value      = px_estimate_value(est);
        snap->entries[n].confidence = px_estimate_confidence(est);
        n++;
    }

    if (n == 0) {
        /* Everything filtered out: nothing to undo. Do not push an
         * empty snapshot — px_undo_count() would advertise an undo
         * step that restores nothing. */
        free(snap->entries);
        free(snap);
        px_node_list_free(&triggered);
        return 0;
    }
    snap->count = n;

    /* Push onto stack */
    snap->next = g_undo_stack;
    g_undo_stack = snap;
    g_undo_count++;

    px_node_list_free(&triggered);
    return n;
}

int px_undo(void) {
    if (!g_undo_stack) return 0;

    px_undo_snapshot* snap = g_undo_stack;
    g_undo_stack = snap->next;
    g_undo_count--;

    /* Restore each entry */
    int restored = 0;
    for (int i = 0; i < snap->count; i++) {
        px_estimate* est = snap->entries[i].est;
        if (est) {
            px_estimate_set(est,
                            snap->entries[i].value,
                            snap->entries[i].confidence);
            restored++;
        }
    }

    free(snap->entries);
    free(snap);
    return restored;
}
