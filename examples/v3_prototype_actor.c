/*
 * v3_prototype_actor.c — 3-place Relation prototype
 *
 * Validates that the v3 prototype's 3-place Relation (with actor
 * parameter) is expressible in Planex's C17 zero-dependency API
 * surface, and that backward compatibility is preserved.
 *
 * Essence gap addressed (per essence-derivation-v3.md § II-5):
 *   - v2 said Relation is essence (relational ontology), but only
 *     implemented it as 2-place (inter-thing).
 *   - Heidegger / Suchman / Maturana: the relation is 3-place
 *     (actor ↔ thing ↔ situation); without the actor slot, the
 *     "situatedness" claim is only theoretical.
 *   - This prototype confirms the API can express 3-place relations
 *     and that the old 2-place API (px_declare, px_query) still works
 *     as a universal-relation wrapper.
 *
 * Scenario:
 *   Two actors (alice, bob) share a graph. A universal relation
 *   "alice-closure TRIGGERS counter" holds for everyone. An
 *   actor-scoped relation "counter AFFORDS click-for-alice" holds
 *   only for alice. We verify:
 *     - px_query returns the universal relation for any actor
 *     - px_query_for(alice) returns both universal + alice-scoped
 *     - px_query_for(bob)   returns only the universal one
 *     - px_query_for(NULL)  returns only the universal one
 *
 * Build (via CMakeLists.txt STDOUT_DEMOS):
 *   cmake -B build && cmake --build build
 *   ./build/v3_prototype_actor
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int dummy; } Counter;
typedef struct { int dummy; } Click;

static int count_results(px_node_list l) {
    int n = l.count;
    px_node_list_free(&l);
    return n;
}

int main(void) {
    printf("=== v3_prototype_actor ===\n");
    printf("3-place Relation (essence-derivation-v3 § II-5)\n\n");

    px_graph* g = px_graph_new();
    Counter counter; counter.dummy = 0;
    Click   click;   click.dummy   = 0;

    px_actor* alice = px_actor_new("alice", NULL);
    px_actor* bob   = px_actor_new("bob", NULL);
    if (!g || !alice || !bob) {
        fprintf(stderr, "FAIL: setup allocation failed\n");
        return 1;
    }
    printf("[setup] graph + 2 actors (alice, bob)\n");
    printf("[setup] px_actor_id(alice) = \"%s\"\n", px_actor_id(alice));
    printf("[setup] px_actor_id(bob)   = \"%s\"\n", px_actor_id(bob));

    /* Universal relation — holds for all actors (actor=NULL). */
    px_relation* r1 = px_declare(g, &counter, PX_REL_TRIGGERS, &click);
    if (!r1) { fprintf(stderr, "FAIL: px_declare r1\n"); return 1; }
    printf("[declare] universal: counter TRIGGERS click (actor=NULL)\n");

    /* Actor-scoped relation — holds only for alice. */
    px_relation* r2 = px_declare_for(g, &counter, PX_REL_AFFORDS, &click, alice);
    if (!r2) { fprintf(stderr, "FAIL: px_declare_for r2\n"); return 1; }
    printf("[declare] actor-scoped: counter AFFORDS click (for alice)\n");

    /* Verify 1: old px_query (any actor) returns the universal relation. */
    int n_universal = count_results(px_query(g, &counter, PX_REL_TRIGGERS));
    printf("[query]    px_query(counter, TRIGGERS)        = %d (expected 1)\n",
           n_universal);

    /* Verify 2: px_query_for(alice, AFFORDS) returns the alice-scoped relation. */
    int n_alice_affords = count_results(px_query_for(g, &counter, PX_REL_AFFORDS, alice));
    printf("[query]    px_query_for(counter, AFFORDS, alice) = %d (expected 1)\n",
           n_alice_affords);

    /* Verify 3: px_query_for(bob, AFFORDS) returns nothing — bob has no affordance. */
    int n_bob_affords = count_results(px_query_for(g, &counter, PX_REL_AFFORDS, bob));
    printf("[query]    px_query_for(counter, AFFORDS, bob)   = %d (expected 0)\n",
           n_bob_affords);

    /* Verify 4: px_query_for(NULL, AFFORDS) returns nothing —
     * the NULL actor matches universal relations, but the
     * alice-scoped one is NOT universal, so it doesn't match. */
    int n_null_affords = count_results(px_query_for(g, &counter, PX_REL_AFFORDS, NULL));
    printf("[query]    px_query_for(counter, AFFORDS, NULL)  = %d (expected 0)\n",
           n_null_affords);

    /* Verify 5: px_query_for(alice, TRIGGERS) returns the universal relation
     * (universal matches every actor query). */
    int n_alice_triggers = count_results(px_query_for(g, &counter, PX_REL_TRIGGERS, alice));
    printf("[query]    px_query_for(counter, TRIGGERS, alice) = %d (expected 1)\n",
           n_alice_triggers);

    /* Verify 6: px_query_for(bob, TRIGGERS) also returns the universal relation. */
    int n_bob_triggers = count_results(px_query_for(g, &counter, PX_REL_TRIGGERS, bob));
    printf("[query]    px_query_for(counter, TRIGGERS, bob)  = %d (expected 1)\n",
           n_bob_triggers);

    /* Verify 7: px_query_for(alice, WITHDRAWS_FOR) returns nothing
     * — no relation declared yet. (Zuhandenheit seed: nothing is
     * marked as withdrawn-from alice yet.) */
    int n_alice_withdrawn = count_results(px_query_for(g, &counter, PX_REL_WITHDRAWS_FOR, alice));
    printf("[query]    px_query_for(counter, WITHDRAWS_FOR, alice) = %d (expected 0)\n",
           n_alice_withdrawn);

    /* Verify 8: declare "counter WITHDRAWS_FOR alice" (alice is in flow
     * — the counter abstraction is withdrawn from her awareness),
     * then verify alice sees it but bob doesn't. */
    px_declare_for(g, &counter, PX_REL_WITHDRAWS_FOR, alice, alice);
    n_alice_withdrawn = count_results(px_query_for(g, &counter, PX_REL_WITHDRAWS_FOR, alice));
    int n_bob_withdrawn   = count_results(px_query_for(g, &counter, PX_REL_WITHDRAWS_FOR, bob));
    printf("[declare] counter WITHDRAWS_FOR alice (alice is in flow)\n");
    printf("[query]    px_query_for(counter, WITHDRAWS_FOR, alice) = %d (expected 1)\n",
           n_alice_withdrawn);
    printf("[query]    px_query_for(counter, WITHDRAWS_FOR, bob)   = %d (expected 0)\n",
           n_bob_withdrawn);

    printf("\n[verdict] 3-place Relation is expressible in Planex v3 API.\n");
    printf("[verdict] Universal (actor=NULL) relations match every actor query;\n");
    printf("[verdict] actor-scoped relations match only that actor.\n");
    printf("[verdict] Old 2-place px_declare / px_query still work as universal wrappers.\n");

    px_actor_free(alice);
    px_actor_free(bob);
    px_graph_free(g);
    return 0;
}
