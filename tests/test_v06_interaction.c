/*
 * test_v06_interaction.c — interaction + leak-retire test suite
 *
 * Validates two layers of the v0.6→v0.7 cycle:
 *
 *   A-F. Interaction (the 7th canonical abstraction, ADR-0018;
 *        landed v0.6 per ADR-0016):
 *     A. Lifecycle — begin/sample/commit/cancel phase machine
 *     B. Trajectory — ring retention, ordering, measures
 *     C. Bridges — phase hook, commit→Closure, phase→Estimate
 *     D. THE INVARIANT — samples are inert (no observer fan-out,
 *        no perception auto-invoke) — what makes a process NOT state
 *        (normative since ADR-0018: this section is the enforcement)
 *     E. Region + affordance — intent compilation as graph query
 *     F. Gesture derivation — swipe/cancel from trajectory measures
 *
 *   G-J. v0.6 leak retires + fixes (audit findings):
 *     G. confidence predictive loop (predict → set → surprise)
 *     H. representamen free_fn (no-leak ownership)
 *     I. a11y query side (assertable announcements)
 *     J. loop feedback budget (iteration_ms / budget_exceeded)
 *
 * Build (or: make test_v06):
 *   cc -std=c17 -I include tests/test_v06_interaction.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c -lm -o build/test_v06_interaction
 */
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
#include <math.h>

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
 * Shared helpers
 * ============================================================ */

static bool eval_true(void* u) { (void)u; return true; }

static void on_inc(px_intent i, void* u) {
    (void)i;
    px_estimate* e = (px_estimate*)u;
    px_estimate_set(e, px_estimate_value(e) + 1.0, 1.0);
}

/* Counting observer (for the inertness invariant). */
static int g_observer_fires = 0;
static void counting_observer(px_estimate* e, void* u) {
    (void)e; (void)u;
    g_observer_fires++;
}

/* Counting perception (for the inertness invariant). */
static int g_perception_fires = 0;
static void* counting_perception(px_estimate* const* in, int n, void* u) {
    (void)in; (void)n; (void)u;
    g_perception_fires++;
    return NULL;
}

static px_int_sample mk_sample(double t, double x, double y) {
    px_int_sample s = { t, x, y, 0.0, 0, 0 };
    return s;
}

/* Hook that captures the sample visible at BEGAN (for a5). */
static void capture_first_sample_hook(px_interaction* it, px_int_phase phase, void* user) {
    if (phase != PX_INT_BEGAN) return;
    const px_int_sample* s = px_interaction_last(it);
    if (s) *(px_int_sample*)user = *s;
}

/* ============================================================
 * A. Interaction lifecycle
 * ============================================================ */

static void test_a1_lifecycle_phases(void) {
    px_interaction* it = px_interaction_new("tap", 8);
    assert(it != NULL);
    assert(px_interaction_phase(it) == PX_INT_IDLE);

    px_interaction_begin(it);
    assert(px_interaction_phase(it) == PX_INT_BEGAN);
    assert(px_interaction_stored(it) == 0);

    px_int_sample s = mk_sample(10.0, 5.0, 5.0);
    px_interaction_sample(it, &s);
    assert(px_interaction_phase(it) == PX_INT_ACTIVE);
    assert(px_interaction_stored(it) == 1);
    assert(px_interaction_total(it) == 1);

    px_interaction_commit(it);
    assert(px_interaction_phase(it) == PX_INT_COMMITTED);

    /* Terminal ops are idempotent no-ops. */
    px_interaction_commit(it);
    px_interaction_cancel(it, "late cancel ignored");
    px_int_sample late = mk_sample(99.0, 0.0, 0.0);
    px_interaction_sample(it, &late);
    assert(px_interaction_phase(it) == PX_INT_COMMITTED);
    assert(px_interaction_total(it) == 1);

    px_interaction_free(it);
}

static void test_a2_cancel_records_reason(void) {
    px_interaction* it = px_interaction_new("drag", 8);
    px_int_sample s = mk_sample(1.0, 1.0, 1.0);
    px_interaction_sample(it, &s);   /* auto-begin */
    assert(px_interaction_phase(it) == PX_INT_ACTIVE);

    px_interaction_cancel(it, "esc pressed");
    assert(px_interaction_phase(it) == PX_INT_CANCELLED);
    assert(strcmp(px_interaction_cancel_reason(it), "esc pressed") == 0);

    px_interaction_free(it);
}

static void test_a3_auto_begin_on_sample(void) {
    px_interaction* it = px_interaction_new("hover", 4);
    px_int_sample s = mk_sample(0.0, 10.0, 10.0);
    px_interaction_sample(it, &s);
    assert(px_interaction_phase(it) == PX_INT_ACTIVE);  /* skipped BEGAN */
    assert(px_interaction_total(it) == 1);

    /* The beginning event belongs to the trajectory: a BEGAN hook
     * sees the first sample (this is what makes drag-origin capture
     * possible at auto-begin). */
    const px_int_sample* first = px_interaction_last(it);
    assert(first != NULL && first->x == 10.0);

    px_interaction_free(it);
}

static void test_a5_began_hook_reads_first_sample(void) {
    px_interaction* it = px_interaction_new("drag", 4);
    static px_int_sample seen = { 0 };
    px_interaction_on_phase(it, capture_first_sample_hook, &seen);

    px_int_sample s = mk_sample(5.0, 42.0, 88.0);
    px_interaction_sample(it, &s);   /* auto-begin → hook fires */
    assert(seen.x == 42.0 && seen.y == 88.0);

    px_interaction_free(it);
}

static void test_a4_phase_names(void) {
    assert(strcmp(px_int_phase_str(PX_INT_IDLE), "IDLE") == 0);
    assert(strcmp(px_int_phase_str(PX_INT_COMMITTED), "COMMITTED") == 0);
    assert(strcmp(px_int_phase_str((px_int_phase)99), "?") == 0);
}

/* ============================================================
 * B. Trajectory — ring retention + measures
 * ============================================================ */

static void test_b1_ring_retention(void) {
    /* capacity 4, feed 10 → stored clamps to 4, total counts 10. */
    px_interaction* it = px_interaction_new("scroll", 4);
    for (int i = 0; i < 10; i++) {
        px_int_sample s = mk_sample((double)i, (double)i, 0.0);
        px_interaction_sample(it, &s);
    }
    assert(px_interaction_total(it) == 10);
    assert(px_interaction_stored(it) == 4);

    /* Oldest retained is sample 6 (t=6); newest is t=9. */
    const px_int_sample* first = px_interaction_at(it, 0);
    const px_int_sample* last  = px_interaction_last(it);
    assert(first != NULL && last != NULL);
    assert(first->t_ms == 6.0);
    assert(last->t_ms == 9.0);

    /* Out-of-range queries return NULL. */
    assert(px_interaction_at(it, 4) == NULL);
    assert(px_interaction_at(it, -1) == NULL);

    /* Last on empty interaction is NULL. */
    px_interaction* e = px_interaction_new("empty", 4);
    assert(px_interaction_last(e) == NULL);
    px_interaction_free(e);
    px_interaction_free(it);
}

static void test_b2_measures(void) {
    /* Square path: (0,0)→(3,0)→(3,4). displacement=5, path=7. */
    px_interaction* it = px_interaction_new("path", 8);
    px_int_sample a = mk_sample(0.0, 0.0, 0.0);
    px_int_sample b = mk_sample(100.0, 3.0, 0.0);
    px_int_sample c = mk_sample(200.0, 3.0, 4.0);
    px_interaction_sample(it, &a);
    px_interaction_sample(it, &b);
    px_interaction_sample(it, &c);

    assert(fabs(px_interaction_duration_ms(it) - 200.0) < 1e-9);
    assert(fabs(px_interaction_displacement(it) - 5.0) < 1e-9);
    assert(fabs(px_interaction_path_length(it) - 7.0) < 1e-9);
    /* Last leg: 4 units in 100ms → 0.04 units/ms. */
    assert(fabs(px_interaction_velocity(it) - 0.04) < 1e-9);

    px_interaction_free(it);
}

/* ============================================================
 * C. Bridges — hook / closure / estimate
 * ============================================================ */

static int g_hook_phases[8];
static int g_hook_count = 0;
static void recording_hook(px_interaction* it, px_int_phase phase, void* user) {
    (void)user;
    assert(it != NULL);
    g_hook_phases[g_hook_count++] = (int)phase;
}

static void test_c1_hook_fires_at_transitions_only(void) {
    px_interaction* it = px_interaction_new("drag", 8);
    g_hook_count = 0;
    px_interaction_on_phase(it, recording_hook, NULL);

    px_interaction_begin(it);
    for (int i = 0; i < 100; i++) {           /* 100 samples... */
        px_int_sample s = mk_sample((double)i, (double)i, 1.0);
        px_interaction_sample(it, &s);
    }
    px_interaction_commit(it);

    /* ...but the hook fired exactly TWICE: BEGAN + COMMITTED. */
    assert(g_hook_count == 2);
    assert(g_hook_phases[0] == (int)PX_INT_BEGAN);
    assert(g_hook_phases[1] == (int)PX_INT_COMMITTED);

    px_interaction_free(it);
}

static void test_c2_commit_triggers_closure_with_payload(void) {
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("commit reorder", PX_INTENT_DECLARE,
                                    on_inc, eval_true, count);

    /* Payload copied at bind time — intent-as-value. */
    int payload = 42;
    px_interaction* it = px_interaction_new("drag", 8);
    px_interaction_on_commit(it, c, &payload, sizeof(payload));

    px_int_sample s = mk_sample(0.0, 1.0, 1.0);
    px_interaction_sample(it, &s);
    px_interaction_commit(it);

    assert(px_estimate_value(count) == 1.0);          /* closure ran   */
    px_intent last = px_closure_last_intent(c);        /* payload held  */
    assert(last.payload_size == sizeof(int));
    assert(last.payload != NULL && *(int*)last.payload == 42);

    px_interaction_free(it);
    px_closure_free(c);
    px_estimate_free(count);
}

static void test_c3_cancel_triggers_closure(void) {
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("abort", PX_INTENT_EXPRESS,
                                    on_inc, eval_true, count);
    px_interaction* it = px_interaction_new("drag", 8);
    px_interaction_on_cancel(it, c);

    px_int_sample s = mk_sample(0.0, 1.0, 1.0);
    px_interaction_sample(it, &s);
    px_interaction_cancel(it, "pointercancel");
    assert(px_estimate_value(count) == 1.0);

    px_interaction_free(it);
    px_closure_free(c);
    px_estimate_free(count);
}

static void test_c4_publish_phase_estimates(void) {
    px_estimate* phase_est = px_estimate_new(0, 1.0);
    g_observer_fires = 0;
    px_estimate_observe(phase_est, counting_observer, NULL);

    px_interaction* it = px_interaction_new("drag", 8);
    px_interaction_publish_phase(it, phase_est);

    px_interaction_begin(it);
    assert(px_estimate_value(phase_est) == (double)PX_INT_BEGAN);

    px_int_sample s = mk_sample(0.0, 1.0, 1.0);
    px_interaction_sample(it, &s);
    /* ACTIVE is a phase change but NOT a transition callback — the
     * estimate is only published at hook-grade transitions. */
    px_int_sample s2 = mk_sample(1.0, 2.0, 1.0);
    px_interaction_sample(it, &s2);
    assert(g_observer_fires == 1);  /* BEGAN only so far */

    px_interaction_commit(it);
    assert(px_estimate_value(phase_est) == (double)PX_INT_COMMITTED);
    assert(g_observer_fires == 2);  /* BEGAN + COMMITTED */

    px_interaction_free(it);
    px_estimate_free(phase_est);
}

/* ============================================================
 * D. THE INVARIANT — the sample hot path is inert
 * ============================================================ */

static void test_d1_samples_do_not_fire_observers(void) {
    /* hover_drag_4abs.c HACK 1/2: mouse position as Estimate fired
     * observer fan-out on EVERY mouse move. The interaction's sample
     * path must not. */
    px_estimate* unrelated = px_estimate_new(0, 1.0);
    g_observer_fires = 0;
    px_estimate_observe(unrelated, counting_observer, NULL);

    px_interaction* it = px_interaction_new("mousemove", 8);
    for (int i = 0; i < 1000; i++) {
        px_int_sample s = mk_sample((double)i, (double)i, (double)i);
        px_interaction_sample(it, &s);
    }
    assert(g_observer_fires == 0);   /* inert: zero observer fan-out */

    px_interaction_free(it);
    px_estimate_free(unrelated);
}

static void test_d2_samples_do_not_invoke_perceptions(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("watch", counting_perception,
                                          srcs, 1, NULL);
    g_perception_fires = 0;

    px_interaction* it = px_interaction_new("mousemove", 8);
    for (int i = 0; i < 1000; i++) {
        px_int_sample s = mk_sample((double)i, (double)i, (double)i);
        px_interaction_sample(it, &s);
    }
    assert(g_perception_fires == 0); /* inert: zero perception fires */

    px_interaction_free(it);
    px_perception_free(p);
    px_estimate_free(e);
}

static void test_d3_publish_is_the_only_estimate_seam(void) {
    /* The complete matrix: 1000 samples → 0 estimate writes;
     * transitions (begin, commit) → exactly 1 write each. */
    px_estimate* phase_est = px_estimate_new(0, 1.0);
    g_observer_fires = 0;
    px_estimate_observe(phase_est, counting_observer, NULL);

    px_interaction* it = px_interaction_new("drag", 16);
    px_interaction_publish_phase(it, phase_est);
    px_interaction_begin(it);               /* BEGAN → 1 write */
    assert(g_observer_fires == 1);
    for (int i = 0; i < 1000; i++) {
        px_int_sample s = mk_sample((double)i, (double)i, 1.0);
        px_interaction_sample(it, &s);
    }
    assert(g_observer_fires == 1);          /* 1000 samples → 0 writes */
    px_interaction_commit(it);
    assert(g_observer_fires == 2);          /* exactly one more */

    px_interaction_free(it);
    px_estimate_free(phase_est);
}

/* ============================================================
 * E. Region + affordance (intent compilation)
 * ============================================================ */

static void test_e1_region_containment_topmost(void) {
    px_region* a = px_region_new(px_rect_make(0, 0, 100, 100), "a");
    px_region* b = px_region_new(px_rect_make(50, 50, 100, 100), "b"); /* later = on top */

    assert(px_region_at(10, 10) == a);
    assert(px_region_at(149, 149) == b);    /* [50,150) half-open */
    assert(px_region_at(75, 75) == b);      /* overlap → topmost (b) */
    assert(px_region_at(500, 500) == NULL); /* miss */

    /* In-place re-layout keeps identity. */
    px_region_set_rect(a, px_rect_make(0, 0, 10, 10));
    assert(px_region_at(75, 75) == b);      /* a no longer covers it */
    assert(px_region_at(5, 5) == a);

    px_region_free(b);
    px_region_free(a);
}

static void test_e2_afford_at_compiles_intent(void) {
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("increment", PX_INTENT_REQUEST,
                                      on_inc, eval_true, count);

    px_region* r = px_region_new(px_rect_make(20, 40, 280, 32), "inc");
    px_declare(g, r, PX_REL_AFFORDS, inc);

    /* The compile step: physical (x,y) → semantic closure. */
    px_closure* found = px_afford_at(g, 100, 50);
    assert(found == inc);

    /* Miss → NULL (no affordance there). */
    assert(px_afford_at(g, 5, 5) == NULL);
    assert(px_afford_at(NULL, 100, 50) == NULL);

    /* The compiled intent is triggerable and lands in the estimate. */
    px_closure_trigger(found, NULL, 0);
    assert(px_estimate_value(count) == 1.0);

    /* The relation is a normal graph citizen — queryable. */
    assert(px_has_relation(g, r, PX_REL_AFFORDS, inc));

    px_region_free(r);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

/* ============================================================
 * F. Gesture derivation from trajectory measures
 * ============================================================ */

static void test_f1_swipe_derivation(void) {
    /* A swipe = high velocity + high displacement at commit.
     * Derivable from pure trajectory queries — no gesture hardcode. */
    px_interaction* it = px_interaction_new("swipe", 32);
    for (int i = 0; i < 8; i++) {
        px_int_sample s = mk_sample((double)i * 8.0, (double)i * 40.0, 0.0);
        px_interaction_sample(it, &s);
    }
    px_interaction_commit(it);

    double v = px_interaction_velocity(it);        /* 40px / 8ms = 5.0 */
    double d = px_interaction_displacement(it);    /* 280            */
    assert(v > 1.0 && d > 100.0);                  /* "it's a swipe" */

    px_interaction_free(it);
}

static void test_f2_tap_vs_drag(void) {
    /* A tap = tiny displacement; a drag = real path. The same
     * interaction object distinguishes them by MEASURE, not by mode
     * flag — this is what hover_drag_4abs.c could not express. */
    px_interaction* tap = px_interaction_new("tap", 8);
    px_int_sample t1 = mk_sample(0.0, 100.0, 100.0);
    px_int_sample t2 = mk_sample(40.0, 101.0, 100.0);
    px_interaction_sample(tap, &t1);
    px_interaction_sample(tap, &t2);
    px_interaction_commit(tap);
    assert(px_interaction_displacement(tap) < 4.0);

    px_interaction* drag = px_interaction_new("drag", 64);
    for (int i = 0; i < 50; i++) {
        px_int_sample s = mk_sample((double)i * 10.0,
                                     100.0, 100.0 + (double)i * 6.0);
        px_interaction_sample(drag, &s);
    }
    px_interaction_commit(drag);
    assert(px_interaction_displacement(drag) > 200.0);

    px_interaction_free(tap);
    px_interaction_free(drag);
}

/* ============================================================
 * G. confidence predictive loop (v0.6)
 * ============================================================ */

static void test_g1_predict_accuracy_keeps_confidence(void) {
    px_estimate* e = px_estimate_new(25.0, 0.9);
    px_estimate_predict(e, 25.1, 2.0);      /* tolerance 2.0 */
    px_estimate_set(e, 25.2, 1.0);          /* surprise 0.1  */

    assert(fabs(px_estimate_surprise(e) - 0.1) < 1e-9);
    assert(px_estimate_confidence(e) > 0.95);   /* ~exp(-0.05) */

    px_estimate_free(e);
}

static void test_g2_predict_violation_decays_confidence(void) {
    px_estimate* e = px_estimate_new(25.0, 0.9);
    px_estimate_predict(e, 25.0, 2.0);
    px_estimate_set(e, 35.0, 1.0);          /* surprise 10 — 5× tolerance */

    assert(fabs(px_estimate_surprise(e) - 10.0) < 1e-9);
    /* exp(-5) ≈ 0.0067 — the framework sides with the prediction. */
    assert(px_estimate_confidence(e) < 0.01);
    assert(px_estimate_confidence(e) > 0.0);

    px_estimate_free(e);
}

static void test_g3_prediction_is_one_shot(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_predict(e, 100.0, 1.0);
    px_estimate_set(e, 0.0, 1.0);           /* resolves (surprise 100) */
    double after_first = px_estimate_confidence(e);
    assert(after_first < 0.001);

    px_estimate_set(e, 0.0, 1.0);           /* no prediction pending */
    assert(px_estimate_confidence(e) == 1.0);   /* caller's value stands */
    assert(px_estimate_surprise(e) == 0.0);     /* surprise reset       */

    px_estimate_free(e);
}

static void test_g4_no_prediction_no_surprise(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_set(e, 7.0, 0.5);
    assert(px_estimate_surprise(e) == 0.0);
    assert(px_estimate_confidence(e) == 0.5);
    px_estimate_free(e);
}

/* ============================================================
 * H. representamen free_fn (v0.6 leak retire)
 * ============================================================ */

static int g_freed_representamens = 0;
static void free_counting(void* rep) {
    (void)rep;
    g_freed_representamens++;
}

static void* perceiving_malloc(px_estimate* const* in, int n, void* u) {
    (void)in; (void)n; (void)u;
    /* Every fire leaks a malloc'd denotation — v0.5 had NO way to
     * reclaim these. With free_fn the perception reclaims them. */
    return malloc(64);
}

static void test_h1_free_fn_reclaims_representamens(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("leaky", perceiving_malloc,
                                          srcs, 1, NULL);
    px_perception_set_free_fn(p, free_counting);

    g_freed_representamens = 0;
    for (int i = 0; i < 5; i++) {
        px_estimate_set(e, (double)i, 1.0);   /* auto-invoke fires fn */
    }
    /* Each set fires the perception; the previous cache is freed
     * through the destructor on the NEXT fire (4 reclaimable). */
    px_perception_free(p);                     /* frees the 5th       */
    assert(g_freed_representamens == 5);

    px_estimate_free(e);
}

/* ============================================================
 * I. a11y query side (v0.6)
 * ============================================================ */

static void test_i1_a11y_query_side(void) {
    px_a11y* a = px_a11y_new(NULL);
    px_a11y_set_verbose(a, false);   /* silence stderr — data survives */

    px_a11y_set_role(a, PX_A11Y_ROLE_BUTTON);
    px_a11y_set_name(a, "Increment counter");
    px_a11y_set_value(a, "42");
    px_a11y_set_state(a, PX_A11Y_STATE_ENABLED | PX_A11Y_STATE_FOCUSED);

    assert(px_a11y_get_role(a) == PX_A11Y_ROLE_BUTTON);
    assert(strcmp(px_a11y_get_name(a), "Increment counter") == 0);
    assert(strcmp(px_a11y_get_value(a), "42") == 0);
    assert(px_a11y_get_state(a) ==
           (PX_A11Y_STATE_ENABLED | PX_A11Y_STATE_FOCUSED));
    assert(px_a11y_is_verbose(a) == false);

    px_a11y_announce(a, "Counter incremented to 43");
    px_a11y_announce(a, "Counter incremented to 44");

    assert(px_a11y_announcement_count(a) == 2);
    assert(strcmp(px_a11y_announcement(a, 0),
                  "Counter incremented to 43") == 0);
    assert(strcmp(px_a11y_announcement(a, 1),
                  "Counter incremented to 44") == 0);
    assert(px_a11y_announcement(a, 2) == NULL);

    px_a11y_free(a);
}

static void test_i2_a11y_announcement_ring_wraps(void) {
    px_a11y* a = px_a11y_new(NULL);
    px_a11y_set_verbose(a, false);

    char msg[32];
    for (int i = 0; i < PX_A11Y_ANNOUNCE_CAPACITY + 5; i++) {
        snprintf(msg, sizeof(msg), "msg %d", i);
        px_a11y_announce(a, msg);
    }
    /* Ring retains the newest CAPACITY messages. */
    assert(px_a11y_announcement_count(a) == PX_A11Y_ANNOUNCE_CAPACITY);
    snprintf(msg, sizeof(msg), "msg %d", PX_A11Y_ANNOUNCE_CAPACITY + 4);
    assert(strcmp(px_a11y_announcement(
                      a, PX_A11Y_ANNOUNCE_CAPACITY - 1), msg) == 0);

    px_a11y_free(a);
}

/* ============================================================
 * J. loop feedback budget (v0.6)
 * ============================================================ */

static void* perceive_null(px_estimate* const* in, int n, void* u) {
    (void)in; (void)n; (void)u;
    return NULL;
}

static void test_j1_budget_audit_dimensions(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
                                    on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    /* v0.7 Line 2: budget is a CONTRACT — the default is one 60fps
     * frame, not 0. (v0.6 had no default; j1 originally asserted 0.) */
    assert(px_loop_budget(loop) == PX_LOOP_DEFAULT_BUDGET_MS);
    px_loop_set_budget(loop, 16.0);
    assert(px_loop_budget(loop) == 16.0);

    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);

    px_loop_audit_entry entries[4];
    int n = px_loop_audit_get(loop, entries, 4);
    assert(n == 2);
    for (int i = 0; i < n; i++) {
        assert(entries[i].budget_ms == 16.0);
        assert(entries[i].iteration_ms >= 0.0);
        /* A trivial action must not exceed a 16ms budget. */
        assert(entries[i].budget_exceeded == false);
    }

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_j2_budget_explicit_opt_out(void) {
    /* v0.7 Line 2: renamed semantics — the budget is ON by default;
     * disabling is an explicit opt-out. A loop without a deadline is
     * a decision, not an accident. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
                                    on_inc, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    /* Default: one frame. */
    assert(px_loop_budget(loop) == PX_LOOP_DEFAULT_BUDGET_MS);
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.budget_ms == PX_LOOP_DEFAULT_BUDGET_MS);
    assert(entry.budget_exceeded == false);
    assert(entry.iteration_ms >= 0.0);

    /* Explicit opt-out: 0 means "no deadline, on purpose". */
    px_loop_set_budget(loop, 0.0);
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry entries2[2];
    assert(px_loop_audit_get(loop, entries2, 2) == 2);
    assert(entries2[0].budget_ms == PX_LOOP_DEFAULT_BUDGET_MS); /* first */
    assert(entries2[1].budget_ms == 0.0);                      /* opt-out */
    assert(entries2[1].budget_exceeded == false);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex v0.6 prototype + leak-retire verification\n");
    printf("=================================================\n");
    printf("Interaction (7th canonical per ADR-0018; prototype protocol per ADR-0016) + audit fixes.\n\n");

    printf("[A] Interaction lifecycle\n");
    TEST(a1_lifecycle_phases);
    TEST(a2_cancel_records_reason);
    TEST(a3_auto_begin_on_sample);
    TEST(a4_phase_names);
    TEST(a5_began_hook_reads_first_sample);

    printf("\n[B] Trajectory\n");
    TEST(b1_ring_retention);
    TEST(b2_measures);

    printf("\n[C] Bridges\n");
    TEST(c1_hook_fires_at_transitions_only);
    TEST(c2_commit_triggers_closure_with_payload);
    TEST(c3_cancel_triggers_closure);
    TEST(c4_publish_phase_estimates);

    printf("\n[D] THE INVARIANT — hot path is inert\n");
    TEST(d1_samples_do_not_fire_observers);
    TEST(d2_samples_do_not_invoke_perceptions);
    TEST(d3_publish_is_the_only_estimate_seam);

    printf("\n[E] Region + affordance (intent compilation)\n");
    TEST(e1_region_containment_topmost);
    TEST(e2_afford_at_compiles_intent);

    printf("\n[F] Gesture derivation\n");
    TEST(f1_swipe_derivation);
    TEST(f2_tap_vs_drag);

    printf("\n[G] confidence predictive loop\n");
    TEST(g1_predict_accuracy_keeps_confidence);
    TEST(g2_predict_violation_decays_confidence);
    TEST(g3_prediction_is_one_shot);
    TEST(g4_no_prediction_no_surprise);

    printf("\n[H] representamen free_fn\n");
    TEST(h1_free_fn_reclaims_representamens);

    printf("\n[I] a11y query side\n");
    TEST(i1_a11y_query_side);
    TEST(i2_a11y_announcement_ring_wraps);

    printf("\n[J] loop feedback budget\n");
    TEST(j1_budget_audit_dimensions);
    TEST(j2_budget_explicit_opt_out);

    printf("\n----------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    printf("\nWhat this suite closes:\n");
    printf("  - Interaction: hover/drag/gesture as PROCESS, hot path inert\n");
    printf("  - Intent compilation: affordance query (D-A1 audit finding)\n");
    printf("  - confidence: predictive loop, framework-side consumer\n");
    printf("  - representamen leak: free_fn ownership\n");
    printf("  - a11y: assertable query side (bridges still stubbed)\n");
    printf("  - feedback: iteration_ms + budget in the audit\n");
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
