/* v4/src/relation.c — essence #6: Relational ontology (3-place)
 *
 * UI is a network of relations, not a tree of components. Relations
 * are first-class: queryable, constrainable, subscribable.
 *
 * Per Heidegger / Simmel: relation is primitive; things are stable
 * configurations of relations. Per Suchman / situatedness: relations
 * are SITUATED — they hold for an actor, not universally.
 *
 * v4 BREAK: the CANONICAL constructor is 3-place (with actor).
 * There is no 2-place wrapper macro. If you want the universal
 * relation, pass actor=NULL.
 *
 * Storage: simple array of relation triples (a, kind, b, actor).
 * Capacity auto-grows. Query is O(N) per call — fine for v4
 * verification scale. A real impl would index by (node, kind)
 * and by (node, kind, actor).
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_relation {
    void*        a;
    px_rel_kind  kind;
    void*        b;
    px_actor*    actor;   /* NULL = universal */
};

struct px_graph {
    px_relation* relations;
    int          count;
    int          capacity;
};

px_graph* px_graph_new(void) {
    px_graph* g = (px_graph*)calloc(1, sizeof(px_graph));
    if (!g) return NULL;
    g->capacity = 16;
    g->relations = (px_relation*)malloc(sizeof(px_relation) * g->capacity);
    if (!g->relations) { free(g); return NULL; }
    g->count = 0;
    return g;
}

void px_graph_free(px_graph* g) {
    if (!g) return;
    free(g->relations);
    free(g);
}

static int grow_graph(px_graph* g) {
    int new_cap = g->capacity * 2;
    px_relation* new_rel = (px_relation*)realloc(g->relations,
                                                  sizeof(px_relation) * new_cap);
    if (!new_rel) return -1;
    g->relations = new_rel;
    g->capacity = new_cap;
    return 0;
}

px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind,
                          void* b, px_actor* actor) {
    if (!g || !a || !b) return NULL;
    if (kind < 0 || kind >= PX_REL_COUNT) return NULL;
    if (g->count >= g->capacity) {
        if (grow_graph(g) != 0) return NULL;
    }
    px_relation* r = &g->relations[g->count++];
    r->a = a;
    r->kind = kind;
    r->b = b;
    r->actor = actor;
    return r;
}

/* A relation R matches query (a, kind, b, actor_q) iff:
 *   - R.a == a, R.kind == kind, R.b == b
 *   - R.actor == NULL (universal) OR R.actor == actor_q
 */
static bool relation_matches(const px_relation* r,
                              void* a, px_rel_kind kind, void* b,
                              px_actor* actor_q) {
    if (r->a != a) return false;
    if (r->kind != kind) return false;
    if (r->b != b) return false;
    if (r->actor != NULL && r->actor != actor_q) return false;
    return true;
}

bool px_has_relation(px_graph* g, void* a, px_rel_kind kind,
                      void* b, px_actor* actor) {
    if (!g) return false;
    for (int i = 0; i < g->count; i++) {
        if (relation_matches(&g->relations[i], a, kind, b, actor)) {
            return true;
        }
    }
    return false;
}

px_node_list px_query(px_graph* g, void* node, px_rel_kind kind,
                        px_actor* actor) {
    px_node_list result = { NULL, 0 };
    if (!g) return result;

    /* First pass: count matches. */
    int n = 0;
    for (int i = 0; i < g->count; i++) {
        const px_relation* r = &g->relations[i];
        /* match where node is on either side; report the OTHER node */
        if (r->kind != kind) continue;
        if (r->actor != NULL && r->actor != actor) continue;
        if (r->a == node || r->b == node) {
            n++;
        }
    }
    if (n == 0) return result;

    result.items = (void**)calloc(n, sizeof(void*));
    if (!result.items) { result.count = 0; return result; }
    int idx = 0;
    for (int i = 0; i < g->count && idx < n; i++) {
        const px_relation* r = &g->relations[i];
        if (r->kind != kind) continue;
        if (r->actor != NULL && r->actor != actor) continue;
        if (r->a == node) {
            result.items[idx++] = r->b;
        } else if (r->b == node) {
            result.items[idx++] = r->a;
        }
    }
    result.count = idx;
    return result;
}

void px_node_list_free(px_node_list* list) {
    if (!list) return;
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

int px_graph_count(const px_graph* g) {
    return g ? g->count : 0;
}

const char* px_rel_kind_str(px_rel_kind k) {
    switch (k) {
        case PX_REL_BESIDE:        return "BESIDE";
        case PX_REL_DEPENDS_ON:    return "DEPENDS_ON";
        case PX_REL_TRIGGERS:      return "TRIGGERS";
        case PX_REL_VARIES_WITH:   return "VARIES_WITH";
        case PX_REL_AFFORDS:      return "AFFORDS";
        case PX_REL_CONTAINS:      return "CONTAINS";
        case PX_REL_WITHDRAWS_FOR: return "WITHDRAWS_FOR";
        case PX_REL_PRESENTS_FOR:  return "PRESENTS_FOR";
        case PX_REL_INTERPRETS_AS:  return "INTERPRETS_AS";
        default:                   return "(unknown)";
    }
}
