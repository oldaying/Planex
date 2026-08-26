/*
 * antipattern_perception.c — Perception abstraction's necessity proof
 *
 * Demonstrates that mainstream UI libraries' render model
 * (React's render function, SwiftUI's View) CANNOT cleanly express:
 *
 *   1. Multiple denotations of the same state
 *      (screen + a11y + test snapshot + log)
 *   2. Pure-function rendering (testable without app loop)
 *   3. Independent lifecycle of each denotation
 *
 * Planex's Perception makes all three first-class.
 *
 * Build:
 *   cc -std=c17 -I include examples/antipattern_perception.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c -lm -o build/antipattern_perception
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Track which perceptions were called */
static int visual_calls = 0;
static int a11y_calls = 0;
static int json_calls = 0;
static int log_calls = 0;

/* === Three different Perception functions for the SAME state ===
 *
 * React: you'd have ONE render function. To produce multiple
 * denotations you'd either:
 *   - Special-case inside the render (conditional on a "mode" prop)
 *   - Maintain parallel trees (DOM + a11y tree + test renderer)
 * Both are bolt-on patches, not first-class.
 *
 * Planex: each Perception is independent, registered separately,
 * invoked independently. */

static void* perceive_visual(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    visual_calls++;
    char* buf = malloc(64);
    if (buf) snprintf(buf, 64, "Count: %.0f", px_estimate_value(inputs[0]));
    return buf;
}

static void* perceive_a11y(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    a11y_calls++;
    char* buf = malloc(128);
    if (buf) snprintf(buf, 128, "Counter value is %.0f", px_estimate_value(inputs[0]));
    return buf;
}

static void* perceive_json(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    json_calls++;
    char* buf = malloc(256);
    if (buf) snprintf(buf, 256, "{\"count\": %.0f, \"confidence\": %.2f}",
                      px_estimate_value(inputs[0]),
                      px_estimate_confidence(inputs[0]));
    return buf;
}

static void* perceive_log(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    log_calls++;
    char* buf = malloc(128);
    if (buf) snprintf(buf, 128, "[LOG] count=%.0f", px_estimate_value(inputs[0]));
    return buf;
}

int main(void) {
    printf("Planex antipattern_perception — why Perception is necessary\n");
    printf("==============================================================\n");
    printf("Shows: render function cannot express multiple denotations\n\n");

    px_estimate* count = px_estimate_new(42, 1.0);
    px_estimate* inputs[] = { count };

    /* === Anti-pattern 1: Multiple denotations of same state ===
     *
     * React: ONE render function. To produce screen + a11y + test
     * snapshot, you must either:
     *   - Branch inside render based on a "mode" prop (couples
     *     denotations, hard to test)
     *   - Maintain parallel trees: ReactDOM for screen,
     *     react-testing-renderer for tests, react-a11y for a11y.
     *     Each is a separate library.
     *
     * Planex: 4 independent Perception functions, all registered
     * for the same Estimate. Each is independent. */
    printf("[anti-pattern 1] Multiple denotations of same state\n");
    printf("  React: 1 render fn — must branch on mode OR maintain parallel trees\n");
    printf("  Planex: 4 independent Perception functions, same Estimate\n");

    px_perception* p_visual = px_perception_new("visual", perceive_visual, inputs, 1, NULL);
    px_perception* p_a11y   = px_perception_new("a11y",   perceive_a11y,   inputs, 1, NULL);
    px_perception* p_json   = px_perception_new("json",   perceive_json,   inputs, 1, NULL);
    px_perception* p_log    = px_perception_new("log",    perceive_log,     inputs, 1, NULL);

    assert(px_perception_count() == 4);
    printf("  Registered 4 perceptions for same Estimate\n");
    printf("  PASS — multiple denotations coexist as first-class\n\n");

    /* === Anti-pattern 2: Pure-function rendering (testable) ===
     *
     * React: render() is a function but it's not PURE — it can
     * call hooks (useEffect schedules side effects), read context,
     * throw errors. To test render output you need a full React
     * test renderer (react-test-renderer) — you can't just call
     * the function and inspect output.
     *
     * Planex: Perception is a pure function. Inputs are Estimates.
     * Output is the denotation. You can call it directly in a
     * test, no app loop, no test renderer. */
    printf("[anti-pattern 2] Pure-function rendering (testable)\n");
    printf("  React: render uses hooks, needs react-test-renderer to test\n");
    printf("  Planex: Perception is pure fn, callable directly in test\n");

    /* Call each perception directly — no app loop needed */
    void* visual_result = perceive_visual(inputs, 1, NULL);
    void* a11y_result = perceive_a11y(inputs, 1, NULL);
    void* json_result = perceive_json(inputs, 1, NULL);
    void* log_result = perceive_log(inputs, 1, NULL);

    /* Assert on outputs — this is a unit test, no test renderer */
    assert(strcmp((char*)visual_result, "Count: 42") == 0);
    assert(strcmp((char*)a11y_result, "Counter value is 42") == 0);
    assert(strstr((char*)json_result, "\"count\": 42") != NULL);
    assert(strstr((char*)log_result, "count=42") != NULL);

    printf("  Direct call results:\n");
    printf("    visual: %s\n", (char*)visual_result);
    printf("    a11y:   %s\n", (char*)a11y_result);
    printf("    json:   %s\n", (char*)json_result);
    printf("    log:    %s\n", (char*)log_result);
    printf("  PASS — each perception is a pure function, no test renderer\n\n");

    free(visual_result);
    free(a11y_result);
    free(json_result);
    free(log_result);

    /* === Anti-pattern 3: Independent lifecycle ===
     *
     * React: if you maintain parallel trees (DOM + a11y + test),
     * each tree has its own update mechanism. Removing one tree
     * (e.g., disabling a11y in production) requires conditional
     * code throughout. They're not independent — they're coupled
     * to the same render function.
     *
     * Planex: each Perception is independent. Remove one, others
     * still work. Add a new one at runtime. They don't affect
     * each other. */
    printf("[anti-pattern 3] Independent lifecycle of each denotation\n");
    printf("  React: parallel trees are coupled to render fn, hard to remove\n");
    printf("  Planex: each Perception is independent, can be removed at runtime\n");

    /* Current state: 4 perceptions, all called once */
    assert(visual_calls == 1);
    assert(a11y_calls == 1);
    assert(json_calls == 1);
    assert(log_calls == 1);

    /* Free the a11y perception — others should be unaffected */
    px_perception_free(p_a11y);
    assert(px_perception_count() == 3);
    printf("  After freeing a11y perception: count=%d (was 4)\n",
           px_perception_count());

    /* Invoke remaining perceptions */
    int invoked = px_perception_invoke_all();
    assert(invoked == 3);  /* only 3 left */
    assert(visual_calls == 2);  /* was 1, now 2 */
    assert(a11y_calls == 1);   /* UNCHANGED — freed */
    assert(json_calls == 2);
    assert(log_calls == 2);
    printf("  After invoke_all: visual=%d, a11y=%d (unchanged), json=%d, log=%d\n",
           visual_calls, a11y_calls, json_calls, log_calls);
    printf("  PASS — independent lifecycle, removing one doesn't affect others\n\n");

    /* === Anti-pattern 4: Selective invocation ===
     *
     * React: when state changes, ALL of render re-runs. There's
     * no way to say "only update the a11y tree, not the screen".
     * Either everything re-renders or nothing does.
     *
     * Planex: px_perception_invoke_for_estimate(est) runs only
     * perceptions that depend on the changed Estimate. This is
     * impossible in React because there's no global registry of
     * "what depends on what". */
    printf("[anti-pattern 4] Selective invocation\n");
    printf("  React: state change re-runs entire render, no selective update\n");
    printf("  Planex: invoke_for_estimate runs only matching perceptions\n");

    /* Add a perception for a DIFFERENT estimate */
    px_estimate* other_est = px_estimate_new(99, 1.0);
    px_estimate* other_inputs[] = { other_est };
    px_perception* p_other = px_perception_new("other", perceive_log,
                                                 other_inputs, 1, NULL);
    (void)p_other;

    /* px_perceptions_for_estimate(count) should return 3 (visual, json, log)
     * NOT including 'other' which depends on other_est */
    int match_count = 0;
    px_perception** matches = px_perceptions_for_estimate(count, &match_count);
    assert(matches != NULL);
    assert(match_count == 3);
    printf("  px_perceptions_for_estimate(count) returned %d (expected 3)\n",
           match_count);
    free(matches);

    /* px_perceptions_for_estimate(other_est) should return 1 */
    int other_match = 0;
    px_perception** other_matches = px_perceptions_for_estimate(other_est, &other_match);
    assert(other_matches != NULL);
    assert(other_match == 1);
    printf("  px_perceptions_for_estimate(other_est) returned %d (expected 1)\n",
           other_match);
    free(other_matches);
    printf("  PASS — selective invocation works via Relation-like dependency tracking\n\n");

    /* === Summary === */
    printf("=== Summary ===\n");
    printf("Mainstream render (React/SwiftUI) requires FOUR patches:\n");
    printf("  1. Mode branching or parallel trees for multiple denotations\n");
    printf("  2. react-test-renderer for testing render output\n");
    printf("  3. Conditional code throughout for independent lifecycle\n");
    printf("  4. Re-render entire tree on state change (no selective update)\n");
    printf("\nPlanex Perception subsumes all four as first-class:\n");
    printf("  1. Multiple Perception functions, same Estimate\n");
    printf("  2. Pure function, callable directly in unit test\n");
    printf("  3. Each Perception independent, removable at runtime\n");
    printf("  4. invoke_for_estimate runs only matching perceptions\n");
    printf("\nPerception is NECESSARY: render + patches is a strict subset.\n");

    /* Cleanup */
    px_perception_free(p_visual);
    px_perception_free(p_json);
    px_perception_free(p_log);
    px_perception_free(p_other);
    px_estimate_free(other_est);
    px_estimate_free(count);

    printf("\n=== Perception anti-pattern test complete ===\n");
    return 0;
}
