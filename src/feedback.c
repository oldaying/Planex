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

#define PX_LOOP_AUDIT_CAPACITY 1024

struct px_loop {
    px_closure*    closure;
    px_perception* perception;

    /* Pause state. When paused, step() is a no-op. */
    bool           paused;

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
                        bool perception_invoked) {
    px_loop_audit_entry* e = &loop->audit[loop->audit_head];
    e->closure_triggered = closure_triggered;
    e->perception_invoked = perception_invoked;
    e->timestamp_ms      = px_now_ms();

    loop->audit_head = (loop->audit_head + 1) % PX_LOOP_AUDIT_CAPACITY;
    loop->audit_count++;
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

    bool closure_triggered = false;
    bool perception_invoked = false;

    /* 1. Always trigger closure. Callers who want view-only refresh
     *    use px_loop_step_view_only instead. The closure's action
     *    will run with the provided payload (or NULL if no payload). */
    px_closure_trigger(loop->closure, trigger_payload, trigger_size);
    closure_triggered = true;

    /* 2. Invoke perception. The perception reads the (possibly changed)
     *    Estimate state and produces its denotation.
     *
     *    Design note: px_perception_invoke_for_estimate() requires an
     *    Estimate to query for. The loop's perception may have multiple
     *    source estimates, so we use invoke_all() which runs every
     *    registered perception. This is broader than strictly needed
     *    (loops over-perceive), but it's consistent with Planex's global
     *    perception registry design.
     *
     *    Future refinement (post-v0.4): add px_perception_invoke(p) to
     *    the API to invoke a single perception. */
    int invoked = px_perception_invoke_all();
    if (invoked > 0) {
        perception_invoked = true;
    }

    /* 3. Record this iteration. */
    audit_push(loop, closure_triggered, perception_invoked);

    return perception_invoked ? 1 : 0;
}

int px_loop_step_view_only(px_loop* loop) {
    if (!loop) return 0;
    if (loop->paused) return 0;

    /* Invoke perception without triggering closure. */
    int invoked = px_perception_invoke_all();

    audit_push(loop, false, invoked > 0);
    return invoked > 0 ? 1 : 0;
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
        }
        px_perception_invoke_all();
        replayed++;
    }
    return replayed;
}

void px_loop_audit_clear(px_loop* loop) {
    if (!loop) return;
    loop->audit_count = 0;
    loop->audit_head  = 0;
    /* Optionally zero the buffer for clean replay state. */
    memset(loop->audit, 0,
           PX_LOOP_AUDIT_CAPACITY * sizeof(px_loop_audit_entry));
}
