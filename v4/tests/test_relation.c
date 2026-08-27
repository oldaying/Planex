/* v4/tests/test_relation.c — essence #6: Relational ontology (3-place)
 *
 * Verifies: 3-place relations, actor-scoped queries, universal vs
 * situated relations, breakdown bridge (PX_REL_PRESENTS_FOR).
 *
 * v4 BREAK verified: px_declare requires actor parameter (no 2-place
 * wrapper macro). actor=NULL means universal.
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
    px_graph* g = px_graph_new();
    ASSERT(g != NULL, "graph_new");
    ASSERT(px_graph_count(g) == 0, "empty graph");

    /* three actors + a universal marker */
    px_actor* alice = px_actor_new("alice", NULL);
    px_actor* bob   = px_actor_new("bob", NULL);

    /* nodes are just opaque void* — use stack ints */
    int node_a = 1, node_b = 2, node_c = 3;

    /* universal relation: actor=NULL means it holds for all actors */
    px_relation* r1 = px_declare(g, &node_a, PX_REL_TRIGGERS, &node_b, NULL);
    ASSERT(r1 != NULL, "declare universal (actor=NULL)");
    ASSERT(px_graph_count(g) == 1, "count=1");

    /* situated relation: holds only for alice */
    px_relation* r2 = px_declare(g, &node_a, PX_REL_AFFORDS, &node_c, alice);
    ASSERT(r2 != NULL, "declare situated (actor=alice)");

    /* universal queries see both */
    ASSERT(px_has_relation(g, &node_a, PX_REL_TRIGGERS, &node_b, NULL),
           "has universal (q=NULL) -> universal present");
    ASSERT(px_has_relation(g, &node_a, PX_REL_TRIGGERS, &node_b, alice),
           "has universal (q=alice) -> matches (universal matches all)");
    ASSERT(px_has_relation(g, &node_a, PX_REL_TRIGGERS, &node_b, bob),
           "has universal (q=bob) -> matches (universal matches all)");

    /* situated queries only see their own actor's relations */
    ASSERT(px_has_relation(g, &node_a, PX_REL_AFFORDS, &node_c, alice),
           "has situated (q=alice) -> matches (alice's own)");
    ASSERT(!px_has_relation(g, &node_a, PX_REL_AFFORDS, &node_c, bob),
           "has situated (q=bob) -> no match (alice's only)");
    ASSERT(!px_has_relation(g, &node_a, PX_REL_AFFORDS, &node_c, NULL),
           "has situated (q=NULL) -> no match (situated not universal)");

    /* query: all nodes related to a via TRIGGERS, q=alice */
    px_node_list list = px_query(g, &node_a, PX_REL_TRIGGERS, alice);
    ASSERT(list.count == 1, "alice query TRIGGERS -> 1 (universal match)");
    ASSERT(list.items[0] == &node_b, "alice query TRIGGERS -> node_b");
    px_node_list_free(&list);

    /* query: all nodes related to a via AFFORDS, q=bob -> 0 (situated to alice) */
    list = px_query(g, &node_a, PX_REL_AFFORDS, bob);
    ASSERT(list.count == 0, "bob query AFFORDS -> 0 (situated to alice)");
    px_node_list_free(&list);

    /* query: all nodes related to a via AFFORDS, q=alice -> 1 */
    list = px_query(g, &node_a, PX_REL_AFFORDS, alice);
    ASSERT(list.count == 1, "alice query AFFORDS -> 1");
    ASSERT(list.items[0] == &node_c, "alice query AFFORDS -> node_c");
    px_node_list_free(&list);

    /* Zuhandenheit / breakdown relations */
    px_declare(g, &node_a, PX_REL_WITHDRAWS_FOR, &node_b, alice);
    px_declare(g, &node_a, PX_REL_PRESENTS_FOR, &node_c, bob);

    ASSERT(px_has_relation(g, &node_a, PX_REL_WITHDRAWS_FOR, &node_b, alice),
           "WITHDRAWS_FOR alice");
    ASSERT(!px_has_relation(g, &node_a, PX_REL_WITHDRAWS_FOR, &node_b, bob),
           "WITHDRAWS_FOR not bob");

    /* rel_kind_str sanity */
    ASSERT(strcmp(px_rel_kind_str(PX_REL_TRIGGERS), "TRIGGERS") == 0,
           "rel_kind_str TRIGGERS");
    ASSERT(strcmp(px_rel_kind_str(PX_REL_PRESENTS_FOR), "PRESENTS_FOR") == 0,
           "rel_kind_str PRESENTS_FOR");

    px_actor_free(alice);
    px_actor_free(bob);
    px_graph_free(g);
    printf("test_relation: ALL PASS\n");
    return 0;
}
