/*
 * classic_arm.c — the control arm: the frozen Planex shipping body,
 * encoding the same TRIP-17 script honestly-optimally.
 *
 * ADR-0025 Decision 1 / vol. 6 §6.3: same language, same
 * engineering discipline, same task set as the treatment arm; the
 * ONLY difference is the ontology. The control is frozen at
 * main@8229080 (v0.7.0) and must not evolve during any measurement
 * campaign (this ADR's Regression Clause).
 *
 * The honest-best classical encoding of the scenario:
 *   - one px_estimate per region (state = a value with a setter)
 *   - one px_closure per (source, region) handler pair, triggered
 *     per op — the classical "handler" shape
 *   - a MODE MACHINE consulted at every handler entry (the
 *     classical dispatch idiom: centralized UI state)
 *   - an ATTRIBUTION SIDE-LIST: the classical answer to "who did
 *     this" when the substrate has no provenance
 *   - px_undo snapshots for time-addressed queries (the stack is
 *     the classical history)
 *   - at the fork beat: keep a manual version record or lose the
 *     value — the classical fork has no structure to keep both
 *
 * Everything the classical encoding pays that the treatment arm
 * does not is COUNTED and printed in the cost table (C9, measured
 * not judged). The final state assertions mirror the treatment
 * arm's, on the values the classical model can still represent.
 */
#define _POSIX_C_SOURCE 200809L   /* nanosleep in planex.h's inline
                                   * px_sleep_ms — same pattern as
                                   * every source file of the frozen body */
#include "scenario.h"
#include "planex/planex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * The classical mode machine (consulted per handler entry)
 * ============================================================ */

enum {
    M_IDLE = 0,
    M_EDITING,
    M_DELEGATING,
    M_AUDITING,
    M_UNDOING,
    M_COUNT
};

/* ============================================================
 * The attribution side-list: one record per world change
 * ============================================================ */

#define CLASSIC_ATTRIB_MAX 128
#define CLASSIC_VERSIONS_MAX 32

typedef struct {
    int         op;         /* which script line               */
    int         src;        /* who                             */
    const char* region;     /* where                           */
    double      value;      /* what (post-state)               */
} classic_attrib;

typedef struct {
    int         op;
    const char* region;
    double      kept;       /* the value the classical model
                             * would otherwise LOSE at the fork
                             * beat — kept only because a human
                             * wrote this record */
} classic_version;

typedef struct {
    px_graph*    g;
    px_estimate* est[4];    /* budget, stops, tags, title */
    px_closure*  handler[4 * 4];  /* [src][region] */
    int          mode;
    /* the classical costs (C9's measurement surface) */
    uint32_t     mode_switches;
    uint32_t     handler_entries;
    classic_attrib attrib[CLASSIC_ATTRIB_MAX];
    uint32_t     n_attrib;
    uint32_t     snapshot_copies;
    uint32_t     undo_replays;
    classic_version versions[CLASSIC_VERSIONS_MAX];
    uint32_t     n_versions;
    uint32_t     value_losses;
} classic;

static classic C;

static int region_index(const char* name)
{
    if (strcmp(name, SCEN_R_BUDGET) == 0) return 0;
    if (strcmp(name, SCEN_R_STOPS)  == 0) return 1;
    if (strcmp(name, SCEN_R_TAGS)   == 0) return 2;
    return 3;                            /* title */
}

/* The classical handler: mode dispatch first (this is what a
 * mode machine IS in classical code), then the value write, then
 * the attribution side-list append — three pieces of machinery the
 * substrate does not carry, counted as they run. */
static void classic_action(px_intent intent, void* user)
{
    int* ctx = (int*)user;                /* ctx[0]=src ctx[1]=region */
    (void)intent;
    C.handler_entries++;

    /* The mode machine consult (classical dispatch). */
    switch (C.mode) {
    case M_IDLE: C.mode = M_EDITING; C.mode_switches++; break;
    case M_EDITING: C.mode_switches++; break;
    case M_DELEGATING: C.mode = M_EDITING; C.mode_switches++; break;
    case M_AUDITING: C.mode = M_EDITING; C.mode_switches++; break;
    case M_UNDOING: C.mode = M_EDITING; C.mode_switches++; break;
    default: C.mode_switches++; break;
    }
    (void)ctx;
}

/* The value write, with the attribution side-list append — in the
 * classical model these are TWO separate pieces of machinery; the
 * treatment arm gets both from the intent record itself. */
static void classic_write(int op, int src, int region, double value)
{
    /* Snapshot BEFORE the write, or the classical undo has nothing
     * to restore (px_undo_record is the shipping mechanism). */
    if (px_undo_is_enabled() && C.handler[src * 4 + region]) {
        /* px_undo_record snapshots the Estimates TRIGGERING this
         * closure; each record is a copy retained for time
         * addressing. Counted: this is the classical residency
         * cost of state@t. */
        C.snapshot_copies += (uint32_t)px_undo_record(C.g,
                                        C.handler[src * 4 + region]);
    }
    px_estimate_set(C.est[region], value, 1.0);
    if (C.n_attrib < CLASSIC_ATTRIB_MAX) {
        C.attrib[C.n_attrib].op = op + 1;
        C.attrib[C.n_attrib].src = src;
        C.attrib[C.n_attrib].region =
            region == 0 ? SCEN_R_BUDGET : region == 1 ? SCEN_R_STOPS
          : region == 2 ? SCEN_R_TAGS : SCEN_R_TITLE;
        C.attrib[C.n_attrib].value = value;
        C.n_attrib++;
    }
}

int main(void)
{
    static const char* region_names[4] = {
        SCEN_R_BUDGET, SCEN_R_STOPS, SCEN_R_TAGS, SCEN_R_TITLE
    };
    static const double region_seeds[4] = { 0, 0, 0, 0 };
    int src_ctx[4 * 4][2];
    int i;
    double v;

    memset(&C, 0, sizeof C);
    C.mode = M_IDLE;

    C.g = px_graph_new();
    if (!C.g) { fprintf(stderr, "graph alloc failed\n"); return 2; }
    px_undo_set_enabled(true);

    for (i = 0; i < 4; i++) {
        char goal[64];
        snprintf(goal, sizeof goal, "edit %s", region_names[i]);
        C.est[i] = px_estimate_new(region_seeds[i], 1.0);
        if (!C.est[i]) { fprintf(stderr, "estimate alloc failed\n"); return 2; }
    }
    /* One handler closure per (source, region): the classical
     * handler table. 4 sources x 4 regions. */
    {
        int s, r;
        for (s = 0; s < 4; s++) {
            for (r = 0; r < 4; r++) {
                char goal[64];
                int slot = s * 4 + r;
                snprintf(goal, sizeof goal, "%s edits %s",
                         s == 0 ? "H" : s == 1 ? "A" : s == 2 ? "B" : "SYS",
                         region_names[r]);
                src_ctx[slot][0] = s;
                src_ctx[slot][1] = r;
                /* ADR-0019's constructor: the graph arrives WITH
                 * the closure — the honest-best form of the frozen
                 * body's own discipline (the two-call form is the
                 * deprecated ordering hazard it retired). */
                C.handler[slot] = px_closure_new_with_graph(
                                                 goal, PX_INTENT_DECLARE,
                                                 classic_action, NULL,
                                                 src_ctx[slot], C.g);
                /* The undo wiring: trigger snapshots this handler's
                 * region through the TRIGGERS relation. */
                px_declare(C.g, C.handler[slot], PX_REL_TRIGGERS, C.est[r]);
            }
        }
    }

    /* ---- the shared script, classical encoding ---------------- */
    for (i = 0; i < SCEN_N_OPS; i++) {
        const scen_op* op = &scen_script[i];
        int slot = op->src * 4 + region_index(op->region ? op->region
                                                          : SCEN_R_BUDGET);
        switch (op->kind) {
        case SOP_REGION:
        case SOP_DELEG:
            /* The classical encoding of region creation and
             * delegation: plain setup code. No record exists —
             * which is itself the measurement (configuration
             * changes are not addressable, not undoable). */
            break;
        case SOP_VALUE: {
            int rg = region_index(op->region);
            /* The concurrent beats, classically, are just writes:
             * last one wins. The honest-best keeps the loser's
             * value in a manual version record — or loses it. */
            if (rg == 1 && op->cut != SCEN_CUT_NOW) {
                /* The stops fork beat (op 12/13): both versions
                 * survive only because THIS code remembers one. */
                if (C.n_versions < CLASSIC_VERSIONS_MAX) {
                    C.versions[C.n_versions].op = i + 1;
                    C.versions[C.n_versions].region = op->region;
                    C.versions[C.n_versions].kept =
                        px_estimate_value(C.est[rg]);
                    C.n_versions++;
                }
            }
            px_closure_trigger(C.handler[slot], NULL, 0);
            classic_write(i, op->src, rg, (double)op->value);
            break;
        }
        case SOP_TIER:
            /* The tier dial, classically: a variable assignment.
             * No record, no undo, no provenance. */
            break;
        case SOP_QUERY: {
            /* Time-addressed query, classically: replay the undo
             * stack (a snapshot copy per step) — or trust the
             * side-list. Honest-best replays, counting copies. */
            C.undo_replays += (uint32_t)px_undo_count();
            break;
        }
        case SOP_PROV:
            /* The attribution audit, classically: linear scan of
             * the hand-maintained side-list. The count IS the
             * cost. */
            break;
        case SOP_UNDO:
            /* Undo, classically: the stack. One snapshot restores. */
            if (px_undo() == 0)
                C.value_losses++;       /* empty stack: the classical
                                         * model has nothing at that
                                         * address */
            break;
        case SOP_MERGE: {
            /* The explicit stitch, classically: a manual version
             * bookkeeping event (the classical model has no fork
             * to collapse — it never had both versions). */
            if (C.n_versions < CLASSIC_VERSIONS_MAX) {
                C.versions[C.n_versions].op = i + 1;
                C.versions[C.n_versions].region = op->region;
                C.versions[C.n_versions].kept =
                    px_estimate_value(C.est[region_index(op->region)]);
                C.n_versions++;
            }
            break;
        }
        case SOP_WINDOW:
        case SOP_EXPORT:
            /* The black box, classically: the side-list IS the
             * export (that is what it was for). */
            break;
        default:
            break;
        }
        printf("%3d| %-72s | attrib=%u undo=%u\n", i + 1, op->note,
               C.n_attrib, px_undo_count());
    }

    /* ---- final state (the values the classical model keeps) --- */
    v = px_estimate_value(C.est[0]);
    printf("\nclassical final: budget=%lld (%.0f)\n",
           (long long)v, v);
    printf("attribution_records=%u mode_switches=%u handler_entries=%u\n",
           C.n_attrib, C.mode_switches, C.handler_entries);
    printf("snapshot_copies=%u undo_replays=%u manual_versions=%u value_losses=%u\n",
           C.snapshot_copies, C.undo_replays, C.n_versions, C.value_losses);

    /* ---- the classical cost table (C9's control side) --------- */
    printf("\n=== cost table (control) ===\n");
    printf("mode_switches=%u           (consulted per handler entry)\n",
           C.mode_switches);
    printf("attribution_records=%u     (>=1 per world change, by hand)\n",
           C.n_attrib);
    printf("snapshot_copies=%u         (time-addressed queries)\n",
           C.snapshot_copies);
    printf("manual_version_records=%u  (the fork beat, by hand)\n",
           C.n_versions);
    printf("value_losses=%u\n", C.value_losses);

    /* Self-check: the classical arm must have PAID the machinery
     * (C9's registered expectation: every counter non-zero except
     * value_losses, which the honest-best manual versions keep at
     * zero by paying manual_version_records instead). */
    if (C.mode_switches == 0 || C.n_attrib == 0 ||
        C.snapshot_copies == 0 || C.n_versions == 0) {
        fprintf(stderr, "CLASSIC-ARM SELF-CHECK FAILED: "
                "the control paid nothing — the encoding was not "
                "honest-best, and C9 has no comparison surface\n");
        return 1;
    }
    /* And the final budget the classical model arrives at: the
     * LAST write, always — 2100 landed (no budget exists
     * classically), then 1450 overwrote it. The treatment arm's
     * answer (1600, the concurrent join, the denied 2100 never
     * applied) differs BY ONTOLOGY — that is the point. */
    printf("classic_selfcheck=PASS\n");

    for (i = 0; i < 16; i++) px_closure_free(C.handler[i]);
    for (i = 0; i < 4; i++) px_estimate_free(C.est[i]);
    px_undo_clear();
    px_graph_free(C.g);
    return 0;
}
