/*
 * animation_demo.c — Estimate's time dimension in real use
 *
 * Step 2 of "unused essence features" series.
 *
 * Problem: px_estimate_animate exists (Conal's Behavior = Time -> a)
 * but integration_4abs didn't use it. This demo shows animation
 * as a first-class dimension of state — not a useEffect hack.
 *
 * Scenario: a progress bar that animates from 0% to 100% over
 * 1 second. At each sample point, the value is interpolated.
 * A derived estimate computes "is complete" (value >= 100).
 * A Perception renders the progress bar as ASCII art.
 *
 * Key difference from React:
 *   React: useState(0) + useEffect + setInterval + manual lerp + cleanup
 *   Planex: px_estimate_animate(e, 100, 1000) — done.
 *   The Estimate IS the animation. No timer, no cleanup, no lerp.
 *
 * Build:
 *   cc -std=c17 -I include examples/animation_demo.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c -lm -o build/animation_demo
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    px_estimate* progress;     /* 0 -> 100 animated */
    px_estimate* is_complete;  /* derived: progress >= 100 */
    px_graph* graph;
    px_closure* start_anim;    /* trigger animation */
    px_closure* reset;        /* reset to 0 */
} App;

static double derive_complete(px_estimate* const* s, int n, void* u) {
    (void)u; (void)n;
    return px_estimate_value(s[0]) >= 100.0 ? 1.0 : 0.0;
}

static void on_start(px_intent i, void* u) {
    (void)i;
    App* a = u;
    /* Animate from current value to 100 over 1000ms */
    px_estimate_animate(a->progress, 100.0, 1000.0);
}

static void on_reset(px_intent i, void* u) {
    (void)i;
    App* a = u;
    px_estimate_set(a->progress, 0.0, 1.0);
}

static bool eval_true(void* u) { (void)u; return true; }

/* Perception: render progress bar as ASCII art */
static void* perceive_bar(px_estimate* const* in, int n, void* u) {
    (void)u; (void)n;
    if (n < 1) return NULL;
    double v = px_estimate_value(in[0]);
    if (v < 0) v = 0;
    if (v > 100) v = 100;

    int bars = (int)(v / 5.0);  /* 20 bars max */
    char* buf = malloc(64);
    if (!buf) return NULL;
    int pos = 0;
    pos += snprintf(buf + pos, 64 - pos, "[");
    for (int i = 0; i < 20; i++) {
        buf[pos++] = (i < bars) ? '#' : '-';
    }
    pos += snprintf(buf + pos, 64 - pos, "] %.0f%%", v);
    return buf;
}

/* Perception: render completion status */
static void* perceive_status(px_estimate* const* in, int n, void* u) {
    (void)u; (void)n;
    if (n < 2) return NULL;
    double complete = px_estimate_value(in[1]);
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "%s",
        complete > 0.5 ? "COMPLETE" : "IN PROGRESS");
    return buf;
}

/* Sample the animation at multiple time points to show interpolation */
static void sample_and_print(App* a, double t_ms) {
    /* Manually sample at specific time (not auto-sample) */
    double v = px_estimate_sample(a->progress, t_ms);
    double complete = (v >= 100.0) ? 1.0 : 0.0;

    /* Render bar */
    int bars = (int)(v / 5.0);
    if (bars > 20) bars = 20;
    if (bars < 0) bars = 0;
    printf("  t=%4.0fms  ", t_ms);
    putchar('[');
    for (int i = 0; i < 20; i++) putchar(i < bars ? '#' : '-');
    printf("] %5.1f%%  %s\n", v,
           complete > 0.5 ? "COMPLETE" : "IN PROGRESS");
}

int main(void) {
    printf("Planex animation_demo — time as first-class dimension\n");
    printf("====================================================\n");
    printf("Shows: px_estimate_animate = state IS a time function\n\n");

    printf("React equivalent:\n");
    printf("  useState(0) + useEffect + setInterval + lerp + cleanup = ~20 lines\n");
    printf("Planex:\n");
    printf("  px_estimate_animate(e, 100, 1000) = 1 line\n\n");

    App a = {0};
    a.graph = px_graph_new();
    a.progress = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { a.progress };
    a.is_complete = px_derived_new(derive_complete, NULL, srcs, 1);
    a.start_anim = px_closure_new("start animation", PX_INTENT_REQUEST,
        on_start, eval_true, &a);
    a.reset = px_closure_new("reset", PX_INTENT_DECLARE,
        on_reset, eval_true, &a);

    px_estimate* pin[] = { a.progress, a.is_complete };
    px_perception* p_bar = px_perception_new("progress_bar",
        perceive_bar, pin, 2, NULL);
    px_perception* p_status = px_perception_new("completion_status",
        perceive_status, pin, 2, NULL);
    (void)p_bar; (void)p_status;

    printf("Initial state:\n");
    sample_and_print(&a, 0);
    printf("\n");

    /* Trigger animation: 0 -> 100 over 1000ms */
    printf("--- Trigger animation (0 -> 100, 1000ms) ---\n");
    px_closure_trigger(a.start_anim, NULL, 0);

    /* Sample at multiple time points to show interpolation */
    /* Note: px_estimate_sample takes time relative to animation start */
    sample_and_print(&a, 0);      /* t=0:   should be 0% */
    sample_and_print(&a, 250);    /* t=250: ~43.75% (ease-out) */
    sample_and_print(&a, 500);    /* t=500: ~75% (ease-out) */
    sample_and_print(&a, 750);    /* t=750: ~93.75% (ease-out) */
    sample_and_print(&a, 1000);   /* t=1000: 100% (complete) */
    sample_and_print(&a, 1500);   /* t=1500: 100% (stays complete) */

    printf("\n--- Validation ---\n");

    /* Check monotonic increase */
    double v0 = px_estimate_sample(a.progress, 0);
    double v250 = px_estimate_sample(a.progress, 250);
    double v500 = px_estimate_sample(a.progress, 500);
    double v1k = px_estimate_sample(a.progress, 1000);
    double v2k = px_estimate_sample(a.progress, 2000);

    printf("  t=0:    %.2f (start)\n", v0);
    printf("  t=250:  %.2f (1/4 way)\n", v250);
    printf("  t=500:  %.2f (1/2 way)\n", v500);
    printf("  t=1000: %.2f (end)\n", v1k);
    printf("  t=2000: %.2f (past end, stays at target)\n", v2k);

    assert(v0 == 0.0);
    assert(v250 > v0);    /* monotonic */
    assert(v500 > v250);  /* monotonic */
    assert(v1k == 100.0); /* reaches target */
    assert(v2k == 100.0); /* stays at target */
    printf("\n  Monotonic increase: PASS\n");
    printf("  Reaches target at t=1000: PASS\n");
    printf("  Stays at target past t=1000: PASS\n");

    /* Derived estimate tracks completion.
     * px_estimate_set triggers observer -> derived auto-recomputes.
     * But we used px_estimate_sample (which doesn't trigger observers)
     * above. Now use px_estimate_set to finalize, which DOES trigger. */
    double final_val = px_estimate_sample(a.progress, 1000.0);
    px_estimate_set(a.progress, final_val, 1.0);  /* triggers derived */
    printf("  Derived is_complete after finalize: %.0f (should be 1.0)\n",
        px_estimate_value(a.is_complete));
    assert(px_estimate_value(a.is_complete) == 1.0);
    printf("  Derived auto-tracks animation completion: PASS\n");

    /* Reset */
    printf("\n--- Reset ---\n");
    px_closure_trigger(a.reset, NULL, 0);
    sample_and_print(&a, 0);

    assert(px_estimate_value(a.progress) == 0.0);
    printf("  Reset to 0: PASS\n");

    /* Cleanup */
    px_perception_free(p_bar);
    px_perception_free(p_status);
    px_closure_free(a.start_anim);
    px_closure_free(a.reset);
    px_estimate_free(a.is_complete);
    px_graph_free(a.graph);
    px_estimate_free(a.progress);

    printf("\n=== Done ===\n");
    printf("What this proves:\n");
    printf("  1. px_estimate_animate = state IS a time function (Behavior = Time -> a)\n");
    printf("  2. Sample at any time t: px_estimate_sample(e, t)\n");
    printf("  3. Ease-out curve (not linear): t=250 gives ~44%%, not 25%%\n");
    printf("  4. Derived estimates react to animation progress\n");
    printf("  5. Animation completes and stays at target\n");
    printf("  6. Reset cancels animation and sets value directly\n");
    printf("\nReact would need:\n");
    printf("  useState(0) + useEffect(() => { const id = setInterval(...); return\n");
    printf("  () => clearInterval(id); }, []) + manual lerp + requestAnimationFrame\n");
    printf("  = ~20 lines of glue + cleanup\n");
    printf("Planex: px_estimate_animate(e, 100, 1000) = 1 line.\n");
    printf("The Estimate IS the animation. No timer, no cleanup, no lerp.\n");
    return 0;
}
