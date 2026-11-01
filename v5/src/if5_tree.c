/*
 * if5_tree.c — S15 seated: the semantic tree, first class
 * (ADR-0026 Decision 3c).
 *
 * Vol. 4 seed S15's statement is this module's contract: the
 * accessibility tree is the ontology — semantic structure is the
 * work, vision is one projection, screen-reader users are
 * first-class testers. Vol. 7 ch. 3 seated S15 (+ S17, in
 * if5_schedule.c) as the replacement design object for the dead
 * reproducibility claim, with the refutation condition this module
 * must survive: implementable WITHOUT borrowing reproduction
 * primacy. Operationally: the tree carries the interface's semantic
 * content — name, value, role, tier, contributions, PROV anchor —
 * and NO visual field exists anywhere in the v5 core (the rebound
 * gate's v5 section bans the vocabulary; a semantic tree that
 * needed a rectangle to exist would be the reproduction primacy
 * the court killed, sneaking back in as a field name).
 *
 * The walk: depth-first pre-order over region containment
 * (the S15 parent), children in declaration order, roots in
 * declaration order. Deterministic and total — every region
 * appears exactly once at every view (V4's test surface). The
 * containment forest is well-founded by construction: creation
 * validates parent < the new region's id, so no cycle can form.
 */
#include "if5_internal.h"

#include <string.h>

/* ============================================================
 * The node's PROV anchor
 * ============================================================ */

/*
 * The newest contributor at THIS view — the record whose value the
 * fold ended on, not merely any record on the region. The filter
 * is the fold's own (region, applied, not adjudication-excluded,
 * value-type, not view-revoked, on the selected chain when the
 * region is forked) so the anchor can never name a record the fold
 * did not read. 0 = the region has no contributor at this view.
 */
static uint32_t if5__last_contributor(if5_world* w, uint16_t rid,
                                      const if5_view* v)
{
    const if5_region* r = if5__region(w, rid);
    const if5_payload_type* ty;
    bool lattice, forked;
    uint32_t head = 0, i;

    if (!r) return 0;
    ty = if5__type(w, r->ptype);
    lattice = ty && ty->sq;
    forked = r->head_b != 0 && !lattice;
    if (forked) head = if5__selected_head(w, r, v);

    for (i = v->cut < w->n_log ? v->cut : w->n_log; i > 0; i--) {
        const if5_intent* rec = &w->log[i - 1];
        if (rec->region != rid) continue;
        if (!(rec->flags & IF5_F_APPLIED)) continue;
        if (rec->flags & IF5_F_EXCLUDED) continue;
        if (!if5__is_value_type(rec->ptype)) continue;
        if (if5__is_excluded(w, rec, v)) continue;
        if (forked && !if5__on_selected_chain(w, head, rec)) continue;
        return i;
    }
    return 0;
}

/* ============================================================
 * The walk
 * ============================================================ */

/* The count-only callback: never stops the walk. */
static bool if5__count_only(const if5_node* n, void* user)
{
    (void)n; (void)user;
    return true;
}

/*
 * Iterative depth-first pre-order. The explicit stack avoids
 * unbounded recursion on deep chains; pushing children in REVERSE
 * declaration order makes the first-declared child pop first, which
 * is the registered order. Stack capacity: every region can be on
 * the stack at most once (a region is pushed only when first
 * reached, and the forest is acyclic), so IF5_MAX_REGIONS bounds it.
 */
uint32_t if5_tree_walk(if5_world* w, const if5_view* v,
                       if5_tree_cb cb, void* user)
{
    if5_view def;
    uint32_t stack[IF5_MAX_REGIONS];
    int32_t top = 0;
    uint32_t visited = 0;
    uint16_t i;

    if (!w) return 0;
    if (!cb) cb = if5__count_only;      /* count-only mode: the
                                         * totality check's cheapest
                                         * form (V4) */
    if (!v) { def = if5_now(w); v = &def; }

    /* Roots (parent 0) in declaration order: push in reverse. */
    for (i = w->n_regions; i >= 1; i--) {
        if (w->regions[i - 1].parent == 0)
            stack[top++] = (uint32_t)i;
        if (i == 1) break;               /* unsigned wrap guard */
    }

    while (top > 0) {
        uint16_t rid = (uint16_t)stack[--top];
        const if5_region* r = if5__region(w, rid);
        if5_node node;
        uint16_t depth = 0;

        if (!r) continue;

        /* Depth: walk the parent chain. Well-founded (parent <
         * rid by construction), so this terminates. */
        {
            uint16_t p = r->parent;
            while (p != 0) {
                depth++;
                p = w->regions[p - 1].parent;
            }
        }

        memset(&node, 0, sizeof node);
        memcpy(node.name, r->name, IF5_NAME_MAX);
        node.name[IF5_NAME_MAX - 1] = '\0';
        node.value  = if5_state_at(w, rid, v, &node.contrib);
        node.last_iid = if5__last_contributor(w, rid, v);
        node.rid    = rid;
        node.parent = r->parent;
        node.depth  = depth;
        node.ptype  = r->ptype;
        node.tier   = r->tier;

        visited++;
        if (!cb(&node, user)) break;

        /* Children in declaration order: push in reverse. */
        for (i = w->n_regions; i >= 1; i--) {
            if (w->regions[i - 1].parent == rid)
                stack[top++] = (uint32_t)i;
            if (i == 1) break;           /* unsigned wrap guard */
        }
    }
    return visited;
}
