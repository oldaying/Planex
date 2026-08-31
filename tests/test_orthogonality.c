/*
 * test_orthogonality.c — essence orthogonality test suite
 *
 * Proves (or exposes gaps in) the claim that Planex's 4 abstractions
 * — Relation, Estimate, Closure, Perception — are truly orthogonal,
 * not implicitly coupled.
 *
 * Each test category addresses one aspect of orthogonality:
 *
 *   A. Removal:     remove abstraction X, others still work?
 *   B. Swap:        swap abstraction X's impl, others unaffected?
 *   C. Composition: any subset used independently?
 *   D. Essence:     do the ADR claims actually hold?
 *   E. Glitch:      known FRP problems — documented, not hidden
 *
 * SKIP markers indicate essence claims that are NOT yet implemented.
 * These are gaps to close, not bugs to hide.
 *
 * Run: make test_ortho
 *      ./build/test_orthogonality
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

static int g_tests_run  = 0;
static int g_tests_pass = 0;
static int g_tests_skip = 0;

#define TEST(name) do {                                  \
    g_tests_run++;                                       \
    printf("  [TEST] %-44s ", #name);                   \
    test_##name();                                       \
    printf("OK\n");                                      \
    g_tests_pass++;                                      \
} while (0)

#define SKIP(name, reason) do {                          \
    g_tests_run++;                                       \
    g_tests_skip++;                                      \
    printf("  [SKIP] %-44s %s\n", #name, reason);      \
} while (0)

/* ============================================================
 * Helpers (shared by multiple tests)
 * ============================================================ */

static void on_inc_simple(px_intent i, void* u) {
    (void)i;
    px_estimate* e = (px_estimate*)u;
    px_estimate_set(e, px_estimate_value(e) + 1.0, 1.0);
}

static void on_set_value(px_intent i, void* u) {
    px_estimate* e = (px_estimate*)u;
    if (i.payload_size == sizeof(int) && i.payload) {
        int v = *(int*)i.payload;
        px_estimate_set(e, (double)v, 1.0);
    }
}

static bool eval_true(void* u) { (void)u; return true; }

/* Perception context — since px_perception_invoke_all() and
 * px_perception_invoke_for_estimate() discard the fn's return value,
 * we store the denotation in the ctx for test verification. */
typedef struct {
    char last_result[64];
    int  invoke_count;
} perc_ctx;

static void* perceive_double_ctx(px_estimate* const* in, int n, void* u) {
    perc_ctx* ctx = (perc_ctx*)u;
    ctx->invoke_count++;
    if (n < 1 || !in) return NULL;
    snprintf(ctx->last_result, sizeof(ctx->last_result),
             "%.0f", px_estimate_value(in[0]) * 2.0);
    return NULL;
}

static void* perceive_noop_ctx(px_estimate* const* in, int n, void* u) {
    perc_ctx* ctx = (perc_ctx*)u;
    ctx->invoke_count++;
    (void)in; (void)n;
    snprintf(ctx->last_result, sizeof(ctx->last_result), "fired");
    return NULL;
}

static double derive_double(px_estimate* const* s, int n, void* u) {
    (void)u; (void)n;
    return px_estimate_value(s[0]) * 2.0;
}

static double derive_plus_one(px_estimate* const* s, int n, void* u) {
    (void)u; (void)n;
    return px_estimate_value(s[0]) + 1.0;
}

static double derive_sum(px_estimate* const* s, int n, void* u) {
    (void)u;
    double sum = 0;
    for (int i = 0; i < n; i++) sum += px_estimate_value(s[i]);
    return sum;
}

static int g_obs_count = 0;
static void observe_inc(px_estimate* e, void* u) {
    (void)e; (void)u;
    g_obs_count++;
}

/* ============================================================
 * A. Removal Tests — abstractions work without each other
 * ============================================================
 *
 * Each test removes ONE abstraction and verifies the others
 * still function. This proves the abstractions are not
 * implicitly required by each other.
 * ============================================================ */

static void test_a1_without_perception(void) {
    /* Setup: graph + estimate + closure + undo.
     * NO perceptions created. */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, count);
    px_closure_bind_graph(inc, g);
    px_declare(g, inc, PX_REL_TRIGGERS, count);

    px_undo_clear();
    px_undo_set_enabled(true);

    /* No perceptions exist throughout */
    assert(px_perception_count() == 0);

    /* Trigger closure → estimate changes, undo recorded */
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 1.0);
    assert(px_undo_count() == 1);
    assert(px_perception_count() == 0);

    /* Undo works without perception */
    int r = px_undo();
    assert(r == 1);
    assert(px_estimate_value(count) == 0.0);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

static void test_a2_without_undo_binding(void) {
    /* Setup: graph + estimate + closure, but closure NOT bound to graph.
     * Undo is globally enabled, but no binding → no snapshots. */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, count);
    px_declare(g, inc, PX_REL_TRIGGERS, count);
    /* NOTE: do NOT call px_closure_bind_graph */

    px_undo_clear();
    px_undo_set_enabled(true);

    /* Trigger — estimate changes, but undo stack stays empty */
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 1.0);
    assert(px_undo_count() == 0);

    /* Trigger again — still no undo recorded */
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 2.0);
    assert(px_undo_count() == 0);

    /* Undo is no-op */
    int r = px_undo();
    assert(r == 0);
    assert(px_estimate_value(count) == 2.0);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

static void test_a3_without_derived(void) {
    /* Setup: estimate + closure + undo, but NO derived estimates.
     * Proves derived is a feature ON TOP of estimate, not required. */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    /* NOTE: no px_derived_new */

    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, count);
    px_closure_bind_graph(inc, g);
    px_declare(g, inc, PX_REL_TRIGGERS, count);

    px_undo_clear();
    px_undo_set_enabled(true);

    /* Add our own observer to count firings */
    g_obs_count = 0;
    px_estimate_observe(count, observe_inc, NULL);

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 1.0);
    /* Only our explicit observer fires (1) — no derived to add extra firings */
    assert(g_obs_count == 1);

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 2.0);
    assert(g_obs_count == 2);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

static void test_a4_without_animate(void) {
    /* Setup: estimate, only use px_estimate_set — never animate.
     * Proves the time dimension is opt-in, not required. */
    px_estimate* e = px_estimate_new(5.0, 1.0);

    /* Never call px_estimate_animate */
    px_estimate_set(e, 10.0, 1.0);
    assert(px_estimate_value(e) == 10.0);
    assert(!px_estimate_is_animating(e));

    /* Sample returns static value (no animation trajectory) */
    double s1 = px_estimate_sample(e, 1000);
    double s2 = px_estimate_sample(e, 5000);
    assert(s1 == 10.0);
    assert(s2 == 10.0);

    px_estimate_free(e);
}

static void test_a5_without_explicit_feedback(void) {
    /* Setup: closure with promise/declare NOT called manually.
     * Proves closure auto-derives status from evaluation alone. */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, count);
    /* NOTE: never call px_closure_promise / declare / fail / set_feedback */

    px_closure_trigger(inc, NULL, 0);

    /* Without explicit feedback, closure auto-sets DONE because eval_true */
    assert(px_closure_get_status(inc) == PX_CLOSURE_DONE);
    /* Feedback auto-generated, not empty */
    assert(px_closure_feedback(inc)[0] != 0);

    px_closure_free(inc);
    px_estimate_free(count);
    px_graph_free(g);
}

/* ============================================================
 * B. Swap Tests — implementation independence
 * ============================================================
 *
 * Each test changes ONE aspect of an abstraction's implementation
 * and verifies others are unaffected. This proves the abstraction
 * boundary is respected.
 * ============================================================ */

static void test_b1_relation_empty_graph(void) {
    /* Use a graph that has NO TRIGGERS edges declared.
     * Undo should be no-op even with bind_graph + enabled. */
    px_graph* g = px_graph_new();
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, e);
    px_closure_bind_graph(inc, g);
    /* NOTE: do NOT declare any TRIGGERS edges */

    px_undo_clear();
    px_undo_set_enabled(true);

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(e) == 1.0);
    /* No TRIGGERS edges → no snapshot recorded */
    assert(px_undo_count() == 0);

    /* Undo is no-op */
    int r = px_undo();
    assert(r == 0);
    assert(px_estimate_value(e) == 1.0);

    px_undo_set_enabled(false);
    px_closure_free(inc);
    px_estimate_free(e);
    px_graph_free(g);
}

static void test_b2_perception_independent_units(void) {
    /* Perceptions are independent units — register 3, free 2, the 3rd still works. */
    px_estimate* e = px_estimate_new(5.0, 1.0);
    px_estimate* srcs[] = { e };

    perc_ctx ctx1 = {0}, ctx2 = {0}, ctx3 = {0};

    int before = px_perception_count();
    px_perception* p1 = px_perception_new("p1", perceive_double_ctx, srcs, 1, &ctx1);
    px_perception* p2 = px_perception_new("p2", perceive_double_ctx, srcs, 1, &ctx2);
    px_perception* p3 = px_perception_new("p3", perceive_double_ctx, srcs, 1, &ctx3);
    assert(px_perception_count() == before + 3);

    /* Invoke all — all 3 fire */
    int invoked = px_perception_invoke_all();
    assert(invoked >= 3);
    assert(ctx1.invoke_count == 1);
    assert(ctx2.invoke_count == 1);
    assert(ctx3.invoke_count == 1);
    assert(strcmp(ctx1.last_result, "10") == 0);
    assert(strcmp(ctx2.last_result, "10") == 0);
    assert(strcmp(ctx3.last_result, "10") == 0);

    /* Free p1 and p2 — p3 still works */
    px_perception_free(p1);
    px_perception_free(p2);
    assert(px_perception_count() == before + 1);

    /* Reset ctx3 and invoke again — only p3 fires */
    ctx3 = (perc_ctx){0};
    px_perception_invoke_for_estimate(e);
    assert(ctx3.invoke_count == 1);
    assert(strcmp(ctx3.last_result, "10") == 0);
    /* ctx1 and ctx2 should NOT have been invoked (they're freed) */
    assert(ctx1.invoke_count == 1);  /* unchanged from before */
    assert(ctx2.invoke_count == 1);

    /* Change estimate — p3 reads live state */
    ctx3 = (perc_ctx){0};
    px_estimate_set(e, 7.0, 1.0);
    px_perception_invoke_for_estimate(e);
    assert(strcmp(ctx3.last_result, "14") == 0);

    px_perception_free(p3);
    px_estimate_free(e);
}

static void test_b3_time_source_deterministic(void) {
    /* Animation sampling is a pure function of t.
     * Same t → same value, regardless of wall clock. */
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate_animate(e, 100.0, 1000.0);

    /* Sample at specific t — values determined by ease-out formula */
    double v0   = px_estimate_sample(e, 0);
    double v250 = px_estimate_sample(e, 250);
    double v500 = px_estimate_sample(e, 500);
    double v1k  = px_estimate_sample(e, 1000);

    assert(v0   == 0.0);
    /* ease-out: 1 - (1-p)^2 where p = t/dur
     * p=0.25 → eased=0.4375 → v=43.75 */
    assert(v250 > 43.0 && v250 < 44.0);
    /* p=0.5 → eased=0.75 → v=75 */
    assert(v500 == 75.0);
    /* p=1.0 → eased=1.0 → v=100 */
    assert(v1k  == 100.0);

    /* Same t → same value (purity) */
    double v250_again = px_estimate_sample(e, 250);
    assert(v250 == v250_again);

    px_estimate_free(e);
}

/* ============================================================
 * C. Composition Tests — any subset used independently
 * ============================================================
 *
 * Each test uses only a SUBSET of the 4 abstractions and
 * verifies they work without the others.
 * ============================================================ */

static void test_c1_only_relation_estimate(void) {
    /* Only Relation + Estimate — no closures, no perceptions */
    px_graph* g = px_graph_new();
    px_estimate* a = px_estimate_new(0, 1.0);
    px_estimate* b = px_estimate_new(0, 1.0);

    /* Declare relations directly between estimates */
    px_declare(g, a, PX_REL_VARIES_WITH, b);
    px_declare(g, a, PX_REL_DEPENDS_ON, b);

    assert(px_graph_count(g) == 2);
    assert(px_has_relation(g, a, PX_REL_VARIES_WITH, b));
    assert(px_has_relation(g, a, PX_REL_DEPENDS_ON, b));

    /* Query works */
    px_node_list list = px_query(g, a, PX_REL_VARIES_WITH);
    assert(list.count == 1);
    assert(list.items[0] == b);
    px_node_list_free(&list);

    px_estimate_free(a);
    px_estimate_free(b);
    px_graph_free(g);
}

static void test_c2_only_estimate_closure(void) {
    /* Estimate + Closure only — no graph, no perceptions */
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* inc = px_closure_new("inc", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, count);
    /* NOTE: no graph, no bind_graph, no perceptions */

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 1.0);

    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(count) == 2.0);

    /* Closure + Estimate work without Relation and Perception */
    px_closure_free(inc);
    px_estimate_free(count);
}

static void test_c3_only_estimate_perception(void) {
    /* Estimate + Perception only — no closures, no relations */
    px_estimate* e = px_estimate_new(42.0, 1.0);
    px_estimate* srcs[] = { e };

    perc_ctx ctx = {0};
    int before = px_perception_count();
    px_perception* p = px_perception_new("solo", perceive_double_ctx, srcs, 1, &ctx);
    assert(px_perception_count() == before + 1);

    /* Perception reads estimate value — invoke via estimate */
    px_perception_invoke_for_estimate(e);
    assert(ctx.invoke_count == 1);
    assert(strcmp(ctx.last_result, "84") == 0);  /* 42 * 2 */

    /* Change estimate directly — perception reflects new value */
    ctx = (perc_ctx){0};
    px_estimate_set(e, 10.0, 1.0);
    px_perception_invoke_for_estimate(e);
    assert(strcmp(ctx.last_result, "20") == 0);

    px_perception_free(p);
    px_estimate_free(e);
}

static int g_c4_action_count = 0;
static void on_c4_action(px_intent i, void* u) {
    (void)i; (void)u;
    g_c4_action_count++;
}

static void test_c4_only_closure_perception(void) {
    /* Closure + Perception, NO estimates, NO graph.
     * Degenerate but proves Closure and Perception can stand alone. */
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST,
        on_c4_action, eval_true, NULL);

    /* Perception with 0 sources — legal but trivial */
    perc_ctx ctx = {0};
    int before = px_perception_count();
    px_perception* p = px_perception_new("solo_p", perceive_noop_ctx, NULL, 0, &ctx);
    assert(px_perception_count() == before + 1);

    /* Closure triggers — action runs (no state to change) */
    int before_count = g_c4_action_count;
    px_closure_trigger(c, NULL, 0);
    assert(g_c4_action_count == before_count + 1);

    /* Perception fires via invoke_all (no estimate to target) */
    int invoked = px_perception_invoke_all();
    assert(invoked >= 1);
    assert(ctx.invoke_count == 1);
    assert(strcmp(ctx.last_result, "fired") == 0);

    px_perception_free(p);
    px_closure_free(c);
}

/* ============================================================
 * D. Essence Claims Validation
 * ============================================================
 *
 * Each test verifies a specific claim made in the ADRs.
 * If a claim is unfulfilled, the test is marked SKIP with
 * a clear reason.
 * ============================================================ */

static void test_d1_intent_replay(void) {
    /* ADR claim: "intent is a value, enables undo/redo/replay/AI agent driving"
     *
     * This test verifies intent CAN be captured and replayed via the
     * px_closure_replay API — fulfilling the ADR claim.
     */
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* set_to = px_closure_new("set to value", PX_INTENT_DECLARE,
        on_set_value, eval_true, count);

    int payload = 42;
    px_closure_trigger(set_to, &payload, sizeof(payload));
    assert(px_estimate_value(count) == 42.0);

    /* Capture last intent — intent is a value */
    px_intent captured = px_closure_last_intent(set_to);
    assert(captured.kind == PX_INTENT_DECLARE);
    assert(captured.payload_size == sizeof(int));
    assert(captured.payload != NULL);
    assert(*(int*)captured.payload == 42);

    /* Reset estimate directly (bypassing closure) */
    px_estimate_set(count, 0, 1.0);
    assert(px_estimate_value(count) == 0.0);

    /* Replay the captured intent — action runs again, state restored */
    px_closure_replay(set_to, captured);
    assert(px_estimate_value(count) == 42.0);

    px_closure_free(set_to);
    px_estimate_free(count);
}

static void test_d5_undo_replay_is_redo(void) {
    /* The killer use case for intent-as-value:
     *
     *   trigger → undo → replay = redo
     *
     * This is impossible in mainstream UI libraries without
     * manually re-calling the action with the original payload.
     * Planex does it via captured intent value + replay API.
     */
    px_graph* g = px_graph_new();
    px_estimate* count = px_estimate_new(0, 1.0);
    px_closure* set_to = px_closure_new("set to value", PX_INTENT_DECLARE,
        on_set_value, eval_true, count);
    px_closure_bind_graph(set_to, g);
    px_declare(g, set_to, PX_REL_TRIGGERS, count);

    px_undo_clear();
    px_undo_set_enabled(true);

    /* 1. Trigger: set count to 42 */
    int payload = 42;
    px_closure_trigger(set_to, &payload, sizeof(payload));
    assert(px_estimate_value(count) == 42.0);
    assert(px_undo_count() == 1);

    /* 2. Capture the intent (intent is a value) */
    px_intent captured = px_closure_last_intent(set_to);
    assert(*(int*)captured.payload == 42);

    /* 3. Undo: count reverts to 0 */
    px_undo();
    assert(px_estimate_value(count) == 0.0);
    assert(px_undo_count() == 0);

    /* 4. Replay: count goes back to 42 (REDO).
     *    Replay snapshots the pre-replay state (0) before action runs,
     *    so undo_count becomes 1 again. */
    px_closure_replay(set_to, captured);
    assert(px_estimate_value(count) == 42.0);
    assert(px_undo_count() == 1);

    /* 5. Undo the replay: count reverts to 0 again */
    px_undo();
    assert(px_estimate_value(count) == 0.0);
    assert(px_undo_count() == 0);

    /* 6. Replay again: count goes back to 42 (redo again).
     *    Same captured intent, replayed multiple times —
     *    proves intent is a value, not a one-shot. */
    px_closure_replay(set_to, captured);
    assert(px_estimate_value(count) == 42.0);

    px_undo_set_enabled(false);
    px_closure_free(set_to);
    px_estimate_free(count);
    px_graph_free(g);
}

static void test_d2_perception_purity(void) {
    /* Perception is a pure function: same input → same output,
     * no side effects on the source. */
    px_estimate* e = px_estimate_new(7.0, 1.0);
    px_estimate* srcs[] = { e };

    perc_ctx ctx1 = {0}, ctx2 = {0};
    px_perception* p1 = px_perception_new("pure1", perceive_double_ctx, srcs, 1, &ctx1);
    px_perception* p2 = px_perception_new("pure2", perceive_double_ctx, srcs, 1, &ctx2);

    /* Same input → same output (both read e=7, both write "14") */
    px_perception_invoke_for_estimate(e);
    assert(strcmp(ctx1.last_result, ctx2.last_result) == 0);
    assert(strcmp(ctx1.last_result, "14") == 0);

    /* Different input → different output */
    ctx1 = (perc_ctx){0};
    ctx2 = (perc_ctx){0};
    px_estimate_set(e, 8.0, 1.0);
    px_perception_invoke_for_estimate(e);
    assert(strcmp(ctx1.last_result, "16") == 0);
    assert(strcmp(ctx2.last_result, "16") == 0);

    /* Perception has no side effects on source estimate */
    double before = px_estimate_value(e);
    px_perception_invoke_for_estimate(e);
    double after = px_estimate_value(e);
    assert(before == after);

    px_perception_free(p1);
    px_perception_free(p2);
    px_estimate_free(e);
}

static void test_d3_undo_scope_correctness(void) {
    /* Two closures, each TRIGGERS a DIFFERENT estimate.
     * Undo of closure B should restore only B's estimate, not A's. */
    px_graph* g = px_graph_new();
    px_estimate* x = px_estimate_new(0, 1.0);
    px_estimate* y = px_estimate_new(0, 1.0);

    px_closure* inc_x = px_closure_new("inc x", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, x);
    px_closure* inc_y = px_closure_new("inc y", PX_INTENT_REQUEST,
        on_inc_simple, eval_true, y);

    /* Each closure TRIGGERS its own estimate ONLY */
    px_declare(g, inc_x, PX_REL_TRIGGERS, x);
    px_declare(g, inc_y, PX_REL_TRIGGERS, y);
    /* NOTE: inc_x does NOT trigger y, inc_y does NOT trigger x */

    px_closure_bind_graph(inc_x, g);
    px_closure_bind_graph(inc_y, g);

    px_undo_clear();
    px_undo_set_enabled(true);

    /* Trigger both */
    px_closure_trigger(inc_x, NULL, 0);
    assert(px_estimate_value(x) == 1.0);
    assert(px_estimate_value(y) == 0.0);

    px_closure_trigger(inc_y, NULL, 0);
    assert(px_estimate_value(x) == 1.0);
    assert(px_estimate_value(y) == 1.0);

    assert(px_undo_count() == 2);

    /* Undo last (inc_y): should restore y to 0, leave x at 1 */
    px_undo();
    assert(px_estimate_value(x) == 1.0);  /* unchanged */
    assert(px_estimate_value(y) == 0.0);  /* restored */

    /* Undo previous (inc_x): should restore x to 0 */
    px_undo();
    assert(px_estimate_value(x) == 0.0);
    assert(px_estimate_value(y) == 0.0);

    assert(px_undo_count() == 0);

    px_undo_set_enabled(false);
    px_closure_free(inc_x);
    px_closure_free(inc_y);
    px_estimate_free(x);
    px_estimate_free(y);
    px_graph_free(g);
}

static void test_d4_observer_transitivity(void) {
    /* Chain: A → B=A*2 → C=B+1
     * When A changes, B should auto-update, then C should auto-update. */
    px_estimate* a = px_estimate_new(5.0, 1.0);
    px_estimate* a_srcs[] = { a };
    px_estimate* b = px_derived_new(derive_double, NULL, a_srcs, 1);
    px_estimate* b_srcs[] = { b };
    px_estimate* c = px_derived_new(derive_plus_one, NULL, b_srcs, 1);

    /* Initial: A=5, B=10, C=11 */
    assert(px_estimate_value(a) == 5.0);
    assert(px_estimate_value(b) == 10.0);
    assert(px_estimate_value(c) == 11.0);

    /* Change A → B and C cascade */
    px_estimate_set(a, 7.0, 1.0);
    assert(px_estimate_value(a) == 7.0);
    assert(px_estimate_value(b) == 14.0);
    assert(px_estimate_value(c) == 15.0);

    /* Change A again — verify cascade remains correct */
    px_estimate_set(a, 0.0, 1.0);
    assert(px_estimate_value(a) == 0.0);
    assert(px_estimate_value(b) == 0.0);
    assert(px_estimate_value(c) == 1.0);

    px_estimate_free(c);
    px_estimate_free(b);
    px_estimate_free(a);
}

/* ============================================================
 * E. Glitch Detection — known FRP problems, documented
 * ============================================================
 *
 * Glitches are intermediate inconsistent states observed during
 * a multi-source update. Conal Elliott's FRP papers discuss this
 * extensively. Planex inherits the problem by being FRP-inspired.
 *
 * These tests DOCUMENT the glitches — they are not bugs to fix
 * in tests, they are architectural properties to be aware of.
 * ============================================================ */

static int g_e1_obs_count = 0;
static void observe_e1(px_estimate* e, void* u) {
    (void)e; (void)u;
    g_e1_obs_count++;
}

static void test_e1_multi_source_glitch(void) {
    /* S = A + B.
     * When A and B both change "logically together" (in 2 separate sets),
     * S observer fires TWICE — once with intermediate state (S=1),
     * once with final state (S=2).
     *
     * Ideal system would batch updates and fire S observer once with
     * final value (S=2). Planex currently fires twice.
     *
     * This test DOCUMENTS the glitch. */
    px_estimate* a = px_estimate_new(0, 1.0);
    px_estimate* b = px_estimate_new(0, 1.0);
    px_estimate* srcs[] = { a, b };
    px_estimate* s = px_derived_new(derive_sum, NULL, srcs, 2);

    g_e1_obs_count = 0;
    px_estimate_observe(s, observe_e1, NULL);

    /* "Logical transaction": set A=1 AND B=1, but in 2 separate calls */
    px_estimate_set(a, 1.0, 1.0);  /* S=1, observer fires */
    px_estimate_set(b, 1.0, 1.0);  /* S=2, observer fires again */

    /* Final state is correct */
    assert(px_estimate_value(s) == 2.0);

    /* But observer fired twice — glitch */
    assert(g_e1_obs_count == 2);
    printf("(glitch: %d firings, ideal=1) ", g_e1_obs_count);

    px_estimate_free(s);
    px_estimate_free(b);
    px_estimate_free(a);
}

static int g_e2_obs_count = 0;
static void observe_e2(px_estimate* e, void* u) {
    (void)e; (void)u;
    g_e2_obs_count++;
}

static void test_e2_cascading_glitch(void) {
    /* A → B=A*2 → C=B+1.
     * When A changes, B fires, then C fires (cascade).
     * C observer fires exactly ONCE because the cascade is single-source
     * at each level — no intermediate bad state.
     *
     * Multi-source is where glitches happen (see e1). */
    px_estimate* a = px_estimate_new(5.0, 1.0);
    px_estimate* a_srcs[] = { a };
    px_estimate* b = px_derived_new(derive_double, NULL, a_srcs, 1);
    px_estimate* b_srcs[] = { b };
    px_estimate* c = px_derived_new(derive_plus_one, NULL, b_srcs, 1);

    g_e2_obs_count = 0;
    px_estimate_observe(c, observe_e2, NULL);

    px_estimate_set(a, 7.0, 1.0);

    /* Final value correct */
    assert(px_estimate_value(c) == 15.0);
    /* Observer fired exactly once (single cascade, no glitch) */
    assert(g_e2_obs_count == 1);

    px_estimate_free(c);
    px_estimate_free(b);
    px_estimate_free(a);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex orthogonality test suite\n");
    printf("================================\n");
    printf("Proves (or exposes gaps in) the claim that the 4 abstractions\n");
    printf("— Relation, Estimate, Closure, Perception — are truly orthogonal.\n\n");

    printf("[A] Removal Tests — abstractions work without each other\n");
    TEST(a1_without_perception);
    TEST(a2_without_undo_binding);
    TEST(a3_without_derived);
    TEST(a4_without_animate);
    TEST(a5_without_explicit_feedback);

    printf("\n[B] Swap Tests — implementation independence\n");
    TEST(b1_relation_empty_graph);
    TEST(b2_perception_independent_units);
    TEST(b3_time_source_deterministic);

    printf("\n[C] Composition Tests — any subset used independently\n");
    TEST(c1_only_relation_estimate);
    TEST(c2_only_estimate_closure);
    TEST(c3_only_estimate_perception);
    TEST(c4_only_closure_perception);

    printf("\n[D] Essence Claims Validation\n");
    TEST(d1_intent_replay);
    TEST(d2_perception_purity);
    TEST(d3_undo_scope_correctness);
    TEST(d4_observer_transitivity);
    TEST(d5_undo_replay_is_redo);

    printf("\n[E] Glitch Detection — known FRP problems, documented\n");
    TEST(e1_multi_source_glitch);
    TEST(e2_cascading_glitch);
    SKIP(e3_transaction_rollback, "(transaction API not implemented)");

    printf("\n----------------\n");
    printf("%d/%d passed (%d skipped)\n",
        g_tests_pass, g_tests_run, g_tests_skip);
    printf("\nEssence gaps documented:\n");
    printf("  - transaction begin/commit/rollback (E3 skipped)\n");
    printf("  - glitch-free multi-source update   (E1 documented)\n");
    printf("\nEssence gaps closed by this suite:\n");
    printf("  - px_closure_replay(intent)         (D1 + D5 PASS)\n");
    return (g_tests_pass + g_tests_skip == g_tests_run) ? 0 : 1;
}
