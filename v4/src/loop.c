/* v4/src/loop.c — essence #7: Loop topology
 *
 * The closed loop of:
 *   intent -> action -> state change -> perception -> interpretant
 *           -> perlocution -> next intent
 *
 * v4 BREAK: px_loop_new now takes FOUR bindings (Closure, Perception,
 * Interpretant, Perlocution). v0.4 / v3 took only two. The loop now
 * binds all four essence dimensions of the return edge:
 *   - representamen produced?       (perception_invoked)
 *   - interpretant constructed?      (interpretant_constructed)
 *   - perlocution emitted?           (perlocution_kind)
 *   - breakdown transitioned?       (breakdown_transition)
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

#define MAX_AUDIT 256

struct px_loop {
    px_closure*        closure;       /* weak */
    px_perception*     perception;    /* weak */
    px_interpretant*   interpretant;  /* weak, may be NULL */
    px_perlocution*    perlocution;   /* weak, may be NULL */

    bool               paused;

    /* audit ring buffer */
    px_loop_audit_entry entries[MAX_AUDIT];
    int                count;

    /* pending breakdown transition to apply to the next iteration */
    int                pending_bd_transition;
    char*              pending_bd_reason;
};

px_loop* px_loop_new(px_closure* c,
                       px_perception* p,
                       px_interpretant* it,
                       px_perlocution* per) {
    if (!c || !p) return NULL;
    px_loop* loop = (px_loop*)calloc(1, sizeof(px_loop));
    if (!loop) return NULL;
    loop->closure = c;
    loop->perception = p;
    loop->interpretant = it;
    loop->perlocution = per;
    loop->paused = false;
    loop->count = 0;
    loop->pending_bd_transition = 0;
    loop->pending_bd_reason = NULL;
    return loop;
}

void px_loop_free(px_loop* loop) {
    if (!loop) return;
    free(loop->pending_bd_reason);
    free(loop);
}

static void record_audit(px_loop* loop,
                          bool closure_triggered,
                          bool perception_invoked,
                          bool interpretant_constructed,
                          int  perlocution_kind) {
    if (loop->count >= MAX_AUDIT) {
        /* overwrite oldest (ring buffer) — but for simplicity in v4
         * verification we just cap at MAX_AUDIT and don't overwrite. */
        return;
    }
    px_loop_audit_entry* e = &loop->entries[loop->count++];
    e->closure_triggered = closure_triggered;
    e->perception_invoked = perception_invoked;
    e->interpretant_constructed = interpretant_constructed;
    e->perlocution_kind = perlocution_kind;
    e->breakdown_transition = loop->pending_bd_transition;
    e->timestamp_ms = px_now_ms();
    /* clear pending after applying */
    loop->pending_bd_transition = 0;
    free(loop->pending_bd_reason);
    loop->pending_bd_reason = NULL;
}

int px_loop_step(px_loop* loop, void* trigger_payload, size_t size) {
    if (!loop || loop->paused) return 0;

    bool triggered = false;
    if (trigger_payload || size > 0) {
        px_closure_trigger(loop->closure, trigger_payload, size);
        triggered = true;
    }

    /* invoke perception -> produce representamen */
    void* representamen = px_perception_invoke(loop->perception);
    bool perception_invoked = (representamen != NULL);

    /* interpretant: predict the actor's actual interpretant */
    bool interpretant_constructed = false;
    if (loop->interpretant && representamen) {
        void* predicted = px_interpretant_predict(loop->interpretant, representamen);
        if (predicted) {
            interpretant_constructed = true;
        }
    }

    /* read current perlocution kind */
    int perloc_kind = PX_PERLOC_UNSPECIFIED;
    if (loop->perlocution) {
        perloc_kind = (int)px_perlocution_kind_get(loop->perlocution);
    }

    record_audit(loop, triggered, perception_invoked,
                 interpretant_constructed, perloc_kind);
    return perception_invoked ? 1 : 0;
}

int px_loop_step_view_only(px_loop* loop) {
    if (!loop || loop->paused) return 0;
    /* no trigger; just invoke perception + interpretant + read perloc */
    void* representamen = px_perception_invoke(loop->perception);
    bool perception_invoked = (representamen != NULL);
    bool interpretant_constructed = false;
    if (loop->interpretant && representamen) {
        void* predicted = px_interpretant_predict(loop->interpretant, representamen);
        if (predicted) interpretant_constructed = true;
    }
    int perloc_kind = PX_PERLOC_UNSPECIFIED;
    if (loop->perlocution) {
        perloc_kind = (int)px_perlocution_kind_get(loop->perlocution);
    }
    record_audit(loop, false, perception_invoked,
                 interpretant_constructed, perloc_kind);
    return perception_invoked ? 1 : 0;
}

void px_loop_pause(px_loop* loop) {
    if (loop) loop->paused = true;
}

void px_loop_resume(px_loop* loop) {
    if (loop) loop->paused = false;
}

bool px_loop_is_paused(const px_loop* loop) {
    return loop ? loop->paused : false;
}

int px_loop_audit_count(const px_loop* loop) {
    return loop ? loop->count : 0;
}

int px_loop_audit_get(const px_loop* loop,
                       px_loop_audit_entry* out, int max_entries) {
    if (!loop || !out || max_entries <= 0) return 0;
    int n = loop->count;
    if (n > max_entries) n = max_entries;
    memcpy(out, loop->entries, sizeof(px_loop_audit_entry) * n);
    return n;
}

int px_loop_replay(px_loop* loop, int n) {
    if (!loop || n <= 0) return 0;
    if (n > loop->count) n = loop->count;
    int replayed = 0;
    for (int i = loop->count - n; i < loop->count; i++) {
        px_loop_audit_entry* e = &loop->entries[i];
        if (e->closure_triggered) {
            /* re-trigger with the closure's last intent (since we
             * don't have the original payload here). This is a v4
             * simplification; a real impl would store the payload. */
            px_intent last = px_closure_last_intent(loop->closure);
            px_closure_replay(loop->closure, last);
            replayed++;
        }
        if (e->perception_invoked) {
            px_perception_invoke(loop->perception);
        }
    }
    return replayed;
}

void px_loop_audit_clear(px_loop* loop) {
    if (!loop) return;
    loop->count = 0;
    loop->pending_bd_transition = 0;
    free(loop->pending_bd_reason);
    loop->pending_bd_reason = NULL;
}

void px_loop_mark_breakdown(px_loop* loop, int transition, const char* reason) {
    if (!loop) return;
    loop->pending_bd_transition = transition;
    free(loop->pending_bd_reason);
    if (reason) {
        loop->pending_bd_reason = (char*)malloc(strlen(reason) + 1);
        if (loop->pending_bd_reason) strcpy(loop->pending_bd_reason, reason);
    } else {
        loop->pending_bd_reason = NULL;
    }
}
