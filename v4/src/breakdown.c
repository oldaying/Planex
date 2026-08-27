/* v4/src/breakdown.c — essence #8: Zuhandenheit / Vorhandenheit
 *
 * Per Heidegger (Zuhandenheit/Vorhandenheit), Winograd/Flores
 * (breakdown-recovery), Dourish (embodiment), Suchman (situatedness):
 * a UI that cannot break down is not a UI. Breakdown is the moment
 * the boundary becomes visible to the actor.
 *
 * Records *semantic* breakdown — the actor's interpretant no longer
 * matches the system's representamen — distinguished from operational
 * loop stall (which px_loop audit captures via perception_invoked=false).
 *
 * Per actor: A's breakdown is not B's. Has a recovery path.
 *
 * In v3 this was a prototype 6th abstraction; in v4 it is canonical.
 *
 * Storage: per-actor linked list of breakdowns. We index by actor
 * via a small global array (verification-scale only).
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_breakdown {
    px_actor*          actor;     /* weak ref */
    px_breakdown_kind  kind;
    char*              reason;
    void*              related;   /* weak ref, opaque */
    bool               recovered;
    char*              recovery_how;
    struct px_breakdown* next;    /* linked list per actor */
};

/* Per-actor breakdown list head. */
#define MAX_ACTORS 32
static struct {
    px_actor*       actor;
    px_breakdown*   head;
    int             count;
} g_actor_breakdowns[MAX_ACTORS];
static int g_actor_breakdowns_count = 0;

static int find_actor_slot(px_actor* actor) {
    if (!actor) return -1;
    for (int i = 0; i < g_actor_breakdowns_count; i++) {
        if (g_actor_breakdowns[i].actor == actor) return i;
    }
    if (g_actor_breakdowns_count < MAX_ACTORS) {
        int idx = g_actor_breakdowns_count++;
        g_actor_breakdowns[idx].actor = actor;
        g_actor_breakdowns[idx].head = NULL;
        g_actor_breakdowns[idx].count = 0;
        return idx;
    }
    return -1;
}

px_breakdown* px_breakdown_record(px_actor* actor,
                                    px_breakdown_kind kind,
                                    const char* reason,
                                    void* related) {
    if (!actor) return NULL;
    if (kind <= PX_BD_NONE || kind >= PX_BD_COUNT) return NULL;

    int slot = find_actor_slot(actor);
    if (slot < 0) return NULL;

    px_breakdown* b = (px_breakdown*)calloc(1, sizeof(px_breakdown));
    if (!b) return NULL;
    b->actor = actor;
    b->kind = kind;
    b->related = related;
    b->recovered = false;
    b->recovery_how = NULL;
    b->next = NULL;
    if (reason) {
        b->reason = (char*)malloc(strlen(reason) + 1);
        if (b->reason) strcpy(b->reason, reason);
    }

    /* prepend to head */
    b->next = g_actor_breakdowns[slot].head;
    g_actor_breakdowns[slot].head = b;
    g_actor_breakdowns[slot].count++;
    return b;
}

void px_breakdown_recover(px_breakdown* b, const char* how) {
    if (!b) return;
    b->recovered = true;
    free(b->recovery_how);
    if (how) {
        b->recovery_how = (char*)malloc(strlen(how) + 1);
        if (b->recovery_how) strcpy(b->recovery_how, how);
    } else {
        b->recovery_how = NULL;
    }
}

int px_breakdown_count(px_actor* actor) {
    int slot = find_actor_slot(actor);
    if (slot < 0) return 0;
    return g_actor_breakdowns[slot].count;
}

px_breakdown* px_breakdown_get(px_actor* actor, int idx) {
    int slot = find_actor_slot(actor);
    if (slot < 0) return NULL;
    int i = 0;
    px_breakdown* b = g_actor_breakdowns[slot].head;
    while (b) {
        if (i == idx) return b;
        i++;
        b = b->next;
    }
    return NULL;
}

const char* px_breakdown_reason(const px_breakdown* b) {
    return b ? b->reason : NULL;
}

px_breakdown_kind px_breakdown_kind_get(const px_breakdown* b) {
    return b ? b->kind : PX_BD_NONE;
}

const char* px_breakdown_kind_str(px_breakdown_kind k) {
    switch (k) {
        case PX_BD_NONE:                   return "NONE";
        case PX_BD_INTERPRETANT_MISMATCH:  return "INTERPRETANT_MISMATCH";
        case PX_BD_AFFORDANCE_LOST:         return "AFFORDANCE_LOST";
        case PX_BD_LOOP_STALL:              return "LOOP_STALL";
        case PX_BD_SITUATION_SHIFT:          return "SITUATION_SHIFT";
        default:                            return "(unknown)";
    }
}

bool px_breakdown_is_recovered(const px_breakdown* b) {
    return b ? b->recovered : false;
}

void px_breakdown_to_relation(px_breakdown* b, px_graph* g, void* node) {
    if (!b || !g || !node) return;
    /* declares PX_REL_PRESENTS_FOR(node, actor) — the node is now
     * present-to-hand for this actor (it has broken down). */
    px_declare(g, node, PX_REL_PRESENTS_FOR, b->actor, b->actor);
}

void px_breakdown_reset(void) {
    /* Free all breakdown records and reset the global actor table.
     * Test-only — see header comment. */
    for (int i = 0; i < g_actor_breakdowns_count; i++) {
        px_breakdown* b = g_actor_breakdowns[i].head;
        while (b) {
            px_breakdown* next = b->next;
            free(b->reason);
            free(b->recovery_how);
            free(b);
            b = next;
        }
        g_actor_breakdowns[i].actor = NULL;
        g_actor_breakdowns[i].head = NULL;
        g_actor_breakdowns[i].count = 0;
    }
    g_actor_breakdowns_count = 0;
}
