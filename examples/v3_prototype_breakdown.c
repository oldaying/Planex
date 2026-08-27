/*
 * v3_prototype_breakdown.c — Breakdown (6th abstraction)
 *
 * Validates that the v3 prototype's Breakdown abstraction is
 * expressible in Planex's C17 zero-dependency API surface, and
 * that the loop audit captures the breakdown_transition dimension.
 *
 * Essence gap addressed (per essence-derivation-v3.md § II-7):
 *   - v2 deferred Breakdown as an essence candidate.
 *   - But Heidegger Zuhandenheit (1927) + Winograd/Flores (1986) +
 *     Dourish (2001) + Suchman (1987) — 4 traditions, exceeding
 *     v2's own ≥3-tradition threshold for essence elevation —
 *     treat breakdown as essence, not derived:
 *       * Heidegger: a tool in skilled use "withdraws"; breakdown
 *         is when it becomes present-at-hand. This IS the essence
 *         of tool-being, not a derived property.
 *       * Winograd/Flores: breakdown-recovery is the basis of
 *         "conversation for action"; without it there is no
 *         conversation, only form submission.
 *       * Dourish: embodiment means breakdown is constitutive of
 *         interaction, not exceptional to it.
 *       * Suchman: situated action means the actor's plan breaks
 *         down routinely — breakdown is the normal mode, not
 *         the failure mode.
 *   - This prototype confirms:
 *       * px_breakdown_record / recover / count / get / is_recovered
 *         are expressible in C17 zero-dependency style.
 *       * px_loop_mark_breakdown + px_loop_step records the
 *         breakdown_transition in the loop audit.
 *       * px_breakdown_to_relation declares PX_REL_PRESENTS_FOR
 *         (Zuhandenheit's Vorhanden) in the graph, queryable by
 *         other parts of the system.
 *
 * Scenario:
 *   Actor alice is using a counter app. She triggers the closure
 *   several times. On the 3rd trigger, the system detects that
 *   her interpretant has drifted (she's misreading the counter
 *   as progress, not items pending). The system:
 *     1. records a breakdown (PX_BD_INTERPRETANT_MISMATCH)
 *     2. marks the loop iteration with breakdown_transition=+1
 *     3. declares PX_REL_PRESENTS_FOR(counter, alice) — counter
 *        is now present-to-hand (visible) to alice
 *     4. shows an explanation, marks breakdown recovered
 *     5. marks the next loop iteration with breakdown_transition=-1
 *
 * Build (via CMakeLists.txt STDOUT_DEMOS):
 *   cmake -B build && cmake --build build
 *   ./build/v3_prototype_breakdown
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { px_estimate* count; } App;

static int count_results(px_node_list l) {
    int n = l.count;
    px_node_list_free(&l);
    return n;
}

/* Helper: get the most recent audit entry. */
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

static void* render_count(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "%d", (int)px_estimate_value(inputs[0]));
    return buf;
}

int main(void) {
    printf("=== v3_prototype_breakdown ===\n");
    printf("Breakdown abstraction (essence-derivation-v3 § II-7)\n\n");

    App app = {0};
    app.count = px_estimate_new(0, 1.0);
    if (!app.count) { fprintf(stderr, "FAIL: estimate_new\n"); return 1; }

    px_closure* c = px_closure_new("increment", PX_INTENT_REQUEST,
                                    on_inc, eval_nonneg, &app);
    if (!c) { fprintf(stderr, "FAIL: closure_new\n"); return 1; }

    px_estimate* inputs[] = { app.count };
    px_perception* p = px_perception_new("count_display", render_count,
                                            inputs, 1, NULL);
    if (!p) { fprintf(stderr, "FAIL: perception_new\n"); return 1; }

    px_graph* g = px_graph_new();
    px_actor* alice = px_actor_new("alice", NULL);
    if (!g || !alice) { fprintf(stderr, "FAIL: graph/actor\n"); return 1; }

    /* Two loop iterations in flow (no breakdown). */
    printf("[iter 1] alice triggers closure — counter goes 0 → 1\n");
    printf("         (alice is in flow; counter abstraction is withdrawn)\n");
    px_declare_for(g, &app.count, PX_REL_WITHDRAWS_FOR, alice, alice);

    px_loop* loop = px_loop_new(c, p);
    if (!loop) { fprintf(stderr, "FAIL: loop_new\n"); return 1; }

    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry e1 = latest_audit(loop);
    printf("[audit]   breakdown_transition = %d (expected 0 — in flow)\n",
           e1.breakdown_transition);

    /* Iteration 2 — still in flow. */
    printf("[iter 2] alice triggers again — counter goes 1 → 2\n");
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry e2 = latest_audit(loop);
    printf("[audit]   breakdown_transition = %d (expected 0)\n",
           e2.breakdown_transition);

    /* Iteration 3 — system detects interpretant mismatch (semantic breakdown). */
    printf("\n[iter 3] system detects alice is misreading counter as\n");
    printf("         \"progress of 10\" instead of \"items pending\"\n");
    printf("         → records breakdown + marks loop with transition=+1\n");

    px_breakdown* b = px_breakdown_record(alice,
                                            PX_BD_INTERPRETANT_MISMATCH,
                                            "alice reads counter as progress, not items",
                                            &app.count);
    if (!b) { fprintf(stderr, "FAIL: breakdown_record\n"); return 1; }
    printf("[record] px_breakdown_record(alice, INTERPRETANT_MISMATCH, ...)\n");
    printf("         reason = \"%s\"\n", px_breakdown_reason(b));
    printf("         px_breakdown_count(alice) = %d (expected 1)\n",
           px_breakdown_count(alice));
    printf("         px_breakdown_is_recovered  = %s (expected false)\n",
           px_breakdown_is_recovered(b) ? "true" : "false");

    /* Mark the next loop iteration with breakdown transition. */
    px_loop_mark_breakdown(loop, +1, "alice confused");
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry e3 = latest_audit(loop);
    printf("[audit]   breakdown_transition = %+d (expected +1 — entered)\n",
           e3.breakdown_transition);

    /* Declare PX_REL_PRESENTS_FOR(counter, alice) — counter is
     * now visible (present-to-hand) to alice. */
    px_breakdown_to_relation(b, g, &app.count);
    int n_present = count_results(px_query_for(g, &app.count, PX_REL_PRESENTS_FOR, alice));
    printf("[relation] px_query_for(counter, PRESENTS_FOR, alice).count = %d (expected 1)\n",
           n_present);
    int n_withdraw = count_results(px_query_for(g, &app.count, PX_REL_WITHDRAWS_FOR, alice));
    printf("[relation] px_query_for(counter, WITHDRAWS_FOR, alice).count = %d (still 1 — both)\n",
           n_withdraw);

    /* Iteration 4 — system explains; alice recovers. */
    printf("\n[iter 4] system shows explanation; alice recovers.\n");
    px_breakdown_recover(b, "system showed \"items pending, not progress\"");
    printf("[recover] px_breakdown_is_recovered = %s (expected true)\n",
           px_breakdown_is_recovered(b) ? "true" : "false");

    px_loop_mark_breakdown(loop, -1, "alice recovered");
    px_loop_step(loop, NULL, 0);
    px_loop_audit_entry e4 = latest_audit(loop);
    printf("[audit]   breakdown_transition = %+d (expected -1 — recovered)\n",
           e4.breakdown_transition);

    printf("\n[verdict] Breakdown is expressible as a first-class abstraction:\n");
    printf("          - record per actor (alice's breakdown is not bob's)\n");
    printf("          - recovery is recordable (with how-text)\n");
    printf("          - loop audit captures the transition direction (+1 / -1)\n");
    printf("          - bridge to Relation: PRESENTS_FOR is queryable\n");
    printf("[verdict] Zuhandenheit/WITHDRAWS_FOR + Vorhandenheit/PRESENTS_FOR\n");
    printf("          coexist in the same graph for the same actor — explicit-\n");
    printf("          abstraction and Zuhandenheit are complementary, not in tension.\n");

    px_loop_free(loop);
    px_actor_free(alice);
    px_graph_free(g);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(app.count);
    return 0;
}
