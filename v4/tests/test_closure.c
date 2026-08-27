/* v4/tests/test_closure.c — essence #4: Illocution
 *
 * Verifies: 5 illocutionary forces (Searle), intent is a value
 * (replay-able), action + eval run, last_intent retrievable.
 *
 * v4 BREAK verified: px_intent_kind is const char*, not enum.
 * Custom forces can be passed.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

static int g_action_calls = 0;
static int g_eval_calls = 0;
static double g_state = 0.0;

static void action_double(px_intent intent, void* user) {
    (void)user;
    g_action_calls++;
    if (intent.payload && intent.payload_size == sizeof(double)) {
        double v;
        memcpy(&v, intent.payload, sizeof(double));
        g_state += v;
    }
}

static bool eval_always_true(void* user) {
    (void)user;
    g_eval_calls++;
    return true;
}

int main(void) {
    /* verify all 5 built-in intent kinds are accessible */
    ASSERT(px_intent_kind_eq(PX_INTENT_ASSERT, "ASSERT"), "ASSERT built-in");
    ASSERT(px_intent_kind_eq(PX_INTENT_REQUEST, "REQUEST"), "REQUEST built-in");
    ASSERT(px_intent_kind_eq(PX_INTENT_PROMISE, "PROMISE"), "PROMISE built-in");
    ASSERT(px_intent_kind_eq(PX_INTENT_DECLARE, "DECLARE"), "DECLARE built-in");
    ASSERT(px_intent_kind_eq(PX_INTENT_EXPRESS, "EXPRESS"), "EXPRESS built-in");

    /* custom domain force — proves the symbol system is open */
    px_intent_kind custom = "AUTHORIZE";
    ASSERT(!px_intent_kind_eq(PX_INTENT_ASSERT, custom), "custom != ASSERT");
    ASSERT(px_intent_kind_eq(custom, "AUTHORIZE"), "custom equals itself");

    /* create a closure with REQUEST force */
    px_closure* c = px_closure_new("add 5 to counter",
                                     PX_INTENT_REQUEST,
                                     action_double,
                                     eval_always_true,
                                     NULL);
    ASSERT(c != NULL, "closure_new");
    ASSERT(strcmp(px_closure_goal(c), "add 5 to counter") == 0, "goal");
    ASSERT(px_intent_kind_eq(px_closure_intent_kind(c), PX_INTENT_REQUEST),
           "intent kind stored");

    /* trigger with a double payload */
    g_action_calls = 0;
    g_eval_calls = 0;
    g_state = 0.0;
    double payload = 5.0;
    px_closure_trigger(c, &payload, sizeof(double));
    ASSERT(g_action_calls == 1, "action called once");
    ASSERT(g_eval_calls == 1, "eval called once");
    ASSERT(g_state == 5.0, "state updated by action");
    ASSERT(px_closure_evaluated(c), "closure marked evaluated");

    /* last_intent is retrievable (enables replay) */
    px_intent last = px_closure_last_intent(c);
    ASSERT(px_intent_kind_eq(last.kind, PX_INTENT_REQUEST), "last intent kind");
    ASSERT(last.payload != NULL, "last intent payload non-NULL");
    ASSERT(last.payload_size == sizeof(double), "last intent payload_size");

    /* replay: re-applies the same intent */
    g_state = 0.0;
    g_action_calls = 0;
    px_closure_replay(c, last);
    ASSERT(g_action_calls == 1, "replay calls action once");
    ASSERT(g_state == 5.0, "replay applied same payload");

    /* trigger without payload (size=0) */
    g_action_calls = 0;
    px_closure_trigger(c, NULL, 0);
    ASSERT(g_action_calls == 1, "trigger with NULL payload still calls action");

    px_closure_free(c);
    printf("test_closure: ALL PASS\n");
    return 0;
}
