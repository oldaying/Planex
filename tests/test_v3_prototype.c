/*
 * test_v3_prototype.c — assertion test suite for v3 prototype
 *
 * Validates that the 4 new essence categories from
 * essence-derivation-v3.md (Interpretant, Perlocution, Breakdown,
 * 3-place Relation) are correctly implemented in Planex's C17
 * zero-dependency API surface, and that the v3 prototype does NOT
 * break v0.4's existing API (Estimate, Closure, Perception,
 * Relation, px_loop all still work via the old 2-place signatures).
 *
 * Test groups:
 *   T1. v0.4 backward-compat: px_declare / px_query still work
 *       (the v3 prototype must not break existing API).
 *   T2. 3-place Relation: px_declare_for / px_query_for respect
 *       the actor parameter (universal relations match every
 *       actor query; actor-scoped relations match only that actor).
 *   T3. Closure perlocution sub-API: set/get, kind/text, str helper.
 *   T4. Perception interpretant sub-API: intended_interpretant +
 *       interpret_fn (success + failure paths).
 *   T5. px_loop audit extension: perlocution_kind + interpretant_constructed
 *       + breakdown_transition fields are populated correctly.
 *   T6. Breakdown abstraction: record / recover / count / get /
 *       is_recovered / to_relation bridge.
 *
 * Each test prints [PASS]/[FAIL] and increments a counter.
 * Exit code = number of failures (0 = all passed).
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define TEST(name) printf("  [%s] %s\n", __func__, (name))

/* Helper: get the most recent audit entry (px_loop_audit_get returns
 * entries in chronological order — oldest first. To inspect what
 * the LAST px_loop_step did, we need the newest entry.) */
static px_loop_audit_entry latest_audit(px_loop* loop) {
    px_loop_audit_entry e = {0};
    int n = px_loop_audit_count(loop);
    if (n <= 0) return e;
    px_loop_audit_entry* all = malloc(sizeof(*all) * n);
    if (!all) return e;
    px_loop_audit_get(loop, all, n);
    e = all[n - 1];
    free(all);
    return e;
}

#define ASSERT_EQ(actual, expected, msg)                                     \
    do {                                                                     \
        if ((actual) == (expected)) {                                        \
            printf("    [PASS] %s (got %d)\n", (msg), (int)(actual));         \
            g_pass++;                                                         \
        } else {                                                              \
            printf("    [FAIL] %s (expected %d, got %d)\n",                   \
                   (msg), (int)(expected), (int)(actual));                    \
            g_fail++;                                                         \
        }                                                                     \
    } while (0)

#define ASSERT_TRUE(x, msg)  ASSERT_EQ((x) ? 1 : 0, 1, msg)
#define ASSERT_FALSE(x, msg) ASSERT_EQ((x) ? 1 : 0, 0, msg)

#define ASSERT_STR_EQ(actual, expected, msg)                                  \
    do {                                                                      \
        if ((actual) && (expected) && strcmp((actual), (expected)) == 0) {    \
            printf("    [PASS] %s (got \"%s\")\n", (msg), (actual));            \
            g_pass++;                                                           \
        } else if ((actual) == NULL && (expected) == NULL) {                   \
            printf("    [PASS] %s (both NULL)\n", (msg));                       \
            g_pass++;                                                           \
        } else {                                                                \
            printf("    [FAIL] %s (expected \"%s\", got \"%s\")\n",             \
                   (msg), (expected) ? (expected) : "(null)",                   \
                   (actual) ? (actual) : "(null)");                            \
            g_fail++;                                                           \
        }                                                                       \
    } while (0)

/* ============== T1: v0.4 backward-compat ============== */

static void test_v04_backward_compat(void) {
    TEST("v0.4 backward-compat: px_declare / px_query");
    px_graph* g = px_graph_new();
    int n1, n2;
    px_relation* r = px_declare(g, &n1, PX_REL_TRIGGERS, &n2);
    ASSERT_TRUE(r != NULL, "px_declare returns non-NULL");
    ASSERT_EQ(px_graph_count(g), 1, "graph count is 1");

    px_node_list l = px_query(g, &n1, PX_REL_TRIGGERS);
    ASSERT_EQ(l.count, 1, "px_query finds the declared relation");
    px_node_list_free(&l);

    px_node_list l2 = px_query(g, &n1, PX_REL_AFFORDS);
    ASSERT_EQ(l2.count, 0, "px_query for undeclared kind returns 0");
    px_node_list_free(&l2);

    px_graph_free(g);
}

/* ============== T2: 3-place Relation ============== */

static void test_3place_relation(void) {
    TEST("3-place Relation: actor parameter respected");
    px_graph* g = px_graph_new();
    int node1, node2;
    px_actor* alice = px_actor_new("alice", NULL);
    px_actor* bob   = px_actor_new("bob", NULL);

    /* Universal relation: matches any actor. */
    px_declare(g, &node1, PX_REL_TRIGGERS, &node2);
    /* Actor-scoped: only alice. */
    px_declare_for(g, &node1, PX_REL_AFFORDS, &node2, alice);

    /* Universal matches every actor query. */
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_TRIGGERS, alice).count, 1,
             "alice sees universal TRIGGERS");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_TRIGGERS, bob).count, 1,
             "bob sees universal TRIGGERS");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_TRIGGERS, NULL).count, 1,
             "NULL actor sees universal TRIGGERS");

    /* Actor-scoped only matches that actor. */
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_AFFORDS, alice).count, 1,
             "alice sees her AFFORDS");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_AFFORDS, bob).count, 0,
             "bob does NOT see alice's AFFORDS");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_AFFORDS, NULL).count, 0,
             "NULL actor does NOT see alice-scoped AFFORDS");

    /* Old 2-place px_query is identical to px_query_for with NULL actor. */
    ASSERT_EQ(px_query(g, &node1, PX_REL_AFFORDS).count, 0,
             "old px_query is identical to NULL-actor query");

    /* Zuhandenheit kind: WITHDRAWS_FOR. */
    px_declare_for(g, &node1, PX_REL_WITHDRAWS_FOR, alice, alice);
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_WITHDRAWS_FOR, alice).count, 1,
             "alice has her own WITHDRAWS_FOR");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_WITHDRAWS_FOR, bob).count, 0,
             "bob has no WITHDRAWS_FOR on node1");

    /* Vorhandenheit kind: PRESENTS_FOR. */
    px_declare_for(g, &node1, PX_REL_PRESENTS_FOR, alice, alice);
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_PRESENTS_FOR, alice).count, 1,
             "alice has her PRESENTS_FOR");
    ASSERT_EQ(px_query_for(g, &node1, PX_REL_PRESENTS_FOR, bob).count, 0,
             "bob has no PRESENTS_FOR on node1");

    px_actor_free(alice);
    px_actor_free(bob);
    px_graph_free(g);
}

/* ============== T3: Closure perlocution ============== */

static void on_noop(px_intent intent, void* user) { (void)intent; (void)user; }
static bool eval_true(void* user) { (void)user; return true; }

static void test_closure_perlocution(void) {
    TEST("Closure perlocution sub-API");
    px_closure* c = px_closure_new("test", PX_INTENT_REQUEST,
                                     on_noop, eval_true, NULL);
    ASSERT_TRUE(c != NULL, "closure created");

    /* Default perlocution is UNSPECIFIED. */
    ASSERT_EQ((int)px_closure_perlocution_kind(c),
             (int)PX_PERLOC_UNSPECIFIED,
             "default perlocution = UNSPECIFIED");
    ASSERT_STR_EQ(px_closure_perlocution_text(c), "",
                 "default perlocution_text is empty");

    /* Set INFORM. */
    px_closure_set_perlocution(c, PX_PERLOC_INFORM, "Saved.");
    ASSERT_EQ((int)px_closure_perlocution_kind(c),
             (int)PX_PERLOC_INFORM,
             "perlocution = INFORM after set");
    ASSERT_STR_EQ(px_closure_perlocution_text(c), "Saved.",
                 "perlocution_text = \"Saved.\"");

    /* Set ALERT. */
    px_closure_set_perlocution(c, PX_PERLOC_ALERT,
                              "Saved. 3 fields were auto-corrected.");
    ASSERT_EQ((int)px_closure_perlocution_kind(c),
             (int)PX_PERLOC_ALERT,
             "perlocution = ALERT after set");
    ASSERT_STR_EQ(px_closure_perlocution_text(c),
                 "Saved. 3 fields were auto-corrected.",
                 "perlocution_text updated");

    /* str helper. */
    ASSERT_STR_EQ(px_perlocution_kind_str(PX_PERLOC_UNSPECIFIED),
                 "UNSPECIFIED", "str(UNSPECIFIED)");
    ASSERT_STR_EQ(px_perlocution_kind_str(PX_PERLOC_INFORM),
                 "INFORM", "str(INFORM)");
    ASSERT_STR_EQ(px_perlocution_kind_str(PX_PERLOC_ALERT),
                 "ALERT", "str(ALERT)");
    ASSERT_STR_EQ(px_perlocution_kind_str(PX_PERLOC_SURPRISE),
                 "SURPRISE", "str(SURPRISE)");

    px_closure_free(c);
}

/* ============== T4: Perception interpretant ============== */

static void* render_static(px_estimate* const* inputs, int n, void* user) {
    (void)inputs; (void)n; (void)user;
    return strdup("static");
}

static void* interpret_static(void* repr, px_actor* actor, void* user) {
    (void)actor; (void)user;
    /* Return non-NULL iff repr is non-NULL — simulates a successful
     * interpretation when there is something to interpret. */
    return repr ? (void*)"predicted_interpretant" : NULL;
}

static void* interpret_null(void* repr, px_actor* actor, void* user) {
    (void)repr; (void)actor; (void)user;
    return NULL;  /* always fails to predict */
}

static void test_perception_interpretant(void) {
    TEST("Perception interpretant sub-API");
    px_perception* p = px_perception_new("test_p", render_static, NULL, 0, NULL);
    ASSERT_TRUE(p != NULL, "perception created");

    /* Default: no intended_interpretant. */
    ASSERT_TRUE(px_perception_intended_interpretant(p) == NULL,
               "default intended_interpretant is NULL");

    /* Set intended_interpretant. */
    px_perception_set_intended_interpretant(p, "seven items pending");
    ASSERT_STR_EQ(px_perception_intended_interpretant(p),
                 "seven items pending",
                 "intended_interpretant set correctly");

    /* Without interpret_fn, px_perception_interpret returns NULL. */
    ASSERT_TRUE(px_perception_interpret(p, "7", NULL) == NULL,
               "no interpret_fn → NULL");

    /* With a successful interpret_fn, returns non-NULL. */
    px_perception_set_interpret_fn(p, interpret_static, NULL);
    ASSERT_TRUE(px_perception_interpret(p, "7", NULL) != NULL,
               "interpret_fn returns non-NULL when repr is non-NULL");
    ASSERT_TRUE(px_perception_interpret(p, NULL, NULL) == NULL,
               "interpret_fn returns NULL when repr is NULL");

    /* With a failing interpret_fn, returns NULL. */
    px_perception_set_interpret_fn(p, interpret_null, NULL);
    ASSERT_TRUE(px_perception_interpret(p, "7", NULL) == NULL,
               "failing interpret_fn returns NULL");

    /* Remove interpret_fn entirely. */
    px_perception_set_interpret_fn(p, NULL, NULL);
    ASSERT_TRUE(px_perception_interpret(p, "7", NULL) == NULL,
               "NULL interpret_fn returns NULL");

    px_perception_free(p);
}

/* ============== T5: px_loop audit extension ============== */

static void test_loop_audit_extension(void) {
    TEST("px_loop audit extension (perlocution + interpretant + breakdown)");
    px_estimate* e = px_estimate_new(0, 1.0);
    px_closure* c = px_closure_new("noop", PX_INTENT_REQUEST,
                                     on_noop, eval_true, NULL);
    px_perception* p = px_perception_new("noop_p", render_static,
                                            &e, 1, NULL);
    px_loop* loop = px_loop_new(c, p);

    /* Case A: no perlocution set, no interpret_fn, no breakdown.
     * Audit should record perlocution=UNSPECIFIED, interpretant_constructed=false,
     * breakdown_transition=0. */
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry ea = latest_audit(loop);
    ASSERT_EQ(ea.perlocution_kind, (int)PX_PERLOC_UNSPECIFIED,
             "Case A: perlocution = UNSPECIFIED");
    ASSERT_FALSE(ea.interpretant_constructed,
                 "Case A: interpretant_constructed = false");
    ASSERT_EQ(ea.breakdown_transition, 0,
             "Case A: breakdown_transition = 0");

    /* Case B: set perlocution + register interpret_fn + mark breakdown.
     * Audit should reflect all three. */
    px_closure_set_perlocution(c, PX_PERLOC_ALERT, "warning");
    px_perception_set_interpret_fn(p, interpret_static, NULL);
    px_loop_mark_breakdown(loop, +1, "actor confused");
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry eb = latest_audit(loop);
    ASSERT_EQ(eb.perlocution_kind, (int)PX_PERLOC_ALERT,
             "Case B: perlocution = ALERT");
    ASSERT_TRUE(eb.interpretant_constructed,
                "Case B: interpretant_constructed = true");
    ASSERT_EQ(eb.breakdown_transition, +1,
             "Case B: breakdown_transition = +1");

    /* Case C: mark recovery.
     * breakdown_transition should be -1. */
    px_loop_mark_breakdown(loop, -1, "actor recovered");
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry ec = latest_audit(loop);
    ASSERT_EQ(ec.breakdown_transition, -1,
             "Case C: breakdown_transition = -1 (recovered)");

    /* Case D: pending breakdown is consumed — next iteration has 0. */
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry ed = latest_audit(loop);
    ASSERT_EQ(ed.breakdown_transition, 0,
             "Case D: breakdown_transition = 0 after consumed");

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(e);
}

/* ============== T6: Breakdown abstraction ============== */

static void test_breakdown_abstraction(void) {
    TEST("Breakdown abstraction (record / recover / count / get / to_relation)");
    px_graph* g = px_graph_new();
    px_actor* alice = px_actor_new("alice", NULL);
    px_actor* bob   = px_actor_new("bob", NULL);
    int node_x;

    /* Initially no breakdowns. */
    ASSERT_EQ(px_breakdown_count(alice), 0, "alice has no breakdowns initially");
    ASSERT_EQ(px_breakdown_count(bob), 0, "bob has no breakdowns initially");

    /* Record a breakdown for alice. */
    px_breakdown* b = px_breakdown_record(alice,
                                            PX_BD_INTERPRETANT_MISMATCH,
                                            "alice misread",
                                            &node_x);
    ASSERT_TRUE(b != NULL, "breakdown_record returns non-NULL");
    ASSERT_EQ(px_breakdown_count(alice), 1, "alice has 1 breakdown");
    ASSERT_EQ(px_breakdown_count(bob), 0, "bob still has 0 breakdowns");
    ASSERT_STR_EQ(px_breakdown_reason(b), "alice misread",
                 "breakdown_reason returns the recorded reason");
    ASSERT_FALSE(px_breakdown_is_recovered(b),
                 "breakdown is not recovered by default");

    /* Recover it. */
    px_breakdown_recover(b, "system explained");
    ASSERT_TRUE(px_breakdown_is_recovered(b),
                "breakdown is recovered after recover()");

    /* Record a second breakdown for alice. */
    px_breakdown* b2 = px_breakdown_record(alice,
                                             PX_BD_AFFORDANCE_LOST,
                                             "tool stopped withdrawing",
                                             &node_x);
    ASSERT_TRUE(b2 != NULL, "second breakdown_record returns non-NULL");
    ASSERT_EQ(px_breakdown_count(alice), 2, "alice has 2 breakdowns now");
    ASSERT_EQ(px_breakdown_count(bob), 0, "bob still has 0 breakdowns");

    /* Get by index — most recent first. */
    px_breakdown* got0 = px_breakdown_get(alice, 0);
    ASSERT_TRUE(got0 != NULL, "get(alice, 0) returns non-NULL");
    ASSERT_TRUE(got0 == b2, "get(alice, 0) is the most recent breakdown (b2)");
    ASSERT_STR_EQ(px_breakdown_reason(got0), "tool stopped withdrawing",
                 "get(alice, 0) reason is correct");

    /* Bridge: declare PRESENTS_FOR in the graph. */
    px_breakdown_to_relation(b, g, &node_x);
    ASSERT_EQ(px_query_for(g, &node_x, PX_REL_PRESENTS_FOR, alice).count, 1,
             "PRESENTS_FOR declared for alice");
    ASSERT_EQ(px_query_for(g, &node_x, PX_REL_PRESENTS_FOR, bob).count, 0,
             "PRESENTS_FOR not declared for bob");

    /* str helper. */
    ASSERT_STR_EQ(px_breakdown_kind_str(PX_BD_INTERPRETANT_MISMATCH),
                 "INTERPRETANT_MISMATCH", "str(INTERPRETANT_MISMATCH)");
    ASSERT_STR_EQ(px_breakdown_kind_str(PX_BD_AFFORDANCE_LOST),
                 "AFFORDANCE_LOST", "str(AFFORDANCE_LOST)");

    px_actor_free(alice);
    px_actor_free(bob);
    px_graph_free(g);

    /* NOTE: breakdowns themselves are not freed individually in this
     * prototype (they live in a global list). This is a documented
     * limitation — a future production version should add
     * px_breakdown_free() and a per-actor cleanup. */
}

/* ============== main ============== */

int main(void) {
    printf("=== test_v3_prototype ===\n");
    printf("Assertion tests for v3 prototype (essence-derivation-v3)\n\n");

    test_v04_backward_compat();
    test_3place_relation();
    test_closure_perlocution();
    test_perception_interpretant();
    test_loop_audit_extension();
    test_breakdown_abstraction();

    printf("\n=== Summary ===\n");
    printf("  Passed: %d\n", g_pass);
    printf("  Failed: %d\n", g_fail);
    printf("  Total:  %d\n", g_pass + g_fail);
    return g_fail;
}
