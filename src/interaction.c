/*
 * interaction.c — Interaction (v0.6 prototype: 6th abstraction candidate)
 *
 * A continuous interaction PROCESS with identity, trajectory, and
 * outcome:
 *
 *     begin(ev) → sample(ev)* → commit(ev) | cancel(reason)
 *
 * This is the Option-B prototype from
 * docs/concepts/speculation/continuous-intent-speculation.md, promoted
 * to code by ADR-0016 (proposed) after hover_drag_4abs.c supplied the
 * ADR-0006 evidence: the Estimate hack is "INTOLERABLE for complex
 * gesture/touch UIs".
 *
 * The design invariant that makes this NOT-Estimate:
 *   samples never notify observers and never auto-invoke perceptions.
 *   A 60Hz mouse-move stream flows through sample() at O(1) with zero
 *   fan-out; the semantic world (Closures, Estimates, Perceptions,
 *   Relations) is touched ONLY at phase transitions:
 *
 *     begin → hook          (compile "process started" into intent)
 *     commit/cancel → hook + bound Closure/Estimate bridges
 *
 * Traditions sampled (full derivation in ADR-0016):
 *   - Garnet Interactors (Myers 1990): interaction technique as a
 *     first-class object with start/running/stop states
 *   - CSP (Hoare): a process is prefix + choice — begin → P',
 *     terminate as commit ⊕ cancel
 *   - Statecharts (Harel): the "do action" of ongoing activity that
 *     plain transition models lack
 *   - Direct manipulation (Shneiderman 1983): "continuous
 *     representation of objects + incremental actions with immediate
 *     feedback" — the process is the interaction unit
 *   - FRP (Elliott): Behavior = Time → a covers continuous VALUES;
 *     what it lacks is the bounded process with an OUTCOME, which is
 *     exactly the commit/cancel choice here
 *
 * THREAD SAFETY: like the rest of Planex, single-threaded.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

#define PX_INT_DEFAULT_CAPACITY 32
#define PX_INT_REASON_MAX 96

struct px_interaction {
    char*         name;

    /* Phase machine: IDLE → BEGAN → ACTIVE → (COMMITTED | CANCELLED).
     * Terminal ops are idempotent no-ops after the first call. */
    px_int_phase  phase;

    /* Trajectory ring. `total` counts every sample since begin (for
     * evidence/metrics); `stored` is what the ring retains (≤ cap). */
    px_int_sample* samples;
    int            cap;
    int            head;      /* next write index */
    int            stored;
    int            total;

    /* Outcome record. */
    char           cancel_reason[PX_INT_REASON_MAX];

    /* Bridge 1: phase hook (fires at begin/commit/cancel only). */
    px_int_hook    hook;
    void*          hook_user;

    /* Bridge 2: commit/cancel resolve to Closure triggers. Payloads
     * are copied at BIND time (intent-as-value: replayable). */
    px_closure*    commit_closure;
    void*          commit_payload;
    size_t         commit_payload_size;
    px_closure*    cancel_closure;

    /* Bridge 3: publish phase to an Estimate at transitions only. */
    px_estimate*   phase_estimate;
};

static const char* const k_phase_names[] = {
    "IDLE", "BEGAN", "ACTIVE", "COMMITTED", "CANCELLED",
};

const char* px_int_phase_str(px_int_phase p) {
    int n = (int)(sizeof(k_phase_names) / sizeof(k_phase_names[0]));
    if ((int)p < 0 || (int)p >= n) return "?";
    return k_phase_names[(int)p];
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_interaction* px_interaction_new(const char* name, int capacity) {
    if (!name) return NULL;
    if (capacity <= 0) capacity = PX_INT_DEFAULT_CAPACITY;

    px_interaction* it = (px_interaction*)calloc(1, sizeof(px_interaction));
    if (!it) return NULL;

    it->name = px_strdup(name);
    if (!it->name) {
        free(it);
        return NULL;
    }

    it->samples = (px_int_sample*)calloc((size_t)capacity, sizeof(px_int_sample));
    if (!it->samples) {
        free(it->name);
        free(it);
        return NULL;
    }

    it->cap        = capacity;
    it->phase      = PX_INT_IDLE;
    it->head       = 0;
    it->stored     = 0;
    it->total      = 0;
    it->cancel_reason[0] = 0;
    return it;
}

void px_interaction_free(px_interaction* it) {
    if (!it) return;
    free(it->samples);
    free(it->commit_payload);
    free(it->name);
    free(it);
}

const char* px_interaction_name(const px_interaction* it) {
    return it ? it->name : NULL;
}

/* ============================================================
 * Feeding
 * ============================================================ */

static void transition(px_interaction* it, px_int_phase to) {
    it->phase = to;

    /* Bridge 3 first (publish, so downstream derived values settle),
     * then Bridge 1 (hook may trigger closures), then Bridge 2
     * (bound commit/cancel closure). Order is fixed and documented:
     * publish → hook → bound closure. */
    if (it->phase_estimate) {
        px_estimate_set(it->phase_estimate, (double)to, 1.0);
    }
    if (it->hook) {
        it->hook(it, to, it->hook_user);
    }
    if (to == PX_INT_COMMITTED && it->commit_closure) {
        px_closure_trigger(it->commit_closure,
                           it->commit_payload, it->commit_payload_size);
    }
    if (to == PX_INT_CANCELLED && it->cancel_closure) {
        px_closure_trigger(it->cancel_closure, NULL, 0);
    }
}

void px_interaction_begin(px_interaction* it) {
    if (!it) return;
    if (it->phase != PX_INT_IDLE) return;  /* already begun or terminal */
    it->total = 0;
    transition(it, PX_INT_BEGAN);
}

void px_interaction_sample(px_interaction* it, const px_int_sample* s) {
    if (!it || !s) return;

    /* Terminal: the outcome is decided; late samples are dropped. */
    if (it->phase == PX_INT_COMMITTED || it->phase == PX_INT_CANCELLED) return;

    /* O(1) append. THE design invariant: no observer notification,
     * no perception auto-invoke, no relation update. The hot path
     * is inert — this is what a "process" is, versus "state".
     *
     * The sample is stored BEFORE the auto-begin transition so the
     * BEGAN hook can read the event that started the process — the
     * beginning event belongs to the trajectory. */
    bool was_idle = (it->phase == PX_INT_IDLE);
    it->samples[it->head] = *s;
    it->head = (it->head + 1) % it->cap;
    if (it->stored < it->cap) it->stored++;
    it->total++;

    if (was_idle) {
        /* Auto-begin (convenience: event loops often see move-before-down).
         * Hook + bridges fire with the first sample already readable. */
        transition(it, PX_INT_BEGAN);
        it->phase = PX_INT_ACTIVE;   /* first sample makes it active */
    } else if (it->phase == PX_INT_BEGAN) {
        it->phase = PX_INT_ACTIVE;
    }
}

void px_interaction_commit(px_interaction* it) {
    if (!it) return;
    if (it->phase == PX_INT_COMMITTED || it->phase == PX_INT_CANCELLED) return;
    transition(it, PX_INT_COMMITTED);
}

void px_interaction_cancel(px_interaction* it, const char* reason) {
    if (!it) return;
    if (it->phase == PX_INT_COMMITTED || it->phase == PX_INT_CANCELLED) return;
    if (reason) {
        strncpy(it->cancel_reason, reason, PX_INT_REASON_MAX - 1);
        it->cancel_reason[PX_INT_REASON_MAX - 1] = 0;
    } else {
        strncpy(it->cancel_reason, "(none)", PX_INT_REASON_MAX - 1);
        it->cancel_reason[PX_INT_REASON_MAX - 1] = 0;
    }
    transition(it, PX_INT_CANCELLED);
}

/* ============================================================
 * Queries (pure)
 * ============================================================ */

px_int_phase px_interaction_phase(const px_interaction* it) {
    return it ? it->phase : PX_INT_IDLE;
}

const char* px_interaction_cancel_reason(const px_interaction* it) {
    return it ? it->cancel_reason : NULL;
}

const px_int_sample* px_interaction_last(const px_interaction* it) {
    if (!it || it->stored == 0) return NULL;
    int idx = (it->head - 1 + it->cap) % it->cap;
    return &it->samples[idx];
}

int px_interaction_stored(const px_interaction* it) {
    return it ? it->stored : 0;
}

int px_interaction_total(const px_interaction* it) {
    return it ? it->total : 0;
}

const px_int_sample* px_interaction_at(const px_interaction* it, int i) {
    if (!it || i < 0 || i >= it->stored) return NULL;
    /* i = 0 is the OLDEST retained sample. When the ring wrapped,
     * head points at the oldest; before wrapping, index 0 is. */
    int start = (it->stored >= it->cap) ? it->head : 0;
    return &it->samples[(start + i) % it->cap];
}

double px_interaction_duration_ms(const px_interaction* it) {
    if (!it || it->stored < 2) return 0.0;
    const px_int_sample* first = px_interaction_at(it, 0);
    const px_int_sample* last  = px_interaction_last(it);
    return last->t_ms - first->t_ms;
}

double px_interaction_displacement(const px_interaction* it) {
    if (!it || it->stored < 2) return 0.0;
    const px_int_sample* first = px_interaction_at(it, 0);
    const px_int_sample* last  = px_interaction_last(it);
    double dx = last->x - first->x;
    double dy = last->y - first->y;
    return sqrt(dx * dx + dy * dy);
}

double px_interaction_path_length(const px_interaction* it) {
    if (!it || it->stored < 2) return 0.0;
    double len = 0.0;
    for (int i = 0; i + 1 < it->stored; i++) {
        const px_int_sample* a = px_interaction_at(it, i);
        const px_int_sample* b = px_interaction_at(it, i + 1);
        double dx = b->x - a->x;
        double dy = b->y - a->y;
        len += sqrt(dx * dx + dy * dy);
    }
    return len;
}

double px_interaction_velocity(const px_interaction* it) {
    if (!it || it->stored < 2) return 0.0;
    const px_int_sample* prev = px_interaction_at(it, it->stored - 2);
    const px_int_sample* last = px_interaction_last(it);
    double dt = last->t_ms - prev->t_ms;
    if (dt <= 0.0) return 0.0;
    double dx = last->x - prev->x;
    double dy = last->y - prev->y;
    return sqrt(dx * dx + dy * dy) / dt;
}

/* ============================================================
 * Bridges
 * ============================================================ */

void px_interaction_on_phase(px_interaction* it, px_int_hook fn, void* user) {
    if (!it) return;
    it->hook      = fn;
    it->hook_user = user;
}

void px_interaction_on_commit(px_interaction* it, px_closure* c,
                              void* payload, size_t payload_size) {
    if (!it) return;
    it->commit_closure = c;
    free(it->commit_payload);
    it->commit_payload = NULL;
    it->commit_payload_size = 0;
    if (payload && payload_size > 0) {
        it->commit_payload = malloc(payload_size);
        if (it->commit_payload) {
            memcpy(it->commit_payload, payload, payload_size);
            it->commit_payload_size = payload_size;
        }
    }
}

void px_interaction_on_cancel(px_interaction* it, px_closure* c) {
    if (!it) return;
    it->cancel_closure = c;
}

void px_interaction_publish_phase(px_interaction* it, px_estimate* est) {
    if (!it) return;
    it->phase_estimate = est;
}
