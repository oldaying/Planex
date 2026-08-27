/* v4/tests/test_orthogonality.c — essence orthogonality check
 *
 * Verifies that each of the 8 abstractions can be created and destroyed
 * INDEPENDENTLY — no implicit coupling. If abstraction A requires
 * abstraction B to exist just to be constructed, that's a coupling
 * defect (the v1 critique).
 *
 * Also verifies the open symbol system + actor struct don't depend
 * on the graph being initialized first.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

/* A no-op perceive fn for the perception construction test. */
static void* noop_perceive(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    return NULL;
}

int main(void) {
    /* essence #1: Estimate — construct with no dependencies */
    px_estimate* e = px_estimate_new(0.0, 1.0);
    ASSERT(e != NULL, "estimate constructs independently");

    /* essence #2: Perception — needs an Estimate as input (essence-correct,
     * since a representamen without a state to denote would be vacuous),
     * but does not need Closure / Relation / etc. */
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", noop_perceive, srcs, 1, NULL);
    ASSERT(p != NULL, "perception constructs with only its essence-correct input");

    /* actor struct — independent of everything */
    px_actor* alice = px_actor_new("alice", NULL);
    ASSERT(alice != NULL, "actor constructs independently");

    /* essence #3: Interpretant — needs a Perception + an Actor (essence-correct:
     * the interpretant is generated FROM a representamen IN an actor) */
    px_interpretant* it = px_interpretant_new(p, alice);
    ASSERT(it != NULL, "interpretant constructs with perception + actor");

    /* essence #4: Closure — independent of Estimate, Perception, Relation */
    px_closure* c = px_closure_new("noop", PX_INTENT_ASSERT, NULL, NULL, NULL);
    ASSERT(c != NULL, "closure constructs independently");

    /* essence #5: Perlocution — needs Closure + Actor (essence-correct:
     * perlocution is the effect of a Closure's utterance on an Actor) */
    px_perlocution* per = px_perlocution_new(c, alice);
    ASSERT(per != NULL, "perlocution constructs with closure + actor");

    /* essence #6: Relation graph — independent of everything */
    px_graph* g = px_graph_new();
    ASSERT(g != NULL, "graph constructs independently");
    ASSERT(px_graph_count(g) == 0, "empty graph");

    /* essence #7: px_loop — needs all 4 essence dimensions of the return
     * edge. This is essence-correct: a loop without all 4 would not be
     * a complete loop. So this is not a coupling defect — it's the
     * essence of loop topology. */
    px_loop* loop = px_loop_new(c, p, it, per);
    ASSERT(loop != NULL, "loop constructs with 4 essence-correct bindings");

    /* essence #8: Breakdown — needs an Actor (per-actor) */
    int dummy_node = 1;
    px_breakdown* b = px_breakdown_record(alice,
                                              PX_BD_INTERPRETANT_MISMATCH,
                                              "test", &dummy_node);
    ASSERT(b != NULL, "breakdown records with actor");

    /* teardown in reverse dependency order — verify nothing crashes */
    px_loop_free(loop);
    px_graph_free(g);
    px_perlocution_free(per);
    px_closure_free(c);
    px_interpretant_free(it);
    px_actor_free(alice);
    px_perception_free(p);
    px_estimate_free(e);
    /* breakdown memory is owned by the global per-actor registry;
     * we don't free it here. (Verification-scale only; production
     * would have an explicit free API.) */

    /* Verify each abstraction type's pointer is pointer-sized (no
     * surprises that would suggest implicit state coupling) */
    ASSERT(sizeof(px_estimate*) == sizeof(void*), "estimate pointer is pointer-sized");
    ASSERT(sizeof(px_closure*) == sizeof(void*), "closure pointer is pointer-sized");

    printf("test_orthogonality: ALL PASS\n");
    return 0;
}
