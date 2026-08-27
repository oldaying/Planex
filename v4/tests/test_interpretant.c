/* v4/tests/test_interpretant.c — essence #3: Peirce interpretant (NEW)
 *
 * Verifies: intended vs actual interpretant, predict via interpret_fn,
 * match predicate (true/false). When match fails, that's a Breakdown
 * candidate (also tested in test_breakdown.c).
 *
 * This abstraction is NEW in v4. In v3 it was a sub-API of Perception.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

/* A simple interpret_fn: ignores the representamen, returns a fixed
 * string as the predicted actual interpretant (stored in user data). */
static void* fixed_interpret_fn(void* representamen, px_actor* actor, void* user) {
    (void)representamen; (void)actor;
    return user;
}

/* A no-op perceive fn that returns a static string as the representamen. */
static void* noop_perceive(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    static char buf[] = "saved";
    return buf;
}

int main(void) {
    /* build a perception + actor for binding */
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { e };

    px_perception* p = px_perception_new("save_status", noop_perceive,
                                          srcs, 1, NULL);
    ASSERT(p != NULL, "perception for representamen source");

    px_actor* alice = px_actor_new("alice", NULL);
    ASSERT(alice != NULL, "actor alice");

    /* create interpretant bound to perception + actor */
    px_interpretant* it = px_interpretant_new(p, alice);
    ASSERT(it != NULL, "interpretant_new");

    /* system sets the intended interpretant */
    px_interpretant_set_intended(it, "the save succeeded");
    ASSERT(strcmp(px_interpretant_intended(it), "the save succeeded") == 0,
           "intended retrievable");

    /* without an interpret_fn, predict returns NULL */
    ASSERT(px_interpretant_predict(it, "irrelevant") == NULL,
           "no interpret_fn => NULL prediction");

    /* register an interpret_fn that returns a fixed string */
    px_interpretant_set_interpret_fn(it, fixed_interpret_fn, "the save succeeded");
    void* predicted = px_interpretant_predict(it, "saved");
    ASSERT(predicted != NULL, "interpret_fn returned non-NULL");
    ASSERT(strcmp((char*)predicted, "the save succeeded") == 0,
           "predicted matches user string");

    /* match predicate: actual (predicted) matches intended? */
    ASSERT(px_interpretant_matches_intended(it, predicted),
           "actual matches intended -> no breakdown");

    /* now change the actual (simulating: actor misread) */
    px_interpretant_set_interpret_fn(it, fixed_interpret_fn, "the save failed");
    void* wrong_predicted = px_interpretant_predict(it, "saved");
    ASSERT(!px_interpretant_matches_intended(it, wrong_predicted),
           "mismatch -> Breakdown candidate");

    /* NULL actual -> match returns false */
    ASSERT(!px_interpretant_matches_intended(it, NULL),
           "NULL actual => no match");

    /* NULL intended -> match returns false */
    px_interpretant_set_intended(it, NULL);
    ASSERT(!px_interpretant_matches_intended(it, "anything"),
           "NULL intended => no match");

    px_interpretant_free(it);
    px_actor_free(alice);
    px_perception_free(p);
    px_estimate_free(e);
    printf("test_interpretant: ALL PASS\n");
    return 0;
}
