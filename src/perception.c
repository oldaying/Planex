/*
 * perception.c — Perception (4th abstraction, machine → user direction)
 *
 * Per ADR-0005: Perception is a first-class abstraction covering
 * Norman's stages 5 (Perception) and 6 (Interpretation, user-side).
 * It is the denotation of state — how Estimates become perceivable.
 *
 * A perception is a pure function: it takes a set of Estimates as
 * input and returns a denotation (pixel buffer, a11y tree, log
 * string, etc.). Same inputs → same output, no side effects.
 *
 * Multiple perceptions can coexist for the same Estimates — one
 * for screen pixels, one for a11y, one for headless test snapshots.
 *
 * Phase 2 (this version, v0.3):
 *   - px_perceptions_for_estimate() returns matching perceptions
 *   - px_perception_invoke_all() runs every registered perception
 *   - Used by app loop to drive rendering
 *
 * Phase 1 (v0.2) status: API existed but px_perceptions_for_estimate
 * was a stub. Phase 2 implements it properly.
 *
 * Validated by three (c)-route prototypes + new Phase 2 demos:
 *   - examples/counter_denotative.c (4 unit tests pass)
 *   - examples/calculator_denotative.c (4 unit tests pass)
 *   - examples/counter_interactive.c (2 unit tests pass + 1273 frames 60fps)
 *   - examples/perception_smoke.c (9 API tests pass)
 *   - examples/perception_phase2.c (new Phase 2 demo)
 *
 * Inspired by:
 *   - Conal Elliott — denotative design (pure-function denotation)
 *   - Don Norman — 7-stage model (stages 5-6 evaluation side)
 *   - Winograd/Flores — speech-act theory (declaration as machine→user)
 *
 * THREAD SAFETY: This module uses a global linked list (g_perceptions).
 * Not thread-safe. Planex is designed as single-threaded single-app.
 * If multi-threading is needed in the future, add a mutex around
 * g_perceptions access, or switch to per-thread registry.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

/* ============================================================
 * Perception struct
 * ============================================================ */

struct px_perception {
    char*           name;
    px_perceive_fn  fn;
    px_estimate**    inputs;       /* copied array */
    int             n_inputs;
    void*           user;

    /* v3 prototype: interpretant sub-API.
     * The intended_interpretant is the system's *declared* meaning —
     * what the system *wanted* the actor to take the representamen
     * to mean. The interpret_fn (if non-NULL) is called by the loop
     * after the perceive fn, to predict the actor's *actual*
     * interpretant given the representamen + actor. */
    char*              intended_interpretant;
    px_interpret_fn    interpret_fn;
    void*             interpret_user;

    /* v0.5 Phase 2: representamen cache for "fire at most once per turn".
     *
     * When auto-invocation (px_estimate_set → invoke_for_estimate) fires
     * this perception, the fn's return value is cached here. A
     * subsequent px_perception_invoke_single on this perception (e.g.
     * by px_loop_step) returns the cached value WITHOUT re-firing the
     * fn. This avoids the double-fire bug that previously existed
     * between Phase 2 auto-invocation and the loop's explicit invocation.
     *
     * The cache is invalidated at the start of each "turn" (each
     * px_loop_step / px_loop_step_view_only / px_loop_replay iteration)
     * via the internal px__perception_clear_cache / clear_all_caches
     * helpers. Outside a loop, the cache is only meaningful for the
     * duration of a single px_estimate_set → auto-invoke call chain;
     * it is not consulted by any other path.
     *
     * Ownership (v0.6): if a free_fn is registered via
     * px_perception_set_free_fn, the OLD cached representamen is freed
     * through it before the cache is overwritten (fire_and_cache) and
     * on perception free. Without a free_fn the old behavior stands —
     * the previous value is leaked (perception fns are heterogeneous,
     * the framework cannot guess the destructor). This retires the
     * documented v0.5 "the previous value is leaked" limitation for
     * perceptions that declare their denotation type. */
    void*           last_representamen;
    bool            has_last;

    /* v0.6: optional destructor for representamens produced by fn.
     * NULL = caller-managed (leak-on-overwrite, v0.5 behavior). */
    void          (*free_fn)(void* representamen);
};

/* ============================================================
 * Global registry (Phase 1: simple linked list)
 *
 * Phase 2 will replace this with an indexed structure for fast
 * px_perceptions_for_estimate() queries. For now, a linked list
 * is enough — Planex apps typically have 1-10 perceptions.
 * ============================================================ */

typedef struct px_perception_node {
    px_perception*                p;
    struct px_perception_node*    next;
} px_perception_node;

static px_perception_node* g_perceptions = NULL;
static int                 g_perception_count = 0;

/* ============================================================
 * API
 * ============================================================ */

px_perception* px_perception_new(
    const char*     name,
    px_perceive_fn   fn,
    px_estimate**    inputs,
    int              n_inputs,
    void*            user) {

    if (!name || !fn) return NULL;
    if (n_inputs < 0) return NULL;
    if (n_inputs > 0 && !inputs) return NULL;

    px_perception* p = (px_perception*)calloc(1, sizeof(px_perception));
    if (!p) return NULL;

    p->name = px_strdup(name);
    if (!p->name) {
        free(p);
        return NULL;
    }

    p->fn = fn;
    p->n_inputs = n_inputs;
    p->user = user;

    if (n_inputs > 0) {
        p->inputs = (px_estimate**)calloc(n_inputs, sizeof(px_estimate*));
        if (!p->inputs) {
            free(p->name);
            free(p);
            return NULL;
        }
        memcpy(p->inputs, inputs, n_inputs * sizeof(px_estimate*));
    }

    /* Register in global list */
    px_perception_node* node = (px_perception_node*)calloc(1, sizeof(px_perception_node));
    if (!node) {
        free(p->inputs);
        free(p->name);
        free(p);
        return NULL;
    }
    node->p = p;
    node->next = g_perceptions;
    g_perceptions = node;
    g_perception_count++;

    return p;
}

void px_perception_free(px_perception* p) {
    if (!p) return;

    /* v0.6: free the cached representamen through the registered
     * destructor, if any. */
    if (p->has_last && p->last_representamen && p->free_fn) {
        p->free_fn(p->last_representamen);
    }

    /* Remove from registry */
    px_perception_node** pp = &g_perceptions;
    while (*pp) {
        if ((*pp)->p == p) {
            px_perception_node* to_free = *pp;
            *pp = to_free->next;
            free(to_free);
            g_perception_count--;
            break;
        }
        pp = &((*pp)->next);
    }

    free(p->inputs);
    free(p->intended_interpretant);   /* v3 prototype */
    free(p->name);
    free(p);
}

const char* px_perception_name(const px_perception* p) {
    return p ? p->name : NULL;
}

/* Phase 2: iterate registry, find perceptions whose inputs contain est.
 * Returns a heap-allocated array of perception pointers (caller frees).
 * *out_count is set to the number of matches (0 if none). */
px_perception** px_perceptions_for_estimate(px_estimate* est, int* out_count) {
    if (out_count) *out_count = 0;
    if (!est) return NULL;

    /* First pass: count matches */
    int match_count = 0;
    px_perception_node* node = g_perceptions;
    while (node) {
        for (int i = 0; i < node->p->n_inputs; i++) {
            if (node->p->inputs[i] == est) {
                match_count++;
                break;
            }
        }
        node = node->next;
    }

    if (match_count == 0) return NULL;

    /* Allocate result array */
    px_perception** result = (px_perception**)calloc(match_count, sizeof(px_perception*));
    if (!result) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    /* Second pass: collect matches */
    int idx = 0;
    node = g_perceptions;
    while (node) {
        for (int i = 0; i < node->p->n_inputs; i++) {
            if (node->p->inputs[i] == est) {
                result[idx++] = node->p;
                break;
            }
        }
        node = node->next;
    }

    if (out_count) *out_count = match_count;
    return result;
}

/* ============================================================
 * v0.5 Phase 2: representamen cache + fire_and_cache helper
 *
 * The cache enables "fire at most once per turn" semantics. A turn
 * is bounded by px_loop_step / px_loop_step_view_only / each
 * px_loop_replay iteration / each px_estimate_set outside a loop.
 * At the start of each turn, the relevant caches are cleared.
 * During the turn, the first fire fills the cache; subsequent
 * invoke_single / invoke_all calls for the same perception return
 * the cached value without re-firing.
 *
 * This avoids the v0.4 double-fire bug: when px_estimate_set (called
 * from a closure action inside px_loop_step) auto-invoked the bound
 * perception, and px_loop_step then called invoke_single on the same
 * perception, the fn fired twice. With caching, the second call
 * returns the cached representamen.
 * ============================================================ */

static void fire_and_cache(px_perception* p) {
    if (!p || !p->fn) return;
    /* v0.6: if a destructor is registered, the previous cached value
     * is freed through it BEFORE being overwritten — this is the fix
     * for the documented v0.5 leak ("the previous value is leaked").
     * Without a destructor we keep the v0.5 behavior (ownership is
     * unknown to the framework). */
    if (p->has_last && p->last_representamen && p->free_fn) {
        p->free_fn(p->last_representamen);
    }
    p->last_representamen = p->fn(p->inputs, p->n_inputs, p->user);
    p->has_last = true;
}

/* v0.6: register the destructor used to free cached representamens.
 *
 * Perception fns are heterogeneous (px_fb*, char*, JSON strings, ...)
 * so the framework cannot guess how to free a denotation. The caller
 * who KNOWS the denotation type registers its destructor here; from
 * then on, every cache overwrite and px_perception_free frees the old
 * value through it. NULL restores the v0.5 leak-on-overwrite behavior.
 *
 * Example:
 *   px_perception_set_free_fn(p, (void (*)(void*))px_fb_free);
 *
 * Note: the loop's interpretant (px_perception_interpret's product)
 * is owned by the interpret_fn, not by this mechanism — see
 * feedback.c for that known limitation. */
void px_perception_set_free_fn(px_perception* p, void (*free_fn)(void*)) {
    if (!p) return;
    p->free_fn = free_fn;
}

/* Internal: clear cache on a single perception. Called by loop
 * functions at the start of each turn. NOT part of public API.
 *
 * v0.6 ownership contract: once a representamen is cached, the
 * perception owns it; px_perception_invoke_single returns a BORROWED
 * pointer, valid until the next turn boundary (this clear, the next
 * fire, or px_perception_free). If a free_fn is registered, the value
 * is freed HERE — holding an invoke_single result across turns was
 * never valid (it would be stale/leaked in v0.5 too). */
void px__perception_clear_cache(px_perception* p) {
    if (!p) return;
    if (p->has_last && p->last_representamen && p->free_fn) {
        p->free_fn(p->last_representamen);
    }
    p->has_last = false;
    p->last_representamen = NULL;
}

/* Internal: clear cache on all registered perceptions. Called by
 * view-only step + replay (which iterate all perceptions). */
void px__perception_clear_all_caches(void) {
    px_perception_node* node = g_perceptions;
    while (node) {
        px__perception_clear_cache(node->p);
        node = node->next;
    }
}

/* Phase 2: invoke all registered perceptions.
 * For each perception, call its fn with its inputs and user.
 * ALWAYS fires (no cache-based skip) — the cache is only consulted
 * by px_perception_invoke_single (the loop's "get representamen"
 * path). Outside a loop, each invoke_all call is expected to fire
 * all perceptions. Inside a loop, the loop function clears caches
 * at turn start so invoke_single below either returns the freshly
 * cached result (if auto-invocation fired) or fires explicitly.
 *
 * Returns the number of perceptions actually invoked (fn called). */
int px_perception_invoke_all(void) {
    int invoked = 0;
    px_perception_node* node = g_perceptions;
    while (node) {
        if (node->p->fn) {
            fire_and_cache(node->p);
            invoked++;
        }
        node = node->next;
    }
    return invoked;
}

/* Phase 2: invoke perceptions that depend on the given Estimate.
 * Useful when only one Estimate changed — only re-run the perceptions
 * that need to update. Returns the number of perceptions invoked.
 *
 * v0.5: auto-invoked by px_estimate_set when an estimate changes.
 * Also callable manually for diagnostic / test purposes. */
int px_perception_invoke_for_estimate(px_estimate* est) {
    if (!est) return 0;

    int count = 0;
    px_perception** matching = px_perceptions_for_estimate(est, &count);
    if (!matching || count == 0) {
        free(matching);
        return 0;
    }

    int invoked = 0;
    for (int i = 0; i < count; i++) {
        px_perception* p = matching[i];
        if (p->fn) {
            fire_and_cache(p);
            invoked++;
        }
    }

    free(matching);
    return invoked;
}

int px_perception_count(void) {
    return g_perception_count;
}

/* v0.5: invoke a single perception's fn and return the produced
 * representamen. Used by px_loop_step to obtain the representamen
 * before calling px_perception_interpret.
 *
 * If the perception's cache is valid (has_last=true) — meaning
 * auto-invocation already fired it this turn — returns the cached
 * representamen without re-firing. Otherwise, fires the fn and
 * caches the result.
 *
 * Returns NULL if p is NULL, p->fn is NULL, or the fn returned NULL. */
void* px_perception_invoke_single(px_perception* p) {
    if (!p) return NULL;
    if (p->has_last) {
        /* Cache is valid — auto-invocation already fired this turn.
         * Return cached representamen without re-firing. */
        return p->last_representamen;
    }
    if (!p->fn) return NULL;
    fire_and_cache(p);
    return p->last_representamen;
}

/* ============================================================
 * v3 prototype — interpretant sub-API
 *
 * The intended_interpretant is the system's *declared* meaning —
 * what the system *wanted* the actor to take the representamen to
 * mean. Example: when rendering "7", the system declares the
 * intended_interpretant as "seven items pending" — distinct from
 * "7 of 10 progress" or "queue length 7".
 *
 * The interpret_fn (if non-NULL) is a Layer 5 hook: a function that,
 * given the produced representamen + actor, predicts the actor's
 * *actual* interpretant. NULL means no prediction (Layer 5 not
 * implemented for this perception).
 *
 * The loop's audit records interpretant_constructed=true iff an
 * interpret_fn was registered and successfully returned a non-NULL
 * interpretant for this iteration.
 *
 * Inspired by Peirce's triadic sign relation (representamen ↔
 * object ↔ interpretant). Without this third term, Planex's model
 * of UI was binary (state → representation), missing the actor's
 * generated meaning.
 * ============================================================ */

void px_perception_set_intended_interpretant(px_perception* p,
                                               const char* semantics) {
    if (!p || !semantics) return;
    free(p->intended_interpretant);
    p->intended_interpretant = px_strdup(semantics);
}

void px_perception_set_interpret_fn(px_perception* p,
                                      px_interpret_fn fn,
                                      void* user) {
    if (!p) return;
    p->interpret_fn  = fn;
    p->interpret_user = user;
}

const char* px_perception_intended_interpretant(const px_perception* p) {
    return p ? p->intended_interpretant : NULL;
}

/* Internal helper for px_loop_step to call after the perceive fn.
 * Returns non-NULL iff an interpret_fn was registered and returned
 * a non-NULL interpretant for this representamen + actor. */
void* px_perception_interpret(px_perception* p,
                                 void* representamen,
                                 px_actor* actor) {
    if (!p || !p->interpret_fn) return NULL;
    return p->interpret_fn(representamen, actor, p->interpret_user);
}
