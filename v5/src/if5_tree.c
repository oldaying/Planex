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
 * (the FOLDED parent, since ADR-0027 Decision 3 — see
 * if5_parent_at below), children in declaration order, roots in
 * declaration order. Deterministic and total — every region
 * appears exactly once at every view (V4's test surface). The
 * containment forest is well-founded by construction: creation
 * validates parent < the new region's id, and every reparent
 * validates the same invariant, so no cycle can form.
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
 * Containment as content (ADR-0027 Decision 3 —
 * ADR-0026 Known issue 1 discharged)
 *
 * The tree's containment FOLDS: the parent at a view is the last
 * live reparent record for the region, else the creation seed
 * (the single-create REGION record's aux word; batch-created
 * regions seed at root — the frozen blob layout's meaning). The
 * working-set parent field is a CACHE of the live fold, the same
 * way region.cache caches the folded value: reparent application
 * updates it (if5_core.c's structural switch), undo does NOT
 * invalidate it — the fold is the truth, the cache is for "now"
 * in the no-undo common case.
 *
 * M8's regression duty: in a world with NO reparent records the
 * folded parent IS the seed, which IS the working-set parent (it
 * was never written) — the walk is identical in shape to R1's,
 * and the test suite checks it against an independent in-test
 * reimplementation of the R1 order.
 * ============================================================ */

uint32_t if5_reparent(if5_world* w, if5_sid src, uint16_t child,
                      uint16_t new_parent)
{
    if5_region* c;
    if5_submit_opts o;

    if (!w || src >= w->n_sources) return 0;
    c = if5__region(w, child);
    if (!c) return 0;
    /* The destination must exist and must not create a cycle:
     * parent < child is the creation invariant, and it is kept
     * here too (a containment forest stays well-founded exactly
     * because the invariant is total, not just at creation). */
    if (new_parent != 0) {
        if5_region* p = if5__region(w, new_parent);
        if (!p || new_parent >= child) return 0;
    }

    /* An ordinary submission: region = the moved child, value =
     * the destination. The record is structural (excluded from
     * the value fold), applied unconditionally, and cached into
     * the working set by the core's own switch. */
    memset(&o, 0, sizeof o);
    o.observed_cut = w->n_log;
    return if5_submit(w, src, child, IF5_T_REPARENT,
                      (uint64_t)new_parent, &o);
}

uint16_t if5_parent_at(if5_world* w, const if5_view* v, uint16_t rid)
{
    if5_view def;
    uint16_t seed;
    uint16_t parent = 0;
    bool found = false;
    uint32_t i;

    if (!w) return 0;
    if (!if5__region(w, rid)) return 0;
    if (!v) { def = if5_now(w); v = &def; }

    seed = if5__seed_parent(w, rid);

    /* The fold: the LAST live reparent for this region at the
     * view, else the seed. Live = applied, not view-excluded —
     * the ordinary record filters, the same ones every other fold
     * applies (an undone structural move restores the prior tree
     * through exactly the mechanism every other undo uses). */
    for (i = 0; i < v->cut && i < w->n_log; i++) {
        const if5_intent* rec = &w->log[i];
        if (rec->region != rid) continue;
        if (rec->ptype != IF5_T_REPARENT) continue;
        if (!(rec->flags & IF5_F_APPLIED)) continue;
        if (rec->flags & IF5_F_EXCLUDED) continue;
        if (if5__is_excluded(w, rec, v)) continue;
        parent = (uint16_t)(((uint64_t)rec->value[1] << 32
                            | rec->value[0]) & 0xFFFFu);
        found = true;
    }

    return found ? parent : seed;
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
 *
 * Since ADR-0027 Decision 3 the walk's containment is the FOLDED
 * parent (if5_parent_at), not the working-set cache — the tree at
 * a VIEW is the tree the view's history yields, which is what
 * makes an undone reparent restore the prior tree. In worlds
 * without reparent records the two coincide (M8's regression
 * duty), and the walk pays one fold scan per node for it — the
 * registered cost, kept honest here rather than hidden behind a
 * cache the undo path would have to invalidate.
 */
uint32_t if5_tree_walk(if5_world* w, const if5_view* v,
                       if5_tree_cb cb, void* user)
{
    if5_view def;
    uint32_t stack[IF5_MAX_REGIONS];
    uint16_t fparent[IF5_MAX_REGIONS];   /* the folded parent, once
                                          * per node (the walk's
                                          * own memo — one fold per
                                          * REGION, not per edge) */
    int32_t top = 0;
    uint32_t visited = 0;
    uint16_t i;

    if (!w) return 0;
    if (!cb) cb = if5__count_only;      /* count-only mode: the
                                         * totality check's cheapest
                                         * form (V4) */
    if (!v) { def = if5_now(w); v = &def; }

    for (i = 0; i < w->n_regions && i < IF5_MAX_REGIONS; i++)
        fparent[i] = if5_parent_at(w, v, (uint16_t)(i + 1));

    /* Roots (folded parent 0) in declaration order: reverse push. */
    for (i = w->n_regions; i >= 1; i--) {
        if (fparent[i - 1] == 0)
            stack[top++] = (uint32_t)i;
        if (i == 1) break;               /* unsigned wrap guard */
    }

    while (top > 0) {
        uint16_t rid = (uint16_t)stack[--top];
        const if5_region* r = if5__region(w, rid);
        if5_node node;
        uint16_t depth = 0;

        if (!r) continue;

        /* Depth: walk the FOLDED parent chain. Well-founded (a
         * reparent's destination is validated parent < child), so
         * this terminates. */
        {
            uint16_t p = fparent[rid - 1];
            while (p != 0) {
                depth++;
                p = (p <= w->n_regions && p <= IF5_MAX_REGIONS)
                    ? fparent[p - 1] : 0;
            }
        }

        memset(&node, 0, sizeof node);
        memcpy(node.name, r->name, IF5_NAME_MAX);
        node.name[IF5_NAME_MAX - 1] = '\0';
        node.value  = if5_state_at(w, rid, v, &node.contrib);
        node.last_iid = if5__last_contributor(w, rid, v);
        node.rid    = rid;
        node.parent = fparent[rid - 1];
        node.depth  = depth;
        node.ptype  = r->ptype;
        node.tier   = r->tier;

        visited++;
        if (!cb(&node, user)) break;

        /* Children (folded) in declaration order: reverse push. */
        for (i = w->n_regions; i >= 1; i--) {
            if (fparent[i - 1] == rid)
                stack[top++] = (uint32_t)i;
            if (i == 1) break;           /* unsigned wrap guard */
        }
    }
    return visited;
}
