/*
 * estimate.c — Estimate (state with time + uncertainty)
 *
 * An Estimate is a value that:
 *   - Carries a confidence (Bayesian flavor)
 *   - May be a trajectory over time (for animation)
 *   - Notifies observers when it changes
 *   - May be *derived* from other estimates (Stage 3 — automatic
 *     dependency tracking)
 *
 * Inspired by:
 *   - Conal Elliott FRP (Behavior = Time → a)
 *   - Friston predictive coding (state as posterior estimate)
 *   - Spreadsheet cells (auto-dependency tracking)
 *
 * Implementation:
 *   - Linear value + animation trajectory (from → to over duration_ms
 *     with ease-out curve).
 *   - Observer linked list (set() fires notifications).
 *   - Derived estimates (Stage 3): a px_estimate may have a non-NULL
 *     `derived` field, which holds the recompute function + sources.
 *     The derived estimate subscribes to each source; when a source
 *     fires, derived_recompute() is called, which calls fn and
 *     px_estimate_set() to update the derived value.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

/* Windows: use QueryPerformanceCounter for monotonic time */
double px_now_ms(void) {
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart * 1000.0;
}
#else
#include <time.h>

/* POSIX: use clock_gettime(CLOCK_MONOTONIC) */
double px_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
}
#endif

typedef struct px_observer {
    px_estimate_observer fn;
    void*                user;
    struct px_observer*  next;
} px_observer;

/* Forward declaration — used by px_estimate_value() before full definition */
static void notify(px_estimate* e);

/* Forward decl */
struct px_estimate;

/* State for derived estimates (NULL for regular estimates). */
typedef struct {
    px_derive_fn    fn;
    void*           user;
    px_estimate**   sources;     /* owned array of pointers (not the estimates themselves) */
    int             n_sources;
    int             capacity;   /* allocated size of sources array (Stage 19) */
    px_estimate*    self;       /* back-pointer to owning estimate */
} px_derived_state;

struct px_estimate {
    double      value;
    double      confidence;

    /* v0.6: predictive-coding state (Friston). A pending prediction
     * makes the NEXT px_estimate_set resolve it: surprise is recorded,
     * confidence decays by prediction error. One-shot per predict().
     * See px_estimate_predict() docs in planex.h. */
    bool        has_prediction;
    double      predicted_value;
    double      prediction_tolerance;
    double      last_surprise;

    /* Animation trajectory (Planex's "Behavior" kernel) */
    bool        animating;
    double      duration_ms;
    double      from_value;
    double      to_value;
    double      start_time_ms;    /* px_now_ms() at animate() call */

    px_observer* observers;

    /* Derived state — non-NULL if this estimate is derived from others. */
    px_derived_state* derived;

    /* v0.5: cycle-detection flag for px_derived_recompute. Set to true
     * while this estimate is mid-recompute; cleared on return. If a
     * recompute path re-enters this estimate, the flag is true and the
     * re-entrant call returns early, breaking the cycle. See
     * docs/concepts/leak-budgets.md §2 for the L2 leak this retires. */
    bool        recomputing;
};

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_estimate* px_estimate_new(double value, double confidence) {
    px_estimate* e = (px_estimate*)calloc(1, sizeof(px_estimate));
    if (!e) return NULL;
    e->value         = value;
    e->confidence     = confidence;
    e->animating      = false;
    e->duration_ms    = 0;
    e->from_value     = 0;
    e->to_value       = 0;
    e->start_time_ms  = 0;
    e->observers      = NULL;
    e->derived        = NULL;
    e->recomputing    = false;
    return e;
}

void px_estimate_free(px_estimate* e) {
    if (!e) return;
    /* Free observers */
    px_observer* o = e->observers;
    while (o) {
        px_observer* next = o->next;
        free(o);
        o = next;
    }
    /* Free derived state */
    if (e->derived) {
        free(e->derived->sources);
        free(e->derived);
    }
    free(e);
}

/* v0.5: px_estimate_value is now a pure query (const-correct).
 * It returns the cached value without auto-sampling. To bring the
 * cached value up to current time during an animation, call
 * px_estimate_advance(e, px_now_ms()) first. See header doc. */
double px_estimate_value(const px_estimate* e) {
    if (!e) return 0.0;
    return e->value;
}

double px_estimate_confidence(const px_estimate* e) {
    return e ? e->confidence : 0.0;
}

static void notify(px_estimate* e) {
    for (px_observer* o = e->observers; o; o = o->next) {
        o->fn(e, o->user);
    }
}

/* v0.5: px_estimate_advance explicitly advances the animation state
 * to time t_ms. If animation has finished (elapsed >= duration),
 * finalizes: clears animating, sets value=target, fires observers.
 * If mid-animation, caches the interpolated value at t_ms (does NOT
 * fire observers — continuous change is not a discrete event).
 * If not animating, no-op. See header doc + leak-budgets.md §2. */
void px_estimate_advance(px_estimate* e, double t_ms) {
    if (!e || !e->animating) return;
    double elapsed = t_ms - e->start_time_ms;
    if (elapsed >= e->duration_ms) {
        /* Animation finished — finalize */
        e->animating = false;
        e->value = e->to_value;
        notify(e);
    } else if (elapsed >= 0) {
        /* Mid-animation: cache interpolated value (no notify —
         * continuous change is not a discrete event). */
        e->value = px_estimate_sample(e, elapsed);
    }
    /* elapsed < 0 (time travel before animation start): no-op */
}

void px_estimate_set(px_estimate* e, double value, double confidence) {
    if (!e) return;
    e->animating  = false;
    e->value      = value;
    e->confidence = confidence;

    /* v0.6: resolve a pending prediction (predictive-coding loop).
     * This is confidence's framework-side consumer: an observation that
     * violates a registered prediction reduces confidence even when the
     * caller asserts 1.0. The prediction is one-shot. */
    if (e->has_prediction) {
        double tol = e->prediction_tolerance > 0 ? e->prediction_tolerance : 1.0;
        e->last_surprise = fabs(value - e->predicted_value);
        e->confidence = confidence * exp(-e->last_surprise / tol);
        e->has_prediction = false;
    } else {
        e->last_surprise = 0.0;
    }

    notify(e);
    /* v0.5 Phase 2: auto-invoke perceptions that depend on this
     * estimate. This closes the implicit-seam gap — users no longer
     * need to call px_perception_invoke_for_estimate manually after
     * a state change. See leak-budgets.md §4. */
    extern int px_perception_invoke_for_estimate(px_estimate* est);
    px_perception_invoke_for_estimate(e);
}

/* v0.6: register a prediction for this estimate's next value.
 *
 * The Friston citation in this file's header ("state as posterior
 * estimate") finally gets a runtime counterpart: predict → observe
 * → update. Until v0.6 the confidence field had no framework-side
 * consumer (alternative-perspectives.md School 4: "currently
 * decorative") — users could read and write it, but no framework
 * logic ever responded to it.
 *
 * Contract:
 *   px_estimate_predict(e, expected, tolerance);
 *   px_estimate_set(e, observed, conf);       <- resolves the prediction
 *   px_estimate_surprise(e);                   <- |observed - expected|
 *
 * On the resolving set:
 *   confidence = conf * exp(-|observed - expected| / tolerance)
 *
 * i.e. accurate predictions keep confidence, violated predictions
 * decay it (exponentially in normalized error). tolerance <= 0 is
 * treated as 1.0. The prediction is one-shot: a second set() without
 * a new predict() carries the caller's confidence unchanged.
 *
 * Intended for sensor-fed estimates (the confidence_demo.c pattern).
 * Derived estimates technically work but predictions on them are
 * unusual — recompute overwrites the value from sources. */
void px_estimate_predict(px_estimate* e, double expected, double tolerance) {
    if (!e) return;
    e->has_prediction      = true;
    e->predicted_value     = expected;
    e->prediction_tolerance = tolerance;
}

/* v0.6: absolute prediction error of the last prediction-resolved
 * px_estimate_set; 0.0 when no prediction was pending (or before any
 * set). Pure query. */
double px_estimate_surprise(const px_estimate* e) {
    return e ? e->last_surprise : 0.0;
}

void px_estimate_animate(px_estimate* e, double target, double duration_ms) {
    if (!e || duration_ms <= 0) {
        if (e) px_estimate_set(e, target, e->confidence);
        return;
    }
    /* If already animating, start from current sampled value to avoid jump */
    if (e->animating) {
        e->from_value = px_estimate_sample(e, px_now_ms() - e->start_time_ms);
    } else {
        e->from_value = e->value;
    }
    e->animating     = true;
    e->duration_ms   = duration_ms;
    e->to_value      = target;
    e->start_time_ms = px_now_ms();
}

double px_estimate_sample(px_estimate* e, double t_ms) {
    if (!e) return 0.0;
    if (!e->animating) return e->value;

    double progress = (e->duration_ms > 0) ? (t_ms / e->duration_ms) : 1.0;
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;

    /* ease-out quadratic: 1 - (1-p)^2 */
    double eased = 1.0 - (1.0 - progress) * (1.0 - progress);
    return e->from_value + (e->to_value - e->from_value) * eased;
}

/* v0.5: px_estimate_is_animating is now a pure query. Previously it
 * finalized the animation as a side effect of the check; the new
 * contract requires the caller to call px_estimate_advance first if
 * they want finalization. See header doc + leak-budgets.md §2. */
bool px_estimate_is_animating(const px_estimate* e) {
    if (!e) return false;
    return e->animating;
}

/* v0.5: px_estimate_now is now a const alias of px_estimate_value.
 * Callers needing the animated value at current time should call
 * px_estimate_advance(e, px_now_ms()) first. */
double px_estimate_now(const px_estimate* e) {
    if (!e) return 0.0;
    return e->value;
}

void px_estimate_observe(px_estimate* e, px_estimate_observer fn, void* user) {
    if (!e || !fn) return;
    px_observer* o = (px_observer*)malloc(sizeof(px_observer));
    if (!o) return;
    o->fn   = fn;
    o->user = user;
    o->next = e->observers;
    e->observers = o;
}

/* ============================================================
 * Derived estimate (Stage 3 — automatic dependency tracking)
 *
 * A derived estimate subscribes to each source via the observer
 * mechanism. When any source changes, derived_recompute() runs
 * the user's fn and updates the derived's value via
 * px_estimate_set() — which fires the derived's own observers
 * (and, in v0.5+, auto-invokes perceptions depending on the
 * derived).
 *
 * Cycle detection (v0.5): each px_estimate carries a `recomputing`
 * flag. If recompute is called on an estimate whose flag is already
 * true, the call returns immediately, breaking the cycle. The flag
 * is set on entry and cleared on exit. This means cycles no longer
 * cause stack overflow; values along a cycle may be stale (a
 * cyclic dependency has no well-defined resolution order), but the
 * program continues to run. See leak-budgets.md §2.
 * ============================================================ */

static void derived_on_source_changed(px_estimate* src, void* user) {
    (void)src;
    px_estimate* derived = (px_estimate*)user;
    px_derived_recompute(derived);
}

/* v0.7 Line 2: propagation-depth accounting (see px_derived_recompute).
 * g_px_derive_depth is the CURRENT recompute-chain recursion depth;
 * g_px_derive_depth_peak is the deepest chain observed since the last
 * px_derive_depth_peak() read. px_loop_step reads-and-resets it per
 * iteration for the audit entry. Process-global (single-app design). */
static int g_px_derive_depth      = 0;
static int g_px_derive_depth_peak = 0;

int px_derive_depth_peak(void) {
    /* Pure query — the read side of the accounting pair. The reset is
     * a separate explicit op (px_derive_depth_reset) so neither query
     * carries a side effect (the leak criterion: an apparent getter
     * that mutates is an L2 leak). */
    return g_px_derive_depth_peak;
}

void px_derive_depth_reset(void) {
    g_px_derive_depth_peak = 0;
}

void px_derived_recompute(px_estimate* derived) {
    if (!derived || !derived->derived) return;
    /* v0.5 cycle detection: if this estimate is already mid-recompute,
     * we've re-entered via a cyclic dependency. Return without
     * recursing further. The cycle is broken; values along the cycle
     * may be stale (last writer wins), but no stack overflow. */
    if (derived->recomputing) return;
    derived->recomputing = true;

    /* v0.7 Line 2: propagation-depth accounting — track chain nesting
     * so the audit entry can answer "how deep did propagation go?". */
    g_px_derive_depth++;
    if (g_px_derive_depth > g_px_derive_depth_peak) {
        g_px_derive_depth_peak = g_px_derive_depth;
    }

    px_derived_state* st = derived->derived;
    double new_value = st->fn(st->sources, st->n_sources, st->user);
    /* px_estimate_set fires observers + auto-invokes perceptions.
     * Observers may chain to other deriveds' recompute — that's fine,
     * each has its own recomputing flag. If a downstream derived
     * depends back on us, our flag is still true → its recompute of
     * us returns early. */
    px_estimate_set(derived, new_value, derived->confidence);

    g_px_derive_depth--;
    derived->recomputing = false;
}

px_estimate* px_derived_new(px_derive_fn fn, void* user,
                             px_estimate** sources, int n_sources) {
    if (!fn || !sources || n_sources <= 0) return NULL;

    /* Allocate derived estimate (initial value = 0, will recompute below). */
    px_estimate* e = px_estimate_new(0, 1.0);
    if (!e) return NULL;

    /* Allocate derived state */
    px_derived_state* st = (px_derived_state*)calloc(1, sizeof(px_derived_state));
    if (!st) {
        px_estimate_free(e);
        return NULL;
    }
    st->fn = fn;
    st->user = user;
    st->n_sources = n_sources;
    st->capacity = n_sources;
    st->self = e;

    /* Copy sources array */
    st->sources = (px_estimate**)malloc((size_t)n_sources * sizeof(px_estimate*));
    if (!st->sources) {
        free(st);
        px_estimate_free(e);
        return NULL;
    }
    memcpy(st->sources, sources, (size_t)n_sources * sizeof(px_estimate*));

    e->derived = st;

    /* Subscribe to each source — when any source changes,
     * derived_on_source_changed will fire and recompute us. */
    for (int i = 0; i < n_sources; i++) {
        px_estimate_observe(sources[i], derived_on_source_changed, e);
    }

    /* Compute initial value */
    px_derived_recompute(e);

    return e;
}

/* ============================================================
 * Stage 19: Dynamic sources — add/remove at runtime
 * ============================================================ */

px_estimate* px_derived_new_dynamic(px_derive_fn fn, void* user) {
    if (!fn) return NULL;

    px_estimate* e = px_estimate_new(0, 1.0);
    if (!e) return NULL;

    px_derived_state* st = (px_derived_state*)calloc(1, sizeof(px_derived_state));
    if (!st) {
        px_estimate_free(e);
        return NULL;
    }
    st->fn = fn;
    st->user = user;
    st->n_sources = 0;
    st->capacity = 0;
    st->sources = NULL;
    st->self = e;

    e->derived = st;

    /* Initial value: fn called with 0 sources */
    px_derived_recompute(e);
    return e;
}

int px_derived_add_source(px_estimate* derived, px_estimate* source) {
    if (!derived || !derived->derived || !source) return -1;
    px_derived_state* st = derived->derived;

    /* Grow array if needed */
    if (st->n_sources >= st->capacity) {
        int new_cap = st->capacity == 0 ? 4 : st->capacity * 2;
        px_estimate** new_arr = (px_estimate**)realloc(st->sources,
            (size_t)new_cap * sizeof(px_estimate*));
        if (!new_arr) return -1;
        st->sources = new_arr;
        st->capacity = new_cap;
    }

    st->sources[st->n_sources++] = source;

    /* Subscribe to this source */
    px_estimate_observe(source, derived_on_source_changed, derived);

    /* Recompute with new source set */
    px_derived_recompute(derived);
    return 0;
}

int px_derived_remove_source(px_estimate* derived, px_estimate* source) {
    if (!derived || !derived->derived || !source) return -1;
    px_derived_state* st = derived->derived;

    /* Find and remove source */
    int found = -1;
    for (int i = 0; i < st->n_sources; i++) {
        if (st->sources[i] == source) {
            found = i;
            break;
        }
    }
    if (found < 0) return -1;  /* not a source */

    /* Shift remaining sources down */
    for (int i = found; i < st->n_sources - 1; i++) {
        st->sources[i] = st->sources[i + 1];
    }
    st->n_sources--;

    /* Note: we don't un-observe the source because px_estimate_observe
     * doesn't support removal. The source's observer list will still
     * have a dangling pointer. This is a known limitation — Stage 20
     * can add px_estimate_unobserve(). For now, apps should free the
     * source estimate AFTER removing it from derived, which clears
     * the observer list entirely. */

    /* Recompute without this source */
    px_derived_recompute(derived);
    return 0;
}

int px_derived_source_count(const px_estimate* derived) {
    if (!derived || !derived->derived) return 0;
    return derived->derived->n_sources;
}
