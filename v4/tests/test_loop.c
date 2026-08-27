/* v4/tests/test_loop.c — essence #7: Loop topology (integration of all 8)
 *
 * Verifies: px_loop_step traverses the full loop
 *   trigger closure -> invoke perception -> predict interpretant
 *                       -> read perlocution kind -> mark breakdown
 * Audit records all 5 semantic dimensions.
 *
 * v4 BREAK verified: px_loop_new takes 4 bindings (Closure, Perception,
 * Interpretant, Perlocution), not 2.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

/* the application state */
typedef struct {
    double counter;
    char   last_message[64];
} app_state;

/* closure action: increments the counter */
static void inc_action(px_intent intent, void* user) {
    app_state* s = (app_state*)user;
    if (intent.payload && intent.payload_size == sizeof(double)) {
        double v;
        memcpy(&v, intent.payload, sizeof(double));
        s->counter += v;
    } else {
        s->counter += 1.0;
    }
}

static bool inc_eval(void* user) {
    (void)user;
    return true;
}

/* perceive: emits a representamen describing the counter */
static void* counter_representamen(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    static char buf[64];
    if (n <= 0) return NULL;
    double v = px_estimate_now(inputs[0]);
    snprintf(buf, sizeof(buf), "counter is %.0f", v);
    return buf;
}

/* interpret_fn: predicts the actor understood the representamen literally */
static void* literal_interpret(void* representamen, px_actor* actor, void* user) {
    (void)actor; (void)user;
    /* if the representamen is a string, return a copy of it (the actor
     * took it to mean literally what it said). In a real impl this would
     * be more sophisticated; v4 verifies the channel works. */
    return representamen;
}

int main(void) {
    app_state state = { 0.0, "" };

    /* build the 4 loop bindings */
    px_estimate* counter = px_estimate_new(0.0, 1.0);
    px_estimate* srcs[] = { counter };

    px_closure* c = px_closure_new("increment counter",
                                     PX_INTENT_REQUEST,
                                     inc_action, inc_eval, &state);

    px_perception* p = px_perception_new("counter_text",
                                            counter_representamen,
                                            srcs, 1, NULL);

    px_actor* alice = px_actor_new("alice", NULL);
    px_interpretant* it = px_interpretant_new(p, alice);
    px_interpretant_set_intended(it, "counter is 5");
    px_interpretant_set_interpret_fn(it, literal_interpret, NULL);

    px_perlocution* per = px_perlocution_new(c, alice);

    /* loop_new with all 4 bindings */
    px_loop* loop = px_loop_new(c, p, it, per);
    ASSERT(loop != NULL, "loop_new with 4 bindings");
    ASSERT(px_loop_audit_count(loop) == 0, "audit empty initially");

    /* step 1: trigger with payload 5, no perlocution set yet */
    double payload = 5.0;
    int ran = px_loop_step(loop, &payload, sizeof(double));
    ASSERT(ran == 1, "step ran (perception invoked)");
    ASSERT(state.counter == 5.0, "counter incremented to 5");
    ASSERT(px_estimate_now(counter) == 0.0,
           "estimate not yet updated (action does NOT touch estimate in this test)");

    /* manually update the estimate to match state (simulating a
     * post-action state→estimate sync) */
    px_estimate_set(counter, state.counter, 1.0);

    /* set perlocution to INFORM "counter incremented" */
    px_perlocution_set(per, PX_PERLOC_INFORM, "counter incremented");

    /* step 2: another trigger, now with perlocution set */
    payload = 3.0;
    ran = px_loop_step(loop, &payload, sizeof(double));
    ASSERT(ran == 1, "step 2 ran");
    ASSERT(state.counter == 8.0, "counter now 8");

    /* audit should have 2 entries */
    ASSERT(px_loop_audit_count(loop) == 2, "audit count = 2");

    /* inspect audit entries */
    px_loop_audit_entry entries[4];
    int n = px_loop_audit_get(loop, entries, 4);
    ASSERT(n == 2, "audit_get returned 2");

    /* entry 0: closure triggered, perception invoked, interpretant constructed,
       perlocution UNSPECIFIED (not set yet at step 1), bd=0 */
    ASSERT(entries[0].closure_triggered == true, "entry0 closure triggered");
    ASSERT(entries[0].perception_invoked == true, "entry0 perception invoked");
    ASSERT(entries[0].interpretant_constructed == true, "entry0 interpretant constructed");
    ASSERT(entries[0].perlocution_kind == (int)PX_PERLOC_UNSPECIFIED,
           "entry0 perlocution UNSPECIFIED");
    ASSERT(entries[0].breakdown_transition == 0, "entry0 no breakdown");

    /* entry 1: closure triggered, perception invoked, interpretant constructed,
       perlocution INFORM (set before step 2), bd=0 */
    ASSERT(entries[1].closure_triggered == true, "entry1 closure triggered");
    ASSERT(entries[1].perception_invoked == true, "entry1 perception invoked");
    ASSERT(entries[1].perlocution_kind == (int)PX_PERLOC_INFORM,
           "entry1 perlocution INFORM");

    /* mark a breakdown transition, then step — next audit should record +1 */
    px_loop_mark_breakdown(loop, +1, "alice confused by message");
    ran = px_loop_step(loop, NULL, 0);  /* view-only style trigger */
    ASSERT(px_loop_audit_count(loop) == 3, "audit count = 3 after bd mark");
    n = px_loop_audit_get(loop, entries, 4);
    ASSERT(n == 3, "audit_get returned 3");
    ASSERT(entries[2].breakdown_transition == +1, "entry2 breakdown +1");

    /* pause / resume */
    px_loop_pause(loop);
    ASSERT(px_loop_is_paused(loop), "paused");
    int paused_ran = px_loop_step(loop, NULL, 0);
    ASSERT(paused_ran == 0, "step is no-op while paused");
    ASSERT(px_loop_audit_count(loop) == 3, "audit unchanged while paused");
    px_loop_resume(loop);
    ASSERT(!px_loop_is_paused(loop), "resumed");

    /* clear audit */
    px_loop_audit_clear(loop);
    ASSERT(px_loop_audit_count(loop) == 0, "audit cleared");

    px_loop_free(loop);
    px_perlocution_free(per);
    px_interpretant_free(it);
    px_actor_free(alice);
    px_perception_free(p);
    px_closure_free(c);
    px_estimate_free(counter);
    printf("test_loop: ALL PASS\n");
    return 0;
}
