/*
 * if5_projection.c — Perception as the projection family (ADR-0027
 * Decision 2d, the mechanism layer's fourth mapping).
 *
 * The shipping Perception (src/perception.c) is a pure function
 * from estimates to a denotation, kept in a PROCESS-GLOBAL linked
 * list with discovery and invoke-all. This module re-derives the
 * concept as a WORLD-scoped registry — the world is the unit, and
 * two worlds may project differently (the process-global list was
 * one of the three global registries the shipping tree carried;
 * the mechanism layer carries none).
 *
 * The three frozen R1 projections (semantic/set/history) are
 * registered through this SAME channel at world creation and are
 * WRAPPED, not modified: if5_projection_invoke("semantic", ...)
 * calls the very function if5_project_semantic IS. M4 pins this
 * byte-for-byte — the registry path is the direct path.
 *
 * The extension projections (the tree and the schedule, text-
 * rendered) register through the same public channel applications
 * use — B2's equality-of-standing rule, applied a second time
 * (the payload types were the first). NO screen raster exists
 * here or anywhere in v5 (S15: vision is ONE projection — a
 * CLIENT of the tree; the rebound gate bans the visual-field
 * vocabulary, comments included, which is exactly why this
 * sentence words itself the way it does).
 *
 * Lineage: R2; no slice ancestor.
 */
#include "if5_internal.h"

#include <string.h>

/* --- the two text renderings (extension projections) ------------ */

/* The tree, as text: one line per node, depth-indented, in the
 * walk's registered order (declaration-order DFS over folded
 * containment). The SAME walk the S15 seat runs — rendering rides
 * the structure, it never has a shape of its own. */
typedef struct {
    char*    buf;
    uint32_t cap, len;
    bool     trunc;
} if5__tree_ctx;

static bool if5__tree_line(const if5_node* n, void* user)
{
    if5__tree_ctx* c = (if5__tree_ctx*)user;
    uint32_t i;
    for (i = 0; i < n->depth; i++) {
        c->len += if5__n(c->buf + c->len, c->cap - c->len, "  ");
    }
    c->len += if5__n(c->buf + c->len, c->cap - c->len, "%s=%llu\n",
                     n->name, (unsigned long long)n->value);
    return true;
}

static uint32_t if5__project_tree(if5_world* w, const if5_view* v,
                                  char* buf, uint32_t cap)
{
    if5__tree_ctx c;
    if5_view def;
    if (!w) return 0;
    if (!v) { def = if5_now(w); v = &def; }
    c.buf = buf; c.cap = cap; c.len = 0; c.trunc = false;
    if (buf && cap) buf[0] = '\0';
    (void)if5_tree_walk(w, v, if5__tree_line, &c);
    return c.len;
}

/* The schedule, as text: one line per entry, PROV-anchored. The
 * same pure query the S17 seat runs (if5_schedule); rendering adds
 * only the typography. */
static uint32_t if5__project_schedule(if5_world* w, const if5_view* v,
                                      char* buf, uint32_t cap)
{
    if5_view def;
    if5_sched_entry entries[256];
    uint32_t n, i, len = 0;

    if (!w) return 0;
    if (!v) { def = if5_now(w); v = &def; }
    if (buf && cap) buf[0] = '\0';

    n = if5_schedule(w, v, IF5_SID_NULL, entries, 256);
    for (i = 0; i < n; i++) {
        len += if5__n(buf ? buf + len : NULL, buf ? cap - len : 0,
                      "iid=%u kind=%u mag=%u src=%u\n",
                      entries[i].iid, entries[i].kind,
                      entries[i].magnitude, entries[i].sid);
    }
    /* Truncation is reported the registry's way: the true length,
     * capped output. The caller compares against the cap. */
    return len;
}

/* --- the registry ------------------------------------------------ */

uint8_t if5_projection_register(if5_world* w, const char* name,
                                if5_projection_fn fn)
{
    size_t nl;
    uint32_t i;

    if (!w || !name || !fn) return (uint8_t)-1;
    if (w->n_projections >= IF5_MAX_PROJECTIONS) return (uint8_t)-1;
    nl = strlen(name);
    if (nl == 0 || nl >= IF5_PROJ_NAME_MAX) return (uint8_t)-1;

    /* Names are unique — a reregistration of a live name is a
     * caller error, reported as refusal (the registry never
     * silently rebinds a name: a projection that answers to a name
     * the application already relied on is not replaceable without
     * the application seeing it). */
    for (i = 0; i < w->n_projections; i++) {
        if (strcmp(w->projections[i].name, name) == 0)
            return (uint8_t)-1;
    }

    memcpy(w->projections[w->n_projections].name, name, nl);
    w->projections[w->n_projections].name[nl] = '\0';
    w->projections[w->n_projections].fn = fn;
    return (uint8_t)w->n_projections++;
}

uint32_t if5_projection_count(if5_world* w)
{
    return w ? w->n_projections : 0;
}

const char* if5_projection_name(if5_world* w, uint32_t index)
{
    if (!w || index >= w->n_projections) return NULL;
    return w->projections[index].name;
}

uint32_t if5_projection_invoke(if5_world* w, const if5_view* v,
                               const char* name, char* buf, uint32_t cap)
{
    uint32_t i;
    if (!w || !name) return 0;
    for (i = 0; i < w->n_projections; i++) {
        if (strcmp(w->projections[i].name, name) == 0)
            return w->projections[i].fn(w, v, buf, cap);
    }
    return 0;
}

/* --- the built-ins, through the public channel -------------------- */

/* Thin adapters so the frozen trio registers with the family's
 * signature. These ARE the direct calls — M4's byte-equality
 * assertion is about the routing, and the routing is all that is
 * here. */
static uint32_t if5__proj_semantic(if5_world* w, const if5_view* v,
                                   char* buf, uint32_t cap)
{
    return if5_project_semantic(w, v, buf, cap);
}

static uint32_t if5__proj_set(if5_world* w, const if5_view* v,
                              char* buf, uint32_t cap)
{
    return if5_project_set(w, v, buf, cap);
}

static uint32_t if5__proj_history(if5_world* w, const if5_view* v,
                                  char* buf, uint32_t cap)
{
    return if5_project_history(w, v, buf, cap);
}

void if5__register_builtin_projections(if5_world* w)
{
    /* Order is registration order and is stable: the frozen trio
     * first (semantic, set, history — the R1 differential's exact
     * naming), then the seat renderings (tree, schedule). The ids
     * this fixes are observable through count/name; changing the
     * order would be a silent API change, so it does not change. */
    (void)if5_projection_register(w, "semantic", if5__proj_semantic);
    (void)if5_projection_register(w, "set", if5__proj_set);
    (void)if5_projection_register(w, "history", if5__proj_history);
    (void)if5_projection_register(w, "tree", if5__project_tree);
    (void)if5_projection_register(w, "schedule", if5__project_schedule);
}
