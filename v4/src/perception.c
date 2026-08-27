/* v4/src/perception.c — essence #2: Representamen (sign vehicle)
 *
 * A perception is a pure function: takes Estimates as input, returns
 * a representamen (caller-owned). Same inputs -> same output, no
 * side effects.
 *
 * v4 BREAK: Perception no longer has set_intended_interpretant or
 * set_interpret_fn. Those moved to the Interpretant abstraction
 * (essence #3). Perception now ONLY produces the representamen.
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_perception {
    char*          name;
    px_perceive_fn fn;
    void*          user;
    px_estimate**  inputs;
    int            n_inputs;
};

/* Global registry for the count() introspection. Small capacity. */
#define MAX_PERCEPTIONS 64
static px_perception* g_perceptions[MAX_PERCEPTIONS];
static int            g_perceptions_count = 0;

static void register_perception(px_perception* p) {
    if (g_perceptions_count < MAX_PERCEPTIONS) {
        g_perceptions[g_perceptions_count++] = p;
    }
}

static void unregister_perception(px_perception* p) {
    for (int i = 0; i < g_perceptions_count; i++) {
        if (g_perceptions[i] == p) {
            g_perceptions[i] = g_perceptions[g_perceptions_count - 1];
            g_perceptions_count--;
            return;
        }
    }
}

px_perception* px_perception_new(const char* name,
                                   px_perceive_fn fn,
                                   px_estimate** inputs,
                                   int n_inputs,
                                   void* user) {
    if (!fn || n_inputs < 0) return NULL;
    px_perception* p = (px_perception*)calloc(1, sizeof(px_perception));
    if (!p) return NULL;
    if (name) {
        p->name = (char*)malloc(strlen(name) + 1);
        if (!p->name) { free(p); return NULL; }
        strcpy(p->name, name);
    } else {
        p->name = NULL;
    }
    p->fn = fn;
    p->user = user;
    p->n_inputs = n_inputs;
    if (n_inputs > 0) {
        p->inputs = (px_estimate**)malloc(sizeof(px_estimate*) * n_inputs);
        if (!p->inputs) { free(p->name); free(p); return NULL; }
        memcpy(p->inputs, inputs, sizeof(px_estimate*) * n_inputs);
    } else {
        p->inputs = NULL;
    }
    register_perception(p);
    return p;
}

void px_perception_free(px_perception* p) {
    if (!p) return;
    unregister_perception(p);
    free(p->name);
    free(p->inputs);
    free(p);
}

const char* px_perception_name(const px_perception* p) {
    return p ? p->name : NULL;
}

void* px_perception_invoke(px_perception* p) {
    if (!p || !p->fn) return NULL;
    return p->fn(p->inputs, p->n_inputs, p->user);
}

px_estimate** px_perception_inputs(const px_perception* p, int* out_count) {
    if (out_count) *out_count = p ? p->n_inputs : 0;
    return p ? p->inputs : NULL;
}

int px_perception_count(void) {
    return g_perceptions_count;
}
