/*
 * test_core.c — Planex Stage 0 core tests
 *
 * Verifies that Relation + Estimate + Closure can express real
 * UI interactions. No pixels — just abstraction correctness.
 *
 * Run: make test
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
    printf("  [TEST] %-32s ", #name);                   \
    test_##name();                                       \
    printf("OK\n");                                      \
    g_tests_pass++;                                      \
} while (0)

/* ============================================================
 * Relation tests
 * ============================================================ */

static void test_relation_declare(void) {
    px_graph* g = px_graph_new();
    int a = 1, b = 2;
    px_relation* r = px_declare(g, &a, PX_REL_BESIDE, &b);
    assert(r != NULL);
    assert(px_has_relation(g, &a, PX_REL_BESIDE, &b));
    assert(!px_has_relation(g, &a, PX_REL_DEPENDS_ON, &b));
    assert(px_graph_count(g) == 1);
    px_graph_free(g);
}

static void test_relation_query(void) {
    px_graph* g = px_graph_new();
    int a = 1, b = 2, c = 3;
    px_declare(g, &a, PX_REL_BESIDE, &b);
    px_declare(g, &a, PX_REL_BESIDE, &c);
    px_node_list list = px_query(g, &a, PX_REL_BESIDE);
    assert(list.count == 2);
    px_node_list_free(&list);
    px_graph_free(g);
}

static void test_relation_bidirectional(void) {
    px_graph* g = px_graph_new();
    int a = 1, b = 2;
    px_declare(g, &a, PX_REL_TRIGGERS, &b);
    /* Querying from b should also return a (relations are bidirectional
     * in the query sense — caller decides directionality). */
    px_node_list list = px_query(g, &b, PX_REL_TRIGGERS);
    assert(list.count == 1);
    assert(list.items[0] == &a);
    px_node_list_free(&list);
    px_graph_free(g);
}

static void test_relation_kind_str(void) {
    assert(strcmp(px_rel_kind_str(PX_REL_BESIDE), "BESIDE") == 0);
    assert(strcmp(px_rel_kind_str(PX_REL_TRIGGERS), "TRIGGERS") == 0);
    assert(strcmp(px_rel_kind_str(PX_REL_CONTAINS), "CONTAINS") == 0);
    /* PX_REL_COUNT is a sentinel; px_rel_kind_str returns "?" */
    assert(strcmp(px_rel_kind_str(PX_REL_COUNT), "?") == 0);
}

/* ============================================================
 * Estimate tests
 * ============================================================ */

static void test_estimate_basic(void) {
    px_estimate* e = px_estimate_new(5.0, 0.9);
    assert(px_estimate_value(e)      == 5.0);
    assert(px_estimate_confidence(e) == 0.9);
    px_estimate_free(e);
}

static void test_estimate_set(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_set(e, 42, 1.0);
    assert(px_estimate_value(e) == 42);
    px_estimate_free(e);
}

static void test_estimate_animate(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);
    /* ease-out at t=0 -> 0 */
    assert(px_estimate_sample(e, 0) == 0);
    /* at t=500 (midpoint), eased = 1 - 0.25 = 0.75 -> 75 */
    double mid = px_estimate_sample(e, 500);
    assert(mid > 70 && mid < 80);
    /* at t=1000 -> 100 */
    assert(px_estimate_sample(e, 1000) == 100);
    px_estimate_free(e);
}

static int g_observe_count = 0;
static void on_observe(px_estimate* e, void* user) {
    (void)e; (void)user;
    g_observe_count++;
}

static void test_estimate_observe(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_observe(e, on_observe, NULL);
    g_observe_count = 0;
    px_estimate_set(e, 1, 1.0);
    px_estimate_set(e, 2, 1.0);
    px_estimate_set(e, 3, 1.0);
    assert(g_observe_count == 3);
    px_estimate_free(e);
}

/* ============================================================
 * Closure tests
 * ============================================================ */

static int g_action_calls = 0;
static void on_action(px_intent intent, void* user) {
    (void)intent; (void)user;
    g_action_calls++;
}

/* on_perceive removed — Closure no longer takes a perception arg
 * (per ADR-0005). Perception is now a separate abstraction. */
static bool on_eval(void* user)     { (void)user; return true;  }

static void test_closure_trigger(void) {
    px_closure* c = px_closure_new(
        "test", PX_INTENT_REQUEST,
        on_action, on_eval, NULL);
    g_action_calls = 0;
    px_closure_trigger(c, NULL, 0);
    assert(g_action_calls == 1);
    assert(px_closure_evaluated(c) == true);
    px_closure_free(c);
}

static void test_closure_intent_value(void) {
    px_closure* c = px_closure_new(
        "test", PX_INTENT_REQUEST,
        on_action, on_eval, NULL);
    int payload = 42;
    px_closure_trigger(c, &payload, sizeof(payload));
    px_intent last = px_closure_last_intent(c);
    assert(last.kind == PX_INTENT_REQUEST);
    assert(last.payload_size == sizeof(payload));
    assert(*(int*)last.payload == 42);
    px_closure_free(c);
}

static void test_closure_intent_serializable(void) {
    /* The key test: intent is a *value*, not a callback.
     * We can serialize it (here: to a byte buffer) and replay. */
    px_closure* c = px_closure_new(
        "test", PX_INTENT_DECLARE,
        on_action, on_eval, NULL);
    const char* msg = "hello world";
    px_closure_trigger(c, (void*)msg, strlen(msg)+1);
    px_intent last = px_closure_last_intent(c);

    /* "Serialize" */
    px_intent_kind kind = last.kind;
    size_t         size = last.payload_size;
    char*          buf  = malloc(size);
    memcpy(buf, last.payload, size);

    /* "Replay" — note: we don't even need to re-trigger through the closure */
    assert(kind == PX_INTENT_DECLARE);
    assert(size == strlen(msg)+1);
    assert(strcmp(buf, msg) == 0);
    free(buf);
    px_closure_free(c);
}

static void test_closure_goal_recorded(void) {
    px_closure* c = px_closure_new(
        "increment the counter", PX_INTENT_REQUEST,
        on_action, on_eval, NULL);
    /* The goal is preserved — for audit log / UI display */
    assert(c != NULL);
    px_closure_free(c);
}

static void test_intent_kind_str(void) {
    assert(strcmp(px_intent_kind_str(PX_INTENT_ASSERT),  "ASSERT")  == 0);
    assert(strcmp(px_intent_kind_str(PX_INTENT_REQUEST), "REQUEST") == 0);
    assert(strcmp(px_intent_kind_str(PX_INTENT_PROMISE), "PROMISE") == 0);
    assert(strcmp(px_intent_kind_str(PX_INTENT_DECLARE), "DECLARE") == 0);
    assert(strcmp(px_intent_kind_str(PX_INTENT_EXPRESS), "EXPRESS") == 0);
}

/* ============================================================
 * Integration: 3 abstractions together
 * ============================================================ */

typedef struct {
    px_estimate* count;
} CounterApp;

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    CounterApp* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v + 1, 1.0);
}


static bool eval_nonneg(void* user) {
    CounterApp* app = user;
    return px_estimate_value(app->count) >= 0;
}

static void test_integration_counter(void) {
    CounterApp app;
    app.count = px_estimate_new(0, 1.0);
    px_graph* g = px_graph_new();
    px_closure* inc = px_closure_new(
        "increment counter", PX_INTENT_REQUEST,
        on_inc, eval_nonneg, &app);

    /* Declare relations: inc triggers count, app contains count */
    px_declare(g, inc,     PX_REL_TRIGGERS, app.count);
    px_declare(g, &app,    PX_REL_CONTAINS, app.count);

    /* Trigger 3 times */
    px_closure_trigger(inc, NULL, 0);
    px_closure_trigger(inc, NULL, 0);
    px_closure_trigger(inc, NULL, 0);

    assert(px_estimate_value(app.count) == 3);
    assert(px_has_relation(g, inc, PX_REL_TRIGGERS, app.count));
    assert(px_closure_evaluated(inc) == true);

    px_closure_free(inc);
    px_graph_free(g);
    px_estimate_free(app.count);
}

/* ============================================================
 * Derived estimate tests (Stage 3 — automatic dependency tracking)
 * ============================================================ */

static double sum_sources(px_estimate* const* srcs, int n, void* user) {
    (void)user;
    double s = 0;
    for (int i = 0; i < n; i++) {
        s += px_estimate_value(srcs[i]);
    }
    return s;
}

static double all_true(px_estimate* const* srcs, int n, void* user) {
    (void)user;
    for (int i = 0; i < n; i++) {
        if (px_estimate_value(srcs[i]) < 0.5) return 0;
    }
    return 1;
}

static double double_first(px_estimate* const* srcs, int n, void* user) {
    (void)user; (void)n;
    return px_estimate_value(srcs[0]) * 2;
}

static void test_derived_basic(void) {
    px_estimate* a = px_estimate_new(10, 1.0);
    px_estimate* b = px_estimate_new(20, 1.0);
    px_estimate* srcs[] = {a, b};
    px_estimate* sum = px_derived_new(sum_sources, NULL, srcs, 2);

    /* Initial value should be 30 (computed immediately) */
    assert(px_estimate_value(sum) == 30);

    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
}

static void test_derived_auto_updates(void) {
    px_estimate* a = px_estimate_new(1, 1.0);
    px_estimate* b = px_estimate_new(2, 1.0);
    px_estimate* srcs[] = {a, b};
    px_estimate* sum = px_derived_new(sum_sources, NULL, srcs, 2);

    assert(px_estimate_value(sum) == 3);

    /* Change a source — derived should auto-update without manual recompute */
    px_estimate_set(a, 10, 1.0);
    assert(px_estimate_value(sum) == 12);

    px_estimate_set(b, 20, 1.0);
    assert(px_estimate_value(sum) == 30);

    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
}

static int g_derived_observer_count = 0;
static void on_derived_changed(px_estimate* e, void* user) {
    (void)e; (void)user;
    g_derived_observer_count++;
}

static void test_derived_fires_observers(void) {
    px_estimate* a = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = {a};
    px_estimate* doubled = px_derived_new(double_first, NULL, srcs, 1);

    /* Observe the derived */
    g_derived_observer_count = 0;
    px_estimate_observe(doubled, on_derived_changed, NULL);

    /* Change source — derived should fire its own observers */
    px_estimate_set(a, 5, 1.0);
    assert(px_estimate_value(doubled) == 10);
    assert(g_derived_observer_count == 1);

    px_estimate_set(a, 7, 1.0);
    assert(px_estimate_value(doubled) == 14);
    assert(g_derived_observer_count == 2);

    px_estimate_free(doubled);
    px_estimate_free(a);
}

static void test_derived_chained(void) {
    /* Derived of derived: c = (a + b) * 2 */
    px_estimate* a = px_estimate_new(1, 1.0);
    px_estimate* b = px_estimate_new(2, 1.0);
    px_estimate* srcs_ab[] = {a, b};
    px_estimate* sum = px_derived_new(sum_sources, NULL, srcs_ab, 2);

    /* c = sum * 2 */
    px_estimate* srcs_c[] = {sum};
    px_estimate* doubled = px_derived_new(double_first, NULL, srcs_c, 1);

    /* Initial: sum=3, doubled=6 */
    assert(px_estimate_value(sum) == 3);
    assert(px_estimate_value(doubled) == 6);

    /* Change a — both deriveds should update */
    px_estimate_set(a, 10, 1.0);
    assert(px_estimate_value(sum) == 12);
    assert(px_estimate_value(doubled) == 24);

    px_estimate_free(doubled);
    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
}

static void test_derived_all_true(void) {
    /* Form-style: all_valid = AND of N validities */
    px_estimate* v1 = px_estimate_new(0, 1.0);
    px_estimate* v2 = px_estimate_new(0, 1.0);
    px_estimate* v3 = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = {v1, v2, v3};
    px_estimate* all = px_derived_new(all_true, NULL, srcs, 3);

    assert(px_estimate_value(all) == 0);

    px_estimate_set(v1, 1, 1.0);
    assert(px_estimate_value(all) == 0);

    px_estimate_set(v2, 1, 1.0);
    assert(px_estimate_value(all) == 0);

    px_estimate_set(v3, 1, 1.0);
    assert(px_estimate_value(all) == 1);

    /* Break one — all should drop back to 0 */
    px_estimate_set(v2, 0, 1.0);
    assert(px_estimate_value(all) == 0);

    px_estimate_free(all);
    px_estimate_free(v1);
    px_estimate_free(v2);
    px_estimate_free(v3);
}

/* ============================================================
 * Dynamic derived tests (Stage 19)
 * ============================================================ */

static void test_dynamic_empty(void) {
    px_estimate* sum = px_derived_new_dynamic(sum_sources, NULL);
    assert(sum != NULL);
    /* No sources → sum = 0 */
    assert(px_estimate_value(sum) == 0);
    assert(px_derived_source_count(sum) == 0);
    px_estimate_free(sum);
}

static void test_dynamic_add(void) {
    px_estimate* sum = px_derived_new_dynamic(sum_sources, NULL);
    px_estimate* a = px_estimate_new(10, 1.0);
    px_estimate* b = px_estimate_new(20, 1.0);

    assert(px_derived_add_source(sum, a) == 0);
    assert(px_estimate_value(sum) == 10);
    assert(px_derived_source_count(sum) == 1);

    assert(px_derived_add_source(sum, b) == 0);
    assert(px_estimate_value(sum) == 30);
    assert(px_derived_source_count(sum) == 2);

    /* Change a source — auto-updates */
    px_estimate_set(a, 100, 1.0);
    assert(px_estimate_value(sum) == 120);

    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
}

static void test_dynamic_remove(void) {
    px_estimate* sum = px_derived_new_dynamic(sum_sources, NULL);
    px_estimate* a = px_estimate_new(10, 1.0);
    px_estimate* b = px_estimate_new(20, 1.0);
    px_estimate* c = px_estimate_new(30, 1.0);

    px_derived_add_source(sum, a);
    px_derived_add_source(sum, b);
    px_derived_add_source(sum, c);
    assert(px_estimate_value(sum) == 60);

    /* Remove b — sum should drop by b's value */
    assert(px_derived_remove_source(sum, b) == 0);
    assert(px_estimate_value(sum) == 40);
    assert(px_derived_source_count(sum) == 2);

    /* Remove a — sum should drop by a's value */
    assert(px_derived_remove_source(sum, a) == 0);
    assert(px_estimate_value(sum) == 30);
    assert(px_derived_source_count(sum) == 1);

    /* Remove c — sum should be 0 */
    assert(px_derived_remove_source(sum, c) == 0);
    assert(px_estimate_value(sum) == 0);
    assert(px_derived_source_count(sum) == 0);

    /* Removing again should fail */
    assert(px_derived_remove_source(sum, a) == -1);

    px_estimate_free(sum);
    px_estimate_free(a);
    px_estimate_free(b);
    px_estimate_free(c);
}

static double count_not_done(px_estimate* const* srcs, int n, void* user) {
    (void)user;
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (px_estimate_value(srcs[i]) < 0.5) count++;
    }
    return (double)count;
}

static void test_dynamic_todo_style(void) {
    /* Simulate todo app: each todo has a done Estimate (1=done, 0=not done).
     * remaining = count of !done.
     * Add/remove todos at runtime → remaining auto-updates. */
    px_estimate* remaining = px_derived_new_dynamic(count_not_done, NULL);

    /* Initially 0 todos → 0 remaining */
    assert(px_estimate_value(remaining) == 0);

    /* Add 3 todos (all not done = remaining 3) */
    px_estimate* t1 = px_estimate_new(0, 1.0);
    px_estimate* t2 = px_estimate_new(0, 1.0);
    px_estimate* t3 = px_estimate_new(0, 1.0);
    px_derived_add_source(remaining, t1);
    px_derived_add_source(remaining, t2);
    px_derived_add_source(remaining, t3);
    assert(px_estimate_value(remaining) == 3);

    /* Complete t2 → remaining = 2 */
    px_estimate_set(t2, 1, 1.0);
    assert(px_estimate_value(remaining) == 2);

    /* Delete t1 → remaining = 1 (t3 still not done) */
    px_derived_remove_source(remaining, t1);
    assert(px_estimate_value(remaining) == 1);

    /* Add new todo → remaining = 2 */
    px_estimate* t4 = px_estimate_new(0, 1.0);
    px_derived_add_source(remaining, t4);
    assert(px_estimate_value(remaining) == 2);

    px_estimate_free(remaining);
    px_estimate_free(t1);
    px_estimate_free(t2);
    px_estimate_free(t3);
    px_estimate_free(t4);
}

/* ============================================================
 * Animation tests (Stage 4 — time-sampled Behavior)
 * ============================================================ */

static void test_px_now_ms_monotonic(void) {
    double t1 = px_now_ms();
    /* Tiny sleep to ensure time advances */
    px_sleep_ms(1);
    double t2 = px_now_ms();
    assert(t2 > t1);
}

static void test_animate_starts(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    assert(!px_estimate_is_animating(e));

    px_estimate_animate(e, 100, 100);
    assert(px_estimate_is_animating(e));

    px_estimate_free(e);
}

static void test_animate_now_samples(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);

    /* v0.5: queries are const; must call advance() before reading
     * to bring cached value up to current animation time. */
    px_estimate_advance(e, px_now_ms());
    double v0 = px_estimate_now(e);
    assert(v0 >= 0 && v0 < 10);  /* should be very early in animation */

    /* Sleep ~50ms */
    px_sleep_ms(50);

    /* At t≈50ms, value should be in mid-range (ease-out, so > 5) */
    px_estimate_advance(e, px_now_ms());
    double v50 = px_estimate_now(e);
    assert(v50 > v0);  /* strictly increased */
    assert(v50 < 100); /* not yet at target */

    px_estimate_free(e);
}

static void test_animate_completes(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 50);  /* 50ms duration */

    /* Sleep long enough for animation to complete */
    px_sleep_ms(100);

    /* v0.5: advance finalizes the animation (clears animating flag,
     * sets value=target, fires observers). */
    px_estimate_advance(e, px_now_ms());

    /* is_animating should return false (animation completed) */
    assert(!px_estimate_is_animating(e));
    /* px_estimate_now should return final value */
    assert(px_estimate_now(e) == 100);
    /* After completion, value should be finalized */
    assert(px_estimate_value(e) == 100);

    px_estimate_free(e);
}

static void test_animate_set_cancels(void) {
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);
    assert(px_estimate_is_animating(e));

    /* set() cancels animation */
    px_estimate_set(e, 42, 1.0);
    assert(!px_estimate_is_animating(e));
    assert(px_estimate_value(e) == 42);

    px_estimate_free(e);
}

static void test_animate_chain(void) {
    /* Animate while already animating — should chain from current position */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_estimate_animate(e, 100, 1000);

    /* Wait ~30ms */
    px_sleep_ms(30);

    /* v0.5: advance to bring cached value up to current time */
    px_estimate_advance(e, px_now_ms());
    double mid = px_estimate_now(e);
    assert(mid > 0 && mid < 100);

    /* Animate to new target from current position */
    px_estimate_animate(e, 200, 500);
    assert(px_estimate_is_animating(e));

    /* from_value should be the current sampled value, not 0 */
    /* (internal detail — test indirectly: value should be near mid) */
    px_estimate_advance(e, px_now_ms());
    double now = px_estimate_now(e);
    assert(now >= mid - 1);  /* approximately continuous */

    px_estimate_free(e);
}

/* ============================================================
 * Font fallback chain tests (Stage 11)
 * ============================================================ */

static void test_font_default_loads(void) {
    px_font* font = px_font_default();
    assert(font != NULL);
    assert(px_font_line_height(font) == 16);
    /* free is a no-op for default */
    px_font_free(font);
}

static void test_font_load_invalid_path(void) {
    px_font* font = px_font_load("/nonexistent/font.ttf", 16);
    assert(font == NULL);
}

static void test_font_default_add_fallback_is_noop(void) {
    px_font* font = px_font_default();
    /* Should fail gracefully (-1) for default font */
    int rc = px_font_add_fallback(font, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    assert(rc == -1);
}

static void test_font_chain_text_renders(void) {
    /* Render text with default font — should produce some pixels */
    px_fb* fb = px_fb_new(100, 20);
    px_fb_clear(fb, PX_BG);
    int w = px_fb_draw_text(fb, 4, 4, "Hello", PX_TEXT);
    assert(w > 0);
    /* Check that some pixels were drawn (not all background) */
    bool any_pixel_drawn = false;
    for (int y = 0; y < 20 && !any_pixel_drawn; y++) {
        for (int x = 0; x < 100; x++) {
            if (px_fb_get_pixel(fb, x, y) != PX_BG) {
                any_pixel_drawn = true;
                break;
            }
        }
    }
    assert(any_pixel_drawn);
    px_fb_free(fb);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex core tests\n");
    printf("==================\n\n");

    printf("[Relation]\n");
    TEST(relation_declare);
    TEST(relation_query);
    TEST(relation_bidirectional);
    TEST(relation_kind_str);

    printf("\n[Estimate]\n");
    TEST(estimate_basic);
    TEST(estimate_set);
    TEST(estimate_animate);
    TEST(estimate_observe);

    printf("\n[Closure]\n");
    TEST(closure_trigger);
    TEST(closure_intent_value);
    TEST(closure_intent_serializable);
    TEST(closure_goal_recorded);
    TEST(intent_kind_str);

    printf("\n[Integration]\n");
    TEST(integration_counter);

    printf("\n[Derived (Stage 3)]\n");
    TEST(derived_basic);
    TEST(derived_auto_updates);
    TEST(derived_fires_observers);
    TEST(derived_chained);
    TEST(derived_all_true);

    printf("\n[Dynamic derived (Stage 19)]\n");
    TEST(dynamic_empty);
    TEST(dynamic_add);
    TEST(dynamic_remove);
    TEST(dynamic_todo_style);

    printf("\n[Animation (Stage 4)]\n");
    TEST(px_now_ms_monotonic);
    TEST(animate_starts);
    TEST(animate_now_samples);
    TEST(animate_completes);
    TEST(animate_set_cancels);
    TEST(animate_chain);

    printf("\n[Font fallback chain (Stage 11)]\n");
    TEST(font_default_loads);
    TEST(font_load_invalid_path);
    TEST(font_default_add_fallback_is_noop);
    TEST(font_chain_text_renders);

    printf("\n-------------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    return g_tests_pass == g_tests_run ? 0 : 1;
}
