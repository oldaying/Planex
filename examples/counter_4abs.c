/*
 * counter_4abs.c — minimal demo using all FOUR abstractions
 *
 * This is the canonical "hello world" of Planex post-ADR-0005.
 * It uses Relation + Estimate + Closure + Perception, all four.
 *
 * Replaces the old counter.c (3-abstraction era) which used
 * Closure's perception parameter and an on_render callback.
 *
 * What this demo shows:
 *
 *   1. Estimate — count state
 *   2. Closure  — inc and dec actions (5-stage, no perception param)
 *   3. Relation — DEPENDS_ON declares "render depends on count"
 *                 (also TRIGGERS: inc/dec triggers count change)
 *   4. Perception — render_to_text pure function denoting count
 *
 * The demo is stdout-only (no pixels, no window). It validates
 * that the 4-abstraction API surface works end-to-end. For visual
 * output see counter_denotative.c; for interactive windows see
 * counter_interactive.c.
 *
 * Build:
 *   cc -std=c17 -I include examples/counter_4abs.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/fb.c src/font.c -lm -o build/counter_4abs
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * App state
 * ============================================================ */

typedef struct {
    px_estimate* count;
    px_graph*    graph;
    px_closure*  inc;
    px_closure*  dec;
} CounterApp;

/* Closure actions — 5-stage execution side only */
static void on_inc(px_intent intent, void* user) {
    (void)intent;
    CounterApp* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v + 1, 1.0);
}

static void on_dec(px_intent intent, void* user) {
    (void)intent;
    CounterApp* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v - 1, 1.0);
}

static bool eval_nonneg(void* user) {
    CounterApp* app = user;
    return px_estimate_value(app->count) >= 0;
}

/* ============================================================
 * Perception — pure function denoting state
 *
 * This is the NEW way (post-ADR-0005). Render is no longer a
 * Closure with PX_INTENT_EXPRESS — it's a Perception, a pure
 * function of Estimates that returns a denotation.
 *
 * Here we return a heap-allocated string (caller frees).
 * In a real app this could return px_fb*, a11y_tree, log_entry, etc.
 * ============================================================ */

static void* render_count_to_string(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "Counter: %.0f", px_estimate_value(inputs[0]));
    return buf;
}

/* Second perception — a11y text. Same Estimates, different denotation. */
static void* render_count_to_a11y(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(128);
    if (!buf) return NULL;
    snprintf(buf, 128, "Counter value is %.0f",
             px_estimate_value(inputs[0]));
    return buf;
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex counter_4abs — minimal 4-abstraction demo\n");
    printf("================================================\n");
    printf("Validates: Relation + Estimate + Closure + Perception\n\n");

    CounterApp app = {0};
    app.count = px_estimate_new(0, 1.0);
    app.graph = px_graph_new();

    /* Closure (5-stage, no perception parameter) */
    app.inc = px_closure_new(
        "increment counter",
        PX_INTENT_REQUEST,
        on_inc,
        eval_nonneg,
        &app);
    app.dec = px_closure_new(
        "decrement counter",
        PX_INTENT_REQUEST,
        on_dec,
        eval_nonneg,
        &app);

    /* Relations:
     *   inc TRIGGERS count (clicking inc changes count)
     *   dec TRIGGERS count
     * These are causal relations, distinct from dependency. */
    px_declare(app.graph, app.inc, PX_REL_TRIGGERS, app.count);
    px_declare(app.graph, app.dec, PX_REL_TRIGGERS, app.count);

    /* Perception — registers the render function.
     * Pure function: takes count as input, returns denotation. */
    px_estimate* inputs[] = { app.count };
    px_perception* text_p = px_perception_new(
        "counter_text",
        render_count_to_string,
        inputs, 1, NULL);
    px_perception* a11y_p = px_perception_new(
        "counter_a11y",
        render_count_to_a11y,
        inputs, 1, NULL);

    printf("Setup:\n");
    printf("  Estimate: count = %.0f\n", px_estimate_value(app.count));
    printf("  Closures: inc, dec (5-stage, no perception param)\n");
    printf("  Relations: inc TRIGGERS count, dec TRIGGERS count\n");
    printf("  Perceptions: %d registered\n\n", px_perception_count());

    /* Simulate interactions — trigger Closures, then invoke perceptions */
    printf("Interaction: trigger inc 3 times, dec once\n");
    px_closure_trigger(app.inc, NULL, 0);
    px_closure_trigger(app.inc, NULL, 0);
    px_closure_trigger(app.inc, NULL, 0);
    px_closure_trigger(app.dec, NULL, 0);

    /* v0.5 (Phase 2, landed — the old "Phase 1: manual invocation"
     * comment was retired): perceptions are auto-invoked by
     * px_estimate_set when their source Estimates change, so the
     * triggers above have ALREADY fired both perceptions once per
     * count change. The direct calls below are the diagnostic seam
     * (px_perception_invoke_* exists for tests/explicit re-perceive);
     * normal application code does NOT need them. */
    printf("\nDenotations after interactions:\n");

    /* Invoke text perception */
    void* text_result = render_count_to_string(inputs, 1, NULL);
    printf("  [text perception]    %s\n", (char*)text_result);
    free(text_result);

    /* Invoke a11y perception — same Estimate, different denotation */
    void* a11y_result = render_count_to_a11y(inputs, 1, NULL);
    printf("  [a11y perception]    %s\n", (char*)a11y_result);
    free(a11y_result);

    /* Validate */
    printf("\nValidation:\n");
    double final = px_estimate_value(app.count);
    printf("  Final count: %.0f (expected: 2)\n", final);
    assert(final == 2.0);
    assert(px_perception_count() == 2);
    printf("  Perceptions registered: %d (expected: 2)\n", px_perception_count());
    printf("  Closure inc last evaluated: %s\n",
           px_closure_evaluated(app.inc) ? "true" : "false");

    /* Cleanup */
    px_perception_free(text_p);
    px_perception_free(a11y_p);
    px_closure_free(app.inc);
    px_closure_free(app.dec);
    px_graph_free(app.graph);
    px_estimate_free(app.count);

    printf("\n=== Demo complete ===\n");
    printf("\nWhat this demo shows:\n");
    printf("  1. Estimate (count) — state with value + confidence\n");
    printf("  2. Closure (inc, dec) — 5-stage execution, no perception param\n");
    printf("  3. Relation (TRIGGERS) — inc/dec causally trigger count\n");
    printf("  4. Perception (text, a11y) — pure functions denoting count\n");
    printf("\nKey post-ADR-0005 patterns:\n");
    printf("  - Closure signature is 5-arg (no perception parameter)\n");
    printf("  - Render is NOT a Closure (was PX_INTENT_EXPRESS before)\n");
    printf("  - Multiple perceptions coexist for the same Estimate\n");
    printf("  - Perceptions are pure functions (testable, cacheable)\n");
    return 0;
}
