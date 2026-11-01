/*
 * if5_cause.c — Relation as the cause-graph over the log (ADR-0027
 * Decision 2b, the mechanism layer's second mapping).
 *
 * The shipping Relation (src/relation.c) is an ASSERTED typed edge
 * between opaque pointers, kept in a linked list; nothing ties an
 * edge to evidence. This module re-derives the relation concept as
 * READS of the log's own causality, with the mapping table fixed by
 * the ADR (it is contract, not convenience):
 *
 *   shipping kind    v5 construction                    evidence
 *   --------------   --------------------------------   --------
 *   DEPENDS_ON       if5_cause_edges (PROV edges)      prov_a/prov_b
 *   TRIGGERS         if5_cause_effects / _reachable    vc partial order
 *   VARIES_WITH      if5_cause_concurrents              vc incomparability
 *   CONTAINS         if5_parent_at (the folded tree)    reparent records
 *   BESIDE           NOT CARRIED — layout assertion, no causal content
 *   AFFORDS          NOT CARRIED — presentation vocabulary (S15 ban)
 *   WITHDRAWS_FOR    NOT CARRIED — presentation
 *   PRESENTS_FOR     NOT CARRIED — presentation
 *   INTERPRETS_AS    NOT CARRIED — interpretation is the projection
 *                                          family's job (2d), not an edge
 *
 * Two evidence grades exist and are NOT conflated: the PROV graph
 * is EXPLICIT structure (a merge's two parents, an undo cursor's
 * revoked record — sparse, per-record); the vc partial order is
 * IMPLICIT knowledge dependency (dense, transitive, O(1) to test).
 * if5_cause_reachable answers the knowledge question; the edge
 * queries answer the structure question. M2 checks both against
 * independent recomputations.
 *
 * Purity: a pure query module (S24). Lineage: R2 of the v5
 * derivation; no slice ancestor.
 */
#include "if5_internal.h"

#include <string.h>

uint32_t if5_cause_edges(if5_world* w, uint32_t iid, if5_edge* out,
                         uint32_t cap)
{
    const if5_intent* rec;
    uint32_t n = 0;

    if (!w || iid == 0 || iid > w->n_log) return 0;
    rec = &w->log[iid - 1];

    /* The PROV parents, oldest-first: prov_a is the first parent
     * (the adjudication's causal predecessor; for a CURSOR record
     * it is the record being moved past), prov_b exists only on
     * merge witnesses (the second parent of the stitched pair). */
    if (rec->prov_a) {
        if (out && n < cap) {
            out[n].from_iid = rec->prov_a;
            out[n].to_iid   = iid;
        }
        n++;
    }
    if (rec->prov_b) {
        if (out && n < cap) {
            out[n].from_iid = rec->prov_b;
            out[n].to_iid   = iid;
        }
        n++;
    }
    return n;
}

uint32_t if5_cause_effects(if5_world* w, uint32_t iid, if5_edge* out,
                           uint32_t cap)
{
    uint32_t n = 0, i;

    if (!w || iid == 0 || iid > w->n_log) return 0;

    /* The exact reversal of if5_cause_edges: every record whose
     * prov_a or prov_b IS `iid`. A full-log scan is the honest
     * implementation (the log IS the structure; there is no side
     * index to consult — an index would be working-set state the
     * differential would have to carry, for a query this cheap). */
    for (i = 1; i <= w->n_log; i++) {
        const if5_intent* rec = &w->log[i - 1];
        if (rec->prov_a == iid) {
            if (out && n < cap) {
                out[n].from_iid = iid;
                out[n].to_iid   = i;
            }
            n++;
        }
        if (rec->prov_b == iid) {
            if (out && n < cap) {
                out[n].from_iid = iid;
                out[n].to_iid   = i;
            }
            n++;
        }
    }
    return n;
}

uint32_t if5_cause_concurrents(if5_world* w, const if5_view* v,
                               uint16_t rid, uint32_t* iids,
                               uint32_t cap)
{
    if5_view def;
    /* The view-live contributors on the region — the fold's own
     * filter (shared with if5_estimate_sources, same discipline:
     * the queries can never disagree with the fold). 4096 is a
     * bound, not a capacity claim: the registered scenarios never
     * put a tenth of this on one region (TRIP-17: ~60; E2's hot
     * object: ~3500 over the full run, sampled per checkpoint). */
    uint32_t members[4096];
    uint32_t n_members = 0, n = 0, i, j;
    const if5_payload_type* ty;
    if5_region* r;
    bool lattice, forked;
    uint32_t head = 0;

    if (!w) return 0;
    r = if5__region(w, rid);
    if (!r) return 0;
    if (!v) { def = if5_now(w); v = &def; }

    ty = if5__type(w, r->ptype);
    lattice = ty && ty->sq;
    forked = r->head_b != 0 && !lattice;
    if (forked) head = if5__selected_head(w, r, v);

    for (i = 0; i < v->cut && i < w->n_log && n_members < 4096; i++) {
        const if5_intent* rec = &w->log[i];
        if (rec->region != rid) continue;
        if (!(rec->flags & IF5_F_APPLIED)) continue;
        if (rec->flags & IF5_F_EXCLUDED) continue;
        if (!if5__is_value_type(rec->ptype)) continue;
        if (if5__is_excluded(w, rec, v)) continue;
        if (forked && !if5__on_selected_chain(w, head, rec)) continue;
        members[n_members++] = i + 1;
    }

    /* VARIES_WITH: has at least one vc-incomparable partner on the
     * same region. Pairwise by construction (the enumeration is
     * over MEMBERS, not over pairs — the caller gets the set, the
     * pairs are recoverable by re-querying). */
    for (i = 0; i < n_members; i++) {
        const if5_intent* a = &w->log[members[i] - 1];
        bool has_partner = false;
        for (j = 0; j < n_members && !has_partner; j++) {
            const if5_intent* b = &w->log[members[j] - 1];
            if (i == j) continue;
            if (if5_vc_concurrent(a->vc, b->vc)) has_partner = true;
        }
        if (has_partner) {
            if (iids && n < cap) iids[n] = members[i];
            n++;
        }
    }
    return n;
}

bool if5_cause_reachable(if5_world* w, uint32_t from, uint32_t to)
{
    /* The knowledge-dependency question, answered by the partial
     * order itself: O(1), transitive by definition, and it is the
     * SAME order the adjudicator uses (if5__region_concurrent's
     * complement) — there is exactly one causality in this core,
     * never two that could drift apart. */
    if (!w || from == 0 || to == 0 || from > w->n_log || to > w->n_log)
        return false;
    if (from == to) return true;
    return if5_vc_le(w->log[from - 1].vc, w->log[to - 1].vc);
}
