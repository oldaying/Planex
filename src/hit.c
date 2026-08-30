/*
 * hit.c — Region + affordance query (v0.6 prototype: intent compilation;
 * v0.7 Line 1 gains the routing-side compile step, px_afford_compile)
 *
 * The missing compile step between PHYSICAL events and SEMANTIC intents.
 *
 * Before v0.6, px_app_run dispatched raw coordinates to on_click(x, y)
 * and every application hand-rolled its own hit testing
 * (counter_interactive.c's px_hit_region struct is user code, not
 * library code). The audit's D-A1 finding: "from 'where the user
 * clicked' to 'which Closure fires with what payload' is outsourced
 * to every app" — the translation segment of Norman's execution gulf
 * was outside the framework.
 *
 * The fix is not a new abstraction. Planex ALREADY has the right
 * vocabulary: PX_REL_AFFORDS ("a affords action b", Gibson via the
 * 6-tradition survey). A px_region is pure geometry data that lives
 * as the `a` node of an AFFORDS edge:
 *
 *     px_region* r = px_region_new(px_rect_make(20, 40, 280, 32), "inc");
 *     px_declare(g, r, PX_REL_AFFORDS, inc_closure);
 *     ...
 *     px_closure* c = px_afford_at(g, ev.x, ev.y);
 *     if (c) px_closure_trigger(c, &payload, sizeof(payload));
 *
 * Hit-testing IS an affordance query. The spatial model is data in
 * the Relation graph (queryable, constrainable — like everything
 * else); px_afford_at is just the reader.
 *
 * Registry semantics: like perceptions, regions are process-global.
 * Containment scans run most-recently-declared-first, so later
 * declarations stack "on top" (z-order by declaration order). Use
 * px_region_set_rect for re-layout — the object identity is stable
 * while the geometry moves.
 *
 * THREAD SAFETY: single-threaded, like the rest of Planex.
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

/* Label capacity is public since v0.7 (px_pointer_intent embeds it);
 * defined in planex.h as PX_REGION_LABEL_MAX. */

struct px_region {
    px_rect rect;
    char    label[PX_REGION_LABEL_MAX];
};

/* Process-global registry (head = most recently declared = topmost). */
typedef struct px_region_node {
    px_region*               r;
    struct px_region_node*   next;
} px_region_node;

static px_region_node* g_regions = NULL;
static int             g_region_count = 0;

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_region* px_region_new(px_rect r, const char* label) {
    px_region* reg = (px_region*)calloc(1, sizeof(px_region));
    if (!reg) return NULL;
    reg->rect = r;
    if (label) {
        strncpy(reg->label, label, PX_REGION_LABEL_MAX - 1);
        reg->label[PX_REGION_LABEL_MAX - 1] = 0;
    }

    px_region_node* node = (px_region_node*)calloc(1, sizeof(px_region_node));
    if (!node) {
        free(reg);
        return NULL;
    }
    node->r = reg;
    node->next = g_regions;
    g_regions = node;
    g_region_count++;
    return reg;
}

void px_region_free(px_region* r) {
    if (!r) return;
    px_region_node** pp = &g_regions;
    while (*pp) {
        if ((*pp)->r == r) {
            px_region_node* to_free = *pp;
            *pp = to_free->next;
            free(to_free);
            g_region_count--;
            break;
        }
        pp = &((*pp)->next);
    }
    free(r);
}

px_rect px_region_rect(const px_region* r) {
    px_rect empty = { 0, 0, 0, 0 };
    return r ? r->rect : empty;
}

const char* px_region_label(const px_region* r) {
    return r ? r->label : NULL;
}

void px_region_set_rect(px_region* r, px_rect rect) {
    if (r) r->rect = rect;
}

/* ============================================================
 * Containment queries
 * ============================================================ */

static bool region_contains(const px_region* r, double x, double y) {
    return x >= r->rect.x && x < r->rect.x + r->rect.w &&
           y >= r->rect.y && y < r->rect.y + r->rect.h;
}

px_region* px_region_at(double x, double y) {
    /* Most-recently-declared first = topmost wins. */
    for (px_region_node* node = g_regions; node; node = node->next) {
        if (region_contains(node->r, x, y)) return node->r;
    }
    return NULL;
}

px_closure* px_afford_at(px_graph* g, double x, double y) {
    if (!g) return NULL;

    px_region* top = px_region_at(x, y);
    if (!top) return NULL;

    /* The affordance query: which closures does this region afford?
     * px_query matches both edge directions, so a (region AFFORDS
     * closure) declaration is found from either endpoint. */
    px_node_list list = px_query(g, top, PX_REL_AFFORDS);
    px_closure* result = NULL;
    if (list.count > 0 && list.items) {
        /* Prefer targets that are closures; a reversed declaration
         * (closure AFFORDS region) surfaces the region itself, which
         * callers can filter out by convention. First closure wins. */
        for (int i = 0; i < list.count; i++) {
            if (list.items[i]) {
                result = (px_closure*)list.items[i];
                break;
            }
        }
    }
    px_node_list_free(&list);
    return result;
}

/* v0.7 Line 1: the routing-side compile step. Same resolution as
 * px_afford_at, but produces the full semantic payload the app
 * loop triggers the closure with. The label is EMBEDDED (not
 * pointed to) so the resulting intent is a value: it survives
 * px_closure_last_intent capture and px_closure_replay even if
 * the region is freed in between. Out is zeroed on miss so a
 * stale payload can never leak through the fallback path. */
px_closure* px_afford_compile(px_graph* g, double x, double y,
                              int button, px_pointer_intent* out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!g || !out) return NULL;

    px_closure* c = px_afford_at(g, x, y);
    if (!c) return NULL;

    px_region* top = px_region_at(x, y);
    if (!top) return NULL; /* unreachable given afford_at's contract;
                            * kept defensive: registry could change
                            * between the two scans in a hostile caller */

    out->x = x;
    out->y = y;
    out->button = button;
    strncpy(out->region, px_region_label(top), PX_REGION_LABEL_MAX - 1);
    out->region[PX_REGION_LABEL_MAX - 1] = 0;
    return c;
}
