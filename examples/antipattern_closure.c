/*
 * antipattern_closure.c — Closure abstraction's necessity proof
 *
 * Demonstrates that mainstream UI libraries' event model
 * (React's onClick, Solid's event handler) CANNOT cleanly express:
 *
 *   1. Intent as a value (not a callback) — for serialization/audit
 *   2. Goal as a recorded field — for error messages / debugging
 *   3. Auto-evaluation — runtime knows if action succeeded
 *   4. Async lifecycle (promise/declare/fail) — built-in
 *   5. Speech-act typed Intent — Request/Promise/Declare/Assert/Express
 *
 * Planex's Closure subsumes all five as first-class fields.
 *
 * Build:
 *   cc -std=c17 -I include examples/antipattern_closure.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c -lm -o build/antipattern_closure
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    px_estimate* count;
    int action_calls;
    char last_goal[128];
} App;

static void on_inc(px_intent intent, void* user) {
    App* a = user;
    a->action_calls++;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v + 1, 1.0);
    (void)intent;
}

static bool eval_nonneg(void* user) {
    App* a = user;
    return px_estimate_value(a->count) >= 0;
}

static void on_dec(px_intent intent, void* user) {
    App* a = user;
    a->action_calls++;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v - 1, 1.0);
    (void)intent;
}

int main(void) {
    printf("Planex antipattern_closure — why Closure is necessary\n");
    printf("=========================================================\n");
    printf("Shows: onClick/handler cannot express intent + goal + eval + lifecycle\n\n");

    App app = {0};
    app.count = px_estimate_new(0, 1.0);
    px_graph* g = px_graph_new();

    px_closure* inc = px_closure_new(
        "increment counter",         /* Goal — recorded */
        PX_INTENT_REQUEST,            /* Intent kind — typed value */
        on_inc,                       /* Action */
        eval_nonneg,                  /* Evaluation */
        &app);

    px_closure* dec = px_closure_new(
        "decrement counter",
        PX_INTENT_REQUEST,
        on_dec,
        eval_nonneg,
        &app);

    /* === Anti-pattern 1: Intent as a value, not a callback ===
     *
     * React: onClick={() => doStuff()} — the intent is BURIED inside
     * the callback. You cannot serialize "user clicked the button"
     * without writing your own logging layer. The intent exists only
     * as a transient function call.
     *
     * Planex: Intent is a typed value. px_closure_last_intent(c)
     * returns the last intent as a struct {kind, payload, size}.
     * You can serialize it, log it, replay it. */
    printf("[anti-pattern 1] Intent as a value, not a callback\n");
    printf("  React: onClick={fn} — intent buried in callback, transient\n");
    printf("  Planex: px_closure_last_intent() returns typed value, persistent\n");

    int payload = 42;
    px_closure_trigger(inc, &payload, sizeof(payload));

    px_intent last = px_closure_last_intent(inc);
    assert(last.kind == PX_INTENT_REQUEST);
    assert(last.payload_size == sizeof(int));
    assert(*(int*)last.payload == 42);

    printf("  After trigger: kind=%s, payload=%d (serializable)\n",
           px_intent_kind_str(last.kind), *(int*)last.payload);
    printf("  PASS — intent is a value, can be serialized/replayed\n\n");

    /* === Anti-pattern 2: Goal as a recorded field ===
     *
     * React: onClick={() => increment()} — the goal "increment" is
     * implicit in the function name. If the action fails, the error
     * message can only say "increment threw" — no first-class goal.
     *
     * Planex: Goal is a recorded string. When evaluation fails, the
     * runtime auto-generates: "evaluation failed: goal \"increment
     * counter\" not achieved". The goal is in the closure struct. */
    printf("[anti-pattern 2] Goal as a recorded field\n");
    printf("  React: goal implicit in function name, lost on error\n");
    printf("  Planex: goal is a field, used in error messages\n");

    /* Trigger dec to make count negative, eval will fail */
    px_closure_trigger(dec, NULL, 0);  /* count = 1 -> 0, eval OK */
    px_closure_trigger(dec, NULL, 0);  /* count = 0 -> -1, eval FAILS */

    /* Check the auto-generated feedback includes the goal */
    const char* feedback = px_closure_feedback(dec);
    printf("  After dec to -1: feedback = \"%s\"\n", feedback);
    assert(strstr(feedback, "decrement counter") != NULL);
    assert(strstr(feedback, "not achieved") != NULL);
    assert(px_closure_get_status(dec) == PX_CLOSURE_FAILED);
    printf("  PASS — goal recorded, used in error feedback\n\n");

    /* Reset count for next test */
    px_estimate_set(app.count, 0, 1.0);

    /* === Anti-pattern 3: Auto-evaluation ===
     *
     * React: onClick={fn} — fn returns void. The runtime has NO way
     * to know if the action succeeded. Developer must manually:
     *   - Set a status state
     *   - Catch errors with try/catch
     *   - Show error message separately
     *
     * Planex: px_closure_new takes an evaluation function. After
     * action runs, runtime calls eval(). If false, status auto-sets
     * to FAILED. No try/catch, no manual status. */
    printf("[anti-pattern 3] Auto-evaluation\n");
    printf("  React: onClick fn returns void, no built-in success/failure\n");
    printf("  Planex: eval fn returns bool, runtime auto-sets status\n");

    /* inc with eval_nonneg — count is 0, eval should succeed */
    px_closure_trigger(inc, NULL, 0);  /* count: 0 -> 1 */
    assert(px_closure_evaluated(inc) == true);
    assert(px_closure_get_status(inc) == PX_CLOSURE_DONE);

    printf("  After inc (eval succeeds): status=%s, evaluated=%s\n",
           px_closure_status_str(px_closure_get_status(inc)),
           px_closure_evaluated(inc) ? "true" : "false");
    printf("  PASS — evaluation is automatic, status is auto-set\n\n");

    /* === Anti-pattern 4: Async lifecycle (promise/declare/fail) ===
     *
     * React: for async actions, developer writes:
     *   - setIsLoading(true)
     *   - try { await fetch(); setIsLoading(false) }
     *   - catch (e) { setError(e); setIsLoading(false) }
     * Three state variables, three code paths. The event model has
     * no concept of "this action is async with a lifecycle".
     *
     * Planex: Closure has built-in promise/declare/fail.
     * px_closure_promise("loading") -> status=RUNNING
     * px_closure_declare("done")    -> status=DONE
     * px_closure_fail("error")     -> status=FAILED
     * One abstraction, one lifecycle. */
    printf("[anti-pattern 4] Async lifecycle (promise/declare/fail)\n");
    printf("  React: 3 state vars (isLoading, error, data) for async\n");
    printf("  Planex: closure has promise/declare/fail built-in\n");

    /* Create a closure for "async fetch" (we'll simulate the lifecycle) */
    px_closure* fetch_c = px_closure_new(
        "fetch user data", PX_INTENT_PROMISE,
        NULL,  /* no synchronous action — async only */
        NULL,  /* no eval — async status is set by declare/fail */
        &app);

    /* Simulate async lifecycle */
    px_closure_promise(fetch_c, "Fetching user data...");
    assert(px_closure_get_status(fetch_c) == PX_CLOSURE_RUNNING);
    printf("  After promise: status=%s, feedback=\"%s\"\n",
           px_closure_status_str(px_closure_get_status(fetch_c)),
           px_closure_feedback(fetch_c));

    px_closure_declare(fetch_c, "User data loaded");
    assert(px_closure_get_status(fetch_c) == PX_CLOSURE_DONE);
    printf("  After declare: status=%s, feedback=\"%s\"\n",
           px_closure_status_str(px_closure_get_status(fetch_c)),
           px_closure_feedback(fetch_c));

    /* Test fail path too */
    px_closure_promise(fetch_c, "Re-fetching...");
    px_closure_fail(fetch_c, "Network error");
    assert(px_closure_get_status(fetch_c) == PX_CLOSURE_FAILED);
    printf("  After fail:    status=%s, feedback=\"%s\"\n",
           px_closure_status_str(px_closure_get_status(fetch_c)),
           px_closure_feedback(fetch_c));
    printf("  PASS — async lifecycle is first-class\n\n");

    /* === Anti-pattern 5: Speech-act typed Intent ===
     *
     * React: onClick is just "a function". There's no distinction
     * between "user requests X" vs "system declares X" vs "user
     * expresses feeling X". All are the same void(*)() callback.
     *
     * Planex: Intent has 5 typed kinds (Winograd/Flores speech acts):
     *   ASSERT, REQUEST, PROMISE, DECLARE, EXPRESS
     * Each kind can be handled differently by the runtime. */
    printf("[anti-pattern 5] Speech-act typed Intent\n");
    printf("  React: onClick is just a function, no semantic type\n");
    printf("  Planex: 5 typed kinds (ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS)\n");

    px_closure* c_assert = px_closure_new("state is X", PX_INTENT_ASSERT, NULL, NULL, &app);
    px_closure* c_request = px_closure_new("do X", PX_INTENT_REQUEST, NULL, NULL, &app);
    px_closure* c_promise = px_closure_new("will do X", PX_INTENT_PROMISE, NULL, NULL, &app);
    px_closure* c_declare = px_closure_new("X is done", PX_INTENT_DECLARE, NULL, NULL, &app);
    px_closure* c_express = px_closure_new("feel X", PX_INTENT_EXPRESS, NULL, NULL, &app);

    /* Trigger each and check intent kind is preserved */
    px_closure_trigger(c_assert, NULL, 0);
    px_closure_trigger(c_request, NULL, 0);
    px_closure_trigger(c_promise, NULL, 0);
    px_closure_trigger(c_declare, NULL, 0);
    px_closure_trigger(c_express, NULL, 0);

    assert(px_closure_last_intent(c_assert).kind == PX_INTENT_ASSERT);
    assert(px_closure_last_intent(c_request).kind == PX_INTENT_REQUEST);
    assert(px_closure_last_intent(c_promise).kind == PX_INTENT_PROMISE);
    assert(px_closure_last_intent(c_declare).kind == PX_INTENT_DECLARE);
    assert(px_closure_last_intent(c_express).kind == PX_INTENT_EXPRESS);

    printf("  All 5 kinds preserved: %s, %s, %s, %s, %s\n",
           px_intent_kind_str(PX_INTENT_ASSERT),
           px_intent_kind_str(PX_INTENT_REQUEST),
           px_intent_kind_str(PX_INTENT_PROMISE),
           px_intent_kind_str(PX_INTENT_DECLARE),
           px_intent_kind_str(PX_INTENT_EXPRESS));
    printf("  PASS — Intent has semantic type (Winograd/Flores speech acts)\n\n");

    /* === Summary === */
    printf("=== Summary ===\n");
    printf("Mainstream event (onClick/handler) requires FIVE patches:\n");
    printf("  1. Manual logging layer for intent serialization\n");
    printf("  2. Manual goal tracking for error messages\n");
    printf("  3. try/catch + status state for evaluation\n");
    printf("  4. 3 state vars for async lifecycle\n");
    printf("  5. Convention-only semantic typing of intents\n");
    printf("\nPlanex Closure subsumes all five as first-class:\n");
    printf("  1. px_closure_last_intent() returns typed value\n");
    printf("  2. Goal is a field, used in auto-feedback\n");
    printf("  3. eval fn returns bool, runtime auto-sets status\n");
    printf("  4. promise/declare/fail built-in async lifecycle\n");
    printf("  5. 5 typed Intent kinds (Winograd/Flores)\n");
    printf("\nClosure is NECESSARY: onClick + patches is a strict subset.\n");

    /* Cleanup */
    px_closure_free(c_assert);
    px_closure_free(c_request);
    px_closure_free(c_promise);
    px_closure_free(c_declare);
    px_closure_free(c_express);
    px_closure_free(fetch_c);
    px_closure_free(inc);
    px_closure_free(dec);
    px_graph_free(g);
    px_estimate_free(app.count);

    printf("\n=== Closure anti-pattern test complete ===\n");
    return 0;
}
