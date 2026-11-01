/*
 * if5_schedule.c — S17 seated: the surprise timetable
 * (ADR-0026 Decision 3d).
 *
 * Vol. 4 seed S17's statement is this module's contract: the design
 * object is not the picture but the TIMETABLE of controlled
 * prediction errors — onboarding, diffs, changelogs and AI
 * explanations are projections of this one object. Vol. 7 ch. 3
 * seated S17 (+ S15, in if5_tree.c) as the replacement design
 * object for the dead reproducibility claim.
 *
 * N5's no-new-slots discipline (vol. 5 ch. 8: adjudication seats
 * by accountization, not by new log families) is enforced by
 * CONSTRUCTION: the schedule is a pure query over records the core
 * already logs. It adds no ptype, no struct to the record, no
 * write path — it reads the black box. The one enabling delta is
 * the debit word's class packing (ADR-0026 3d): without it, the
 * black box could not distinguish a surprise debit from an
 * initiative debit, and "when was this user surprised" would be
 * under-determined — the slice retains that under-determination
 * and it is registered as ADR-0026's Known issue #2.
 *
 * Purity: this query must never mutate the durable layer (S24's
 * two-layer boundary). test_v5_seats snapshots the world's durable
 * image around schedule calls and memcmps it.
 */
#include "if5_internal.h"

#include <string.h>

/* Classification: which timetable kind is this record, 0 = none.
 * The view's own exclusion test applies — an undone record did not
 * happen at this view, so it is not on the timetable either. */
static uint8_t if5__sched_kind(const if5_intent* rec)
{
    if (rec->debit != 0 &&
        IF5_DEBIT_CLASS(rec->debit) == IF5_DEBIT_SURPRISE)
        return IF5_SCHED_SURPRISE;
    switch (rec->ptype) {
    case IF5_T_RECEIPT: return IF5_SCHED_BREAKER;
    case IF5_T_TIER:    return IF5_SCHED_TIER;
    case IF5_T_ARBIT:   return IF5_SCHED_ARBIT;
    case IF5_T_MERGE:   return IF5_SCHED_MERGE;
    default:
        if (rec->flags & IF5_F_CONCUR_MERGE) return IF5_SCHED_MERGE;
        return 0;
    }
}

uint32_t if5_schedule(if5_world* w, const if5_view* v, if5_sid src,
                      if5_sched_entry* out, uint32_t cap)
{
    if5_view def;
    uint32_t n = 0, i;
    uint32_t bound;

    if (!w) return 0;
    if (!v) { def = if5_now(w); v = &def; }

    bound = v->cut < w->n_log ? v->cut : w->n_log;
    for (i = 1; i <= bound; i++) {
        const if5_intent* rec = &w->log[i - 1];
        uint8_t kind;
        if5_sched_entry e;

        if (if5__is_excluded(w, rec, v)) continue;   /* not at this view */
        kind = if5__sched_kind(rec);
        if (kind == 0) continue;
        if (src != IF5_SID_NULL && rec->src != src) continue;

        if (!out) { n++; continue; }
        if (n >= cap) { n++; continue; }   /* count past capacity: the
                                           * caller learns the true total */

        memset(&e, 0, sizeof e);
        e.iid = i;
        e.magnitude = kind == IF5_SCHED_SURPRISE ? IF5_DEBIT_SEV(rec->debit)
                                                 : 0;
        e.region = rec->region;
        e.sid    = rec->src;
        e.kind   = kind;
        out[n++] = e;
    }

    /* Counting mode (out == NULL) returns the exact total; filling
     * mode returns min(n, ...) — no: both return n, where n counts
     * every matching record even past cap, so the caller can detect
     * truncation (return > cap means "realloc and ask again"). */
    return n;
}
