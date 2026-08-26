/*
 * async_demo.c — Closure's PROMISE/DECLARE/FAIL lifecycle in real use
 *
 * Step 3 of "unused essence features" series.
 *
 * Problem: Closure has promise/declare/fail (Winograd/Flores speech
 * acts) but integration_4abs only used REQUEST and DECLARE.
 * This demo shows the full async lifecycle:
 *
 *   IDLE -> promise("loading") -> RUNNING
 *                             -> declare("done") -> DONE
 *                             -> fail("error")   -> FAILED
 *
 * Scenario: simulate fetching user data from a server.
 * 3 Closure triggers: success, failure, timeout.
 * Each follows the promise -> declare/fail lifecycle.
 * An Estimate tracks the fetch status (0=idle, 1=loading, 2=done, 3=error).
 * A Perception renders different text for each status.
 *
 * React equivalent:
 *   3 state variables (isLoading, error, data) + try/catch + finally
 *   = ~15 lines of glue per async operation
 *
 * Planex:
 *   px_closure_promise("loading") -> px_closure_declare("done")
 *   = 2 lines, built-in lifecycle
 *
 * Build:
 *   cc -std=c17 -I include examples/async_demo.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c -lm -o build/async_demo
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    px_estimate* status;    /* 0=idle, 1=loading, 2=success, 3=error */
    px_graph* graph;
    px_closure* fetch;      /* triggers async fetch (PROMISE) */
    int scenario;           /* 0=success, 1=fail, 2=timeout */
} App;

/* Status constants */
enum { ST_IDLE=0, ST_LOADING=1, ST_SUCCESS=2, ST_ERROR=3 };

/* Closure action: starts async fetch
 * Uses PROMISE intent kind -> runtime sets status=RUNNING */
static void on_fetch(px_intent i, void* u) {
    (void)i;
    App* a = u;
    /* Promise: "I will fetch data" */
    px_closure_promise(a->fetch, "Fetching user data...");
    px_estimate_set(a->status, ST_LOADING, 1.0);

    /* Simulate async result based on scenario */
    switch (a->scenario) {
        case 0: /* success */
            px_closure_declare(a->fetch, "User data loaded successfully");
            px_estimate_set(a->status, ST_SUCCESS, 1.0);
            break;
        case 1: /* failure */
            px_closure_fail(a->fetch, "Network error: connection refused");
            px_estimate_set(a->status, ST_ERROR, 0.3);  /* low confidence */
            break;
        case 2: /* timeout */
            px_closure_fail(a->fetch, "Timeout: server did not respond");
            px_estimate_set(a->status, ST_ERROR, 0.1);  /* very low confidence */
            break;
    }
}

static bool eval_true(void* u) { (void)u; return true; }

/* Perception: render status text */
static void* perceive_status(px_estimate* const* in, int n, void* u) {
    (void)u; (void)n;
    if (n < 1) return NULL;
    int st = (int)px_estimate_value(in[0]);
    double conf = px_estimate_confidence(in[0]);
    char* buf = malloc(256);
    if (!buf) return NULL;
    switch (st) {
        case ST_IDLE:
            snprintf(buf, 256, "Ready. Press fetch to load data.");
            break;
        case ST_LOADING:
            snprintf(buf, 256, "Loading... (please wait)");
            break;
        case ST_SUCCESS:
            snprintf(buf, 256, "Success! Data loaded. (conf=%.0f%%)", conf*100);
            break;
        case ST_ERROR:
            snprintf(buf, 256, "ERROR! (conf=%.0f%%) — low confidence reading", conf*100);
            break;
        default:
            snprintf(buf, 256, "Unknown status: %d", st);
    }
    return buf;
}

/* Perception: render as JSON (for test snapshots) */
static void* perceive_json(px_estimate* const* in, int n, void* u) {
    (void)u; (void)n;
    if (n < 1) return NULL;
    char* buf = malloc(256);
    if (!buf) return NULL;
    snprintf(buf, 256, "{\"status\":%d,\"confidence\":%.2f}",
        (int)px_estimate_value(in[0]),
        px_estimate_confidence(in[0]));
    return buf;
}

static void run_scenario(App* a, int scenario, const char* name) {
    printf("\n--- %s ---\n", name);
    a->scenario = scenario;

    /* Reset to idle */
    px_estimate_set(a->status, ST_IDLE, 1.0);
    assert(px_closure_get_status(a->fetch) == PX_CLOSURE_IDLE ||
           px_closure_get_status(a->fetch) == PX_CLOSURE_DONE ||
           px_closure_get_status(a->fetch) == PX_CLOSURE_FAILED);

    /* Before fetch */
    px_estimate* pin[] = { a->status };
    void* before = perceive_status(pin, 1, NULL);
    printf("  Before: %s\n", (char*)before);
    printf("  Closure status: %s\n",
        px_closure_status_str(px_closure_get_status(a->fetch)));
    free(before);

    /* Trigger fetch (Closure with PROMISE intent) */
    px_closure_trigger(a->fetch, NULL, 0);

    /* After fetch */
    void* after = perceive_status(pin, 1, NULL);
    void* json = perceive_json(pin, 1, NULL);
    printf("  After:  %s\n", (char*)after);
    printf("  JSON:   %s\n", (char*)json);
    printf("  Closure status: %s\n",
        px_closure_status_str(px_closure_get_status(a->fetch)));
    printf("  Closure feedback: \"%s\"\n",
        px_closure_feedback(a->fetch));
    free(after);
    free(json);
}

int main(void) {
    printf("Planex async_demo — PROMISE/DECLARE/FAIL lifecycle\n");
    printf("=================================================\n");
    printf("Shows: Closure async lifecycle (Winograd/Flores speech acts)\n\n");

    printf("React equivalent:\n");
    printf("  3 state vars (isLoading, error, data) + try/catch/finally\n");
    printf("  = ~15 lines per async operation\n");
    printf("Planex:\n");
    printf("  promise(loading) -> declare(done) or fail(error)\n");
    printf("  = 2 lines, built-in lifecycle\n\n");

    App a = {0};
    a.graph = px_graph_new();
    a.status = px_estimate_new(ST_IDLE, 1.0);
    a.fetch = px_closure_new("fetch user data", PX_INTENT_PROMISE,
        on_fetch, eval_true, &a);

    px_declare(a.graph, a.fetch, PX_REL_TRIGGERS, a.status);

    px_estimate* pin[] = { a.status };
    px_perception* p_status = px_perception_new("status_text",
        perceive_status, pin, 1, NULL);
    px_perception* p_json = px_perception_new("status_json",
        perceive_json, pin, 1, NULL);
    (void)p_status; (void)p_json;

    printf("Initial state:\n");
    printf("  status=%d (IDLE)\n", (int)px_estimate_value(a.status));
    printf("  closure=%s\n\n",
        px_closure_status_str(px_closure_get_status(a.fetch)));

    /* Scenario 1: Success */
    run_scenario(&a, 0, "Scenario 1: SUCCESS");
    assert(px_estimate_value(a.status) == ST_SUCCESS);
    assert(px_closure_get_status(a.fetch) == PX_CLOSURE_DONE);
    assert(px_estimate_confidence(a.status) == 1.0);

    /* Scenario 2: Failure */
    run_scenario(&a, 1, "Scenario 2: NETWORK ERROR");
    assert(px_estimate_value(a.status) == ST_ERROR);
    assert(px_closure_get_status(a.fetch) == PX_CLOSURE_FAILED);
    assert(px_estimate_confidence(a.status) == 0.3);

    /* Scenario 3: Timeout */
    run_scenario(&a, 2, "Scenario 3: TIMEOUT");
    assert(px_estimate_value(a.status) == ST_ERROR);
    assert(px_closure_get_status(a.fetch) == PX_CLOSURE_FAILED);
    assert(px_estimate_confidence(a.status) == 0.1);

    printf("\n=== Validation ===\n");
    printf("  Scenario 1 (success): status=SUCCESS, closure=DONE, conf=1.0 PASS\n");
    printf("  Scenario 2 (error):   status=ERROR,  closure=FAILED, conf=0.3 PASS\n");
    printf("  Scenario 3 (timeout): status=ERROR,  closure=FAILED, conf=0.1 PASS\n");

    printf("\n=== Lifecycle Summary ===\n");
    printf("  IDLE     -> trigger -> promise -> RUNNING\n");
    printf("  RUNNING  -> declare  -> DONE (success)\n");
    printf("  RUNNING  -> fail     -> FAILED (error/timeout)\n");
    printf("\n  Each transition is ONE function call:\n");
    printf("    px_closure_promise(c, \"loading\")  -> RUNNING\n");
    printf("    px_closure_declare(c, \"done\")     -> DONE\n");
    printf("    px_closure_fail(c, \"error\")       -> FAILED\n");

    printf("\n=== Estimate + Closure integration ===\n");
    printf("  Estimate.status tracks 0(idle)->1(loading)->2(success)/3(error)\n");
    printf("  Estimate.confidence varies: 1.0(success) / 0.3(error) / 0.1(timeout)\n");
    printf("  Closure.feedback auto-set: \"Fetching...\" / \"loaded\" / \"error\"\n");
    printf("  Perception renders different text for each status + confidence\n");

    px_perception_free(p_status);
    px_perception_free(p_json);
    px_closure_free(a.fetch);
    px_graph_free(a.graph);
    px_estimate_free(a.status);

    printf("\n=== Done ===\n");
    printf("React: 3 state vars + try/catch/finally + manual status sync\n");
    printf("Planex: promise/declare/fail + Estimate auto-tracks status + conf\n");
    return 0;
}
