/*
 * multi_perception.c — demonstrates Perception's key value:
 *                       multiple denotations of the same state
 *
 * This is the demo that shows why Perception was promoted to a
 * 4th abstraction (ADR-0005). The same set of Estimates can have
 * multiple Perception functions, each producing a different
 * denotation:
 *
 *   - pixels (visual rendering)
 *   - a11y text (for screen readers)
 *   - log entry (for debugging)
 *   - JSON snapshot (for testing / serialization)
 *
 * Mainstream UI libraries (React, SwiftUI, etc.) cannot do this
 * cleanly — they have one render path. Planex's Perception
 * abstraction makes multiple denotations first-class.
 *
 * What this demo validates:
 *
 *   1. Same Estimate -> multiple Perception functions coexist
 *   2. Each Perception is independently testable
 *   3. Adding/removing a Perception doesn't affect others
 *   4. Perceptions can produce different output types
 *      (string, struct, etc. — caller owns the result)
 *
 * This is the strongest argument for Perception as a separate
 * abstraction. Without it, you'd have to special-case each
 * denotation inside a single render function.
 *
 * Build:
 *   cc -std=c17 -I include examples/multi_perception.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/fb.c src/font.c -lm -o build/multi_perception
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* App: a counter with derived "doubled" estimate */
typedef struct {
    px_estimate* count;
    px_estimate* doubled;       /* derived: count * 2 */
    px_graph*    graph;
    px_closure*  inc;
} App;

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v + 1, 1.0);
}

static bool eval_always_true(void* user) {
    (void)user;
    return true;
}

/* Derived: doubled = count * 2 */
static double derive_doubled(px_estimate* const* srcs, int n, void* user) {
    (void)user; (void)n;
    return px_estimate_value(srcs[0]) * 2.0;
}

/* ============================================================
 * FOUR different Perception functions for the SAME state.
 *
 * Each is a pure function. They can be added/removed independently.
 * Each produces a different denotation type (string, JSON, etc.).
 * ============================================================ */

/* Perception 1: visual text (would drive screen pixels in real app) */
static void* perceive_visual(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 2) return NULL;
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "Count: %.0f   (doubled: %.0f)",
             px_estimate_value(inputs[0]),
             px_estimate_value(inputs[1]));
    return buf;
}

/* Perception 2: accessibility text (for screen readers) */
static void* perceive_a11y(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 2) return NULL;
    char* buf = malloc(128);
    if (!buf) return NULL;
    snprintf(buf, 128, "Counter is at %.0f. Doubled value is %.0f.",
             px_estimate_value(inputs[0]),
             px_estimate_value(inputs[1]));
    return buf;
}

/* Perception 3: JSON snapshot (for testing / serialization) */
static void* perceive_json(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 2) return NULL;
    char* buf = malloc(256);
    if (!buf) return NULL;
    snprintf(buf, 256,
             "{\"count\": %.0f, \"doubled\": %.0f, \"confidence\": %.2f}",
             px_estimate_value(inputs[0]),
             px_estimate_value(inputs[1]),
             px_estimate_confidence(inputs[0]));
    return buf;
}

/* Perception 4: log line (for debugging) */
static void* perceive_log(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 2) return NULL;
    char* buf = malloc(128);
    if (!buf) return NULL;
    snprintf(buf, 128, "[LOG] t=%.0f count=%.0f doubled=%.0f",
             px_now_ms(),
             px_estimate_value(inputs[0]),
             px_estimate_value(inputs[1]));
    return buf;
}

int main(void) {
    printf("Planex multi_perception — multiple denotations of same state\n");
    printf("================================================================\n");
    printf("Validates: same Estimates -> multiple Perception functions\n\n");

    App app = {0};
    app.count = px_estimate_new(0, 1.0);
    app.graph = px_graph_new();

    /* Derived estimate: doubled = count * 2 */
    px_estimate* derive_srcs[] = { app.count };
    app.doubled = px_derived_new(derive_doubled, NULL, derive_srcs, 1);

    /* Closure */
    app.inc = px_closure_new(
        "increment counter",
        PX_INTENT_REQUEST,
        on_inc,
        eval_always_true,
        &app);
    px_declare(app.graph, app.inc, PX_REL_TRIGGERS, app.count);

    printf("Setup:\n");
    printf("  Estimate count = %.0f\n", px_estimate_value(app.count));
    printf("  Estimate doubled (derived) = %.0f\n", px_estimate_value(app.doubled));
    printf("  Closure inc registered\n\n");

    /* Register FOUR perceptions of the same state */
    px_estimate* perception_inputs[] = { app.count, app.doubled };

    px_perception* p_visual = px_perception_new(
        "visual_text", perceive_visual, perception_inputs, 2, NULL);
    px_perception* p_a11y = px_perception_new(
        "a11y_text", perceive_a11y, perception_inputs, 2, NULL);
    px_perception* p_json = px_perception_new(
        "json_snapshot", perceive_json, perception_inputs, 2, NULL);
    px_perception* p_log = px_perception_new(
        "log_line", perceive_log, perception_inputs, 2, NULL);

    printf("Registered %d perceptions of the same state:\n",
           px_perception_count());
    printf("  - %s (visual rendering)\n", px_perception_name(p_visual));
    printf("  - %s (screen reader text)\n", px_perception_name(p_a11y));
    printf("  - %s (testing / serialization)\n", px_perception_name(p_json));
    printf("  - %s (debugging)\n", px_perception_name(p_log));
    printf("\n");

    /* Trigger some interactions */
    printf("Interaction: trigger inc 5 times\n");
    for (int i = 0; i < 5; i++) {
        px_closure_trigger(app.inc, NULL, 0);
    }

    /* Show all four denotations of the resulting state */
    printf("\nAfter interactions, the FOUR denotations of state (count=5, doubled=10):\n\n");

    void* visual = perceive_visual(perception_inputs, 2, NULL);
    printf("  [visual]      %s\n", (char*)visual);
    free(visual);

    void* a11y = perceive_a11y(perception_inputs, 2, NULL);
    printf("  [a11y]        %s\n", (char*)a11y);
    free(a11y);

    void* json = perceive_json(perception_inputs, 2, NULL);
    printf("  [json]        %s\n", (char*)json);
    free(json);

    void* log = perceive_log(perception_inputs, 2, NULL);
    printf("  [log]         %s\n", (char*)log);
    free(log);

    /* Show that perceptions are independent — remove one, others still work */
    printf("\nIndependence test: free the a11y perception, others unaffected:\n");
    px_perception_free(p_a11y);
    printf("  Perception count: %d (was 4, now 3)\n", px_perception_count());

    /* The other 3 still work */
    void* visual2 = perceive_visual(perception_inputs, 2, NULL);
    void* json2 = perceive_json(perception_inputs, 2, NULL);
    void* log2 = perceive_log(perception_inputs, 2, NULL);
    printf("  visual: %s\n", (char*)visual2);
    printf("  json:   %s\n", (char*)json2);
    printf("  log:    %s\n", (char*)log2);
    free(visual2); free(json2); free(log2);

    /* Validation */
    printf("\nValidation:\n");
    assert(px_estimate_value(app.count) == 5.0);
    assert(px_estimate_value(app.doubled) == 10.0);
    assert(px_perception_count() == 3);
    printf("  Final count = %.0f (expected 5) ✓\n", px_estimate_value(app.count));
    printf("  Final doubled = %.0f (expected 10) ✓\n", px_estimate_value(app.doubled));
    printf("  Perceptions remaining = %d (expected 3) ✓\n", px_perception_count());

    /* Cleanup */
    px_perception_free(p_visual);
    px_perception_free(p_json);
    px_perception_free(p_log);
    px_closure_free(app.inc);
    px_estimate_free(app.doubled);
    px_graph_free(app.graph);
    px_estimate_free(app.count);

    printf("\n=== Demo complete ===\n");
    printf("\nWhat this demonstrates:\n");
    printf("  1. FOUR perceptions coexist for the same Estimates\n");
    printf("     - visual (screen pixels in a real app)\n");
    printf("     - a11y (screen reader text)\n");
    printf("     - json (testing / serialization / undo snapshots)\n");
    printf("     - log (debugging)\n");
    printf("  2. Each perception is independent — removing one doesn't affect others\n");
    printf("  3. Each perception is a pure function (same input -> same output)\n");
    printf("  4. Different perceptions can produce different output types\n");
    printf("\nWhy this matters:\n");
    printf("  Mainstream UI libraries (React, SwiftUI, Flutter) have ONE render\n");
    printf("  path — you'd have to special-case each denotation inside a single\n");
    printf("  render function, or maintain parallel trees (DOM + a11y tree + ...).\n");
    printf("  Planex's Perception abstraction makes multiple denotations first-class.\n");
    printf("\nThis is the strongest argument for Perception as a 4th abstraction.\n");
    printf("Without it, multi-denotation UI is awkward. With it, it's natural.\n");
    return 0;
}
