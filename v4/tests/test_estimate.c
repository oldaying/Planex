/* v4/tests/test_estimate.c — essence #1: Object / state
 *
 * Verifies: value, confidence, animation (time as first-class),
 * derived estimates (spreadsheet semantics), observers.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

static int g_observer_calls = 0;
static void obs_fn(px_estimate* e, void* user) {
    (void)e; (void)user;
    g_observer_calls++;
}

static double sum_fn(px_estimate* const* srcs, int n, void* user) {
    (void)user;
    double s = 0;
    for (int i = 0; i < n; i++) s += px_estimate_now((px_estimate*)srcs[i]);
    return s;
}

int main(void) {
    /* basic value + confidence */
    px_estimate* e = px_estimate_new(42.0, 0.9);
    ASSERT(e != NULL, "estimate_new");
    ASSERT(px_estimate_value(e) == 42.0, "initial value");
    ASSERT(px_estimate_confidence(e) == 0.9, "confidence");

    /* set + observer */
    px_estimate_observe(e, obs_fn, NULL);
    g_observer_calls = 0;
    px_estimate_set(e, 100.0, 0.5);
    ASSERT(px_estimate_value(e) == 100.0, "set value");
    ASSERT(g_observer_calls == 1, "observer called once");
    ASSERT(px_estimate_confidence(e) == 0.5, "set confidence");

    /* animation: set target, sample without blocking too long */
    px_estimate_animate(e, 200.0, 30.0);  /* 30ms */
    ASSERT(px_estimate_is_animating(e), "animating immediately after animate()");
    /* sleep 50ms so animation finalizes */
    /* (use busy-wait since we don't have a portable sleep in v4) */
    double t_end = px_now_ms() + 50.0;
    while (px_now_ms() < t_end) { /* spin */ }
    double final_val = px_estimate_now(e);
    ASSERT(!px_estimate_is_animating(e), "animation ended");
    ASSERT(final_val == 200.0, "animation reached target");

    /* derived: sum of two estimates */
    px_estimate* a = px_estimate_new(10.0, 1.0);
    px_estimate* b = px_estimate_new(20.0, 1.0);
    px_estimate* srcs[] = { a, b };
    px_estimate* sum = px_derived_new(sum_fn, NULL, srcs, 2);
    ASSERT(sum != NULL, "derived_new");
    ASSERT(px_estimate_value(sum) == 30.0, "derived initial = 10+20");

    /* when a source changes, derived auto-recomputes */
    g_observer_calls = 0;
    px_estimate_observe(sum, obs_fn, NULL);
    px_estimate_set(a, 100.0, 1.0);
    ASSERT(px_estimate_value(sum) == 120.0, "derived recompute after source set");
    ASSERT(g_observer_calls >= 1, "derived observer fired after source change");

    px_estimate_free(e);
    px_estimate_free(a);
    px_estimate_free(b);
    px_estimate_free(sum);
    printf("test_estimate: ALL PASS\n");
    return 0;
}
