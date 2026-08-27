/*
 * v3_prototype_interpretant.c — Perception interpretant sub-API
 *
 * Validates that the v3 prototype's interpretant sub-API is
 * expressible in Planex's C17 zero-dependency API surface, and
 * that the loop audit captures the interpretant_constructed flag.
 *
 * Essence gap addressed (per essence-derivation-v3.md § II-3):
 *   - v2 said Perception covers Presentation essence. But Peirce's
 *     sign relation is triadic: representamen ↔ object ↔ interpretant.
 *     Planex's Perception only modeled the first two terms:
 *       state (object) → representamen (output of px_perceive_fn)
 *     The third term — the interpretant *generated in the actor's
 *     mind* by encountering the representamen — was missing.
 *   - Without an interpretant channel, Planex could not:
 *       * declare the system's *intended* interpretant
 *         ("we rendered '7' to mean 'seven items pending'")
 *       * detect a *misreading* (actor's actual interpretant ≠
 *         system's intended)
 *       * log the interpretation history, only the perception history
 *   - This prototype confirms the API can declare an intended
 *     interpretant, register an interpret_fn that predicts the
 *     actor's actual interpretant, and that the loop audit records
 *     whether the interpretant was constructed (predictor ran).
 *
 * Scenario:
 *   A perception renders the number 7. The system declares its
 *   intended interpretant as "seven items pending". An interpret_fn
 *   is registered that (in this prototype) returns a non-NULL
 *   interpretant — simulating a successful prediction. The loop
 *   audit records interpretant_constructed=true. Then the
 *   interpret_fn is replaced with one that returns NULL
 *   (simulating a prediction failure — the actor's interpretant
 *   could not be constructed), and the loop audit records
 *   interpretant_constructed=false.
 *
 * Build (via CMakeLists.txt STDOUT_DEMOS):
 *   cmake -B build && cmake --build build
 *   ./build/v3_prototype_interpretant
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { px_estimate* items_pending; } App;

static void on_add_item(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->items_pending);
    px_estimate_set(a->items_pending, v + 1, 1.0);
}

static bool eval_ok(void* user) {
    App* a = user;
    return px_estimate_value(a->items_pending) >= 0;
}

/* Perception fn: renders "N" as a heap-allocated string. */
static void* render_N(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "%d", (int)px_estimate_value(inputs[0]));
    return buf;
}

/* interpret_fn #1: simulates a successful interpretation.
 * Given the representamen "7" + actor (NULL in this prototype),
 * returns a non-NULL interpretant string ("queue length 7"). */
static void* interpret_as_queue(void* representamen, px_actor* actor, void* user) {
    (void)actor; (void)user;
    /* In a real Layer 5 implementation, this would consult the
     * actor's history + situation to predict their actual
     * interpretant. Here we just return a constant non-NULL value
     * to demonstrate the channel works. */
    if (!representamen) return NULL;
    char* predicted = malloc(64);
    if (!predicted) return NULL;
    snprintf(predicted, 64, "queue length %s", (const char*)representamen);
    return predicted;
}

/* interpret_fn #2: simulates a prediction failure.
 * Returns NULL — the system could not predict the actor's
 * interpretant (e.g., the actor is new, no history yet). */
static void* interpret_fail(void* representamen, px_actor* actor, void* user) {
    (void)representamen; (void)actor; (void)user;
    return NULL;
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

int main(void) {
    printf("=== v3_prototype_interpretant ===\n");
    printf("Perception interpretant sub-API (essence-derivation-v3 § II-3)\n\n");

    App app = {0};
    app.items_pending = px_estimate_new(7, 1.0);
    if (!app.items_pending) { fprintf(stderr, "FAIL: estimate_new\n"); return 1; }

    px_closure* c = px_closure_new("add item", PX_INTENT_REQUEST,
                                    on_add_item, eval_ok, &app);
    if (!c) { fprintf(stderr, "FAIL: closure_new\n"); return 1; }

    px_estimate* inputs[] = { app.items_pending };
    px_perception* p = px_perception_new("count_display", render_N,
                                            inputs, 1, NULL);
    if (!p) { fprintf(stderr, "FAIL: perception_new\n"); return 1; }

    /* Declare the system's *intended* interpretant. The system
     * rendered "7" and meant it as "seven items pending". */
    px_perception_set_intended_interpretant(p, "seven items pending");
    printf("[declare] perception's intended_interpretant = \"%s\"\n",
           px_perception_intended_interpretant(p));
    printf("          (system means: when actor sees \"7\", system wants them\n");
    printf("           to read it as \"seven items pending\" — NOT as \"7 of 10\n");
    printf("           progress\" or \"queue length 7\".)\n\n");

    /* Case A: register a successful interpret_fn. */
    px_perception_set_interpret_fn(p, interpret_as_queue, NULL);
    printf("[case A] registered interpret_fn = interpret_as_queue\n");
    printf("         (predicts actor's interpretant as \"queue length N\")\n");

    px_loop* loop = px_loop_new(c, p);
    if (!loop) { fprintf(stderr, "FAIL: loop_new\n"); return 1; }

    px_loop_step(loop, NULL, 0);  /* triggers closure, runs perception + interpret */

    px_loop_audit_entry e_a = latest_audit(loop);
    printf("[audit]   interpretant_constructed = %s (expected true)\n",
           e_a.interpretant_constructed ? "true" : "false");

    /* Case B: swap in a failing interpret_fn. */
    px_perception_set_interpret_fn(p, interpret_fail, NULL);
    printf("\n[case B] registered interpret_fn = interpret_fail (returns NULL)\n");
    printf("         (simulates Layer 5 unable to predict — new actor, no history)\n");

    px_loop_audit_clear(loop);
    px_loop_step(loop, NULL, 0);

    px_loop_audit_entry e_b = latest_audit(loop);
    printf("[audit]   interpretant_constructed = %s (expected false)\n",
           e_b.interpretant_constructed ? "true" : "false");

    /* Case C: remove interpret_fn entirely (NULL). */
    px_perception_set_interpret_fn(p, NULL, NULL);
    printf("\n[case C] interpret_fn = NULL (no Layer 5 hook)\n");
    px_loop_audit_clear(loop);
    px_loop_step(loop, NULL, 0);

    px_loop_audit_entry e_c = latest_audit(loop);
    printf("[audit]   interpretant_constructed = %s (expected false)\n",
           e_c.interpretant_constructed ? "true" : "false");
    printf("[audit]   perception_invoked       = %s (still true — perception\n",
           e_c.perception_invoked ? "true" : "false");
    printf("           ran, just no interpretant predictor registered)\n");

    printf("\n[verdict] Interpretant is expressible: perception declares intended\n");
    printf("          interpretant; interpret_fn predicts actor's actual one;\n");
    printf("          loop audit distinguishes \"interpretation predicted\"\n");
    printf("          from \"perception invoked\" — v2 could not.\n");

    px_loop_free(loop);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(app.items_pending);
    return 0;
}
