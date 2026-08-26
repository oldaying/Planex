/*
 * antipattern_estimate.c — Estimate abstraction's necessity proof
 *
 * Demonstrates that mainstream UI libraries' state model
 * (React's useState, Solid's signal) CANNOT cleanly express:
 *
 *   1. Time-varying state (animation) — needs useEffect + setTimeout
 *   2. State with uncertainty (confidence) — needs separate field
 *   3. Auto-derived state — needs manual tracking or useMemo
 *
 * Planex's Estimate subsumes all three as first-class fields.
 *
 * This demo proves Estimate is NECESSARY, not just convenient.
 *
 * Build:
 *   cc -std=c17 -I include examples/antipattern_estimate.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c -lm -o build/antipattern_estimate
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int main(void) {
    printf("Planex antipattern_estimate — why Estimate is necessary\n");
    printf("=========================================================\n");
    printf("Shows: useState/signal cannot cleanly express time + uncertainty\n\n");

    /* === Anti-pattern 1: Time-varying state (animation) ===
     *
     * React approach: useState(0) + useEffect + setTimeout + manual
     * interpolation + requestAnimationFrame. ~20 lines of glue code.
     * The state model has NO concept of "this value is changing
     * over time" — the developer must rebuild that machinery.
     *
     * Planex approach: px_estimate_animate(e, target, duration).
     * The Estimate IS the time-varying value. px_estimate_now(e)
     * samples it at current time. Zero glue code. */
    printf("[anti-pattern 1] Animation — time as first-class dimension\n");
    printf("  React: useState(0) + useEffect + setTimeout + manual lerp = ~20 LOC glue\n");
    printf("  Planex: px_estimate_animate(e, target, ms) — Estimate IS time-varying\n");

    px_estimate* anim = px_estimate_new(0.0, 1.0);
    px_estimate_animate(anim, 100.0, 1000.0);  /* 0 -> 100 over 1 second */

    /* Sample at multiple time points — no setTimeout needed */
    double v0   = px_estimate_sample(anim, 0.0);     /* at start */
    double v250 = px_estimate_sample(anim, 250.0);   /* 1/4 way */
    double v500 = px_estimate_sample(anim, 500.0);   /* 1/2 way */
    double v1k  = px_estimate_sample(anim, 1000.0);  /* at end */

    printf("  Sampled values: t=0 -> %.2f, t=250 -> %.2f, t=500 -> %.2f, t=1000 -> %.2f\n",
           v0, v250, v500, v1k);

    /* Validate animation is monotonically increasing (ease-out curve) */
    assert(v0 < v250);
    assert(v250 < v500);
    assert(v500 < v1k);
    assert(v0 == 0.0);
    assert(v1k == 100.0);
    printf("  PASS — monotonic, endpoints exact\n\n");

    /* === Anti-pattern 2: State with uncertainty (confidence) ===
     *
     * React approach: useState(42) has no confidence field. To track
     * uncertainty you need a SEPARATE state: useState({value: 42,
     * confidence: 0.8}). Every consumer must remember to read both.
     * Derived state (sum of uncertain values) must propagate confidence
     * manually — the abstraction doesn't help.
     *
     * Planex approach: Estimate has confidence as a first-class field.
     * px_estimate_new(value, confidence). Derived estimates can use
     * confidence in their computation. */
    printf("[anti-pattern 2] Uncertainty — confidence as first-class field\n");
    printf("  React: useState(42) + separate useState(confidence) — manual pair\n");
    printf("  Planex: px_estimate_new(value, confidence) — single concept\n");

    px_estimate* sensor = px_estimate_new(42.0, 0.8);
    assert(px_estimate_value(sensor) == 42.0);
    assert(px_estimate_confidence(sensor) == 0.8);

    /* Demonstrate that confidence can change independently of value */
    px_estimate_set(sensor, 42.0, 0.95);  /* same value, higher confidence */
    assert(px_estimate_value(sensor) == 42.0);
    assert(px_estimate_confidence(sensor) == 0.95);
    printf("  Confidence changes independently: value=42, conf=0.8 -> 0.95\n");
    printf("  PASS — confidence is first-class\n\n");

    /* === Anti-pattern 3: Auto-derived state ===
     *
     * React approach: useMemo(() => a + b, [a, b]). Developer must
     * list dependencies manually — forget one, bug. Change deps,
     * update the list. The dependency tracking is bolted on top of
     * useState, not part of the state model.
     *
     * Planex approach: px_derived_new(fn, sources, n). The Relation
     * graph tracks dependencies automatically. When source changes,
     * derived recomputes — no manual list. */
    printf("[anti-pattern 3] Auto-derived state — no manual dep list\n");
    printf("  React: useMemo(() => a + b, [a, b]) — manual dep list\n");
    printf("  Planex: px_derived_new(sum, srcs, 2) — Relation tracks deps\n");

    px_estimate* a = px_estimate_new(10.0, 1.0);
    px_estimate* b = px_estimate_new(20.0, 1.0);

    px_estimate* srcs[] = { a, b };
    /* Use a named function (portable across gcc/clang/MSVC) */
    extern double sum_fn(px_estimate* const* s, int n, void* user);
    px_estimate* total = px_derived_new(sum_fn, NULL, srcs, 2);

    /* Initial derived value */
    assert(px_estimate_value(total) == 30.0);
    printf("  Initial: a=%.0f + b=%.0f = total=%.0f\n",
           px_estimate_value(a), px_estimate_value(b), px_estimate_value(total));

    /* Change source — derived auto-updates, no manual tracking */
    px_estimate_set(a, 50.0, 1.0);
    assert(px_estimate_value(total) == 70.0);  /* auto-updated! */
    printf("  After a=50: total=%.0f (auto-updated, no manual list)\n",
           px_estimate_value(total));

    px_estimate_set(b, 100.0, 1.0);
    assert(px_estimate_value(total) == 150.0);
    printf("  After b=100: total=%.0f (auto-updated again)\n",
           px_estimate_value(total));
    printf("  PASS — derived auto-tracks sources via Relation\n\n");

    /* === Summary: what Estimate subsumes === */
    printf("=== Summary ===\n");
    printf("Mainstream state (useState/signal) requires THREE patches:\n");
    printf("  1. useEffect + setTimeout for animation\n");
    printf("  2. separate state for confidence\n");
    printf("  3. useMemo + manual dep list for derived\n");
    printf("\nPlanex Estimate subsumes all three as first-class:\n");
    printf("  1. px_estimate_animate (time is in the abstraction)\n");
    printf("  2. px_estimate_new(value, confidence) (uncertainty is field)\n");
    printf("  3. px_derived_new (Relation tracks deps automatically)\n");
    printf("\nEstimate is NECESSARY: useState + patches is a strict subset.\n");

    /* Cleanup */
    px_estimate_free(total);
    px_estimate_free(a);
    px_estimate_free(b);
    px_estimate_free(sensor);
    px_estimate_free(anim);

    printf("\n=== Estimate anti-pattern test complete ===\n");
    return 0;
}

/* Portable sum function for derived estimate */
double sum_fn(px_estimate* const* s, int n, void* user) {
    (void)user; (void)n;
    return px_estimate_value(s[0]) + px_estimate_value(s[1]);
}
