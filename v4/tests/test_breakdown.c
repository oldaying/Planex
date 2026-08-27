/* v4/tests/test_breakdown.c — essence #8: Zuhandenheit / Vorhandenheit
 *
 * Verifies: per-actor breakdown (A's is not B's), recovery path,
 * relation bridge (PX_REL_PRESENTS_FOR declared when breakdown occurs).
 *
 * In v3 this was a prototype 6th abstraction; in v4 it is canonical.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

int main(void) {
    px_actor* alice = px_actor_new("alice", NULL);
    px_actor* bob   = px_actor_new("bob", NULL);
    px_graph* g = px_graph_new();

    int node_save = 1;

    /* alice encounters an interpretant mismatch on the save button */
    px_breakdown* b1 = px_breakdown_record(alice,
                                              PX_BD_INTERPRETANT_MISMATCH,
                                              "alice thought 'Save' meant 'Save As'",
                                              &node_save);
    ASSERT(b1 != NULL, "record alice breakdown");
    ASSERT(px_breakdown_count(alice) == 1, "alice count = 1");
    ASSERT(px_breakdown_count(bob) == 0, "bob count = 0 (per-actor)");
    ASSERT(!px_breakdown_is_recovered(b1), "not recovered initially");
    ASSERT(px_breakdown_kind_get(b1) == PX_BD_INTERPRETANT_MISMATCH,
           "kind INTERPRETANT_MISMATCH");
    ASSERT(strcmp(px_breakdown_reason(b1),
                   "alice thought 'Save' meant 'Save As'") == 0,
           "reason retrievable");

    /* bob has a different breakdown on a different node */
    int node_form = 2;
    px_breakdown* b2 = px_breakdown_record(bob,
                                              PX_BD_AFFORDANCE_LOST,
                                              "bob can't see the submit button",
                                              &node_form);
    ASSERT(b2 != NULL, "record bob breakdown");
    ASSERT(px_breakdown_count(bob) == 1, "bob count = 1");
    ASSERT(px_breakdown_count(alice) == 1, "alice still count = 1");

    /* retrieve by index */
    ASSERT(px_breakdown_get(alice, 0) == b1, "alice[0] == b1");
    ASSERT(px_breakdown_get(bob, 0) == b2, "bob[0] == b2");
    ASSERT(px_breakdown_get(alice, 1) == NULL, "alice[1] == NULL");
    ASSERT(px_breakdown_get(alice, -1) == NULL, "alice[-1] == NULL");

    /* alice recovers via undo */
    px_breakdown_recover(b1, "alice pressed Ctrl+Z and re-read the dialog");
    ASSERT(px_breakdown_is_recovered(b1), "alice recovered");
    ASSERT(!px_breakdown_is_recovered(b2), "bob not recovered");

    /* bridge to relation: a breakdown declares PRESENTS_FOR for the
     * related node + actor */
    ASSERT(!px_has_relation(g, &node_form, PX_REL_PRESENTS_FOR, bob, bob),
           "before bridge: no PRESENTS_FOR bob");
    px_breakdown_to_relation(b2, g, &node_form);
    ASSERT(px_has_relation(g, &node_form, PX_REL_PRESENTS_FOR, bob, bob),
           "after bridge: PRESENTS_FOR bob declared");
    ASSERT(!px_has_relation(g, &node_form, PX_REL_PRESENTS_FOR, alice, alice),
           "PRESENTS_FOR is for bob, not alice");

    /* kind_str */
    ASSERT(strcmp(px_breakdown_kind_str(PX_BD_INTERPRETANT_MISMATCH),
                   "INTERPRETANT_MISMATCH") == 0,
           "kind_str INTERPRETANT_MISMATCH");
    ASSERT(strcmp(px_breakdown_kind_str(PX_BD_AFFORDANCE_LOST),
                   "AFFORDANCE_LOST") == 0,
           "kind_str AFFORDANCE_LOST");
    ASSERT(strcmp(px_breakdown_kind_str(PX_BD_LOOP_STALL),
                   "LOOP_STALL") == 0,
           "kind_str LOOP_STALL");
    ASSERT(strcmp(px_breakdown_kind_str(PX_BD_SITUATION_SHIFT),
                   "SITUATION_SHIFT") == 0,
           "kind_str SITUATION_SHIFT");

    px_graph_free(g);
    px_actor_free(alice);
    px_actor_free(bob);
    printf("test_breakdown: ALL PASS\n");
    return 0;
}
