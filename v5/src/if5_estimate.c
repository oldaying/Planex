/*
 * if5_estimate.c — Estimate as a fold (ADR-0027 Decision 2a, the
 * mechanism layer's first mapping).
 *
 * The shipping Estimate (src/estimate.c) is a mutable cell: the
 * value, the confidence, and the trajectory live INSIDE the cell,
 * observers are notified on set, and derived estimates recompute
 * through an explicit subscription protocol. This module is that
 * abstraction re-derived over the durable core: an estimate is a
 * QUERY at an address, and everything the cell carried becomes
 * either structural or addressable —
 *
 *   value        = if5_state_at (the fold the core already is)
 *   confidence   = the EVIDENCE ordinal (see below — an ordinal,
 *                  never a probability; the header says so and the
 *                  ADR's CAVEATS make misuse explicit)
 *   trajectory   = if5_estimate_trace: the fold at every cut in a
 *                  range — the shipping animation is the two-sample
 *                  special case, and TIME becomes an addressable
 *                  dimension instead of a field inside the cell
 *   dependency   = if5_estimate_sources: the contributing records,
 *                  made explicit. The shipping tree needed Stage 3's
 *                  derived-estimate machinery (subscriptions, cycle
 *                  detection, propagation-depth accounting) because
 *                  dependency was NOT structural there; here a
 *                  region's fold already depends on exactly its
 *                  contributing records, so the tracking is the
 *                  structure and the machinery is gone.
 *
 * Purity: this module never mutates anything (S24; the seats suite
 * memcmps the durable image around these calls, and M1 re-asserts
 * it here). Lineage: R2 of the v5 derivation; no slice ancestor —
 * the slice had no estimate counterpart (which is why this mapping
 * is property-gated, not differential-gated; ADR-0027 Decision 5).
 */
#include "if5_internal.h"

#include <string.h>

/* The evidence ordinal, read off the region's adjudication state:
 * the log can support an evidence STATE, not a caller's prior.
 *
 *   3 CHAIN   no fork, no concurrent merge: one line of applied work
 *   2 MERGED  concurrent work present and stitched (the lattice's
 *             join decided a single value)
 *   1 FORKED  the region is forked and unresolved at this view:
 *             two versions are live — the fold addresses one chain
 *             (the view's selection), the other is still there
 *   0 NONE    no live contributing record at this view
 *
 * The ordinal is ordered by "how many independent lines of applied
 * evidence support ONE addressable value" — not by probability,
 * not by recency, not by trust.
 */
static uint8_t if5__evidence_of(const if5_world* w, const if5_region* r,
                                const if5_view* v)
{
    const if5_payload_type* ty = if5__type((if5_world*)w, r->ptype);
    bool lattice = ty && ty->sq;
    bool saw_merge = false;
    uint16_t rid = (uint16_t)(r - w->regions + 1);
    uint32_t i;

    for (i = 0; i < v->cut && i < w->n_log; i++) {
        const if5_intent* rec = &w->log[i];
        if (rec->region != rid) continue;
        if (rec->flags & IF5_F_CONCUR_MERGE) saw_merge = true;
    }

    /* The region-level fork signal: a NON-LATTICE region with a live
     * second head — two versions, no join to decide one. A lattice
     * region's head_b after a stitch is the merge witness, covered
     * by saw_merge; reporting it as FORKED would misreport stitched
     * work, so the fork test is gated on the lattice's absence. */
    if (r->head_b != 0 && !lattice) return IF5_CONF_FORKED;
    return saw_merge ? IF5_CONF_MERGED : IF5_CONF_CHAIN;
}

bool if5_estimate_at(if5_world* w, uint16_t rid, const if5_view* v,
                     if5_estimate* out)
{
    if5_region* r;
    if5_view def;
    uint32_t contrib = 0;

    if (!out) return false;
    memset(out, 0, sizeof *out);
    if (!w) return false;
    r = if5__region(w, rid);
    if (!r) return false;
    if (!v) { def = if5_now(w); v = &def; }

    out->value = if5_state_at(w, rid, v, &contrib);
    out->contributions = contrib;
    out->last_iid = r->last_iid;         /* anchor, same field the
                                          * tree's PROV walk uses */
    /* The anchor must be view-live: a region whose last contributor
     * is past the view's cut has no contributor AT the view. Walk
     * back to the view's newest live contributor (the same filter
     * the tree's if5__last_contributor applies — shared through
     * the core so the two can never disagree). */
    {
        uint32_t i;
        out->last_iid = 0;
        for (i = v->cut < w->n_log ? v->cut : w->n_log; i > 0; i--) {
            const if5_intent* rec = &w->log[i - 1];
            if (rec->region != rid) continue;
            if (!(rec->flags & IF5_F_APPLIED)) continue;
            if (rec->flags & IF5_F_EXCLUDED) continue;
            if (!if5__is_value_type(rec->ptype)) continue;
            if (if5__is_excluded(w, rec, v)) continue;
            out->last_iid = i;
            break;
        }
    }
    out->evidence = (contrib == 0) ? IF5_CONF_NONE
                                   : if5__evidence_of(w, r, v);
    return true;
}

uint32_t if5_estimate_trace(if5_world* w, uint16_t rid,
                            const if5_view* from, const if5_view* to,
                            uint64_t* out, uint32_t cap)
{
    if5_view def;
    uint32_t lo, hi, n, i;

    if (!w) return 0;
    if (!from) { def = if5_now(w); lo = def.cut; }
    else lo = from->cut;
    if (!to) { def = if5_now(w); hi = def.cut; }
    else hi = to->cut;
    if (hi > w->n_log) hi = w->n_log;
    if (lo >= hi) return 0;

    n = hi - lo;
    if (!out) return n;

    /* The fold at every integer cut in (lo, hi] — the trajectory is
     * the ADDRESSABLE form of what the shipping cell animated. */
    for (i = 0; i < n && i < cap; i++) {
        if5_view at;
        memset(&at, 0, sizeof at);
        at.cut = lo + 1 + i;
        out[i] = if5_state_at(w, rid, &at, NULL);
    }
    return n;
}

uint32_t if5_estimate_sources(if5_world* w, uint16_t rid,
                              const if5_view* v, uint32_t* iids,
                              uint32_t cap)
{
    if5_region* r;
    if5_view def;
    const if5_payload_type* ty;
    bool lattice, forked;
    uint32_t head = 0, n = 0, i;

    if (!w) return 0;
    r = if5__region(w, rid);
    if (!r) return 0;
    if (!v) { def = if5_now(w); v = &def; }

    /* The fold's own filter, walked oldest-first: the dependency set
     * is exactly the set of records the fold read — the same filter
     * if5_state_at applies, shared so the two cannot diverge (the
     * same discipline as the tree's PROV anchor). */
    ty = if5__type(w, r->ptype);
    lattice = ty && ty->sq;
    forked = r->head_b != 0 && !lattice;
    if (forked) head = if5__selected_head(w, r, v);

    for (i = 0; i < v->cut && i < w->n_log; i++) {
        const if5_intent* rec = &w->log[i];
        if (rec->region != rid) continue;
        if (!(rec->flags & IF5_F_APPLIED)) continue;
        if (rec->flags & IF5_F_EXCLUDED) continue;
        if (!if5__is_value_type(rec->ptype)) continue;
        if (if5__is_excluded(w, rec, v)) continue;
        if (forked && !if5__on_selected_chain(w, head, rec)) continue;
        if (iids && n < cap) iids[n] = i + 1;
        n++;
    }
    return n;
}
