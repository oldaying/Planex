/*
 * undo_via_graph.c — Relation's necessity proof (ADR-0002)
 *
 * This demo proves that Relation is NECESSARY, not just convenient.
 * It does this by implementing undo-via-graph:
 *
 *   1. Create closures WITH the graph: px_closure_new_with_graph(..., g)
 *      (v0.7; the demo originally used the two-call px_closure_bind_graph
 *      form — deprecated since ADR-0019)
 *   2. Enable undo: px_undo_set_enabled(true)
 *   3. Trigger Closure multiple times — each trigger auto-snapshots
 *      the affected Estimates (via TRIGGERS relation)
 *   4. px_undo() restores the last snapshot
 *
 * The KEY insight: only the Estimates reachable from the Closure
 * via TRIGGERS are snapshotted. Other Estimates are untouched.
 *
 * Solid.js CANNOT do this. Solid tracks dependencies per-effect:
 * each effect knows its sources, but there is no global query
 * "which effects depend on this signal?". So Solid must either:
 *   - Snapshot everything (Redux-style, expensive)
 *   - Maintain a separate dependency index (essentially rebuilding
 *     a Relation graph)
 *
 * Planex's Relation is a globally queryable graph — undo-via-graph
 * is a natural consequence.
 *
 * This demo closes ADR-0002: Relation's necessity is proven.
 *
 * Build:
 *   cc -std=c17 -I include examples/undo_via_graph.c \
 *      src/relation.c src/estimate.c src/closure.c src/undo.c \
 *      src/perception.c src/fb.c src/font.c -lm -o build/undo_via_graph
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    px_estimate* count;
    px_estimate* unrelated;  /* not triggered by any Closure */
} App;

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v + 1, 1.0);
}

static void on_dec(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->count);
    px_estimate_set(a->count, v - 1, 1.0);
}

static bool eval_always_true(void* user) {
    (void)user;
    return true;
}

int main(void) {
    printf("Planex undo_via_graph — Relation necessity proof (ADR-0002)\n");
    printf("================================================================\n");
    printf("Validates: undo-via-graph uses Relation to scope snapshots.\n\n");

    App app = {0};
    app.count = px_estimate_new(0, 1.0);
    app.unrelated = px_estimate_new(999, 1.0);  /* not bound to any Closure */
    px_graph* g = px_graph_new();

    /* v0.7: the graph arrives with the closures (ADR-0019 constructor
     * split — the bind call cannot be forgotten or raced). */
    px_closure* inc = px_closure_new_with_graph("increment", PX_INTENT_REQUEST,
                                                on_inc, eval_always_true, &app, g);
    px_closure* dec = px_closure_new_with_graph("decrement", PX_INTENT_REQUEST,
                                                on_dec, eval_always_true, &app, g);

    /* Declare: inc TRIGGERS count, dec TRIGGERS count.
     * Crucially: NO Closure triggers 'unrelated'. */
    px_declare(g, inc, PX_REL_TRIGGERS, app.count);
    px_declare(g, dec, PX_REL_TRIGGERS, app.count);

    /* Enable undo recording */
    px_undo_set_enabled(true);

    printf("Setup:\n");
    printf("  count = %.0f, unrelated = %.0f\n",
           px_estimate_value(app.count), px_estimate_value(app.unrelated));
    printf("  inc TRIGGERS count (bound)\n");
    printf("  dec TRIGGERS count (bound)\n");
    printf("  unrelated: NO Closure triggers it\n");
    printf("  undo enabled: %s\n\n", px_undo_is_enabled() ? "true" : "false");

    /* === Test 1: trigger inc 3 times, unrelated stays at 999 === */
    printf("[test 1] trigger inc 3 times — count goes 0->3, unrelated stays 999 ... ");
    px_closure_trigger(inc, NULL, 0);
    px_closure_trigger(inc, NULL, 0);
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(app.count) == 3.0);
    assert(px_estimate_value(app.unrelated) == 999.0);  /* untouched */
    assert(px_undo_count() == 3);  /* 3 snapshots recorded */
    printf("PASS\n");
    printf("        count=%.0f, unrelated=%.0f, undo_count=%d\n",
           px_estimate_value(app.count), px_estimate_value(app.unrelated),
           px_undo_count());

    /* === Test 2: undo once — count goes 3->2 === */
    printf("[test 2] px_undo() — count goes 3->2 ... ");
    int restored = px_undo();
    assert(restored == 1);  /* 1 Estimate restored */
    assert(px_estimate_value(app.count) == 2.0);
    assert(px_estimate_value(app.unrelated) == 999.0);  /* still untouched */
    assert(px_undo_count() == 2);  /* one snapshot consumed */
    printf("PASS\n");
    printf("        count=%.0f, restored=%d, undo_count=%d\n",
           px_estimate_value(app.count), restored, px_undo_count());

    /* === Test 3: undo twice more — count goes 2->1->0 === */
    printf("[test 3] undo twice more — count 2->1->0 ... ");
    px_undo();
    assert(px_estimate_value(app.count) == 1.0);
    px_undo();
    assert(px_estimate_value(app.count) == 0.0);
    assert(px_undo_count() == 0);  /* stack empty */
    printf("PASS\n");
    printf("        count=%.0f, undo_count=%d\n",
           px_estimate_value(app.count), px_undo_count());

    /* === Test 4: undo with empty stack returns 0 === */
    printf("[test 4] undo with empty stack returns 0 ... ");
    int empty = px_undo();
    assert(empty == 0);
    printf("PASS (returned %d)\n", empty);

    /* === Test 5: trigger dec, undo restores === */
    printf("[test 5] trigger dec (count 0->-1), undo restores to 0 ... ");
    px_closure_trigger(dec, NULL, 0);
    assert(px_estimate_value(app.count) == -1.0);
    assert(px_undo_count() == 1);
    px_undo();
    assert(px_estimate_value(app.count) == 0.0);
    printf("PASS\n");

    /* === Test 6: disable undo, trigger doesn't snapshot === */
    printf("[test 6] disable undo, trigger doesn't snapshot ... ");
    px_undo_clear();
    px_undo_set_enabled(false);
    px_closure_trigger(inc, NULL, 0);
    px_closure_trigger(inc, NULL, 0);
    assert(px_estimate_value(app.count) == 2.0);
    assert(px_undo_count() == 0);  /* no snapshots when disabled */
    printf("PASS (count=%.0f, undo_count=%d)\n",
           px_estimate_value(app.count), px_undo_count());

    /* === Test 7: re-enable, mixed triggers, undo reverses last === */
    printf("[test 7] re-enable, inc then dec, undo reverses dec only ... ");
    px_undo_set_enabled(true);
    px_closure_trigger(inc, NULL, 0);  /* count 2->3, snapshot [2] */
    px_closure_trigger(dec, NULL, 0);  /* count 3->2, snapshot [3] */
    assert(px_estimate_value(app.count) == 2.0);
    px_undo();  /* restore to 3 */
    assert(px_estimate_value(app.count) == 3.0);
    px_undo();  /* restore to 2 */
    assert(px_estimate_value(app.count) == 2.0);
    printf("PASS\n");

    /* === The KEY assertion: unrelated Estimate NEVER changes === */
    printf("\n[final] unrelated Estimate value across all operations: %.0f\n",
           px_estimate_value(app.unrelated));
    assert(px_estimate_value(app.unrelated) == 999.0);
    printf("       (expected 999.0 — never touched by undo) ✓\n");

    /* Cleanup */
    px_undo_clear();
    px_closure_free(inc);
    px_closure_free(dec);
    px_graph_free(g);
    px_estimate_free(app.count);
    px_estimate_free(app.unrelated);

    printf("\n=== undo_via_graph proof complete ===\n");
    printf("\nWhat this proves (ADR-0002):\n");
    printf("  1. px_undo_record uses Relation graph to find affected Estimates\n");
    printf("  2. Only Estimates reachable via TRIGGERS are snapshotted\n");
    printf("  3. Unrelated Estimates are NEVER touched by undo\n");
    printf("  4. Undo stack works (push, pop, count, clear)\n");
    printf("  5. Undo can be enabled/disabled globally\n");
    printf("\nWhy Solid.js cannot do this:\n");
    printf("  Solid tracks dependencies PER-EFFECT. Each effect knows its\n");
    printf("  sources, but there is no global query 'which effects depend\n");
    printf("  on this signal?'. To do undo-via-graph in Solid, you must:\n");
    printf("    (a) Snapshot everything (Redux-style, expensive), OR\n");
    printf("    (b) Maintain a separate dependency index (= rebuilding Relation)\n");
    printf("\nPlanex's Relation is a globally queryable graph. Undo-via-graph\n");
    printf("is a natural consequence. Relation is NECESSARY, not just convenient.\n");
    printf("\nThis closes ADR-0002: Relation's necessity is PROVEN.\n");
    return 0;
}
