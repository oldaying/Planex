/*
 * test_feedback.c — Feedback (closed-loop coupling) test suite
 *
 * Validates the v0.4 essence category "Feedback" (px_loop).
 * Per essence-derivation-v2.md and ADR-0008, Feedback is the closed
 * loop of:
 *   intent → action → state → perception → next intent
 *
 * Test categories:
 *   A. Lifecycle — loop create/free
 *   B. Step — one iteration fires both trigger and perception
 *   C. Pause/resume — interrupt the loop
 *   D. Audit — loop records iteration history
 *   E. Replay — re-run recorded iterations
 *   F. Essence — Feedback solves what 4 abstractions alone can't
 *
 * Build: cc -std=c17 -I include tests/test_feedback.c \
 *          src/relation.c src/estimate.c src/closure.c src/perception.c \
 *          src/undo.c src/feedback.c -lm -o build/test_feedback
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
 * Helpers
 * ============================================================ */

static void on_inc(px_intent i, void* u) {
    (void)i;
    px_estimate* e = (px_estimate*)u;
    px_estimate_set(e, px_estimate_value(e) + 1.0, 1.0);
}

static bool eval_true(void* u) { (void)u; return true; }

static int g_perception_count = 0;
static void* perceive_count(px_estimate* const* in, int n, void* u) {
    (void)in; (void)n; (void)u;
    g_perception_count++;
    return NULL;
}

/* ============================================================
 * A. Lifecycle
 * ============================================================ */

static void test_a1_loop_new_free(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);
    assert(loop != NULL);
    assert(!px_loop_is_paused(loop));
    assert(px_loop_audit_count(loop) == 0);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_a2_loop_new_null_args(void) {
    /* NULL closure or perception should fail. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    assert(px_loop_new(NULL, p) == NULL);

    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    assert(px_loop_new(c, NULL) == NULL);

    px_closure_free(c);
    px_perception_free(p);
    px_estimate_free(e);
}

/* ============================================================
 * B. Step
 * ============================================================ */

static void test_b1_step_triggers_and_perceives(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    g_perception_count = 0;
    int r = px_loop_step(loop, NULL, 0);  /* trigger with no payload */
    assert(r == 1);  /* perception ran */
    assert(px_estimate_value(e) == 1.0);  /* closure triggered */
    assert(g_perception_count == 1);  /* perception fired once */
    assert(px_loop_audit_count(loop) == 1);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_b2_step_view_only(void) {
    px_estimate* e = px_estimate_new(42, 1.0);
    px_closure* c = px_closure_new("noop", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    g_perception_count = 0;
    int r = px_loop_step_view_only(loop);
    assert(r == 1);
    assert(px_estimate_value(e) == 42);  /* unchanged — closure NOT triggered */
    assert(g_perception_count == 1);  /* perception fired */
    assert(px_loop_audit_count(loop) == 1);

    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.closure_triggered == false);
    assert(entry.perception_invoked == true);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_b3_step_multiple_iterations(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    g_perception_count = 0;
    for (int i = 0; i < 5; i++) {
        px_loop_step(loop, NULL, 0);
    }
    assert(px_estimate_value(e) == 5.0);
    assert(g_perception_count == 5);
    assert(px_loop_audit_count(loop) == 5);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * C. Pause / resume
 * ============================================================ */

static void test_c1_pause_blocks_step(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);
    assert(!px_loop_is_paused(loop));

    px_loop_pause(loop);
    assert(px_loop_is_paused(loop));

    g_perception_count = 0;
    int r = px_loop_step(loop, NULL, 0);
    assert(r == 0);  /* paused — no-op */
    assert(px_estimate_value(e) == 0);  /* unchanged */
    assert(g_perception_count == 0);  /* perception NOT fired */
    assert(px_loop_audit_count(loop) == 0);  /* no audit entry */

    px_loop_resume(loop);
    assert(!px_loop_is_paused(loop));
    r = px_loop_step(loop, NULL, 0);
    assert(r == 1);
    assert(px_estimate_value(e) == 1.0);
    assert(px_loop_audit_count(loop) == 1);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * D. Audit
 * ============================================================ */

static void test_d1_audit_records_correctly(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    /* 3 trigger+perceive iterations, then 2 view-only. */
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    px_loop_step_view_only(loop);
    px_loop_step_view_only(loop);

    assert(px_loop_audit_count(loop) == 5);

    px_loop_audit_entry entries[5];
    int n = px_loop_audit_get(loop, entries, 5);
    assert(n == 5);

    /* Chronological order: first 3 have trigger+perceive, last 2 perceive only. */
    assert(entries[0].closure_triggered == true);
    assert(entries[0].perception_invoked == true);
    assert(entries[1].closure_triggered == true);
    assert(entries[2].closure_triggered == true);
    assert(entries[3].closure_triggered == false);  /* view-only */
    assert(entries[3].perception_invoked == true);
    assert(entries[4].closure_triggered == false);

    /* Timestamps should be monotonic non-decreasing. */
    assert(entries[1].timestamp_ms >= entries[0].timestamp_ms);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_d2_audit_clear(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    assert(px_loop_audit_count(loop) == 2);

    px_loop_audit_clear(loop);
    assert(px_loop_audit_count(loop) == 0);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * E. Replay
 * ============================================================ */

static void test_e1_replay_last_n(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    /* 3 trigger iterations: count goes 0→1→2→3 */
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    assert(px_estimate_value(e) == 3.0);

    /* Replay last 2: re-triggers 2 times, count goes 3→4→5 */
    g_perception_count = 0;
    int replayed = px_loop_replay(loop, 2);
    assert(replayed == 2);
    assert(px_estimate_value(e) == 5.0);
    assert(g_perception_count == 2);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_e2_replay_more_than_stored(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    /* Only 2 stored, ask for 5 — should clamp to 2. */

    int replayed = px_loop_replay(loop, 5);
    assert(replayed == 2);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * F. Essence — what Feedback enables that 4 abstractions alone can't
 * ============================================================ */

static void test_f1_audit_visibility(void) {
    /* Without Feedback, you can't ask "which perception fired after which trigger?"
     * With Feedback, the audit log answers exactly that. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    /* Mix of trigger and view-only iterations. */
    px_loop_step(loop, NULL, 0);          /* trigger + perceive */
    px_loop_step_view_only(loop);         /* perceive only */
    px_loop_step(loop, NULL, 0);          /* trigger + perceive */

    /* Audit can answer: "was the 2nd iteration a trigger?"
     * Without Feedback, this is impossible to query. */
    px_loop_audit_entry entries[3];
    int n = px_loop_audit_get(loop, entries, 3);
    assert(n == 3);
    assert(entries[1].closure_triggered == false);  /* 2nd was view-only */

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_f2_interruption(void) {
    /* Without Feedback, you can't pause the trigger→perceive loop.
     * With Feedback, px_loop_pause lets you batch updates without
     * re-rendering between each. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    /* Pause, do 3 closures directly (state changes), then resume and
     * do 1 step. Perception should only fire once (after resume),
     * not 3 times during pause. */
    px_loop_pause(loop);
    px_closure_trigger(c, NULL, 0);
    px_closure_trigger(c, NULL, 0);
    px_closure_trigger(c, NULL, 0);
    /* While paused, loop step is no-op. */
    px_loop_step(loop, NULL, 0);
    assert(px_estimate_value(e) == 3.0);

    /* Resume — one step now triggers perception. */
    g_perception_count = 0;
    px_loop_resume(loop);
    px_loop_step(loop, NULL, 0);
    assert(g_perception_count == 1);  /* only 1 perception, not 4 */

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_f3_replay_for_testing(void) {
    /* Without Feedback, you can't replay a trigger→perceive sequence.
     * With Feedback, px_loop_replay lets you re-run recorded iterations
     * for testing/debugging. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_count, srcs, 1, NULL);

    px_loop* loop = px_loop_new(c, p);

    /* Record 3 iterations. */
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    double v1 = px_estimate_value(e);  /* should be 3 */

    /* Reset state, replay. */
    px_estimate_set(e, 0, 1.0);
    g_perception_count = 0;
    int replayed = px_loop_replay(loop, 3);
    assert(replayed == 3);
    assert(px_estimate_value(e) == v1);  /* same final state */
    assert(g_perception_count == 3);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex Feedback (px_loop) test suite — v0.4\n");
    printf("=============================================\n");
    printf("Tests the 5th essence category: closed-loop coupling.\n");
    printf("Per essence-derivation-v2.md and ADR-0008.\n\n");

    printf("[A] Lifecycle\n");
    TEST(a1_loop_new_free);
    TEST(a2_loop_new_null_args);

    printf("\n[B] Step\n");
    TEST(b1_step_triggers_and_perceives);
    TEST(b2_step_view_only);
    TEST(b3_step_multiple_iterations);

    printf("\n[C] Pause / resume\n");
    TEST(c1_pause_blocks_step);

    printf("\n[D] Audit\n");
    TEST(d1_audit_records_correctly);
    TEST(d2_audit_clear);

    printf("\n[E] Replay\n");
    TEST(e1_replay_last_n);
    TEST(e2_replay_more_than_stored);

    printf("\n[F] Essence — what Feedback enables\n");
    TEST(f1_audit_visibility);
    TEST(f2_interruption);
    TEST(f3_replay_for_testing);

    printf("\n----------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    printf("\nFeedback is the 5th essence category. With v0.4, Planex\n");
    printf("implements 5 of 5 essence categories (was 4 of 5 in v0.3).\n");
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
