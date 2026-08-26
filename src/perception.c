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

/* Phase 2: invoke all registered perceptions.
 * For each perception, call its fn with its inputs and user.
 * The denotation (return value) is the caller's responsibility —
 * the perception fn returns it, the caller (e.g., app loop) decides
 * what to do with it (blit to screen, log, etc.).
 *
 * Returns the number of perceptions invoked. */
int px_perception_invoke_all(void) {
    int invoked = 0;
    px_perception_node* node = g_perceptions;
    while (node) {
        if (node->p->fn) {
            node->p->fn(node->p->inputs, node->p->n_inputs, node->p->user);
            invoked++;
        }
        node = node->next;
    }
    return invoked;
}

/* Phase 2: invoke perceptions that depend on the given Estimate.
 * Useful when only one Estimate changed — only re-run the perceptions
 * that need to update. Returns the number of perceptions invoked. */
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
            p->fn(p->inputs, p->n_inputs, p->user);
            invoked++;
        }
    }

    free(matching);
    return invoked;
}

int px_perception_count(void) {
    return g_perception_count;
}
