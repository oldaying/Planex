/*
 * perception_smoke.c — Phase 1 smoke test for Perception API
 *
 * Validates that the Phase 1 API surface works:
 *   - px_closure_new without perception parameter (new signature)
 *   - px_perception_new registers in global registry
 *   - px_perception_count tracks registrations
 *   - px_perception_name returns correct name
 *   - px_perceptions_for_estimate is a stub (returns NULL, count=0)
 *
 * This is NOT a full app — just API surface validation.
 * Run after Phase 1 to confirm the API change didn't break anything.
 *
 * Build:
 *   cc -std=c17 -I include examples/perception_smoke.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/fb.c src/font.c -lm -o build/perception_smoke
 *
 * Run:
 *   ./build/perception_smoke
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test app */
typedef struct {
    px_estimate* count;
} App;

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v + 1, 1.0);
}

static bool eval_nonneg(void* user) {
    App* a = user;
    return px_estimate_value(a->count) >= 0;
}

/* Pure perception function: takes Estimates, returns "denotation"
 * (here just a heap-allocated string for testing) */
static void* render_to_string(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "count = %.0f", px_estimate_value(inputs[0]));
    return buf;
}

int main(void) {
    printf("Planex perception_smoke — Phase 1 API validation\n");
    printf("================================================\n\n");

    /* Verify px_perception_count starts at 0 */
    printf("[test 1] px_perception_count() == 0 initially ... ");
    assert(px_perception_count() == 0);
    printf("PASS\n");

    /* Set up app + Closure with NEW signature (no perception param) */
    printf("[test 2] px_closure_new without perception parameter ... ");
    App app = {0};
    app.count = px_estimate_new(0, 1.0);

    px_closure* inc = px_closure_new(
        "increment counter",
        PX_INTENT_REQUEST,
        on_inc,
        eval_nonneg,
        &app);

    assert(inc != NULL);
    printf("PASS\n");

    /* Trigger Closure — should still work */
    printf("[test 3] Closure triggers and updates Estimate ... ");
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(app.count) == 1.0);
    printf("PASS (count = %.0f)\n", px_estimate_value(app.count));

    /* Create a Perception */
    printf("[test 4] px_perception_new registers in registry ... ");
    px_estimate* inputs[] = { app.count };
    px_perception* p1 = px_perception_new(
        "counter_pixels",
        render_to_string,
        inputs, 1,
        NULL);
    assert(p1 != NULL);
    assert(px_perception_count() == 1);
    printf("PASS\n");

    /* Verify name */
    printf("[test 5] px_perception_name returns correct name ... ");
    assert(strcmp(px_perception_name(p1), "counter_pixels") == 0);
    printf("PASS\n");

    /* Create second Perception (multiple perceptions coexist) */
    printf("[test 6] Multiple perceptions coexist ... ");
    px_perception* p2 = px_perception_new(
        "counter_a11y",
        render_to_string,
        inputs, 1,
        NULL);
    assert(p2 != NULL);
    assert(px_perception_count() == 2);
    printf("PASS (count = %d)\n", px_perception_count());

    /* Phase 2: px_perceptions_for_estimate returns matching perceptions */
    printf("[test 7] px_perceptions_for_estimate returns 2 matches ... ");
    int n_found = -1;
    px_perception** found = px_perceptions_for_estimate(app.count, &n_found);
    assert(found != NULL);
    assert(n_found == 2);  /* p1 and p2 both depend on app.count */
    free(found);
    printf("PASS (count=%d)\n", n_found);

    /* Free one perception, verify count drops */
    printf("[test 8] px_perception_free removes from registry ... ");
    px_perception_free(p1);
    assert(px_perception_count() == 1);
    printf("PASS (count = %d)\n", px_perception_count());

    /* Cleanup */
    px_perception_free(p2);
    px_closure_free(inc);
    px_estimate_free(app.count);

    printf("[test 9] After cleanup, count back to 0 ... ");
    assert(px_perception_count() == 0);
    printf("PASS\n");

    printf("\n=== All Phase 1 API tests PASS ===\n");
    printf("\nWhat this validates:\n");
    printf("  1. px_closure_new signature changed (no perception param)\n");
    printf("  2. Closure still triggers correctly\n");
    printf("  3. Perception registry works (add/remove/count)\n");
    printf("  4. Multiple perceptions coexist\n");
    printf("  5. Phase 1 stub for px_perceptions_for_estimate works\n");
    printf("\nNext: Phase 2 will implement actual perception driving\n");
    printf("(replace on_render callback with perception invocation).\n");
    return 0;
}
