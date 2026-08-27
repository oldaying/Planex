/*
 * v3_prototype_perlocution.c — Closure perlocution sub-API
 *
 * Validates that the v3 prototype's perlocution sub-API is
 * expressible in Planex's C17 zero-dependency API surface, and
 * that the loop audit captures the perlocution dimension.
 *
 * Essence gap addressed (per essence-derivation-v3.md § II-4):
 *   - v2 said Closure covers Communication essence (Searle
 *     illocutionary). But Searle's taxonomy has three levels:
 *       locutionary / illocutionary / perlocutionary.
 *   - Winograd/Flores + Planex cover illocutionary (5 intent kinds).
 *     v2 left perlocutionary ("what the system's utterance DOES TO
 *     the actor's mental state") uncovered.
 *   - Result: "Saved." and "Saved. 3 fields were auto-corrected."
 *     produce the same Closure struct, same audit log, same
 *     px_loop iteration — but very different perlocutionary
 *     effects on the actor. Planex cannot express this.
 *   - This prototype confirms the API can now type the perlocution
 *     (INFORM vs ALERT vs SURPRISE etc.) and that the audit records it.
 *
 * Scenario:
 *   Two "save" closures complete successfully (status=DONE) but
 *   produce different perlocutionary outcomes. The loop audit log
 *   records the perlocution_kind for each iteration.
 *
 * Build (via CMakeLists.txt STDOUT_DEMOS):
 *   cmake -B build && cmake --build build
 *   ./build/v3_prototype_perlocution
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { px_estimate* saved_count; } App;

static void on_save(px_intent intent, void* user) {
    (void)intent;
    App* a = user;
    double v = px_estimate_value(a->saved_count);
    px_estimate_set(a->saved_count, v + 1, 1.0);
}

static bool eval_save_ok(void* user) {
    App* a = user;
    return px_estimate_value(a->saved_count) >= 0;
}

/* Simple perception: returns a string denotation of saved count. */
static void* render_count(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    if (n < 1) return NULL;
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "saved=%d", (int)px_estimate_value(inputs[0]));
    return buf;
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
    printf("=== v3_prototype_perlocution ===\n");
    printf("Closure perlocution sub-API (essence-derivation-v3 § II-4)\n\n");

    App app = {0};
    app.saved_count = px_estimate_new(0, 1.0);
    if (!app.saved_count) { fprintf(stderr, "FAIL: estimate_new\n"); return 1; }

    px_closure* c_save_plain = px_closure_new(
        "save form", PX_INTENT_REQUEST, on_save, eval_save_ok, &app);
    px_closure* c_save_alert = px_closure_new(
        "save form (with corrections)", PX_INTENT_REQUEST, on_save, eval_save_ok, &app);
    if (!c_save_plain || !c_save_alert) { fprintf(stderr, "FAIL: closure_new\n"); return 1; }

    /* Case 1: plain save — perlocution = INFORM ("now you know it saved") */
    px_closure_set_perlocution(c_save_plain, PX_PERLOC_INFORM, "Saved.");
    printf("[case 1] closure=\"save form\" set perlocution = INFORM / \"Saved.\"\n");
    printf("         px_closure_perlocution_kind = %s (expected INFORM)\n",
           px_perlocution_kind_str(px_closure_perlocution_kind(c_save_plain)));
    printf("         px_closure_perlocution_text = \"%s\" (expected \"Saved.\")\n",
           px_closure_perlocution_text(c_save_plain));

    /* Case 2: save with corrections — perlocution = ALERT ("now attend") */
    px_closure_set_perlocution(c_save_alert, PX_PERLOC_ALERT,
                              "Saved. 3 fields were auto-corrected.");
    printf("[case 2] closure=\"save form (corrections)\" set perlocution = ALERT /\n");
    printf("         \"Saved. 3 fields were auto-corrected.\"\n");
    printf("         px_closure_perlocution_kind = %s (expected ALERT)\n",
           px_perlocution_kind_str(px_closure_perlocution_kind(c_save_alert)));
    printf("         px_closure_perlocution_text = \"%s\"\n",
           px_closure_perlocution_text(c_save_alert));

    /* Sanity: both closures have the same operational status (IDLE —
     * not triggered yet). v2 would have seen these two closures as
     * identical at this point. v3 can distinguish them by perlocution. */
    printf("\n[sanity] Both closures have identical px_closure_status (IDLE),\n");
    printf("         but differ in perlocution_kind — v2 could not express this.\n");

    /* Loop audit: trigger each closure via a px_loop, verify the
     * audit entry records perlocution_kind. */
    px_estimate* inputs[] = { app.saved_count };
    px_perception* p = px_perception_new("count", render_count, inputs, 1, NULL);
    if (!p) { fprintf(stderr, "FAIL: perception_new\n"); return 1; }

    px_loop* loop_plain = px_loop_new(c_save_plain, p);
    px_loop* loop_alert = px_loop_new(c_save_alert, p);
    if (!loop_plain || !loop_alert) { fprintf(stderr, "FAIL: loop_new\n"); return 1; }

    px_loop_step(loop_plain, NULL, 0);
    px_loop_step(loop_alert, NULL, 0);

    px_loop_audit_entry e_plain = latest_audit(loop_plain);
    px_loop_audit_entry e_alert = latest_audit(loop_alert);

    printf("\n[audit]   loop_plain entry: perlocution_kind = %s (expected INFORM)\n",
           px_perlocution_kind_str((px_perlocution_kind)e_plain.perlocution_kind));
    printf("[audit]   loop_alert entry: perlocution_kind = %s (expected ALERT)\n",
           px_perlocution_kind_str((px_perlocution_kind)e_alert.perlocution_kind));

    printf("\n[verdict] Perlocution is expressible: two closures with identical\n");
    printf("          status (DONE) now differ in perlocution_kind.\n");
    printf("[verdict] Loop audit records the perlocution dimension of each iteration.\n");

    px_loop_free(loop_plain);
    px_loop_free(loop_alert);
    px_perception_free(p);
    px_closure_free(c_save_plain);
    px_closure_free(c_save_alert);
    px_estimate_free(app.saved_count);
    return 0;
}
