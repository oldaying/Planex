/*
 * relation.c — Relation graph (basic existence)
 *
 * UI is a network of relations, not a tree of components.
 * A relation is a typed edge between two nodes (opaque pointers).
 *
 * Implementation: simple linked list. O(n) queries. Adequate for
 * Stage 0 validation; Stage 1+ will switch to a hash table for
 * O(1) lookup and add spatial indexing for layout.
 *
 * Inspired by:
 *   - Sketchpad (Sutherland 1963) ring structure
 *   - Bevy ECS entity relationships
 *   - Christopher Alexander's semilattice (vs tree)
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_relation {
    void*          a;
    px_rel_kind    kind;
    void*          b;
    px_actor*      actor;     /* v3 prototype: NULL = universal */
    px_relation*   next;
};

struct px_graph {
    px_relation* head;
    int          count;
};

static const char* const k_rel_names[] = {
    "BESIDE",         /* PX_REL_BESIDE          */
    "DEPENDS_ON",     /* PX_REL_DEPENDS_ON      */
    "TRIGGERS",       /* PX_REL_TRIGGERS         */
    "VARIES_WITH",    /* PX_REL_VARIES_WITH      */
    "AFFORDS",        /* PX_REL_AFFORDS          */
    "CONTAINS",       /* PX_REL_CONTAINS          */
    "WITHDRAWS_FOR",  /* PX_REL_WITHDRAWS_FOR   */
    "PRESENTS_FOR",   /* PX_REL_PRESENTS_FOR    */
    "INTERPRETS_AS",   /* PX_REL_INTERPRETS_AS   */
    "COUNT",          /* PX_REL_COUNT             */
};

/* ============================================================
 * Graph lifecycle
 * ============================================================ */

px_graph* px_graph_new(void) {
    px_graph* g = (px_graph*)calloc(1, sizeof(px_graph));
    return g;
}

void px_graph_free(px_graph* g) {
    if (!g) return;
    px_relation* r = g->head;
    while (r) {
        px_relation* next = r->next;
        free(r);
        r = next;
    }
    free(g);
}

int px_graph_count(const px_graph* g) {
    return g ? g->count : 0;
}

/* ============================================================
 * Declare / query
 * ============================================================ */

px_relation* px_declare_for(px_graph* g, void* a, px_rel_kind kind,
                            void* b, px_actor* actor) {
    if (!g || !a || !b) return NULL;
    if (kind < 0 || kind >= PX_REL_COUNT) return NULL;

    px_relation* r = (px_relation*)malloc(sizeof(px_relation));
    if (!r) return NULL;
    r->a      = a;
    r->kind   = kind;
    r->b      = b;
    r->actor  = actor;
    r->next   = g->head;
    g->head   = r;
    g->count++;
    return r;
}

/* Backward-compat wrapper: old 2-place px_declare is equivalent to
 * px_declare_for with actor=NULL (relation holds universally). */
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b) {
    return px_declare_for(g, a, kind, b, NULL);
}

bool px_has_relation(px_graph* g, void* a, px_rel_kind kind, void* b) {
    if (!g || !a || !b) return false;
    for (px_relation* r = g->head; r; r = r->next) {
        if (r->a == a && r->kind == kind && r->b == b) return true;
    }
    return false;
}

px_node_list px_query(px_graph* g, void* node, px_rel_kind kind) {
    /* Backward-compat wrapper: old px_query returns nodes for any actor
     * (i.e., actor=NULL in the 3-place model). */
    return px_query_for(g, node, kind, NULL);
}

/* v3 prototype: 3-place query.
 * Matches relations where:
 *   - r->kind == kind
 *   - (r->a == node OR r->b == node)
 *   - (r->actor == actor OR r->actor == NULL)
 * The NULL-actor case lets "universal" relations match every actor query,
 * preserving the old px_query semantics.
 */
px_node_list px_query_for(px_graph* g, void* node, px_rel_kind kind,
                          px_actor* actor) {
    px_node_list list = { NULL, 0 };
    if (!g || !node) return list;

    /* Two passes: count, then fill. */
    int cap = 0;
    for (px_relation* r = g->head; r; r = r->next) {
        if (r->kind != kind) continue;
        if (r->actor != NULL && r->actor != actor) continue;
        if (r->a == node || r->b == node) cap++;
    }
    if (cap == 0) return list;

    list.items = (void**)malloc((size_t)cap * sizeof(void*));
    if (!list.items) return list;

    int i = 0;
    for (px_relation* r = g->head; r; r = r->next) {
        if (r->kind != kind) continue;
        if (r->actor != NULL && r->actor != actor) continue;
        if (r->a == node) {
            list.items[i++] = r->b;
        } else if (r->b == node) {
            list.items[i++] = r->a;
        }
    }
    list.count = i;
    return list;
}

void px_node_list_free(px_node_list* list) {
    if (!list) return;
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

/* ============================================================
 * Debug
 * ============================================================ */

const char* px_rel_kind_str(px_rel_kind k) {
    if (k < 0 || k >= PX_REL_COUNT) return "?";
    return k_rel_names[k];
}
