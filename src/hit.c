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
     * closure) declaration is found from either endpoint.
     *
     * v0.8 (Line 2): targets are FILTERED BY KIND. AFFORDS edges
     * may target closures (discrete acts) or interactions (drag
     * processes) — the closure form resolves only closure targets,
     * by px_is_closure identity. This also makes the code match
     * what its comment always claimed ("first closure wins"): the
     * pre-v0.8 code cast the first non-NULL target blindly, a
     * latent type confusion the process form would have tripped
     * over. Non-closure, non-interaction targets (an estimate on
     * an odd declaration) resolve nothing — safer than v0.8 too. */
    px_node_list list = px_query(g, top, PX_REL_AFFORDS);
    px_closure* result = NULL;
    for (int i = 0; i < list.count; i++) {
        if (list.items[i] && px_is_closure(list.items[i])) {
            /* px_query returns edges newest-first (px_declare
             * prepends), so the first closure target found is the
             * LAST declared — the pinned last-declared-first rule. */
            result = (px_closure*)list.items[i];
            break;
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

/* ============================================================
 * v0.8 (Line 1) — the keyboard channel: focus ring + key compile
 *
 * The ring is DERIVED from what the app already declared: a
 * region is focusable iff it affords at least one closure. The
 * registry is a stack (head = newest), so creation order is the
 * stack read backward — the same list the z-order scan reads
 * forward. One registry, two honest projections: z-order (newest
 * first, for pointer containment) and focus order (oldest first,
 * for traversal). No layout-derived focus order: layout is
 * Perception's business, not Relation's.
 * ============================================================ */

/* Does this region afford at least one closure in `g`?
 * v0.8 (Line 2): KIND-FILTERED — a region affording only a process
 * (the slider) is NOT focusable: Enter/Space compile closures, and
 * keyboard process-activation does not exist yet (recorded in
 * ADR-0021 CAVEATS). The Line 1 ring semantics are pinned by
 * tests/test_v08.c a4 and unchanged by the process form. */
static bool region_is_focusable(px_graph* g, px_region* r) {
    if (!g || !r) return false;
    px_node_list list = px_query(g, r, PX_REL_AFFORDS);
    bool focusable = false;
    for (int i = 0; i < list.count; i++) {
        if (list.items[i] && px_is_closure(list.items[i])) {
            focusable = true;
            break;
        }
    }
    px_node_list_free(&list);
    return focusable;
}

/* Walk the ring in creation order. `from` NULL = start at head.
 * Returns the focusable region at ring offset `offset` counting
 * from `from` (may wrap); NULL if the ring is empty. offset is
 * signed: +1 = next, -1 = prev, 0 = `from`'s normalized position
 * (head when `from` is nowhere). */
static px_region* focus_ring_walk(px_graph* g, px_region* from, int offset) {
    if (!g) return NULL;

    /* Collect focusable regions; the registry walk yields
     * newest-first, so reverse in place to get CREATION order
     * (oldest first) — the ring's order. */
    int n = g_region_count;
    if (n <= 0) return NULL;
    px_region** ring = (px_region**)calloc((size_t)n, sizeof(px_region*));
    if (!ring) return NULL;

    int count = 0;
    for (px_region_node* node = g_regions; node; node = node->next) {
        if (region_is_focusable(g, node->r)) ring[count++] = node->r;
    }
    if (count == 0) { free(ring); return NULL; }
    for (int i = 0; i < count / 2; i++) {
        px_region* tmp   = ring[i];
        ring[i]          = ring[count - 1 - i];
        ring[count - 1 - i] = tmp;
    }

    /* Locate `from`. Nowhere — NULL, freed, or unfocusable —
     * resolves to the HEAD for either direction: the first Tab
     * (or Shift-Tab) from nowhere focuses the ring's first
     * region, matching px_afford_focus_first. */
    int at = -1;
    if (from) {
        for (int i = 0; i < count; i++) {
            if (ring[i] == from) { at = i; break; }
        }
    }
    if (at < 0) {
        px_region* head = ring[0];
        free(ring);
        return head;
    }

    /* Signed walk with wraparound; the % count keeps it a ring. */
    int idx = at + offset;
    idx %= count;
    if (idx < 0) idx += count;

    px_region* result = ring[idx];
    free(ring);
    return result;
}

px_region* px_afford_focus_first(px_graph* g) {
    return focus_ring_walk(g, NULL, 0);
}

px_region* px_afford_focus_next(px_graph* g, px_region* from) {
    return focus_ring_walk(g, from, +1);
}

px_region* px_afford_focus_prev(px_graph* g, px_region* from) {
    return focus_ring_walk(g, from, -1);
}

/* The keyboard compile: same resolution as a pointer compile —
 * the region's last-declared AFFORDS closure wins — but the
 * routing key is the FOCUS, not a position. The payload is a
 * value by the same construction as px_pointer_intent: the label
 * is embedded, so the intent survives capture and replay after
 * the region is freed. Miss zeroes the payload so a stale value
 * can never leak through the raw-key fallback. */
px_closure* px_afford_compile_focus(px_graph* g, px_region* focused,
                                    int key, px_key_intent* out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!g || !out) return NULL;
    if (!focused || !region_is_focusable(g, focused)) return NULL;

    px_node_list list = px_query(g, focused, PX_REL_AFFORDS);
    px_closure* result = NULL;
    /* px_query returns edges newest-first (px_declare prepends),
     * so the FIRST closure target is the LAST declared — the same
     * last-declared-first rule px_afford_at applies (v0.8 Line 2:
     * kind-filtered; process targets never resolve as closures). */
    for (int i = 0; i < list.count; i++) {
        if (list.items[i] && px_is_closure(list.items[i])) {
            result = (px_closure*)list.items[i];
            break;
        }
    }
    px_node_list_free(&list);
    if (!result) return NULL;

    out->key = key;
    strncpy(out->region, px_region_label(focused), PX_REGION_LABEL_MAX - 1);
    out->region[PX_REGION_LABEL_MAX - 1] = 0;
    return result;
}

/* ============================================================
 * v0.8 (Line 2) — the process form: drag-begin afford
 *
 * The SAME relation, the SECOND resolution form: an AFFORDS edge
 * whose target is a px_interaction resolves a pointer-down to a
 * PROCESS (the inert-trajectory machine), not to a one-shot
 * closure. Drag-ability becomes graph data — the L15b retire.
 *
 * The dual-form rule: when a region affords BOTH closures and a
 * process, the PROCESS owns the down. The press is genuinely
 * ambiguous (tap vs drag); only the trajectory resolves it — the
 * tap is a small-displacement COMMIT, and the process's own
 * bridges reach the discrete act. Regions affording only closures
 * keep the v0.7 immediate-trigger semantics, byte-for-byte.
 * ============================================================ */

bool px_region_affords_process(px_graph* g, const px_region* r) {
    if (!g || !r) return false;
    px_node_list list = px_query(g, (void*)r, PX_REL_AFFORDS);
    bool affords = false;
    for (int i = 0; i < list.count; i++) {
        if (list.items[i] && px_is_interaction(list.items[i])) {
            affords = true;
            break;
        }
    }
    px_node_list_free(&list);
    return affords;
}

px_interaction* px_afford_compile_process(px_graph* g, double x, double y,
                                          int button, px_drag_intent* out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!g || !out) return NULL;

    px_region* top = px_region_at(x, y);
    if (!top) return NULL;

    /* Same registry + graph + rule as the closure form, with the
     * kind filter flipped: the last-declared AFFORDS edge whose
     * target is a px_interaction wins. */
    px_node_list list = px_query(g, top, PX_REL_AFFORDS);
    px_interaction* result = NULL;
    for (int i = 0; i < list.count; i++) {
        if (list.items[i] && px_is_interaction(list.items[i])) {
            result = (px_interaction*)list.items[i];
            break;
        }
    }
    px_node_list_free(&list);
    if (!result) return NULL;

    /* The value contract, same construction as the pointer and key
     * intents: the label is EMBEDDED, so the compile product is a
     * value — it survives capture and replay after the region is
     * freed. The press position and button are CONTEXT (they seed
     * the first trajectory sample in px_app_run), never routing
     * keys — the routing key was the region. */
    out->x = x;
    out->y = y;
    out->button = button;
    strncpy(out->region, px_region_label(top), PX_REGION_LABEL_MAX - 1);
    out->region[PX_REGION_LABEL_MAX - 1] = 0;
    return result;
}
