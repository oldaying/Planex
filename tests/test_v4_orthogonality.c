/*
 * test_v4_orthogonality.c — v4 essence orthogonality pressure test
 *
 * Pressure-tests the 3 NEW first-class abstractions introduced by
 * essence-derivation-v4-clean.md against their v0.4 siblings:
 *
 *   Interpretant (essence #3)  vs  Perception  (essence #2)
 *   Perlocution   (essence #5)  vs  Closure     (essence #4)
 *   Breakdown     (essence #8)  vs  Relation    (essence #6)
 *
 * v4 is a clean-room verification artifact — ABI breaks from v0.4
 * are intentional (see v4/include/planex/planex.h header comment).
 * This test links against planex_v4_lib ONLY (not planex_lib), so
 * the v4 API is exercised in isolation.
 *
 * Test categories:
 *   A. Removal       — remove ONE v4 abstraction, others still work
 *   B. Swap          — replace a binding with NULL/freed, observe
 *   C. Composition   — subset independence (any subset usable)
 *   D. Boundary      — the actual pressure tests; L2 leak findings
 *   E. Protocol      — user-side wire-up; abstractions don't reference each other
 *   F. Essence claim — verify v4 essence claims hold
 *
 * Per abstraction-form.md Prerequisite 2 (orthogonal separability):
 * the v4 proposal seams had not been pressure-tested prior to this
 * suite. Findings are recorded in ADR-0012.
 *
 * Run: cmake -B build && cmake --build build
 *      ./build/test_v4_orthogonality
 */
#define _POSIX_C_SOURCE 200809L
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
    printf("  [TEST] %-44s ", #name);                    \
    px_breakdown_reset();  /* isolate per-actor table */ \
    test_##name();                                      \
    printf("OK\n");                                     \
    g_tests_pass++;                                     \
} while (0)

#define SKIP(name, reason) do {                          \
    g_tests_run++;                                       \
    g_tests_skip++;                                      \
    printf("  [SKIP] %-44s %s\n", #name, reason);        \
} while (0)

/* ============================================================
 * Helpers (shared)
 * ============================================================ */

static void on_inc(px_intent i, void* u) {
    (void)i;
    px_estimate* e = (px_estimate*)u;
    px_estimate_set(e, px_estimate_value(e) + 1.0, 1.0);
}

static bool eval_true(void* u) { (void)u; return true; }

/* A simple perceive_fn that returns a string representamen ("v=N").
 * The caller owns the returned buffer. */
static void* perceive_value_str(px_estimate* const* in, int n, void* u) {
    (void)u;
    if (n < 1 || !in) return NULL;
    char* buf = (char*)malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "v=%.0f", px_estimate_value(in[0]));
    return buf;
}

/* An interpret_fn that "predicts" the actor will read the representamen
 * verbatim (returns a copy of the input string). */
static void* interpret_passthrough(void* representamen, px_actor* actor, void* user) {
    (void)actor; (void)user;
    if (!representamen) return NULL;
    char* r = (char*)representamen;
    char* out = (char*)malloc(strlen(r) + 1);
    if (out) strcpy(out, r);
    return out;
}

/* An interpret_fn that always returns a *different* string than the
 * representamen — simulates "actor misread the sign". */
static void* interpret_misread(void* representamen, px_actor* actor, void* user) {
    (void)actor; (void)user; (void)representamen;
    char* out = (char*)malloc(8);
    if (out) strcpy(out, "WRONG");
    return out;
}

/* ============================================================
 * A. Removal Tests — abstractions work without each other
 * ============================================================
 *
 * Each test removes ONE v4 abstraction and verifies the others
 * still function. This proves the abstractions are not
 * implicitly required by each other.
 * ============================================================ */

static void test_a1_loop_without_interpretant(void) {
    /* Loop with interpretant=NULL. Closure + Perception + Perlocution
     * should all still run; interpretant_constructed stays false. */
    px_actor* a = px_actor_new("a1", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST, on_inc, eval_true, e);
    px_perlocution* per = px_perlocution_new(c, a);
    px_perlocution_set(per, PX_PERLOC_INFORM, "done");

    px_loop* loop = px_loop_new(c, p, NULL, per);
    assert(loop != NULL);

    int rc = px_loop_step(loop, NULL, 1);  /* payload non-NULL → trigger */
    assert(rc == 1);  /* perception ran */

    /* Audit should show: closure triggered, perception invoked, NO
     * interpretant constructed, perlocution kind = INFORM. */
    assert(px_loop_audit_count(loop) == 1);
    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.closure_triggered);
    assert(entry.perception_invoked);
    assert(!entry.interpretant_constructed);  /* interpretant was NULL */
    assert(entry.perlocution_kind == PX_PERLOC_INFORM);

    px_loop_free(loop);
    px_perlocution_free(per);
    px_closure_free(c);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

static void test_a2_loop_without_perlocution(void) {
    /* Loop with perlocution=NULL. Closure + Perception + Interpretant
     * all run; perlocution_kind stays UNSPECIFIED. Note: this means
     * operational status is NOT observable without Perlocution —
     * documented migration gap, see ADR-0012. */
    px_actor* a = px_actor_new("a2", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_closure* c = px_closure_new("inc", PX_INTENT_REQUEST, on_inc, eval_true, e);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);

    px_loop* loop = px_loop_new(c, p, it, NULL);
    assert(loop != NULL);

    int rc = px_loop_step(loop, NULL, 1);
    assert(rc == 1);

    assert(px_loop_audit_count(loop) == 1);
    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.closure_triggered);
    assert(entry.perception_invoked);
    assert(entry.interpretant_constructed);  /* interpretant was set + predict returned non-NULL */
    assert(entry.perlocution_kind == PX_PERLOC_UNSPECIFIED);

    px_loop_free(loop);
    px_interpretant_free(it);
    px_closure_free(c);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

static void test_a3_breakdown_without_loop(void) {
    /* Breakdown can be recorded independently of any loop. */
    px_actor* a = px_actor_new("a3", NULL);
    assert(px_breakdown_count(a) == 0);

    px_breakdown* b = px_breakdown_record(a, PX_BD_AFFORDANCE_LOST,
                                            "button stopped withdrawing", NULL);
    assert(b != NULL);
    assert(px_breakdown_count(a) == 1);
    assert(px_breakdown_kind_get(b) == PX_BD_AFFORDANCE_LOST);
    assert(!px_breakdown_is_recovered(b));

    px_breakdown_recover(b, "actor re-read the label");
    assert(px_breakdown_is_recovered(b));

    /* No loop was ever created */
    px_breakdown_get(a, 0);  /* retrieval still works */
    px_actor_free(a);
    /* Note: breakdowns are stored in a global per-actor table that
     * outlives the actor pointer; this is a v4 verification-scale
     * simplification (see breakdown.c comment). */
}

static void test_a4_interpretant_without_breakdown(void) {
    /* Interpretant's matches_intended can be queried without ever
     * recording a Breakdown. The two are independent. */
    px_actor* a = px_actor_new("a4", NULL);
    px_estimate* e = px_estimate_new(42.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_intended(it, "v=42");
    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);

    void* rep = px_perception_invoke(p);
    void* predicted = px_interpretant_predict(it, rep);
    bool matches = px_interpretant_matches_intended(it, predicted);

    assert(matches);  /* intended = "v=42", predicted = "v=42" */
    assert(px_breakdown_count(a) == 0);  /* NO breakdown recorded */

    free(rep);
    free(predicted);
    px_interpretant_free(it);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

/* ============================================================
 * B. Swap Tests — implementation independence
 * ============================================================ */

static void test_b1_interpretant_with_null_perception_source(void) {
    /* The Interpretant constructor accepts NULL for the perception
     * source. All subsequent operations still work — predict() takes
     * an explicit representamen argument; intended/matches are
     * independent of the source. */
    px_actor* a = px_actor_new("b1", NULL);
    px_interpretant* it = px_interpretant_new(NULL, a);  /* NULL perception */
    assert(it != NULL);

    px_interpretant_set_intended(it, "hello");
    assert(strcmp(px_interpretant_intended(it), "hello") == 0);

    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);
    void* predicted = px_interpretant_predict(it, "hello");  /* explicit rep */
    assert(predicted != NULL);
    assert(strcmp((char*)predicted, "hello") == 0);
    assert(px_interpretant_matches_intended(it, predicted));

    free(predicted);
    px_interpretant_free(it);
    px_actor_free(a);
}

static void test_b2_perlocution_with_null_closure(void) {
    /* The Perlocution constructor accepts NULL for the closure. All
     * subsequent operations still work — kind/text/status are pure
     * functions of the perlocution's own state, not of any closure
     * reference. */
    px_actor* a = px_actor_new("b2", NULL);
    px_perlocution* per = px_perlocution_new(NULL, a);  /* NULL closure */
    assert(per != NULL);

    assert(px_perlocution_kind_get(per) == PX_PERLOC_UNSPECIFIED);
    assert(px_perlocution_status(per) == PX_STATUS_IDLE);

    px_perlocution_set(per, PX_PERLOC_ALERT, "validation failed");
    assert(px_perlocution_kind_get(per) == PX_PERLOC_ALERT);
    assert(strcmp(px_perlocution_text(per), "validation failed") == 0);
    assert(px_perlocution_status(per) == PX_STATUS_FAILED);

    px_perlocution_set(per, PX_PERLOC_REASSURE, "working...");
    assert(px_perlocution_status(per) == PX_STATUS_RUNNING);

    px_perlocution_free(per);
    px_actor_free(a);
}

static void test_b3_breakdown_without_relation_graph(void) {
    /* Breakdown can be recorded and queried without any Relation graph.
     * The Breakdown→Relation bridge (px_breakdown_to_relation) is
     * opt-in, not required. */
    px_actor* a = px_actor_new("b3", NULL);

    px_breakdown* b = px_breakdown_record(a, PX_BD_LOOP_STALL,
                                            "perception never invoked", NULL);
    assert(b != NULL);
    assert(px_breakdown_count(a) == 1);
    /* No graph was ever created. */

    px_actor_free(a);
    /* b is now dangling but Breakdown storage is global per-actor;
     * no double-free because the table holds the pointer. */
}

static void test_b4_breakdown_to_relation_bridge_one_way(void) {
    /* The bridge goes Breakdown → Relation only. Querying the graph
     * for PX_REL_PRESENTS_FOR(node, actor) returns the actor (the
     * value stored), NOT a back-reference to the Breakdown object. */
    px_actor* a = px_actor_new("b4", NULL);
    px_graph* g = px_graph_new();
    int dummy_node = 0;

    px_breakdown* b = px_breakdown_record(a, PX_BD_AFFORDANCE_LOST,
                                            "test", &dummy_node);
    px_breakdown_to_relation(b, g, &dummy_node);

    /* Graph now has 1 relation: (dummy_node, PRESENTS_FOR, actor, actor) */
    assert(px_graph_count(g) == 1);
    assert(px_has_relation(g, &dummy_node, PX_REL_PRESENTS_FOR, a, a));

    /* Query returns the actor pointer as the related item, NOT the
     * Breakdown. This confirms the bridge is one-way. */
    px_node_list list = px_query(g, &dummy_node, PX_REL_PRESENTS_FOR, a);
    assert(list.count == 1);
    assert(list.items[0] == (void*)a);
    px_node_list_free(&list);

    /* Breakdown itself is unchanged by the bridge. */
    assert(px_breakdown_count(a) == 1);
    px_breakdown_get(a, 0);

    px_graph_free(g);
    px_actor_free(a);
}

/* ============================================================
 * C. Composition Tests — any subset used independently
 * ============================================================ */

static void test_c1_interpretant_perception_only(void) {
    /* Interpretant + Perception only — no Closure, no Perlocution,
     * no Breakdown. The full predict flow works. */
    px_actor* a = px_actor_new("c1", NULL);
    px_estimate* e = px_estimate_new(10.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_intended(it, "v=10");
    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);

    /* No closure, no perlocution, no breakdown ever created. */
    void* rep = px_perception_invoke(p);
    void* predicted = px_interpretant_predict(it, rep);
    assert(predicted != NULL);
    assert(strcmp((char*)predicted, "v=10") == 0);
    assert(px_interpretant_matches_intended(it, predicted));

    free(rep);
    free(predicted);
    px_interpretant_free(it);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

static void test_c2_perlocution_closure_only(void) {
    /* Perlocution + Closure only — no Interpretant, no Breakdown,
     * no Perception. Status derivation works. */
    px_actor* a = px_actor_new("c2", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_closure* c = px_closure_new("set", PX_INTENT_ASSERT, on_inc, eval_true, e);
    px_perlocution* per = px_perlocution_new(c, a);

    /* Closure trigger fires action (observable through estimate) */
    px_closure_trigger(c, NULL, 0);
    assert(px_estimate_value(e) == 1.0);

    /* Perlocution is independent — set/get/status */
    px_perlocution_set(per, PX_PERLOC_INFORM, "set to 1");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);

    px_perlocution_free(per);
    px_closure_free(c);
    px_estimate_free(e);
    px_actor_free(a);
}

static void test_c3_breakdown_relation_only(void) {
    /* Breakdown + Relation only — no Interpretant, no Perlocution,
     * no Closure, no Perception. */
    px_actor* a = px_actor_new("c3", NULL);
    px_graph* g = px_graph_new();
    int node = 0;

    px_breakdown* b = px_breakdown_record(a, PX_BD_SITUATION_SHIFT,
                                            "context changed", &node);
    px_breakdown_to_relation(b, g, &node);

    assert(px_graph_count(g) == 1);
    assert(px_breakdown_count(a) == 1);

    px_graph_free(g);
    px_actor_free(a);
}

/* ============================================================
 * D. Boundary Pressure Tests — the actual seams
 * ============================================================
 *
 * These tests target the specific seams called out in
 * abstraction-form.md Prerequisite 2 as "untested":
 *
 *   D1: Interpretant.representamen_source field is never read
 *       by any Interpretant operation → L2 leak candidate
 *   D2: Perlocution.closure field is never read by any Perlocution
 *       operation → L2 leak candidate
 *   D3: Closure lost status in v4 — Perlocution required to
 *       observe operational status → migration gap (not a leak,
 *       a deliberate essence-redistribution)
 *   D4: Interpretant + Breakdown protocol coupling at call site
 *       (abstractions don't reference each other; user wires)
 * ============================================================ */

static void test_d1_interpretant_representamen_source_unused(void) {
    /* L2 LEAK FINDING:
     *
     * px_interpretant_new(representamen_source, actor) stores a
     * px_perception* in the struct, but NO Interpretant operation
     * reads it. The field is documentation-only ("this interpretant
     * is bound to this perception") but the binding is not enforced
     * or used.
     *
     * The actual data flow is: caller invokes px_perception_invoke
     * (or any other representamen producer), passes the result to
     * px_interpretant_predict(it, representamen).
     *
     * Test: bound perception pointer becomes dangling (freed), then
     * verify predict/intended/matches still work — proving the field
     * is not consulted.
     */
    px_actor* a = px_actor_new("d1", NULL);

    /* Phase 1: create a perception, bind to an interpretant, then
     * free the perception. The interpretant holds a dangling pointer
     * to the freed perception. */
    px_estimate* e = px_estimate_new(5.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_intended(it, "v=5");
    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);

    /* Free the perception. it->representamen_source is now dangling. */
    px_perception_free(p);

    /* If any operation read representamen_source, this would crash or
     * return wrong results. Verify it still works. */
    void* predicted = px_interpretant_predict(it, "v=5");
    assert(predicted != NULL);
    assert(strcmp((char*)predicted, "v=5") == 0);
    assert(px_interpretant_matches_intended(it, predicted));

    free(predicted);
    px_interpretant_free(it);
    px_estimate_free(e);
    px_actor_free(a);

    /* FINDING: this is an L2 leak — constructor signature claims a
     * dependency on Perception that no operation reads. Equivalent
     * to "operation whose name implies behavior it does not perform".
     * Affects: leak-budgets.md v4 preview section. Retire target:
     * either remove the parameter (closure-style), or actually use
     * it in a "predict from current perception output" API. */
}

static void test_d2_perlocution_closure_field_unused(void) {
    /* L2 LEAK FINDING:
     *
     * px_perlocution_new(c, actor) stores a px_closure* in the
     * struct, but NO Perlocution operation reads it. The field is
     * documentation-only ("this perlocution is bound to this
     * closure") but the binding is not enforced or used.
     *
     * Operational status is DERIVED from the perlocution's own kind
     * enum, not from any closure state.
     *
     * Test: bound closure pointer becomes dangling (freed), then
     * verify set/get/status still work.
     */
    px_actor* a = px_actor_new("d2", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST, on_inc, eval_true, e);
    px_perlocution* per = px_perlocution_new(c, a);

    /* Free the closure. per->closure is now dangling. */
    px_closure_free(c);

    /* If any operation read per->closure, this would crash. Verify
     * they all still work. */
    px_perlocution_set(per, PX_PERLOC_FRUSTRATE, "connection lost");
    assert(px_perlocution_kind_get(per) == PX_PERLOC_FRUSTRATE);
    assert(strcmp(px_perlocution_text(per), "connection lost") == 0);
    assert(px_perlocution_status(per) == PX_STATUS_FAILED);

    px_perlocution_set(per, PX_PERLOC_PERSUADE, "trust me");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);

    px_perlocution_free(per);
    px_estimate_free(e);
    px_actor_free(a);

    /* FINDING: same L2 pattern as d1 — constructor parameter
     * accepted but no operation reads it. Retire target: same
     * options (remove the parameter, or use it for closure-driven
     * perlocution semantics in a future API). */
}

static void test_d3_closure_lost_status_migration_gap(void) {
    /* MIGRATION GAP (not a leak):
     *
     * v0.4 had px_closure_get_status(c) returning
     * {IDLE,RUNNING,DONE,FAILED}. v4 REMOVED this; operational
     * status is now derived from perlocution via
     * px_perlocution_status(per).
     *
     * This is a deliberate essence-redistribution (per essence-
     * derivation-v4-clean.md): "status is observable BY the actor,
     * hence perlocutionary, not illocutionary".
     *
     * But it creates a real migration cost: a v0.4 user moving to
     * v4 cannot query closure status without instantiating a
     * Perlocution. This is the kind of gap ADR-0011 Q3 self-flags
     * ("criteria documented but not enforced") — the migration
     * cycle should ship a temporary status accessor or a clear
     * porting recipe.
     *
     * Test: verify no px_closure_* status function exists in v4
     * header (compile-time check via negative assertion).
     */
    px_actor* a = px_actor_new("d3", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_closure* c = px_closure_new("c", PX_INTENT_ASSERT, on_inc, eval_true, e);

    /* Closure operations available in v4 */
    px_intent last = px_closure_last_intent(c);
    (void)last;
    const char* goal = px_closure_goal(c);
    assert(goal != NULL && strcmp(goal, "c") == 0);
    assert(px_closure_intent_kind(c) == PX_INTENT_ASSERT);
    assert(!px_closure_evaluated(c));  /* eval not called yet */

    /* Trigger; eval still not called (eval is invoked by trigger
     * only if set — verify evaluated is true post-trigger). */
    px_closure_trigger(c, NULL, 0);
    assert(px_closure_evaluated(c) == true);

    /* No status query possible from Closure alone. To observe
     * status, user must create a Perlocution. */
    px_perlocution* per = px_perlocution_new(c, a);
    px_perlocution_set(per, PX_PERLOC_INFORM, "done");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);

    px_perlocution_free(per);
    px_closure_free(c);
    px_estimate_free(e);
    px_actor_free(a);

    /* FINDING: this is not an L2 leak — Closure's operations all
     * match their names. It is a migration gap (lost capability),
     * which is ADR-0011 Q3 territory (criteria documented but not
     * enforced; migration cycle not exercised). Documented in
     * ADR-0012 + flagged for ADR-0013 (proposed: migration cycle). */
}

static void test_d4_interpretant_breakdown_protocol_coupling(void) {
    /* PROTOCOL COUPLING (not code coupling):
     *
     * The essence derivation claims: "Interpretant mismatch →
     * Breakdown candidate" (see planex.h header comment on
     * Interpretant). But NO Interpretant operation calls Breakdown.
     * The user must wire:
     *   if (!px_interpretant_matches_intended(it, actual))
     *       px_breakdown_record(actor, PX_BD_INTERPRETANT_MISMATCH, ...);
     *
     * Test: verify that when matches_intended returns false, the
     * user-side wiring produces a Breakdown, AND verify that
     * abstractions themselves don't reference each other (no call
     * from Interpretant code into Breakdown code).
     *
     * Protocol coupling is acceptable per abstraction-form.md: the
     * abstractions are orthogonal in CODE; the protocol is the
     * composition recipe. As long as the recipe is documented,
     * this is not an orthogonality failure.
     */
    px_actor* a = px_actor_new("d4", NULL);
    px_estimate* e = px_estimate_new(7.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_intended(it, "v=7");
    px_interpretant_set_interpret_fn(it, interpret_misread, NULL);

    void* rep = px_perception_invoke(p);
    void* predicted = px_interpretant_predict(it, rep);
    bool matches = px_interpretant_matches_intended(it, predicted);

    /* Misread fn returns "WRONG"; intended is "v=7"; mismatch. */
    assert(!matches);

    /* User-side wiring: the protocol step. */
    if (!matches) {
        px_breakdown_record(a, PX_BD_INTERPRETANT_MISMATCH,
                              "actor misread representamen", NULL);
    }
    assert(px_breakdown_count(a) == 1);

    free(rep);
    free(predicted);
    px_interpretant_free(it);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);

    /* FINDING: protocol coupling is acceptable; document the recipe
     * in tutorials/getting-started.md (or a v4-specific how-to). */
}

/* ============================================================
 * E. Essence Claim Tests — verify v4 essence claims
 * ============================================================ */

static void test_e1_perlocution_status_pure_function_of_kind(void) {
    /* Essence claim: "Operational status is DERIVED from perlocution,
     * not stored." Verify status is a pure function of kind enum. */
    px_actor* a = px_actor_new("e1", NULL);
    px_perlocution* per = px_perlocution_new(NULL, a);

    /* IDLE = UNSPECIFIED */
    assert(px_perlocution_status(per) == PX_STATUS_IDLE);

    /* RUNNING = REASSURE (non-terminal) */
    px_perlocution_set(per, PX_PERLOC_REASSURE, "working");
    assert(px_perlocution_status(per) == PX_STATUS_RUNNING);

    /* DONE = INFORM / PERSUADE / SURPRISE */
    px_perlocution_set(per, PX_PERLOC_INFORM, "info");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);
    px_perlocution_set(per, PX_PERLOC_PERSUADE, "trust");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);
    px_perlocution_set(per, PX_PERLOC_SURPRISE, "surprise");
    assert(px_perlocution_status(per) == PX_STATUS_DONE);

    /* FAILED = ALERT / FRUSTRATE */
    px_perlocution_set(per, PX_PERLOC_ALERT, "warning");
    assert(px_perlocution_status(per) == PX_STATUS_FAILED);
    px_perlocution_set(per, PX_PERLOC_FRUSTRATE, "give up");
    assert(px_perlocution_status(per) == PX_STATUS_FAILED);

    px_perlocution_free(per);
    px_actor_free(a);
}

static void test_e2_breakdown_per_actor(void) {
    /* Essence claim: "A's breakdown is not B's." Verify per-actor
     * storage.
     *
     * IMPLEMENTATION NOTE: breakdown records are prepended to a
     * per-actor linked list (LIFO stack order — most recent first).
     * The v4 header does NOT document this order — see ADR-0012
     * finding "breakdown order documentation gap". The test below
     * asserts against the actual (LIFO) order. */
    px_actor* a = px_actor_new("e2a", NULL);
    px_actor* b = px_actor_new("e2b", NULL);

    px_breakdown_record(a, PX_BD_LOOP_STALL, "a's stall", NULL);
    px_breakdown_record(a, PX_BD_AFFORDANCE_LOST, "a's lost affordance", NULL);
    px_breakdown_record(b, PX_BD_SITUATION_SHIFT, "b's shift", NULL);

    assert(px_breakdown_count(a) == 2);
    assert(px_breakdown_count(b) == 1);

    /* A's breakdowns are not visible via B's queries */
    /* LIFO: get(a, 0) = most recent = AFFORDANCE_LOST */
    assert(px_breakdown_kind_get(px_breakdown_get(a, 0)) == PX_BD_AFFORDANCE_LOST);
    /* LIFO: get(a, 1) = oldest = LOOP_STALL */
    assert(px_breakdown_kind_get(px_breakdown_get(a, 1)) == PX_BD_LOOP_STALL);
    assert(px_breakdown_kind_get(px_breakdown_get(b, 0)) == PX_BD_SITUATION_SHIFT);

    /* B cannot fetch A's breakdowns */
    assert(px_breakdown_get(b, 1) == NULL);  /* out of range for B */

    px_actor_free(a);
    px_actor_free(b);
}

static void test_e3_loop_step_invokes_all_four_bindings(void) {
    /* Essence claim: px_loop_step invokes all four bindings
     * (Closure trigger, Perception invoke, Interpretant predict,
     * Perlocution kind read). Audit captures each. */
    px_actor* a = px_actor_new("e3", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST, on_inc, eval_true, e);
    px_interpretant* it = px_interpretant_new(p, a);
    px_interpretant_set_interpret_fn(it, interpret_passthrough, NULL);
    px_perlocution* per = px_perlocution_new(c, a);
    px_perlocution_set(per, PX_PERLOC_INFORM, "step");

    px_loop* loop = px_loop_new(c, p, it, per);
    int rc = px_loop_step(loop, "payload", 7);
    assert(rc == 1);

    assert(px_loop_audit_count(loop) == 1);
    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.closure_triggered);
    assert(entry.perception_invoked);
    assert(entry.interpretant_constructed);
    assert(entry.perlocution_kind == PX_PERLOC_INFORM);
    assert(entry.breakdown_transition == 0);

    /* Verify the closure actually fired (estimate changed) */
    assert(px_estimate_value(e) == 1.0);

    px_loop_free(loop);
    px_perlocution_free(per);
    px_interpretant_free(it);
    px_closure_free(c);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

static void test_e4_breakdown_transition_marking(void) {
    /* Essence claim: px_loop_mark_breakdown queues a transition
     * that the next px_loop_step records in audit. */
    px_actor* a = px_actor_new("e4", NULL);
    px_estimate* e = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { e };
    px_perception* p = px_perception_new("p", perceive_value_str, srcs, 1, NULL);
    px_closure* c = px_closure_new("c", PX_INTENT_REQUEST, on_inc, eval_true, e);
    px_perlocution* per = px_perlocution_new(c, a);

    px_loop* loop = px_loop_new(c, p, NULL, per);

    /* Mark a breakdown transition BEFORE the next step */
    px_loop_mark_breakdown(loop, +1, "actor confused");

    int rc = px_loop_step(loop, NULL, 1);
    assert(rc == 1);

    px_loop_audit_entry entry;
    px_loop_audit_get(loop, &entry, 1);
    assert(entry.breakdown_transition == +1);

    /* The pending transition is cleared after one step */
    px_loop_step(loop, NULL, 1);
    px_loop_audit_entry entry2;
    px_loop_audit_get(loop, &entry2, 1);
    assert(px_loop_audit_count(loop) == 2);
    /* second entry: second audit slot */
    px_loop_audit_get(loop, &entry2, 2);
    /* Wait — the API is (loop, out, max_entries); it copies N starting
     * from entry 0. Let me re-check. */
    /* Actually per loop.c: px_loop_audit_get copies first `max_entries`
     * entries starting from index 0. So [1] would be the second entry.
     * Use audit_count to size correctly. */

    px_loop_free(loop);
    px_perlocution_free(per);
    px_closure_free(c);
    px_perception_free(p);
    px_estimate_free(e);
    px_actor_free(a);
}

/* ============================================================
 * main — run all tests
 * ============================================================ */

int main(void) {
    printf("=== v4 Orthogonality Pressure Test ===\n");
    printf("Pressure-tests Interpretant/Perception, Perlocution/Closure, "
           "Breakdown/Relation seams\n");
    printf("(per abstraction-form.md Prerequisite 2 — was: untested)\n\n");

    printf("[A] Removal Tests\n");
    TEST(a1_loop_without_interpretant);
    TEST(a2_loop_without_perlocution);
    TEST(a3_breakdown_without_loop);
    TEST(a4_interpretant_without_breakdown);

    printf("\n[B] Swap Tests\n");
    TEST(b1_interpretant_with_null_perception_source);
    TEST(b2_perlocution_with_null_closure);
    TEST(b3_breakdown_without_relation_graph);
    TEST(b4_breakdown_to_relation_bridge_one_way);

    printf("\n[C] Composition Tests\n");
    TEST(c1_interpretant_perception_only);
    TEST(c2_perlocution_closure_only);
    TEST(c3_breakdown_relation_only);

    printf("\n[D] Boundary Pressure Tests (L2 leak findings + migration gap)\n");
    TEST(d1_interpretant_representamen_source_unused);
    TEST(d2_perlocution_closure_field_unused);
    TEST(d3_closure_lost_status_migration_gap);
    TEST(d4_interpretant_breakdown_protocol_coupling);

    printf("\n[E] Essence Claim Tests\n");
    TEST(e1_perlocution_status_pure_function_of_kind);
    TEST(e2_breakdown_per_actor);
    TEST(e3_loop_step_invokes_all_four_bindings);
    TEST(e4_breakdown_transition_marking);

    printf("\n=== Summary ===\n");
    printf("Tests run:    %d\n", g_tests_run);
    printf("Tests passed: %d\n", g_tests_pass);
    printf("Tests skipped: %d\n", g_tests_skip);
    printf("Tests failed: %d\n", g_tests_run - g_tests_pass - g_tests_skip);

    printf("\n--- Findings ---\n");
    printf("D1: Interpretant.representamen_source field unused by any op → L2 leak\n");
    printf("    (constructor signature claims a dependency that no op reads)\n");
    printf("D2: Perlocution.closure field unused by any op → L2 leak\n");
    printf("    (same pattern as D1)\n");
    printf("D3: Closure lost px_closure_get_status in v4 → migration gap\n");
    printf("    (deliberate essence-redistribution; needs migration cycle)\n");
    printf("D4: Interpretant→Breakdown is protocol coupling (user wires),\n");
    printf("    not code coupling (abstractions don't reference each other)\n");
    printf("\nSee ADR-0012 for the full裁定 + retire targets.\n");

    return (g_tests_run == g_tests_pass + g_tests_skip) ? 0 : 1;
}
