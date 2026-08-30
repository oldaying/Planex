/*
 * integration_4abs.c — THE demo: 4 abstractions, ALL features, simultaneously
 *
 * This is the ultimate integration demo. Every feature of every
 * abstraction is exercised in one scenario:
 *
 *   Estimate: value + time (animation) + confidence + derived
 *   Closure: REQUEST + PROMISE + DECLARE + auto-eval + audit log
 *   Relation: TRIGGERS graph + undo-via-graph (scoped snapshots)
 *   Perception: 3 simultaneous denotations (visual + a11y + JSON)
 *
 * Scenario: a "smart counter" that can:
 *   - Increment/decrement (REQUEST intent)
 *   - Animate to a target value (time dimension)
 *   - Simulate async "save" with success/failure (PROMISE/DECLARE/FAIL)
 *   - Track confidence (drops on error, recovers on success)
 *   - Derive "doubled" automatically (Relation DEPENDS_ON)
 *   - Undo any operation (Relation graph-driven scoped snapshots)
 *   - Audit every intent (Closure: intent-as-value)
 *   - Render 3 simultaneous denotations (Perception: visual/a11y/json)
 *
 * React equivalent: 5+ libraries
 *   Redux + redux-undo + react-aria + framer-motion + custom logger
 *   = 5 state models, manual sync
 *
 * Planex: 4 abstractions, 1 state model, native integration
 *
 * Build:
 *   cc -std=c17 -I include examples/integration_4abs.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/undo.c src/fb.c src/font.c -lm -o build/integration_4abs
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    /* Estimate: value + time + confidence + derived */
    px_estimate* count;        /* primary state (value + confidence) */
    px_estimate* doubled;      /* derived: count * 2 */

    /* Relation: dependency graph + undo */
    px_graph* graph;

    /* Closure: typed intents with full lifecycle */
    px_closure* inc;
    px_closure* dec;
    px_closure* reset;
    px_closure* animate_to_10;  /* triggers animation */
    px_closure* save;           /* async: PROMISE -> DECLARE/FAIL */

    /* Audit log */
    char audit[4096];
    int audit_n;
    int audit_count;

    /* Perception call counters */
    int renders, a11ys, jsons;
} App;

/* ============================================================
 * Estimate: derived function
 * ============================================================ */

static double derive_doubled(px_estimate* const* s, int n, void* u) {
    (void)u; (void)n;
    return px_estimate_value(s[0]) * 2.0;
}

/* ============================================================
 * Closure: actions with typed Intent + lifecycle
 * ============================================================ */

static void on_inc(px_intent i, void* u) {
    (void)i; App* a=u;
    px_estimate_set(a->count, px_estimate_value(a->count)+1, 1.0);
}
static void on_dec(px_intent i, void* u) {
    (void)i; App* a=u;
    px_estimate_set(a->count, px_estimate_value(a->count)-1, 1.0);
}
static void on_reset(px_intent i, void* u) {
    (void)i; App* a=u;
    px_estimate_set(a->count, 0.0, 1.0);
}
static void on_animate(px_intent i, void* u) {
    (void)i; App* a=u;
    /* Time dimension: animate count from current to 10 over 500ms */
    px_estimate_animate(a->count, 10.0, 500.0);
}
static void on_save(px_intent i, void* u) {
    (void)i; App* a=u;
    /* Async lifecycle: PROMISE -> DECLARE (success) */
    /* In real app, this would be a network call */
    px_closure_promise(a->save, "Saving to server...");

    /* Simulate: success if count >= 0, fail if negative */
    if (px_estimate_value(a->count) >= 0) {
        px_closure_declare(a->save, "Saved successfully");
        /* Confidence stays high on success */
    } else {
        px_closure_fail(a->save, "Cannot save: count is negative");
        /* Confidence drops on failure */
        px_estimate_set(a->count, px_estimate_value(a->count), 0.3);
    }
}

static bool eval_nonneg(void* u) {
    App* a=u;
    return px_estimate_value(a->count) >= 0;
}
static bool eval_true(void* u) { (void)u; return true; }

/* Audit: record every intent (Closure enables this) */
static void audit_log(App* a, const char* name, px_closure* c) {
    px_intent last = px_closure_last_intent(c);
    const char* status = px_closure_status_str(px_closure_get_status(c));
    const char* fb = px_closure_feedback(c);
    a->audit_count++;
    a->audit_n += snprintf(a->audit + a->audit_n,
        sizeof(a->audit) - a->audit_n,
        "  [%d] %s (%s) status=%s",
        a->audit_count, name,
        px_intent_kind_str(last.kind), status);
    if (fb && fb[0]) {
        a->audit_n += snprintf(a->audit + a->audit_n,
            sizeof(a->audit) - a->audit_n, " fb=\"%s\"", fb);
    }
    a->audit_n += snprintf(a->audit + a->audit_n,
        sizeof(a->audit) - a->audit_n, "\n");
}

/* ============================================================
 * Perception: 3 simultaneous denotations
 * ============================================================ */

static void* p_visual(px_estimate* const* in, int n, void* u) {
    App* a=u; a->renders++; (void)n;
    if (n < 2) return NULL;
    double v=px_estimate_value(in[0]);
    double d=px_estimate_value(in[1]);
    double c=px_estimate_confidence(in[0]);
    char* b=malloc(160); if(!b) return NULL;
    const char* conf_label = c >= 0.8 ? "OK" : c >= 0.5 ? "LOW" : "UNRELIABLE";
    snprintf(b,160,"Count: %.0f  Doubled: %.0f  Conf: %.0f%% [%s]  Undo: %d",
        v, d, c*100, conf_label, px_undo_count());
    return b;
}

static void* p_a11y(px_estimate* const* in, int n, void* u) {
    App* a=u; a->a11ys++; (void)n;
    if (n < 2) return NULL;
    double v=px_estimate_value(in[0]);
    double c=px_estimate_confidence(in[0]);
    char* b=malloc(256); if(!b) return NULL;
    if (c < 0.5)
        snprintf(b,256,"WARNING: Counter value %.0f is unreliable (conf=%.0f%%). Doubled: %.0f.", v, c*100, px_estimate_value(in[1]));
    else if (v < 0)
        snprintf(b,256,"ALERT: Counter is negative at %.0f. Doubled: %.0f. Press R to reset.", v, px_estimate_value(in[1]));
    else
        snprintf(b,256,"Counter: %.0f. Doubled: %.0f. Confidence: %.0f%%.", v, px_estimate_value(in[1]), c*100);
    return b;
}

static void* p_json(px_estimate* const* in, int n, void* u) {
    App* a=u; a->jsons++; (void)n;
    if (n < 2) return NULL;
    char* b=malloc(256); if(!b) return NULL;
    snprintf(b,256,"{\"count\":%.0f,\"doubled\":%.0f,\"conf\":%.2f,\"undo\":%d}",
        px_estimate_value(in[0]), px_estimate_value(in[1]),
        px_estimate_confidence(in[0]), px_undo_count());
    return b;
}

/* ============================================================
 * Helper: show all denotations
 * ============================================================ */

static void show(App* a, const char* label) {
    px_estimate* in[]={a->count, a->doubled};
    void* v=p_visual(in,2,a);
    void* s=p_a11y(in,2,a);
    void* j=p_json(in,2,a);
    printf("  [%s]\n", label);
    printf("    vis:  %s\n", (char*)v);
    printf("    a11y: %s\n", (char*)s);
    printf("    json: %s\n", (char*)j);
    free(v); free(s); free(j);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex integration_4abs — ALL features, ALL abstractions\n");
    printf("========================================================\n");
    printf("Estimate: value + time(animate) + confidence + derived\n");
    printf("Closure:  REQUEST + PROMISE + DECLARE + auto-eval + audit\n");
    printf("Relation: TRIGGERS graph + undo-via-graph\n");
    printf("Percept:  3 denotations (visual + a11y + json)\n\n");

    printf("React equivalent: 5+ libs (Redux+undo, react-aria, framer-motion,\n");
    printf("  custom logger, React Testing Library) = 5 state models, manual sync\n");
    printf("Planex: 4 abstractions, 1 state model, native integration\n\n");

    App a={0};
    a.graph=px_graph_new();
    a.count=px_estimate_new(0, 1.0);
    px_estimate* srcs[]={a.count};
    a.doubled=px_derived_new(derive_doubled, NULL, srcs, 1);

    /* Closures: 5 different Intent kinds */
    a.inc=px_closure_new_with_graph("increment", PX_INTENT_REQUEST, on_inc, eval_nonneg, &a, a.graph);
    a.dec=px_closure_new_with_graph("decrement", PX_INTENT_REQUEST, on_dec, eval_nonneg, &a, a.graph);
    a.reset=px_closure_new_with_graph("reset counter", PX_INTENT_DECLARE, on_reset, eval_true, &a, a.graph);
    a.animate_to_10=px_closure_new_with_graph("animate to 10", PX_INTENT_REQUEST, on_animate, eval_true, &a, a.graph);
    a.save=px_closure_new_with_graph("save to server", PX_INTENT_PROMISE, on_save, eval_true, &a, a.graph);

    /* Relations: all closures trigger count */
    px_declare(a.graph, a.inc,          PX_REL_TRIGGERS, a.count);
    px_declare(a.graph, a.dec,          PX_REL_TRIGGERS, a.count);
    px_declare(a.graph, a.reset,         PX_REL_TRIGGERS, a.count);
    px_declare(a.graph, a.animate_to_10, PX_REL_TRIGGERS, a.count);
    px_declare(a.graph, a.save,         PX_REL_TRIGGERS, a.count);

    /* Undo: graphs bound at construction (v0.7 constructor split,
     * ADR-0019 — the bind call cannot be forgotten). */
    px_undo_set_enabled(true);

    /* Perceptions: 3 denotations */
    px_estimate* pin[]={a.count, a.doubled};
    px_perception* pv=px_perception_new("visual", p_visual, pin, 2, &a);
    px_perception* pa=px_perception_new("a11y",   p_a11y,   pin, 2, &a);
    px_perception* pj=px_perception_new("json",   p_json,   pin, 2, &a);
    (void)pv;(void)pa;(void)pj;

    printf("Setup complete. Perceptions: %d\n\n", px_perception_count());

    /* === Step 1: Increment x3 (REQUEST + undo + derived + 3 denotations) === */
    printf("=== Step 1: Increment x3 (REQUEST) ===\n");
    px_closure_trigger(a.inc, NULL, 0); audit_log(&a, "inc", a.inc);
    px_closure_trigger(a.inc, NULL, 0); audit_log(&a, "inc", a.inc);
    px_closure_trigger(a.inc, NULL, 0); audit_log(&a, "inc", a.inc);
    show(&a, "after inc x3");
    assert(px_estimate_value(a.count)==3);
    assert(px_estimate_value(a.doubled)==6);
    assert(px_estimate_confidence(a.count)==1.0);
    assert(px_undo_count()==3);
    printf("  -> count=3, doubled=6, conf=100%%, undo=3 PASS\n");

    /* === Step 2: Animate to 10 (TIME dimension) === */
    printf("\n=== Step 2: Animate to 10 (TIME dimension) ===\n");
    px_closure_trigger(a.animate_to_10, NULL, 0); audit_log(&a, "animate", a.animate_to_10);

    /* Sample at multiple time points */
    double v0=px_estimate_sample(a.count, 0);
    double v125=px_estimate_sample(a.count, 125);
    double v250=px_estimate_sample(a.count, 250);
    double v500=px_estimate_sample(a.count, 500);
    printf("  t=0:   %.2f\n", v0);
    printf("  t=125: %.2f (ease-out)\n", v125);
    printf("  t=250: %.2f (ease-out)\n", v250);
    printf("  t=500: %.2f (reached target)\n", v500);
    assert(v0 < v125);
    assert(v125 < v250);
    assert(v250 < v500);
    assert(v500 == 10.0);

    /* Finalize: set value to trigger derived update */
    px_estimate_set(a.count, 10.0, 1.0);
    show(&a, "after animate to 10");
    assert(px_estimate_value(a.count)==10);
    assert(px_estimate_value(a.doubled)==20);
    printf("  -> count=10, doubled=20, animation reached target PASS\n");

    /* === Step 3: Save (PROMISE -> DECLARE, async success) === */
    printf("\n=== Step 3: Save (PROMISE -> DECLARE, success) ===\n");
    px_closure_trigger(a.save, NULL, 0); audit_log(&a, "save", a.save);
    show(&a, "after save (success)");
    assert(px_closure_get_status(a.save)==PX_CLOSURE_DONE);
    assert(px_estimate_confidence(a.count)==1.0);
    printf("  -> closure=DONE, conf=100%% PASS\n");
    printf("  feedback: \"%s\"\n", px_closure_feedback(a.save));

    /* === Step 4: Decrement to negative (auto-eval FAILED + confidence drop) === */
    printf("\n=== Step 4: Decrement to -1 (auto-eval FAILED) ===\n");
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    px_closure_trigger(a.dec, NULL, 0); audit_log(&a, "dec", a.dec);
    show(&a, "after dec to -1");
    assert(px_estimate_value(a.count)==-1);
    assert(px_closure_get_status(a.dec)==PX_CLOSURE_FAILED);
    printf("  -> count=-1, closure=FAILED PASS\n");
    printf("  feedback: \"%s\"\n", px_closure_feedback(a.dec));

    /* === Step 5: Save fails (PROMISE -> FAIL, confidence drops) === */
    printf("\n=== Step 5: Save (PROMISE -> FAIL, confidence drops) ===\n");
    px_closure_trigger(a.save, NULL, 0); audit_log(&a, "save(fail)", a.save);
    show(&a, "after save (failure)");
    assert(px_closure_get_status(a.save)==PX_CLOSURE_FAILED);
    assert(px_estimate_confidence(a.count)==0.3);
    printf("  -> closure=FAILED, conf=30%% (dropped!) PASS\n");
    printf("  feedback: \"%s\"\n", px_closure_feedback(a.save));

    /* === Step 6: Undo (Relation graph restores scoped state + confidence) === */
    printf("\n=== Step 6: Undo x2 (Relation: scoped restore) ===\n");
    int r1=px_undo();
    printf("  undo 1: restored=%d, count=%.0f, conf=%.2f\n",
        r1, px_estimate_value(a.count), px_estimate_confidence(a.count));
    int r2=px_undo();
    printf("  undo 2: restored=%d, count=%.0f, conf=%.2f\n",
        r2, px_estimate_value(a.count), px_estimate_confidence(a.count));
    show(&a, "after undo x2");
    /* Undo restores count AND confidence */
    assert(px_estimate_confidence(a.count) > 0.3);
    printf("  -> confidence restored by undo PASS\n");

    /* === Step 7: Reset (DECLARE intent) === */
    printf("\n=== Step 7: Reset (DECLARE) ===\n");
    px_closure_trigger(a.reset, NULL, 0); audit_log(&a, "reset", a.reset);
    show(&a, "after reset");
    assert(px_estimate_value(a.count)==0);
    assert(px_estimate_value(a.doubled)==0);
    printf("  -> count=0, doubled=0 PASS\n");

    /* === Audit Log === */
    printf("\n=== Intent Audit Log (Closure: intent-as-value) ===\n");
    printf("%s", a.audit);
    printf("  Total: %d entries\n", a.audit_count);

    /* === Undo full history === */
    printf("\n=== Undo Full History (Relation: graph-driven) ===\n");
    int undo_total = px_undo_count();
    printf("  Undo steps available: %d\n", undo_total);
    int undo_done = 0;
    while (px_undo_count() > 0) {
        px_undo();
        undo_done++;
    }
    printf("  Undid %d steps, final count=%.0f\n", undo_done, px_estimate_value(a.count));

    /* === Validation === */
    printf("\n=== Validation ===\n");
    assert(px_estimate_value(a.count)==0);
    assert(px_estimate_value(a.doubled)==0);
    assert(a.renders>0);
    assert(a.a11ys>0);
    assert(a.jsons>0);
    assert(a.audit_count>0);
    assert(px_undo_count()==0);
    printf("  count=0 (undo full) OK\n");
    printf("  doubled=0 (derived auto-tracked) OK\n");
    printf("  renders=%d  a11ys=%d  jsons=%d OK\n", a.renders, a.a11ys, a.jsons);
    printf("  audit=%d entries OK\n", a.audit_count);
    printf("  undo=0 (all consumed) OK\n");
    printf("  confidence varied: 1.0 -> 0.3 (error) -> restored (undo) OK\n");
    printf("  animation: 3 -> 10 with ease-out OK\n");
    printf("  async lifecycle: PROMISE -> DECLARE (success) / FAIL (error) OK\n");

    /* Cleanup */
    px_perception_free(pv); px_perception_free(pa); px_perception_free(pj);
    px_undo_clear();
    px_closure_free(a.inc); px_closure_free(a.dec);
    px_closure_free(a.reset); px_closure_free(a.animate_to_10);
    px_closure_free(a.save);
    px_estimate_free(a.doubled);
    px_graph_free(a.graph);
    px_estimate_free(a.count);

    printf("\n=== Done: ALL features, ALL abstractions, 1 state model ===\n\n");
    printf("Features used:\n");
    printf("  Estimate: value + time(animate) + confidence + derived  [4/4 dims]\n");
    printf("  Closure:  REQUEST + DECLARE + PROMISE + auto-eval + audit [5/5 lifecycle]\n");
    printf("  Relation: TRIGGERS + undo-via-graph + scoped snapshots  [3/3 features]\n");
    printf("  Perception: visual + a11y + json (3 denotations)        [3/3 denotations]\n\n");
    printf("React equivalent: 5+ libraries, 5 state models, manual sync.\n");
    printf("Planex: 4 abstractions, 1 state model, native integration.\n");
    return 0;
}
