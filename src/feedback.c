/*
 * feedback.c — Feedback (closed-loop coupling, v0.4 essence category)
 *
 * A px_loop binds a Closure (intent side) to a Perception (view side)
 * into a first-class closed loop:
 *
 *   intent → action → state change → perception → next intent
 *
 * Before v0.4, this loop was implicit — application code manually
 * called px_perception_invoke_for_estimate() after px_closure_trigger().
 * The loop existed structurally but was not first-class.
 *
 * v0.4 promotes the loop to first-class (per ADR-0008). This enables:
 *   - audit: which perception fired after which trigger?
 *   - interrupt: pause the loop (batch updates, modal blocking)
 *   - replay: re-run trigger→perception sequences (testing, debugging)
 *   - breakdown detection: perception failed to fire, loop stalled
 *
 * Design notes:
 * - px_loop does NOT own the closure or perception. Caller frees them.
 * - audit log is a fixed-size ring buffer (1024 entries). When full,
 *   oldest entries are overwritten. This is a Stage 0 limitation;
 *   production may want growable storage.
 * - timestamp is px_now_ms() at iteration start.
 * - replay re-runs the recorded payload, not the original intent
 *   object (intent payload is copied by closure at trigger time,
 *   so we re-trigger with the same payload bytes).
 *
 * Inspired by:
 * - Heidegger: breakdown is the moment feedback fails — essence
 *   revealed by its absence
 * - Suchman: feedback is what makes action situated
 * - Math (CSP/statechart): loop/transition is primitive
 * - Norman: "to be in charge, the user must be informed" (feedback
 *   closes the gulf of evaluation)
 *
 * Thread safety: NOT thread-safe. Use from a single thread (the UI
 * thread), like the rest of Planex.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

/* Internal perception-cache helpers (perception.c). Declared here at
 * file scope so step/view_only/replay all share one declaration. */
extern void px__perception_clear_cache(px_perception*);
extern void px__perception_clear_all_caches(void);

#define PX_LOOP_AUDIT_CAPACITY 1024

struct px_loop {
    px_closure*    closure;
    px_perception* perception;

    /* Pause state. When paused, step() is a no-op. */
    bool           paused;

    /* v0.6: feedback budget (A4 "instantly visible" made measurable).
     * 0 = no budget declared. When > 0, each audit entry records the
     * iteration's wall-clock duration and whether it exceeded the
     * budget — the audit answer to "feedback happened, but was it
     * timely?". See px_loop_set_budget(). */
    double         budget_ms;

    /* v3 prototype: pending breakdown transition for the next step.
     * Set by px_loop_mark_breakdown(). px_loop_step reads and clears
     * it. +1 = entered breakdown, -1 = recovered, 0 = none. */
    int            pending_breakdown_transition;

    /* Audit log — ring buffer. */
    px_loop_audit_entry* audit;
    int                  audit_count;     /* total entries written (may exceed capacity) */
    int                  audit_head;     /* index where next entry will be written */
};

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_loop* px_loop_new(px_closure* c, px_perception* p) {
    if (!c || !p) return NULL;

    px_loop* loop = (px_loop*)calloc(1, sizeof(px_loop));
    if (!loop) return NULL;

    loop->closure    = c;
    loop->perception = p;
    loop->paused     = false;

    loop->audit = (px_loop_audit_entry*)calloc(PX_LOOP_AUDIT_CAPACITY,
                                                sizeof(px_loop_audit_entry));
    if (!loop->audit) {
        free(loop);
        return NULL;
    }
    loop->audit_count = 0;
    loop->audit_head  = 0;
    return loop;
}

void px_loop_free(px_loop* loop) {
    if (!loop) return;
    free(loop->audit);
    free(loop);
}

/* ============================================================
 * Audit ring buffer helpers
 * ============================================================ */

static void audit_push(px_loop* loop,
                        bool closure_triggered,
                        bool perception_invoked,
                        bool interpretant_constructed,
                        int  perlocution_kind,
                        int  breakdown_transition,
                        double iteration_start_ms) {
    px_loop_audit_entry* e = &loop->audit[loop->audit_head];
    e->closure_triggered       = closure_triggered;
    e->perception_invoked      = perception_invoked;
    e->interpretant_constructed = interpretant_constructed;
    e->perlocution_kind        = perlocution_kind;
    e->breakdown_transition    = breakdown_transition;
    e->timestamp_ms            = px_now_ms();

    /* v0.6: feedback-budget dimensions. iteration_ms is the wall-clock
     * duration of this step; budget_exceeded is true when a budget was
     * declared and the iteration ran past it. This gives the "instantly
     * visible" half of the feedback axiom a measurable contract — the
     * audit now answers "was the feedback timely?", not just "did it
     * happen?". */
    e->iteration_ms    = e->timestamp_ms - iteration_start_ms;
    e->budget_ms       = loop->budget_ms;
    e->budget_exceeded = (loop->budget_ms > 0.0)
                          && (e->iteration_ms > loop->budget_ms);

    loop->audit_head = (loop->audit_head + 1) % PX_LOOP_AUDIT_CAPACITY;
    loop->audit_count++;
}

/* v0.6: declare this loop's feedback budget in milliseconds.
 *
 * The feedback axiom (Shneiderman 1983 "immediate visual feedback";
 * Card/Moran/Newell's ~100ms perception thresholds) has an *instantly*
 * dimension that v0.5 could not express: the audit recorded THAT a
 * perception fired, but not WHEN relative to the trigger, and nothing
 * bounded the iteration. With a budget declared, every audit entry
 * carries iteration_ms / budget_ms / budget_exceeded — a loop that
 * consistently exceeds its budget is detectable, queryable evidence
 * (long-running dashboards can assert "no entry in the last hour
 * exceeded 16ms"). budget_ms <= 0 disables the check (default). */
void px_loop_set_budget(px_loop* loop, double budget_ms) {
    if (!loop) return;
    if (budget_ms < 0) budget_ms = 0;
    loop->budget_ms = budget_ms;
}

double px_loop_budget(const px_loop* loop) {
    return loop ? loop->budget_ms : 0.0;
}

/* Read the i-th most recent entry (0 = newest, 1 = previous, etc.)
 * Returns 1 on success, 0 if i is out of range. */
static int audit_read_recent(const px_loop* loop, int i,
                              px_loop_audit_entry* out) {
    if (i < 0) return 0;
    int stored = loop->audit_count < PX_LOOP_AUDIT_CAPACITY
                  ? loop->audit_count
                  : PX_LOOP_AUDIT_CAPACITY;
    if (i >= stored) return 0;
    /* audit_head points to where next write will go; the newest entry
     * is at (audit_head - 1), the previous at (audit_head - 2), etc. */
    int idx = (loop->audit_head - 1 - i + PX_LOOP_AUDIT_CAPACITY)
              % PX_LOOP_AUDIT_CAPACITY;
    *out = loop->audit[idx];
    return 1;
}

/* Read the i-th entry in chronological order (0 = oldest stored).
 * Returns 1 on success, 0 if i is out of range. */
static int audit_read_chrono(const px_loop* loop, int i,
                              px_loop_audit_entry* out) {
    if (i < 0) return 0;
    int stored = loop->audit_count < PX_LOOP_AUDIT_CAPACITY
                  ? loop->audit_count
                  : PX_LOOP_AUDIT_CAPACITY;
    if (i >= stored) return 0;
    /* If audit_count >= capacity, the oldest stored is at audit_head
     * (because the ring wrapped around). If audit_count < capacity,
     * oldest is at index 0. */
    int start = (loop->audit_count >= PX_LOOP_AUDIT_CAPACITY)
                  ? loop->audit_head
                  : 0;
    int idx = (start + i) % PX_LOOP_AUDIT_CAPACITY;
    *out = loop->audit[idx];
    return 1;
}

/* ============================================================
 * Step
 * ============================================================ */

int px_loop_step(px_loop* loop, void* trigger_payload, size_t trigger_size) {
    if (!loop) return 0;
    if (loop->paused) return 0;

    double t0 = px_now_ms();

    /* v0.5: clear bound perception's representamen cache at turn start.
     * This ensures invoke_single below either returns the freshly-cached
     * result (if auto-invocation fired during closure_trigger) or fires
     * explicitly (if no estimate_set happened). Without this, the cache
     * from a previous turn would be returned, double-firing or stale. */
    px__perception_clear_cache(loop->perception);

    bool closure_triggered      = false;
    bool perception_invoked    = false;
    bool interpretant_constructed = false;
    int  perlocution_kind      = (int)PX_PERLOC_UNSPECIFIED;
    int  breakdown_transition  = loop->pending_breakdown_transition;

    /* Clear pending breakdown transition — consumed by this step. */
    loop->pending_breakdown_transition = 0;

    /* 1. Trigger closure. The closure's action runs with the payload
     *    (or NULL if no payload). If the action calls px_estimate_set,
     *    Phase 2 auto-invocation (v0.5) fires dependent perceptions
     *    — including potentially the loop's bound perception. */
    px_closure_trigger(loop->closure, trigger_payload, trigger_size);
    closure_triggered = true;

    /* Capture perlocution_kind AFTER trigger so the action can set it. */
    perlocution_kind = (int)px_closure_perlocution_kind(loop->closure);

    /* 2. Invoke the bound perception explicitly to obtain the
     *    representamen (which auto-invocation discards). This is
     *    the loop's reason to exist — it captures the perception's
     *    output for the audit log + feeds it to the interpret_fn.
     *
     *    v0.5 fix: previously this called px_perception_invoke_all
     *    AND px_perception_invoke_single on the bound perception,
     *    which double-fired it. Now we only call invoke_single,
     *    which both produces the representamen AND counts as the
     *    perception firing for the audit's perception_invoked flag.
     *    Other registered perceptions are auto-invoked via
     *    px_estimate_set during the closure action (if applicable). */
    if (loop->perception) {
        void* representamen = px_perception_invoke_single(loop->perception);
        /* invoke_single returns NULL if p or p->fn is NULL, or if the
         * fn itself returned NULL. For audit purposes, "perception
         * invoked" means "we called (or attempted to call) the bound
         * perception's fn this iteration" — which is true whenever
         * loop->perception is non-NULL. */
        perception_invoked = true;
        void* interpretant = px_perception_interpret(loop->perception,
                                                       representamen, NULL);
        if (interpretant) {
            interpretant_constructed = true;
            /* The interpretant is owned by the interpret_fn — for the
             * prototype, we leak it (would need a free_fn to do better).
             * In a production version, the loop would track interpretant
             * ownership or use a stack-allocated convention. */
        }
    }

    /* 3. Record this iteration with all v3 semantic dimensions + the
     * v0.6 feedback-budget dimensions. */
    audit_push(loop, closure_triggered, perception_invoked,
               interpretant_constructed, perlocution_kind,
               breakdown_transition, t0);

    return perception_invoked ? 1 : 0;
}

int px_loop_step_view_only(px_loop* loop) {
    if (!loop) return 0;
    if (loop->paused) return 0;

    double t0 = px_now_ms();

    /* v0.6 scope retirement (leak-budgets.md px_loop §L2): view-only
     * step now invokes ONLY this loop's bound perception, not every
     * registered perception in the process. Previously this called
     * px_perception_invoke_all() — a loop coupled to perceptions it
     * never declared (multiple loops sharing an estimate would
     * cross-fire). Applications that genuinely want all perceptions
     * refreshed can still call px_perception_invoke_all() directly;
     * the loop's scope is now closed like px_loop_step's. */
    px__perception_clear_cache(loop->perception);
    bool perception_invoked = false;
    if (loop->perception) {
        px_perception_invoke_single(loop->perception);
        perception_invoked = true;
    }

    /* v3: still record perlocution + breakdown fields (no closure
     * triggered, so perlocution_kind = whatever closure currently
     * has, breakdown_transition = whatever is pending). */
    int perlocution_kind = (int)px_closure_perlocution_kind(loop->closure);
    int breakdown_transition = loop->pending_breakdown_transition;
    loop->pending_breakdown_transition = 0;

    audit_push(loop, false, perception_invoked, false, perlocution_kind,
               breakdown_transition, t0);
    return perception_invoked ? 1 : 0;
}

/* ============================================================
 * Pause / resume
 * ============================================================ */

void px_loop_pause(px_loop* loop) {
    if (loop) loop->paused = true;
}

void px_loop_resume(px_loop* loop) {
    if (loop) loop->paused = false;
}

bool px_loop_is_paused(const px_loop* loop) {
    return loop ? loop->paused : false;
}

/* ============================================================
 * Audit
 * ============================================================ */

int px_loop_audit_count(const px_loop* loop) {
    if (!loop) return 0;
    return loop->audit_count < PX_LOOP_AUDIT_CAPACITY
            ? loop->audit_count
            : PX_LOOP_AUDIT_CAPACITY;
}

int px_loop_audit_get(const px_loop* loop,
                       px_loop_audit_entry* out,
                       int max_entries) {
    if (!loop || !out || max_entries <= 0) return 0;

    int stored = px_loop_audit_count(loop);
    int n = (max_entries < stored) ? max_entries : stored;

    /* Read in chronological order (oldest first). */
    for (int i = 0; i < n; i++) {
        if (!audit_read_chrono(loop, i, &out[i])) break;
    }
    return n;
}

/* ============================================================
 * Replay
 * ============================================================ */

int px_loop_replay(px_loop* loop, int n) {
    if (!loop || n <= 0) return 0;

    int stored = px_loop_audit_count(loop);
    if (n > stored) n = stored;

    /* Read the last n entries (most recent first), then replay
     * them in chronological order. */
    px_loop_audit_entry entries[PX_LOOP_AUDIT_CAPACITY];
    if (n > PX_LOOP_AUDIT_CAPACITY) n = PX_LOOP_AUDIT_CAPACITY;

    /* Collect in chronological order: from (n-1) most recent down to 0,
     * then reverse to get oldest-first. */
    for (int i = 0; i < n; i++) {
        /* i-th most recent entry */
        if (!audit_read_recent(loop, n - 1 - i, &entries[i])) {
            n = i;
            break;
        }
    }

    /* Replay: for each entry, re-trigger closure if it was triggered.
     * Perception is always invoked (it reads current state).
     * We do NOT push new audit entries during replay (would pollute
     * the log with replay artifacts). */
    int replayed = 0;
    for (int i = 0; i < n; i++) {
        /* v0.6: clear the bound perception's cache at the start of each
         * replay iteration (scoped, matching step semantics). */
        px__perception_clear_cache(loop->perception);

        if (entries[i].closure_triggered) {
            /* Re-trigger with NULL payload — this re-runs the action
             * with whatever the closure's last_intent was. For closures
             * whose action depends on payload, this is a no-op.
             * The realistic use case for replay is "trigger with same
             * last payload" — captured via px_closure_last_intent. */
            px_intent last = px_closure_last_intent(loop->closure);
            if (last.payload && last.payload_size > 0) {
                px_closure_trigger(loop->closure, last.payload, last.payload_size);
            } else {
                px_closure_trigger(loop->closure, NULL, 0);
            }
            /* closure_trigger → action → estimate_set → auto-invoke
             * fires dependent perceptions. We do NOT call invoke_all
             * for closure_triggered entries to avoid double-firing
             * the perception in the same turn. */
        } else {
            /* View-only iteration — no closure trigger, no auto-invoke.
             * v0.6 scope retirement: fire ONLY this loop's bound
             * perception (previously invoke_all — the same cross-loop
             * leak as view_only step, retired together). */
            if (loop->perception) {
                px_perception_invoke_single(loop->perception);
            }
        }
        replayed++;
    }
    return replayed;
}

void px_loop_audit_clear(px_loop* loop) {
    if (!loop) return;
    loop->audit_count = 0;
    loop->audit_head  = 0;
    loop->pending_breakdown_transition = 0;
    /* Optionally zero the buffer for clean replay state. */
    memset(loop->audit, 0,
           PX_LOOP_AUDIT_CAPACITY * sizeof(px_loop_audit_entry));
}

/* ============================================================
 * v3 prototype — breakdown transition API
 * ============================================================ */

void px_loop_mark_breakdown(px_loop* loop, int transition,
                              const char* reason) {
    if (!loop) return;
    /* Clamp to valid range; reason is unused in this prototype
     * but reserved for future logging (could write to a per-loop
     * breakdown log similar to audit). */
    if (transition > 1)  transition = 1;
    if (transition < -1) transition = -1;
    loop->pending_breakdown_transition = transition;
    (void)reason;  /* unused */
}
