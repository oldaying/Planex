/*
 * test_v07.c — v0.7 roadmap line-by-line verification suite
 *
 * One suite per v0.7-roadmap.md line, accumulated as lines ship:
 *
 *   A. Line 1 — intent compilation routing (afford/region promotion
 *      evidence): px_afford_compile as the window-free compile step,
 *      the px_pointer_intent value contract, and the app-level
 *      routing semantics (afforded click → closure trigger;
 *      unresolved click → raw-coordinate fallback).
 *
 * Build (or: make test_v07):
 *   cc -std=c17 -I include tests/test_v07.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c -lm -o build/test_v07
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
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
 * Shared helpers
 * ============================================================ */

static bool eval_true(void* u) { (void)u; return true; }

typedef struct {
    px_estimate* count;
    px_pointer_intent last_pi;
    int          fires;
    bool         got_payload;
} Sink;

/* A closure action that records the compiled intent it received. */
static void on_sink(px_intent i, void* u) {
    Sink* s = (Sink*)u;
    s->fires++;
    if (i.payload && i.payload_size == sizeof(px_pointer_intent)) {
        s->last_pi = *(const px_pointer_intent*)i.payload;
        s->got_payload = true;
        px_estimate_set(s->count,
                        px_estimate_value(s->count) + 1.0, 1.0);
    }
}

/* Raw-coordinate fallback counter — the app-side on_click stand-in. */
static int g_raw_clicks = 0;
static bool on_raw_click(int x, int y, void* user) {
    (void)x; (void)y; (void)user;
    g_raw_clicks++;
    return true;
}

/* ============================================================
 * A. Line 1 — intent compilation routing
 * ============================================================ */

static void test_a1_compile_resolves_region_to_closure(void) {
    px_graph* g = px_graph_new();
    Sink sink = {0};
    sink.count = px_estimate_new(0, 1.0);

    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
                                     on_sink, eval_true, &sink);
    px_region* r = px_region_new(px_rect_make(20, 40, 280, 32), "inc-button");
    px_declare(g, r, PX_REL_AFFORDS, inc);

    /* The compile step: physical (x, y, button) → (closure, payload). */
    px_pointer_intent pi;
    px_closure* c = px_afford_compile(g, 100, 50, 1, &pi);
    assert(c == inc);
    assert(strcmp(pi.region, "inc-button") == 0);
    assert(pi.x == 100 && pi.y == 50 && pi.button == 1);

    /* Trigger with the compiled payload — the app never sees x/y. */
    px_closure_trigger(c, &pi, sizeof(pi));
    assert(sink.fires == 1);
    assert(sink.got_payload);
    assert(strcmp(sink.last_pi.region, "inc-button") == 0);
    assert(px_estimate_value(sink.count) == 1.0);

    px_region_free(r);
    px_closure_free(inc);
    px_estimate_free(sink.count);
    px_graph_free(g);
}

static void test_a2_compile_miss_zeroes_payload(void) {
    px_graph* g = px_graph_new();
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST, NULL, NULL, NULL);
    px_region* r = px_region_new(px_rect_make(0, 0, 10, 10), "r");
    px_declare(g, r, PX_REL_AFFORDS, c);

    /* Pre-poison the payload: a miss must zero it, not leave stale data. */
    px_pointer_intent pi;
    memset(&pi, 0xAB, sizeof(pi));
    assert(px_afford_compile(g, 500, 500, 1, &pi) == NULL);
    assert(pi.x == 0 && pi.y == 0 && pi.button == 0 && pi.region[0] == 0);

    /* Region hit but no AFFORDS edge for it (region unlinked). */
    px_region* bare = px_region_new(px_rect_make(20, 20, 10, 10), "bare");
    assert(px_afford_compile(g, 25, 25, 1, &pi) == NULL);
    assert(pi.region[0] == 0);

    /* NULL graph / NULL out — safe no-ops. */
    assert(px_afford_compile(NULL, 5, 5, 1, &pi) == NULL);
    assert(px_afford_compile(g, 5, 5, 1, NULL) == NULL);

    px_region_free(bare);
    px_region_free(r);
    px_closure_free(c);
    px_graph_free(g);
}

static void test_a3_intent_is_a_value_replayable(void) {
    /* The denotational-semantics half of the ADR-0011 admission bar:
     * the compiled intent must survive capture and replay as a VALUE.
     * The region label is embedded, so replay works even after the
     * region is freed. */
    px_graph* g = px_graph_new();
    Sink sink = {0};
    sink.count = px_estimate_new(0, 1.0);

    px_closure* act = px_closure_new("act", PX_INTENT_DECLARE,
                                     on_sink, eval_true, &sink);
    px_region* r = px_region_new(px_rect_make(0, 0, 100, 100), "transient-zone");
    px_declare(g, r, PX_REL_AFFORDS, act);

    px_pointer_intent pi;
    px_closure* c = px_afford_compile(g, 42, 42, 3, &pi);
    assert(c == act);
    px_closure_trigger(c, &pi, sizeof(pi));
    assert(sink.fires == 1);

    /* Capture, free the region, replay — payload still meaningful. */
    px_intent captured = px_closure_last_intent(act);
    assert(captured.payload && captured.payload_size == sizeof(px_pointer_intent));
    px_region_free(r);
    px_closure_replay(act, captured);
    assert(sink.fires == 2);
    assert(strcmp(sink.last_pi.region, "transient-zone") == 0);
    assert(sink.last_pi.button == 3);

    px_closure_free(act);
    px_estimate_free(sink.count);
    px_graph_free(g);
}

static void test_a4_topmost_region_wins_compile(void) {
    px_graph* g = px_graph_new();
    Sink a = {0}, b = {0};
    a.count = px_estimate_new(0, 1.0);
    b.count = px_estimate_new(0, 1.0);

    px_closure* ca = px_closure_new("a", PX_INTENT_REQUEST, on_sink, eval_true, &a);
    px_closure* cb = px_closure_new("b", PX_INTENT_REQUEST, on_sink, eval_true, &b);
    px_region* bottom = px_region_new(px_rect_make(0, 0, 100, 100), "bottom");
    px_region* top    = px_region_new(px_rect_make(50, 50, 100, 100), "top");
    px_declare(g, bottom, PX_REL_AFFORDS, ca);
    px_declare(g, top,    PX_REL_AFFORDS, cb); /* later = on top */

    px_pointer_intent pi;
    px_closure* c = px_afford_compile(g, 75, 75, 1, &pi);
    assert(c == cb);
    assert(strcmp(pi.region, "top") == 0);

    /* Outside the overlap, the bottom region compiles normally. */
    c = px_afford_compile(g, 10, 10, 1, &pi);
    assert(c == ca);
    assert(strcmp(pi.region, "bottom") == 0);

    px_region_free(top);
    px_region_free(bottom);
    px_closure_free(cb);
    px_closure_free(ca);
    px_estimate_free(a.count);
    px_estimate_free(b.count);
    px_graph_free(g);
}

static void test_a5_app_desc_routing_semantics(void) {
    /* The app.c contract, exercised without a window:
     * intent_graph set + afforded click → closure, NOT on_click;
     * intent_graph set + unresolved click → on_click fallback;
     * intent_graph NULL → on_click always (opt-in is real). */
    px_graph* g = px_graph_new();
    Sink sink = {0};
    sink.count = px_estimate_new(0, 1.0);

    px_closure* act = px_closure_new("act", PX_INTENT_REQUEST,
                                     on_sink, eval_true, &sink);
    px_region* r = px_region_new(px_rect_make(0, 0, 50, 50), "zone");
    px_declare(g, r, PX_REL_AFFORDS, act);

    g_raw_clicks = 0;

    /* The exact decision app.c runs on PX_EV_MOUSE_DOWN (v0.7 Line 1). */
    #define APP_ROUTE(desc, evx, evy, evb)                            \
        do {                                                           \
            if ((desc)->intent_graph) {                                \
                px_pointer_intent _pi;                                 \
                px_closure* _c = px_afford_compile((desc)->intent_graph,\
                    (double)(evx), (double)(evy), (evb), &_pi);        \
                if (_c) { px_closure_trigger(_c, &_pi, sizeof(_pi)); break; } \
            }                                                          \
            if ((desc)->on_click) on_raw_click(evx, evy, (desc)->user);\
        } while (0)

    px_app_desc routed = {0};
    routed.intent_graph = g;
    routed.on_click = on_raw_click;

    px_app_desc legacy = {0};
    legacy.on_click = on_raw_click;

    /* 1. Afforded click routes to the closure; raw handler stays at 0. */
    APP_ROUTE(&routed, 25, 25, 1);
    assert(sink.fires == 1 && g_raw_clicks == 0);
    assert(strcmp(sink.last_pi.region, "zone") == 0);

    /* 2. Unresolved click (empty space) falls back to raw dispatch. */
    APP_ROUTE(&routed, 500, 500, 1);
    assert(g_raw_clicks == 1 && sink.fires == 1);

    /* 3. Opt-out: no intent_graph → raw dispatch even inside a region. */
    APP_ROUTE(&legacy, 25, 25, 1);
    assert(g_raw_clicks == 2 && sink.fires == 1);

    px_region_free(r);
    px_closure_free(act);
    px_estimate_free(sink.count);
    px_graph_free(g);
}

static void test_a6_label_truncation_is_safe(void) {
    /* A 100-char label must truncate at PX_REGION_LABEL_MAX-1 and
     * stay null-terminated through the compile step. */
    px_graph* g = px_graph_new();
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST, NULL, NULL, NULL);

    char longlabel[128];
    memset(longlabel, 'L', sizeof(longlabel) - 1);
    longlabel[sizeof(longlabel) - 1] = 0;

    px_region* r = px_region_new(px_rect_make(0, 0, 10, 10), longlabel);
    px_declare(g, r, PX_REL_AFFORDS, c);

    px_pointer_intent pi;
    px_closure* found = px_afford_compile(g, 5, 5, 1, &pi);
    assert(found == c);
    assert(strlen(pi.region) == PX_REGION_LABEL_MAX - 1);
    assert(pi.region[PX_REGION_LABEL_MAX - 1] == 0);

    px_region_free(r);
    px_closure_free(c);
    px_graph_free(g);
}

static void test_a7_multi_edge_resolution_is_last_declared(void) {
    /* Found by building palette_afford.c: a region with TWO AFFORDS
     * edges is ambiguous unless the resolution order is pinned.
     * px_declare prepends, px_query walks most-recent-first, so the
     * LAST-declared edge wins. This test pins that rule so ADR-0017
     * can cite it as specified behavior, not accident. */
    px_graph* g = px_graph_new();
    px_closure* first  = px_closure_new("first",  PX_INTENT_REQUEST, NULL, NULL, NULL);
    px_closure* second = px_closure_new("second", PX_INTENT_REQUEST, NULL, NULL, NULL);
    px_region* r = px_region_new(px_rect_make(0, 0, 10, 10), "r");

    px_declare(g, r, PX_REL_AFFORDS, first);   /* declared first */
    px_declare(g, r, PX_REL_AFFORDS, second);  /* declared last = wins */

    px_pointer_intent pi;
    px_closure* c = px_afford_compile(g, 5, 5, 1, &pi);
    assert(c == second);

    px_region_free(r);
    px_closure_free(second);
    px_closure_free(first);
    px_graph_free(g);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex v0.7 line verification\n");
    printf("=============================\n");
    printf("A. Line 1 — intent compilation routing (afford/region).\n\n");

    printf("[A] Intent compilation routing\n");
    TEST(a1_compile_resolves_region_to_closure);
    TEST(a2_compile_miss_zeroes_payload);
    TEST(a3_intent_is_a_value_replayable);
    TEST(a4_topmost_region_wins_compile);
    TEST(a5_app_desc_routing_semantics);
    TEST(a6_label_truncation_is_safe);
    TEST(a7_multi_edge_resolution_is_last_declared);

    printf("\n----------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    printf("\nWhat this suite closes (v0.7-roadmap Line 1):\n");
    printf("  - px_afford_compile: window-free compile step, tested\n");
    printf("    without a backend (the routing decision is pure)\n");
    printf("  - px_pointer_intent is a VALUE: embedded label survives\n");
    printf("    capture + replay after the region is freed\n");
    printf("  - app routing semantics: afforded -> closure, unresolved\n");
    printf("    -> raw fallback, NULL intent_graph -> legacy dispatch\n");
    printf("  - multi-edge resolution: last-declared AFFORDS edge wins\n");
    printf("    (pinned — a region with two affordances is a spec'd rule)\n");
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
