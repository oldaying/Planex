/*
 * perception_phase2.c — Phase 2 validation demo
 *
 * Validates that the Phase 2 Perception runtime works:
 *   - px_perceptions_for_estimate() returns matching perceptions
 *   - px_perception_invoke_all() runs every registered perception
 *   - px_perception_invoke_for_estimate() runs only matching ones
 *
 * This is the v0.3 milestone: Perception is no longer just an API
 * surface — the runtime can actually drive perceptions.
 *
 * What this demo proves:
 *
 *   1. After registering 3 perceptions for the same Estimate,
 *      px_perceptions_for_estimate() returns all 3.
 *   2. px_perception_invoke_all() runs all 3 — their functions fire.
 *   3. px_perception_invoke_for_estimate() runs only the matching
 *      ones — selective re-evaluation works.
 *   4. Perceptions for a DIFFERENT Estimate don't fire when you
 *      invoke_for_estimate(original_est) — isolation works.
 *
 * Build:
 *   cc -std=c17 -I include examples/perception_phase2.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/fb.c src/font.c -lm -o build/perception_phase2
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
/* Keep assert() alive in Release builds: MSVC Release defines NDEBUG
 * (CMake default flags), which compiles this suite's assertions to
 * no-ops — a vacuous pass. Found on the first real Windows run
 * (C4700 on the out-param copy was the tell). */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Counters to track which perception fn was called */
static int call_count_a = 0;
static int call_count_b = 0;
static int call_count_c = 0;
static int call_count_d = 0;

/* Four perception functions — each increments its counter */
static void* perceive_a(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    call_count_a++;
    return NULL;
}
static void* perceive_b(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    call_count_b++;
    return NULL;
}
static void* perceive_c(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    call_count_c++;
    return NULL;
}
/* perceive_d depends on a DIFFERENT estimate — for isolation test */
static void* perceive_d(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    call_count_d++;
    return NULL;
}

int main(void) {
    printf("Planex perception_phase2 — Phase 2 runtime validation\n");
    printf("======================================================\n");
    printf("Validates: px_perceptions_for_estimate + invoke_all + invoke_for_estimate\n\n");

    /* Two Estimates — count_a/b/c depend on est1, count_d depends on est2 */
    px_estimate* est1 = px_estimate_new(0, 1.0);
    px_estimate* est2 = px_estimate_new(0, 1.0);

    /* Register 3 perceptions for est1 (a, b, c) and 1 for est2 (d) */
    px_estimate* inputs1[] = { est1 };
    px_estimate* inputs2[] = { est2 };

    px_perception* pa = px_perception_new("a", perceive_a, inputs1, 1, NULL);
    px_perception* pb = px_perception_new("b", perceive_b, inputs1, 1, NULL);
    px_perception* pc = px_perception_new("c", perceive_c, inputs1, 1, NULL);
    px_perception* pd = px_perception_new("d", perceive_d, inputs2, 1, NULL);
    (void)pa; (void)pb; (void)pc; (void)pd;

    printf("Setup:\n");
    printf("  3 perceptions (a, b, c) depend on est1\n");
    printf("  1 perception  (d)     depends on est2\n");
    printf("  Total registered: %d\n\n", px_perception_count());

    /* Test 1: px_perceptions_for_estimate returns 3 matches for est1 */
    printf("[test 1] px_perceptions_for_estimate(est1) returns 3 ... ");
    int count1 = -1;
    px_perception** matching1 = px_perceptions_for_estimate(est1, &count1);
    assert(matching1 != NULL);
    assert(count1 == 3);
    printf("PASS (count=%d)\n", count1);
    free(matching1);

    /* Test 2: px_perceptions_for_estimate returns 1 match for est2 */
    printf("[test 2] px_perceptions_for_estimate(est2) returns 1 ... ");
    int count2 = -1;
    px_perception** matching2 = px_perceptions_for_estimate(est2, &count2);
    assert(matching2 != NULL);
    assert(count2 == 1);
    printf("PASS (count=%d)\n", count2);
    free(matching2);

    /* Test 3: invoke_all runs all 4 perceptions */
    printf("[test 3] px_perception_invoke_all() runs all 4 ... ");
    int invoked = px_perception_invoke_all();
    assert(invoked == 4);
    assert(call_count_a == 1);
    assert(call_count_b == 1);
    assert(call_count_c == 1);
    assert(call_count_d == 1);
    printf("PASS (invoked=%d, all counters == 1)\n", invoked);

    /* Test 4: invoke_for_estimate(est1) runs only a/b/c, NOT d */
    printf("[test 4] px_perception_invoke_for_estimate(est1) runs 3 ... ");
    int selective = px_perception_invoke_for_estimate(est1);
    assert(selective == 3);
    assert(call_count_a == 2);  /* was 1, now 2 */
    assert(call_count_b == 2);
    assert(call_count_c == 2);
    assert(call_count_d == 1);  /* UNCHANGED — isolation works */
    printf("PASS (invoked=%d, est2's perception stayed at 1)\n", selective);

    /* Test 5: invoke_for_estimate(est2) runs only d */
    printf("[test 5] px_perception_invoke_for_estimate(est2) runs 1 ... ");
    int selective2 = px_perception_invoke_for_estimate(est2);
    assert(selective2 == 1);
    assert(call_count_a == 2);  /* unchanged */
    assert(call_count_b == 2);
    assert(call_count_c == 2);
    assert(call_count_d == 2);  /* now 2 */
    printf("PASS (invoked=%d, est1's perceptions unchanged)\n", selective2);

    /* Test 6: invoke_for_estimate on unknown estimate runs 0 */
    printf("[test 6] invoke_for_estimate on unknown estimate runs 0 ... ");
    px_estimate* unknown = px_estimate_new(99, 1.0);
    int selective3 = px_perception_invoke_for_estimate(unknown);
    assert(selective3 == 0);
    printf("PASS (invoked=%d)\n", selective3);

    /* Test 7: invoke_all after freeing one perception runs 3 */
    printf("[test 7] after free, invoke_all runs 3 (was 4) ... ");
    px_perception_free(pa);
    int invoked_after_free = px_perception_invoke_all();
    assert(invoked_after_free == 3);
    assert(call_count_a == 2);  /* a was freed, doesn't run */
    assert(call_count_b == 3);  /* ran again */
    assert(call_count_c == 3);
    assert(call_count_d == 3);
    printf("PASS (invoked=%d, counter_a unchanged at 2)\n", invoked_after_free);

    /* Cleanup */
    px_estimate_free(unknown);
    px_perception_free(pb);
    px_perception_free(pc);
    px_perception_free(pd);
    px_estimate_free(est1);
    px_estimate_free(est2);

    printf("\nFinal call counts:\n");
    printf("  perceive_a (freed mid-way): %d\n", call_count_a);
    printf("  perceive_b:                  %d\n", call_count_b);
    printf("  perceive_c:                  %d\n", call_count_c);
    printf("  perceive_d:                  %d\n", call_count_d);

    printf("\n=== Phase 2 validation complete ===\n");
    printf("\nWhat this proves:\n");
    printf("  1. px_perceptions_for_estimate() returns correct matching set\n");
    printf("  2. px_perception_invoke_all() runs every registered perception\n");
    printf("  3. px_perception_invoke_for_estimate() runs only matching ones\n");
    printf("  4. Isolation: invoking for est1 doesn't touch est2's perceptions\n");
    printf("  5. Freeing a perception removes it from the invoke set\n");
    printf("\nPhase 2 status: runtime can drive perceptions selectively.\n");
    printf("Next step: integrate into px_app_run so perceptions drive rendering\n");
    printf("(replacing the on_render callback). That requires API change to\n");
    printf("px_app_desc — will be done in a separate commit.\n");
    return 0;
}
