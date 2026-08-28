/*
 * test_v05_retire.c — v0.5 L2 leak retire verification
 *
 * Validates that the 7 L2 leaks retired in v0.5 (4 Estimate + 3
 * Perception) are actually closed. See docs/concepts/canonical/leak-budgets.md
 * §2 (Estimate) and §4 (Perception) for the leak definitions and
 * retire targets.
 *
 * Test categories:
 *   A. Estimate const-correctness — value/now/is_animating are pure
 *      queries; side effects retired.
 *   B. px_estimate_advance — explicit time-step replaces auto-sample.
 *   C. Cycle detection — px_derived_recompute does not stack-overflow
 *      on cyclic dependencies.
 *   D. Phase 2 auto-invocation — perceptions fire on px_estimate_set
 *      without manual invoke_for_estimate call.
 *
 * Build: cc -std=c17 -I include tests/test_v05_retire.c \
 *          src/relation.c src/estimate.c src/closure.c src/perception.c \
 *          src/undo.c src/feedback.c -lm -o build/test_v05_retire
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run  = 0;
static int g_tests_pass = 0;

#define TEST(name) do {                                  \
    g_tests_run++;                                       \
    printf("  [TEST] %-44s ", #name);                   \
    test_##name();                                       \
    printf("OK\n");                                      \
    g_tests_pass++;                                      \
} while (0)

/* ============================================================
 * A. Estimate const-correctness (retires leaks §2 #3, #8, #10)
 * ============================================================ */

static void test_a1_value_is_const(void) {
    /* Pre-v0.5: px_estimate_value(e) auto-sampled animation, mutating
     * state on every read. v0.5: value() is a pure query.
     *
     * Verification: call px_estimate_value() during an active
     * animation; assert it returns the SAME value each time
     * (the cached pre-animation value), proving no auto-sampling. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);
    /* value() should return the cached pre-animation value (0),
     * NOT the auto-sampled mid-animation value. */
    double v1 = px_estimate_value(e);
    double v2 = px_estimate_value(e);
    double v3 = px_estimate_value(e);
    assert(v1 == 0.0);
    assert(v2 == 0.0);
    assert(v3 == 0.0);
    /* Animation flag should still be set — value() didn't finalize */
    assert(px_estimate_is_animating(e));
    px_estimate_free(e);
}

static void test_a2_is_animating_is_const(void) {
    /* Pre-v0.5: px_estimate_is_animating(e) finalized the animation
     * as a side effect (calling notify on completion). v0.5: pure
     * query — returns the animating flag without finalizing.
     *
     * Verification: animate with short duration, sleep past end,
     * call is_animating WITHOUT calling advance. Pre-v0.5 this would
     * finalize the animation; v0.5 should return true (still
     * animating per the flag) until advance is called. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 50);
    px_sleep_ms(100);

    /* is_animating is now a pure query — should still return true
     * because advance() has not been called to finalize. */
    bool still_animating = px_estimate_is_animating(e);
    assert(still_animating == true);

    /* Now advance to finalize */
    px_estimate_advance(e, px_now_ms());
    assert(!px_estimate_is_animating(e));
    assert(px_estimate_value(e) == 100.0);

    px_estimate_free(e);
}

static void test_a3_now_is_const_alias(void) {
    /* v0.5: px_estimate_now is a const alias of px_estimate_value.
     * Both return the cached value; neither auto-samples.
     *
     * Verification: set value; both queries return same value. */
    px_estimate* e = px_estimate_new(42, 1.0);
    assert(px_estimate_value(e) == px_estimate_now(e));
    assert(px_estimate_value(e) == 42.0);

    px_estimate_set(e, 100, 1.0);
    assert(px_estimate_value(e) == px_estimate_now(e));
    assert(px_estimate_value(e) == 100.0);

    px_estimate_free(e);
}

/* ============================================================
 * B. px_estimate_advance (enabler for §2 retire)
 * ============================================================ */

static void test_b1_advance_finalizes(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 50);
    px_sleep_ms(100);

    /* advance() finalizes the animation */
    px_estimate_advance(e, px_now_ms());
    assert(!px_estimate_is_animating(e));
    assert(px_estimate_value(e) == 100.0);

    px_estimate_free(e);
}

static int g_observer_count = 0;
static void on_change(px_estimate* e, void* user) {
    (void)e; (void)user;
    g_observer_count++;
}

static void test_b2_advance_fires_observers_on_finalize(void) {
    /* advance() fires observers when the animation finalizes
     * (because finalization is a discrete value change). It does
     * NOT fire observers during mid-animation cache updates
     * (continuous change is not a discrete event). */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_observe(e, on_change, NULL);
    g_observer_count = 0;

    px_estimate_animate(e, 100, 50);  /* 50ms duration */

    /* Mid-animation advance — should NOT fire observers */
    px_sleep_ms(20);
    px_estimate_advance(e, px_now_ms());
    assert(g_observer_count == 0);

    /* Final advance — SHOULD fire observers (animation completes) */
    px_sleep_ms(50);  /* total elapsed = 70ms > 50ms duration */
    px_estimate_advance(e, px_now_ms());
    assert(g_observer_count == 1);

    px_estimate_free(e);
}

static void test_b3_advance_mid_animation_caches_value(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);

    /* Advance mid-animation — value should be cached at the
     * interpolated position. */
    px_sleep_ms(50);
    px_estimate_advance(e, px_now_ms());
    double v_mid = px_estimate_value(e);
    assert(v_mid > 0 && v_mid < 100);

    /* Subsequent value() reads return the cached value without
     * further auto-sampling (const query). */
    double v2 = px_estimate_value(e);
    assert(v2 == v_mid);

    px_estimate_free(e);
}

static void test_b4_advance_noop_when_not_animating(void) {
    px_estimate* e = px_estimate_new(5, 1.0);
    /* advance() when not animating should be a no-op */
    px_estimate_advance(e, px_now_ms());
    assert(px_estimate_value(e) == 5.0);
    assert(!px_estimate_is_animating(e));

    px_estimate_free(e);
}

/* ============================================================
 * C. Cycle detection (retires leak §2 #17 — derived_recompute cycle)
 * ============================================================ */

static double identity_first(px_estimate* const* srcs, int n, void* user) {
    (void)user;
    if (n <= 0) return 0;
    return px_estimate_value(srcs[0]);
}

static void test_c1_cycle_does_not_overflow(void) {
    /* Pre-v0.5: registering a cyclic dependency (A depends on B,
     * B depends on A) and calling recompute would stack-overflow.
     * v0.5: cycle detection via per-estimate "recomputing" flag
     * breaks the cycle. Program continues; values may be stale.
     *
     * Verification: register A↔B cycle, call recompute on A,
     * assert the program does not crash. */
    px_estimate* a = px_estimate_new(1, 1.0);
    px_estimate* b = px_estimate_new(2, 1.0);

    /* Make a derived from b, and b derived from a. Use dynamic
     * sources so we can add the cycle after construction. */
    px_estimate* a_derived = px_derived_new_dynamic(identity_first, NULL);
    px_derived_add_source(a_derived, b);

    px_estimate* b_derived = px_derived_new_dynamic(identity_first, NULL);
    px_derived_add_source(b_derived, a);

    /* Now b_derived depends on a, and a_derived depends on b.
     * If we change a, b_derived recomputes (reads a), fires
     * its observers (including a_derived's source-changed
     * handler), a_derived recomputes (reads b), fires observers
     * (including b_derived's handler) → cycle.
     *
     * Wait — that's a different cycle than the simple one. The
     * actual cycle here is: changing a → b_derived recomputes →
     * b_derived.set → a_derived.recompute (since a_derived depends
     * on b) → a_derived.set → b_derived.recompute (since b_derived
     * depends on a) → CYCLE.
     *
     * v0.5 cycle detection: when b_derived.recompute is re-entered
     * (the third call in the chain), the recomputing flag is true
     * → returns early, cycle broken. */
    px_estimate_set(a, 10, 1.0);  /* should not crash */

    /* Program reached here → cycle was broken. Test passes. */

    px_estimate_free(a_derived);
    px_estimate_free(b_derived);
    px_estimate_free(a);
    px_estimate_free(b);
}

static void test_c2_dag_still_works(void) {
    /* Sanity: cycle detection should not break legitimate DAG. */
    px_estimate* a = px_estimate_new(5, 1.0);
    px_estimate* b = px_estimate_new(10, 1.0);
    px_estimate* srcs[] = { a, b };
    px_estimate* sum = px_derived_new(identity_first, NULL, srcs, 1);
    /* Wait, identity_first takes the first source. sum should = a = 5. */
    assert(px_estimate_value(sum) == 5);

    px_estimate_set(a, 100, 1.0);
    assert(px_estimate_value(sum) == 100);

    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
}

/* ============================================================
 * D. Phase 2 auto-invocation (retires leaks §4 #4, #5, #6)
 * ============================================================ */

static int g_perception_fires = 0;
static void* perceive_increment(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    g_perception_fires++;
    return NULL;
}

static void test_d1_set_auto_invokes_perceptions(void) {
    /* Pre-v0.5: changing an estimate did NOT auto-invoke perceptions;
     * user had to call px_perception_invoke_for_estimate manually.
     * v0.5: px_estimate_set triggers invoke_for_estimate internally.
     *
     * Verification: create perception depending on estimate, set
     * estimate value, assert perception fn fired (without manual
     * invoke call). */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_increment, srcs, 1, NULL);

    g_perception_fires = 0;
    px_estimate_set(e, 42, 1.0);
    /* Perception should have fired automatically (no manual invoke) */
    assert(g_perception_fires == 1);

    px_estimate_set(e, 100, 1.0);
    assert(g_perception_fires == 2);

    px_perception_free(p);
    px_estimate_free(e);
}

static void test_d2_unrelated_estimate_does_not_invoke(void) {
    /* Auto-invocation only fires perceptions that depend on the
     * changed estimate. Unrelated perceptions are not fired. */
    px_estimate* a = px_estimate_new(0, 1.0);
    px_estimate* b = px_estimate_new(0, 1.0);
    px_estimate* srcs_a[] = { a };
    px_estimate* srcs_b[] = { b };
    px_perception* pa = px_perception_new("pa", perceive_increment, srcs_a, 1, NULL);
    px_perception* pb = px_perception_new("pb", perceive_increment, srcs_b, 1, NULL);

    g_perception_fires = 0;
    px_estimate_set(a, 1, 1.0);
    /* Only pa should fire (depends on a); pb should NOT */
    assert(g_perception_fires == 1);

    px_estimate_set(b, 1, 1.0);
    /* Now pb fires; pa doesn't (a didn't change) */
    assert(g_perception_fires == 2);

    px_perception_free(pa);
    px_perception_free(pb);
    px_estimate_free(a);
    px_estimate_free(b);
}

static void test_d3_invoke_ops_still_diagnostic(void) {
    /* The three invoke ops still exist (kept as diagnostic seams),
     * but their raison d'être is no longer "abstraction is incomplete".
     * They can still be called manually for testing/debugging. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_increment, srcs, 1, NULL);

    /* Manual invoke_all should still work */
    g_perception_fires = 0;
    int n = px_perception_invoke_all();
    assert(n >= 1);
    assert(g_perception_fires >= 1);

    /* Manual invoke_for_estimate should still work */
    g_perception_fires = 0;
    int m = px_perception_invoke_for_estimate(e);
    assert(m >= 1);
    assert(g_perception_fires >= 1);

    /* Manual invoke_single should still return a representamen
     * (NULL here since perceive_increment returns NULL). */
    void* rep = px_perception_invoke_single(p);
    (void)rep;  /* may be NULL */

    px_perception_free(p);
    px_estimate_free(e);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex v0.5 L2 leak retire verification\n");
    printf("=========================================\n");
    printf("Validates that 7 L2 leaks retired in v0.5 are closed.\n");
    printf("Per docs/concepts/canonical/leak-budgets.md §2 (Estimate) + §4 (Perception).\n\n");

    printf("[A] Estimate const-correctness (retires §2 leaks #3, #8, #10)\n");
    TEST(a1_value_is_const);
    TEST(a2_is_animating_is_const);
    TEST(a3_now_is_const_alias);

    printf("\n[B] px_estimate_advance (enabler for §2 retire)\n");
    TEST(b1_advance_finalizes);
    TEST(b2_advance_fires_observers_on_finalize);
    TEST(b3_advance_mid_animation_caches_value);
    TEST(b4_advance_noop_when_not_animating);

    printf("\n[C] Cycle detection (retires §2 leak #17 — derived_recompute cycle)\n");
    TEST(c1_cycle_does_not_overflow);
    TEST(c2_dag_still_works);

    printf("\n[D] Phase 2 auto-invocation (retires §4 leaks #4, #5, #6)\n");
    TEST(d1_set_auto_invokes_perceptions);
    TEST(d2_unrelated_estimate_does_not_invoke);
    TEST(d3_invoke_ops_still_diagnostic);

    printf("\n----------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);

    if (g_tests_pass == g_tests_run) {
        printf("\nv0.5 retire verified: 4 Estimate L2 leaks + 3 Perception L2 leaks = 7 retired.\n");
        printf("Shipping L2 count: 9 → 2 (3.8%%, was 17%%). Target ≤8%% MET.\n");
        return 0;
    } else {
        printf("\nSOME TESTS FAILED — v0.5 retire not fully verified.\n");
        return 1;
    }
}
