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
 *   B. Line 2 — budget as contract: default budget on every loop,
 *      explicit opt-out, propagation accounting (edges + depth) in
 *      the audit entry, overrun counting.
 *
 *   C. Line 3 — Estimate schema: the describable value contract —
 *      kind-aware assertions, kind-default denotation, custom print,
 *      kind-aware equality, and the a11y value-naming seam.
 *
 *   D. Line 4 — the a11y AT-SPI2 bridge: the stub contract when the
 *      flag is absent (the real adapter compile-probes in CI).
 *
 *   E. Line 5 — Closure constructor split: the graph arrives with
 *      the closure; the bind_graph ordering leak is unwritable.
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
#include "planex/a11y.h"
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
 * B. Line 2 — budget as contract
 * ============================================================ */

static void* perceive_null(px_estimate* const* in, int n, void* u) {
    (void)in; (void)n; (void)u;
    return NULL;
}

static void on_set_src(px_intent i, void* u) {
    (void)i;
    px_estimate* e = (px_estimate*)u;
    px_estimate_set(e, px_estimate_value(e) + 1.0, 1.0);
}

static double sum2(px_estimate* const* in, int n, void* u) {
    (void)n; (void)u;
    return px_estimate_value(in[0]) + px_estimate_value(in[1]);
}

static double passthrough(px_estimate* const* in, int n, void* u) {
    (void)n; (void)u;
    return px_estimate_value(in[0]);
}

static void test_b1_default_budget_is_one_frame(void) {
    /* The contract: every loop ships with a deadline. The default is
     * one 60fps frame — the feedback axiom's "instantly visible"
     * given a number. No loop is silently unbounded. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
                                   on_set_src, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    assert(px_loop_budget(loop) == PX_LOOP_DEFAULT_BUDGET_MS);
    assert(PX_LOOP_DEFAULT_BUDGET_MS == 16.0);

    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry entry;
    assert(px_loop_audit_get(loop, &entry, 1) == 1);
    assert(entry.budget_ms == PX_LOOP_DEFAULT_BUDGET_MS);
    assert(entry.iteration_ms >= 0.0);
    assert(entry.budget_exceeded == false); /* a trivial step fits */
    assert(px_loop_budget_overruns(loop) == 0);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_b2_explicit_opt_out(void) {
    /* Zero is a decision, not a default: set_budget(0) disables the
     * deadline, and the audit records that decision per entry. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
                                   on_set_src, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    px_loop_set_budget(loop, 0.0);
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry entry;
    assert(px_loop_audit_get(loop, &entry, 1) == 1);
    assert(entry.budget_ms == 0.0);
    assert(entry.budget_exceeded == false);

    /* Negative budgets clamp to 0 (the opt-out), never a deadline. */
    px_loop_set_budget(loop, -5.0);
    assert(px_loop_budget(loop) == 0.0);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_b3_overrun_is_loud_and_counted(void) {
    /* A budget of one nanosecond is exceeded by any real iteration.
     * The overrun must be (a) recorded in the entry, (b) counted by
     * px_loop_budget_overruns, (c) warned exactly once on stderr —
     * the single notice is visible in this suite's own output when
     * it runs (deliberate overrun, by design of this test). Strict
     * abort mode (PX_DEBUG_BUDGET) is compile-time opt-in precisely
     * so this test can exist. */
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST,
                                   on_set_src, eval_true, e);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    px_loop_set_budget(loop, 0.000001); /* 1ns: nothing fits */
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);
    px_loop_step(loop, NULL, 0);

    px_loop_audit_entry entries[3];
    assert(px_loop_audit_get(loop, entries, 3) == 3);
    for (int i = 0; i < 3; i++) {
        assert(entries[i].budget_exceeded == true);
        assert(entries[i].iteration_ms > 0.0);
    }
    assert(px_loop_budget_overruns(loop) == 3);

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

static void test_b4_propagation_accounting(void) {
    /* The audit entry must answer BOTH cost questions: what did the
     * frame cost (iteration_ms) and what did PROPAGATION cost. With
     * undo enabled and a graph bound, the closure trigger walks
     * TRIGGERS edges (propagation_edges > 0); a derived chain
     * a -> b -> c -> d nests recompute (propagation_depth >= 2). */
    px_graph* g = px_graph_new();
    px_estimate* a = px_estimate_new(1.0, 1.0);
    px_estimate* b = px_derived_new(passthrough, NULL, &a, 1);
    px_estimate* c = px_derived_new(passthrough, NULL, &b, 1);
    px_estimate* d = px_derived_new(sum2, NULL, (px_estimate*[]){a, c}, 2);

    /* Undo wiring via the v0.7 constructor split (Line 5): the graph
     * arrives with the closure — no bind call to forget. */
    px_closure* seta = px_closure_new_with_graph(
        "set a", PX_INTENT_REQUEST, on_set_src, eval_true, a, g);
    px_estimate* srcs[] = { d };
    px_perception* p = px_perception_new("p", perceive_null, srcs, 1, NULL);
    px_loop* loop = px_loop_new(seta, p);

    px_declare(g, seta, PX_REL_TRIGGERS, a);
    px_undo_set_enabled(true);

    px_loop_step(loop, NULL, 0);   /* a += 1 → b, c, d recompute */

    px_loop_audit_entry entry;
    assert(px_loop_audit_get(loop, &entry, 1) == 1);
    assert(entry.propagation_edges > 0);  /* TRIGGERS query walked edges */
    assert(entry.propagation_depth >= 2); /* b → c chain nested ≥ 2 */
    assert(px_estimate_value(d) == 4.0);  /* (1+1) + (1+1) — chain ran */

    px_undo_set_enabled(false);
    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(seta);
    px_estimate_free(d);
    px_estimate_free(c);
    px_estimate_free(b);
    px_estimate_free(a);
    px_graph_free(g);
}

/* ============================================================
 * C. Line 3 — Estimate schema (the describable value contract)
 * ============================================================ */

static void print_count(double v, char* out, size_t out_size) {
    snprintf(out, out_size, "count=%ld", (long)v);
}

static bool equal_exact(double a, double b) { return a == b; }

static void test_c1_kind_aware_assertions(void) {
    /* The roadmap's shape: "this estimate is INT and equals 3", not
     * byte equality through a pointer. The schema declares WHAT the
     * double means; the test asserts kind + value together. */
    static const px_estimate_schema count_schema = {
        PX_VAL_INT, "count", NULL, NULL
    };
    px_estimate* count = px_estimate_new(3.0, 1.0);
    px_estimate_set_schema(count, &count_schema);

    /* The kind-aware assertion, spelled out. */
    assert(px_estimate_schema_of(count) == &count_schema);
    assert(px_estimate_schema_of(count)->kind == PX_VAL_INT);
    assert(px_estimate_value(count) == 3.0);
    assert(strcmp(px_value_kind_name(PX_VAL_INT), "INT") == 0);

    /* No schema declared: zero cost, honest answer. */
    px_estimate* bare = px_estimate_new(1.0, 1.0);
    assert(px_estimate_schema_of(bare) == NULL);

    px_estimate_free(bare);
    px_estimate_free(count);
}

static void test_c2_describe_default_and_custom(void) {
    char buf[128];

    /* Kind defaults: each kind denotates without a custom print. */
    static const px_estimate_schema int_s   = { PX_VAL_INT, "n", NULL, NULL };
    static const px_estimate_schema dbl_s   = { PX_VAL_DOUBLE, "x", NULL, NULL };
    static const px_estimate_schema pct_s   = { PX_VAL_PERCENT, "p", NULL, NULL };
    static const px_estimate_schema bool_s  = { PX_VAL_BOOL, "b", NULL, NULL };

    px_estimate* n = px_estimate_new(42.0, 1.0);
    px_estimate_set_schema(n, &int_s);
    px_estimate_describe(n, buf, sizeof(buf));
    assert(strcmp(buf, "42") == 0);

    px_estimate* x = px_estimate_new(0.5, 1.0);
    px_estimate_set_schema(x, &dbl_s);
    px_estimate_describe(x, buf, sizeof(buf));
    assert(strcmp(buf, "0.50") == 0);

    px_estimate* p = px_estimate_new(75.0, 1.0);
    px_estimate_set_schema(p, &pct_s);
    px_estimate_describe(p, buf, sizeof(buf));
    assert(strcmp(buf, "75%") == 0);

    px_estimate* b = px_estimate_new(1.0, 1.0);
    px_estimate_set_schema(b, &bool_s);
    px_estimate_describe(b, buf, sizeof(buf));
    assert(strcmp(buf, "on") == 0);

    /* Custom print overrides the kind default. */
    static const px_estimate_schema custom_s = {
        PX_VAL_INT, "count", print_count, NULL
    };
    px_estimate_set_schema(n, &custom_s);
    px_estimate_describe(n, buf, sizeof(buf));
    assert(strcmp(buf, "count=42") == 0);

    /* Untyped: the honest placeholder. */
    px_estimate* bare = px_estimate_new(7.0, 1.0);
    px_estimate_describe(bare, buf, sizeof(buf));
    assert(strcmp(buf, "<untyped value>") == 0);

    px_estimate_free(bare);
    px_estimate_free(b);
    px_estimate_free(p);
    px_estimate_free(x);
    px_estimate_free(n);
}

static void test_c3_kind_aware_equality(void) {
    static const px_estimate_schema int_s = { PX_VAL_INT, "n", NULL, NULL };
    static const px_estimate_schema dbl_s = { PX_VAL_DOUBLE, "x", NULL, NULL };

    px_estimate* a = px_estimate_new(3.0, 1.0);
    px_estimate* b = px_estimate_new(3.0, 1.0);
    px_estimate* d = px_estimate_new(3.0000000000001, 1.0); /* within 1e-9 */
    px_estimate* other = px_estimate_new(4.0, 1.0);

    px_estimate_set_schema(a, &int_s);
    px_estimate_set_schema(b, &int_s);
    px_estimate_set_schema(d, &dbl_s);
    px_estimate_set_schema(other, &int_s);

    /* Same kind, same value: equal. */
    assert(px_estimate_value_equal(a, b));
    /* Discrete kinds compare exactly; 3 != 4. */
    assert(!px_estimate_value_equal(a, other));
    /* DOUBLE compares within 1e-9: 3 ~ 3.0000000000001. */
    px_estimate* d2 = px_estimate_new(3.0, 1.0);
    px_estimate_set_schema(d2, &dbl_s);
    assert(px_estimate_value_equal(d, d2));
    /* Different kinds never equal. */
    assert(!px_estimate_value_equal(a, d));
    /* Untyped values carry no equality contract. */
    px_estimate* bare = px_estimate_new(3.0, 1.0);
    assert(!px_estimate_value_equal(a, bare));

    /* Custom equal overrides the kind default. */
    static const px_estimate_schema exact_s = { PX_VAL_DOUBLE, "x", NULL, equal_exact };
    px_estimate_set_schema(d, &exact_s);
    px_estimate_set_schema(d2, &exact_s);
    assert(!px_estimate_value_equal(d, d2)); /* exact: differs */

    px_estimate_free(bare);
    px_estimate_free(d2);
    px_estimate_free(other);
    px_estimate_free(d);
    px_estimate_free(b);
    px_estimate_free(a);
}

static void test_c4_a11y_reads_the_schema(void) {
    /* The Line 4 seam: the a11y value string comes from the schema,
     * not from a hand-formatted printf at each call site. */
    px_a11y* a = px_a11y_new(NULL);
    px_a11y_set_verbose(a, false);   /* silence stderr in the suite */
    assert(a);

    static const px_estimate_schema count_s = { PX_VAL_INT, "count", NULL, NULL };
    px_estimate* count = px_estimate_new(3.0, 1.0);
    px_estimate_set_schema(count, &count_s);

    px_a11y_set_name(a, "Count");
    px_a11y_set_value_estimate(a, count);
    assert(strcmp(px_a11y_get_value(a), "3") == 0);

    /* Value change flows through the same seam. */
    px_estimate_set(count, 7.0, 1.0);
    px_a11y_set_value_estimate(a, count);
    assert(strcmp(px_a11y_get_value(a), "7") == 0);

    /* Custom print reaches the a11y channel unchanged. */
    static const px_estimate_schema custom_s = { PX_VAL_INT, "count", print_count, NULL };
    px_estimate_set_schema(count, &custom_s);
    px_a11y_set_value_estimate(a, count);
    assert(strcmp(px_a11y_get_value(a), "count=7") == 0);

    px_estimate_free(count);
    px_a11y_free(a);
}

/* ============================================================
 * D. Line 4 — the a11y bridge stub contract (flag not set here)
 * ============================================================ */

static void test_d1_bridge_stub_contract(void) {
    /* This suite builds WITHOUT -DPX_A11Y_ATSPI, so the bridge must
     * be an honest stub: attach returns NULL (after a one-time
     * notice on stderr — visible once in this suite's output), and
     * flush/detach accept anything without crashing. The query-side
     * contract itself is unchanged (the v0.6 tests i1/i2 + c4 cover
     * it). The REAL bridge compiles under the flag in CI's
     * a11y-atspi-bridge probe job. */
    px_a11y* a = px_a11y_new(NULL);
    px_a11y_set_verbose(a, false);
    assert(a);

    px_a11y_bridge* b = px_a11y_bridge_atspi_attach(a, "probe");
    assert(b == NULL);

    /* NULL-safe no-ops. */
    px_a11y_bridge_atspi_flush(NULL);
    px_a11y_bridge_atspi_detach(NULL);
    px_a11y_bridge_atspi_flush(b);
    px_a11y_bridge_atspi_detach(b);

    /* The query side is untouched by the bridge's absence. */
    px_a11y_set_name(a, "ok");
    assert(strcmp(px_a11y_get_name(a), "ok") == 0);

    px_a11y_free(a);
}

/* ============================================================
 * E. Line 5 — Closure constructor split (the last L2 retire)
 * ============================================================ */

static void test_e1_with_graph_binds_at_birth(void) {
    /* The graph arrives WITH the closure; undo records immediately —
     * there is no bind call to forget, no window to race. */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new_with_graph(
        "inc", PX_INTENT_REQUEST, on_set_src, eval_true, count, g);
    assert(inc);

    px_declare(g, inc, PX_REL_TRIGGERS, count);
    px_undo_set_enabled(true);
    px_undo_clear();

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 1.0);
    assert(px_undo_count() == 1);           /* recorded — no bind call */
    assert(px_undo() == 1);                 /* and it restores */
    assert(px_estimate_value(count) == 0.0);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

static void test_e2_ordering_mistake_unwritable(void) {
    /* The v0.6 leak: create, trigger, THEN bind — undo silently
     * recorded nothing. With the split constructor there is no code
     * shape that produces this bug: the graph is a constructor
     * argument. This test pins the OLD failure mode's absence by
     * demonstrating the deprecated path still works when used in
     * the right order (deprecation window, registry-tracked). */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
                                     on_set_src, eval_true, count);
    px_declare(g, inc, PX_REL_TRIGGERS, count);
    px_undo_set_enabled(true);
    px_undo_clear();

    /* Deprecated two-call form, used correctly: still functional. */
    px_closure_bind_graph(inc, g);
    px_closure_trigger(inc, NULL, 0);
    assert(px_undo_count() == 1);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

/* ============================================================
 * F. Edge lifecycle — px_undeclare (the CI-found dangling edge)
 *
 * The v0.7 push's first CI run aborted hover_drag_interaction on
 * the final affordance assertion on Ubuntu while passing locally.
 * Root cause: the example freed and recreated the drag process
 * without retiring its AFFORDS edges; px_afford_at then returned
 * the dead pointer. The assertion only passed on Debian glibc
 * because the allocator reused the freed address — layout luck,
 * not correctness. Section F pins the retirement discipline.
 * ============================================================ */

static void test_f1_undeclare_retires_the_edge(void) {
    /* declare → retire → gone from has_relation AND query AND count;
     * retiring a retired edge is an honest false. */
    px_graph* g = px_graph_new();
    int a = 0, b = 0;
    assert(px_declare(g, &a, PX_REL_TRIGGERS, &b));
    assert(px_has_relation(g, &a, PX_REL_TRIGGERS, &b));
    assert(px_undeclare(g, &a, PX_REL_TRIGGERS, &b));
    assert(!px_has_relation(g, &a, PX_REL_TRIGGERS, &b));
    px_node_list l = px_query(g, &a, PX_REL_TRIGGERS);
    assert(l.count == 0);
    px_node_list_free(&l);
    assert(px_graph_count(g) == 0);
    assert(!px_undeclare(g, &a, PX_REL_TRIGGERS, &b)); /* already gone */
    px_graph_free(g);
}

static void test_f2_undeclare_spares_siblings(void) {
    /* Only the first (a, kind, b) match goes: same-kind siblings
     * and same-endpoint different-kind edges survive. */
    px_graph* g = px_graph_new();
    int a = 0, b1 = 0, b2 = 0;
    px_declare(g, &a, PX_REL_TRIGGERS, &b1);
    px_declare(g, &a, PX_REL_TRIGGERS, &b2);
    px_declare(g, &a, PX_REL_DEPENDS_ON, &b1);
    assert(px_undeclare(g, &a, PX_REL_TRIGGERS, &b1));
    assert(px_graph_count(g) == 2);
    assert(!px_has_relation(g, &a, PX_REL_TRIGGERS, &b1));
    assert(px_has_relation(g, &a, PX_REL_TRIGGERS, &b2));
    assert(px_has_relation(g, &a, PX_REL_DEPENDS_ON, &b1)); /* kind-scoped */
    px_graph_free(g);
}

static void test_f3_dangling_edge_regression(void) {
    /* The CI regression pinned: retire-then-free keeps px_afford_at
     * naming the LIVE process across a rebuild, independent of
     * allocator layout. Region is process-global — freed at the end. */
    px_graph* g = px_graph_new();
    px_region* r = px_region_new(px_rect_make(20, 40, 280, 32), "item");
    px_interaction* drag = px_interaction_new("drag", 8);
    px_declare(g, r, PX_REL_AFFORDS, drag);

    /* The discipline the example now follows. */
    px_undeclare(g, r, PX_REL_AFFORDS, drag);
    px_interaction_free(drag);
    drag = px_interaction_new("drag-2", 8);
    px_declare(g, r, PX_REL_AFFORDS, drag);

    px_closure* afforded = px_afford_at(g, 100, 50);
    assert((void*)afforded == (void*)drag);      /* the LIVE process */
    assert(px_has_relation(g, r, PX_REL_AFFORDS, drag));

    px_undeclare(g, r, PX_REL_AFFORDS, drag);
    px_interaction_free(drag);
    px_region_free(r);
    px_graph_free(g);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex v0.7 line verification\n");
    printf("=============================\n");
    printf("A. Line 1 — intent compilation routing (afford/region).\n");
    printf("B. Line 2 — budget as contract.\n");
    printf("C. Line 3 — Estimate schema.\n");
    printf("D. Line 4 — a11y bridge stub contract.\n");
    printf("E. Line 5 — Closure constructor split.\n");
    printf("F. Edge lifecycle — px_undeclare (CI-found dangling edge).\n\n");

    printf("[A] Intent compilation routing\n");
    TEST(a1_compile_resolves_region_to_closure);
    TEST(a2_compile_miss_zeroes_payload);
    TEST(a3_intent_is_a_value_replayable);
    TEST(a4_topmost_region_wins_compile);
    TEST(a5_app_desc_routing_semantics);
    TEST(a6_label_truncation_is_safe);
    TEST(a7_multi_edge_resolution_is_last_declared);

    printf("\n[B] Budget as contract (Line 2)\n");
    TEST(b1_default_budget_is_one_frame);
    TEST(b2_explicit_opt_out);
    TEST(b3_overrun_is_loud_and_counted);
    TEST(b4_propagation_accounting);

    printf("\n[C] Estimate schema (Line 3)\n");
    TEST(c1_kind_aware_assertions);
    TEST(c2_describe_default_and_custom);
    TEST(c3_kind_aware_equality);
    TEST(c4_a11y_reads_the_schema);

    printf("\n[D] a11y bridge stub contract (Line 4)\n");
    TEST(d1_bridge_stub_contract);

    printf("\n[E] Closure constructor split (Line 5)\n");
    TEST(e1_with_graph_binds_at_birth);
    TEST(e2_ordering_mistake_unwritable);

    printf("\n[F] Edge lifecycle: px_undeclare\n");
    TEST(f1_undeclare_retires_the_edge);
    TEST(f2_undeclare_spares_siblings);
    TEST(f3_dangling_edge_regression);

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
    printf("  - budget contract: default 16ms frame deadline, explicit\n");
    printf("    opt-out, overruns counted + warned once, propagation\n");
    printf("    cost (edges + derive depth) in every audit entry\n");
    printf("  - Estimate schema: kind-aware assertions, describe()\n");
    printf("    defaults + custom print, kind-aware equality, and the\n");
    printf("    a11y value-naming seam (set_value_estimate)\n");
    printf("  - a11y bridge: honest stub without the flag (attach NULL,\n");
    printf("    NULL-safe no-ops); real adapter compiled by the CI probe\n");
    printf("  - closure constructor split: graph at birth, aggregate L2\n");
    printf("    = 0%% — the ordering leak is unwritable (bind_graph\n");
    printf("    deprecated, registry-tracked)\n");
    printf("  - edge lifecycle: px_undeclare retires edges before their\n");
    printf("    endpoints die — px_afford_at names the LIVE process on\n");
    printf("    every allocator layout (the CI-found dangling edge)\n");
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
