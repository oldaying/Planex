/* v4/src/estimate.c — essence #1: Object / state
 *
 * Ported from src/estimate.c with minimal changes; this abstraction
 * was already essence-correct in v0.4. The only v4 change is the
 * file location (v4/src/ vs src/).
 *
 * An Estimate carries:
 *   - a value (double)
 *   - a confidence (0.0..1.0; Bayesian / Friston flavor)
 *   - an optional trajectory (Time -> Value) for animations
 *   - observers (notified on set)
 *
 * Derived estimates compute their value from sources via a pure fn.
 */

/* clock_gettime + CLOCK_MONOTONIC require _POSIX_C_SOURCE >= 199309L.
 * Define before including system headers. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ----- monotonic time ----- */

double px_now_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart) * 1000.0 / (double)(freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
#endif
}

/* ----- observer list (small, fixed-capacity) ----- */

#define EST_MAX_OBSERVERS 8

struct px_estimate {
    double value;
    double confidence;

    /* animation */
    bool   animating;
    double anim_start_ms;
    double anim_duration_ms;
    double anim_from;
    double anim_to;

    /* derived */
    bool   is_derived;
    px_derive_fn derive_fn;
    void*  derive_user;
    px_estimate** sources;
    int    n_sources;

    /* observers */
    px_estimate_observer observers[EST_MAX_OBSERVERS];
    void* observer_user[EST_MAX_OBSERVERS];
    int    n_observers;
};

static void notify_observers(px_estimate* e) {
    for (int i = 0; i < e->n_observers; i++) {
        e->observers[i](e, e->observer_user[i]);
    }
}

/* ----- basic API ----- */

px_estimate* px_estimate_new(double value, double confidence) {
    px_estimate* e = (px_estimate*)calloc(1, sizeof(px_estimate));
    if (!e) return NULL;
    e->value = value;
    e->confidence = confidence;
    return e;
}

void px_estimate_free(px_estimate* e) {
    if (!e) return;
    if (e->is_derived && e->sources) {
        free(e->sources);
    }
    free(e);
}

double px_estimate_value(px_estimate* e) {
    if (!e) return 0.0;
    return e->value;
}

double px_estimate_confidence(const px_estimate* e) {
    return e ? e->confidence : 0.0;
}

double px_estimate_now(px_estimate* e) {
    if (!e) return 0.0;
    if (e->animating) {
        double t = px_now_ms() - e->anim_start_ms;
        if (t >= e->anim_duration_ms) {
            e->animating = false;
            e->value = e->anim_to;
        } else {
            e->value = px_estimate_sample(e, t);
        }
    }
    return e->value;
}

bool px_estimate_is_animating(px_estimate* e) {
    if (!e) return false;
    if (e->animating) {
        /* Finalize if past duration */
        double t = px_now_ms() - e->anim_start_ms;
        if (t >= e->anim_duration_ms) {
            e->animating = false;
            e->value = e->anim_to;
        }
    }
    return e->animating;
}

/* Defined later in file (after propagate_change is declared). */
void px_estimate_set(px_estimate* e, double value, double confidence);

void px_estimate_animate(px_estimate* e, double target, double duration_ms) {
    if (!e || duration_ms <= 0) {
        if (e) px_estimate_set(e, target, e ? e->confidence : 1.0);
        return;
    }
    e->animating = true;
    e->anim_start_ms = px_now_ms();
    e->anim_duration_ms = duration_ms;
    e->anim_from = e->value;
    e->anim_to = target;
}

double px_estimate_sample(px_estimate* e, double t_ms) {
    if (!e || !e->animating) return e ? e->value : 0.0;
    double t = t_ms / e->anim_duration_ms;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    /* ease-out cubic: 1 - (1 - t)^3 */
    double ease = 1.0 - pow(1.0 - t, 3.0);
    return e->anim_from + (e->anim_to - e->anim_from) * ease;
}

void px_estimate_observe(px_estimate* e, px_estimate_observer fn, void* user) {
    if (!e || !fn) return;
    if (e->n_observers >= EST_MAX_OBSERVERS) return;
    e->observers[e->n_observers] = fn;
    e->observer_user[e->n_observers] = user;
    e->n_observers++;
}

/* ----- derived estimates ----- */

/* When a source changes, all derived estimates depending on it must
 * recompute. In v4 we use a simple propagation: px_estimate_set on a
 * source calls a global "invalidate dependents" pass. For minimal
 * implementation we maintain a global registry of derived estimates
 * and check each one when any set() happens. This is O(N) per set;
 * fine for v4 verification scale. */

#define MAX_DERIVED 64
static px_estimate* g_derived_registry[MAX_DERIVED];
static int          g_derived_count = 0;

static void register_derived(px_estimate* e) {
    if (g_derived_count < MAX_DERIVED) {
        g_derived_registry[g_derived_count++] = e;
    }
}

/* Called by px_estimate_set when source changes (e is the SOURCE). */
static void propagate_change(px_estimate* source) {
    for (int i = 0; i < g_derived_count; i++) {
        px_estimate* d = g_derived_registry[i];
        if (!d->is_derived) continue;
        for (int s = 0; s < d->n_sources; s++) {
            if (d->sources[s] == source) {
                px_derived_recompute(d);
                notify_observers(d);
                break;
            }
        }
    }
}

/* px_estimate_set: set value + confidence, cancel any animation,
 * notify observers, and propagate to derived estimates that depend
 * on this estimate as a source. */
void px_estimate_set(px_estimate* e, double value, double confidence) {
    if (!e) return;
    e->animating = false;
    e->value = value;
    e->confidence = confidence;
    notify_observers(e);
    /* If this estimate is a source for any derived estimate, recompute. */
    if (!e->is_derived) {
        propagate_change(e);
    }
}

px_estimate* px_derived_new(px_derive_fn fn, void* user,
                             px_estimate** sources, int n_sources) {
    if (!fn || n_sources <= 0) return NULL;
    px_estimate* e = (px_estimate*)calloc(1, sizeof(px_estimate));
    if (!e) return NULL;
    e->is_derived = true;
    e->derive_fn = fn;
    e->derive_user = user;
    e->n_sources = n_sources;
    e->sources = (px_estimate**)malloc(sizeof(px_estimate*) * n_sources);
    if (!e->sources) { free(e); return NULL; }
    memcpy(e->sources, sources, sizeof(px_estimate*) * n_sources);
    register_derived(e);
    px_derived_recompute(e);
    return e;
}

void px_derived_recompute(px_estimate* derived) {
    if (!derived || !derived->is_derived) return;
    double v = derived->derive_fn(derived->sources, derived->n_sources,
                                   derived->derive_user);
    derived->value = v;
}
