/*
 * breakdown.c — Breakdown (v3 prototype — the "6th abstraction" claim was
 * never admitted: the canonical 6th is intent compilation per ADR-0017)
 *
 * Per Heidegger (1927, Zuhandenheit/Vorhandenheit), Winograd/Flores
 * (1986, Understanding Computers and Cognition), Dourish (2001,
 * Where the Action Is), Suchman (1987, Plans and Situated Actions):
 * a UI that cannot break down is not a UI. Breakdown is the moment
 * the boundary becomes visible to the actor — the tool that was
 * withdrawn (Zuhanden) in skilled use becomes present-at-hand
 * (Vorhanden) when the user no longer understands what the system
 * is telling them.
 *
 * This abstraction records *semantic* breakdown — the actor's
 * interpretant no longer matches the system's representamen — which
 * is distinct from operational loop stall (captured by px_loop audit
 * via perception_invoked=false). In semantic breakdown the
 * representamen is still arriving; the interpretant has broken.
 *
 * A Breakdown is:
 *   - *per actor*: A's breakdown is not B's.
 *   - *per situation*: the same actor in a different situation may
 *     not be in breakdown.
 *   - *recoverable*: the actor (or system on the actor's behalf)
 *     can restore the interpretant via explanation, undo, or
 *     adaptation.
 *
 * The bridge to Relation: px_breakdown_to_relation() declares
 * PX_REL_PRESENTS_FOR(node, actor) for the related node + actor,
 * marking the node as present-to-hand (it has broken down for this
 * actor). px_loop_mark_breakdown() marks the next loop iteration's
 * audit entry as containing a breakdown transition (+1 entered,
 * -1 recovered).
 *
 * Status: v3 prototype. See essence-derivation-v3.md Part V.7 and
 * ADR-0009 (Proposed). Inspired by:
 *   - Heidegger 1927 (Being and Time, Zuhandenheit/Vorhandenheit)
 *   - Winograd/Flores 1986 (breakdown-recovery as basis of UI)
 *   - Dourish 2001 (Where the Action Is, embodiment)
 *   - Suchman 1987 (Plans and Situated Actions)
 *
 * Storage: a global per-actor linked list. O(n) lookup per actor.
 * Adequate for the v3 prototype (apps typically have 1-10 actors);
 * a future production version may use a hash table per actor.
 *
 * Thread safety: NOT thread-safe.
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

struct px_breakdown {
    px_actor*           actor;        /* who broke down (NOT owned) */
    px_breakdown_kind   kind;
    char*               reason;        /* free text */
    void*               related;       /* node the breakdown concerns (NOT owned) */
    bool                recovered;
    char*               recovery_how;  /* how it was recovered (NULL if not) */
    struct px_breakdown* next;
};

/* Global linked list, shared across all actors. px_breakdown_count
 * and px_breakdown_get filter by actor. */
static struct px_breakdown* g_breakdowns = NULL;
static int                  g_breakdown_count = 0;

static const char* const k_breakdown_names[] = {
    "NONE",                 /* PX_BD_NONE                   */
    "INTERPRETANT_MISMATCH", /* PX_BD_INTERPRETANT_MISMATCH */
    "AFFORDANCE_LOST",       /* PX_BD_AFFORDANCE_LOST       */
    "LOOP_STALL",            /* PX_BD_LOOP_STALL            */
    "SITUATION_SHIFT",       /* PX_BD_SITUATION_SHIFT       */
    "COUNT",                /* PX_BD_COUNT                  */
};

/* ============================================================
 * Record / recover
 * ============================================================ */

px_breakdown* px_breakdown_record(px_actor* actor,
                                    px_breakdown_kind kind,
                                    const char* reason,
                                    void* related) {
    if (!actor) return NULL;
    if (kind <= PX_BD_NONE || kind >= PX_BD_COUNT) return NULL;

    struct px_breakdown* b =
        (struct px_breakdown*)calloc(1, sizeof(struct px_breakdown));
    if (!b) return NULL;

    b->actor    = actor;
    b->kind     = kind;
    b->related  = related;
    b->recovered = false;
    b->recovery_how = NULL;

    if (reason) {
        b->reason = px_strdup(reason);
        if (!b->reason) {
            free(b);
            return NULL;
        }
    }

    /* Prepend to global list. */
    b->next = g_breakdowns;
    g_breakdowns = b;
    g_breakdown_count++;

    return (px_breakdown*)b;
}

void px_breakdown_recover(px_breakdown* b_public, const char* how) {
    if (!b_public) return;
    struct px_breakdown* b = (struct px_breakdown*)b_public;
    b->recovered = true;
    if (b->recovery_how) {
        free(b->recovery_how);
        b->recovery_how = NULL;
    }
    if (how) {
        b->recovery_how = px_strdup(how);
    }
}

/* ============================================================
 * Query
 * ============================================================ */

int px_breakdown_count(px_actor* actor) {
    if (!actor) return 0;
    int n = 0;
    for (struct px_breakdown* b = g_breakdowns; b; b = b->next) {
        if (b->actor == actor) n++;
    }
    return n;
}

px_breakdown* px_breakdown_get(px_actor* actor, int idx) {
    if (!actor || idx < 0) return NULL;
    int i = 0;
    for (struct px_breakdown* b = g_breakdowns; b; b = b->next) {
        if (b->actor == actor) {
            if (i == idx) return (px_breakdown*)b;
            i++;
        }
    }
    return NULL;
}

const char* px_breakdown_reason(const px_breakdown* b_public) {
    if (!b_public) return NULL;
    const struct px_breakdown* b = (const struct px_breakdown*)b_public;
    return b->reason;
}

const char* px_breakdown_kind_str(px_breakdown_kind k) {
    int n = (int)(sizeof(k_breakdown_names) / sizeof(k_breakdown_names[0]));
    if ((int)k < 0 || (int)k >= n) return "?";
    return k_breakdown_names[k];
}

bool px_breakdown_is_recovered(const px_breakdown* b_public) {
    if (!b_public) return false;
    const struct px_breakdown* b = (const struct px_breakdown*)b_public;
    return b->recovered;
}

/* ============================================================
 * Bridge to Relation
 *
 * Declares PX_REL_PRESENTS_FOR(related, actor) in the graph —
 * the related node is now present-to-hand (Vorhanden) for this
 * actor (it has broken down). This makes the breakdown visible
 * in the Relation graph that other parts of the system can
 * query.
 *
 * Note: this is the only function in the breakdown API that
 * takes a graph — px_breakdown_record itself stores the
 * breakdown per-actor without needing the graph. The graph
 * bridge is opt-in: only callers who want the breakdown to be
 * visible to relation-querying code need to call it.
 * ============================================================ */

void px_breakdown_to_relation(px_breakdown* b_public, px_graph* g,
                                 void* node) {
    if (!b_public || !g || !node) return;
    struct px_breakdown* b = (struct px_breakdown*)b_public;
    if (!b->actor) return;
    /* Use 3-place px_declare_for with the actor parameter —
     * this relation holds only for this actor, not universally. */
    px_declare_for(g, node, PX_REL_PRESENTS_FOR, b->actor, b->actor);
}
