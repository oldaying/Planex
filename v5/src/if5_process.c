/*
 * if5_process.c — px_interaction as the adjudication surface
 * (ADR-0027 Decision 2g, the mechanism layer's seventh mapping).
 *
 * The shipping interaction (src/interaction.c) is a continuous
 * process with identity, trajectory, and outcome:
 * begin(ev) -> sample(ev)* -> commit(ev) | cancel(reason), with
 * the invariant that samples never notify observers and never
 * auto-invoke perceptions. This module re-derives the concept
 * with the process's PHASES carrying causal weight:
 *
 *   begin    opens the process IN THE LOG — an IF5_T_PROCESS
 *            record through the ordinary submit channel, so the
 *            opening is itself adjudicated, undoable, and
 *            provable. Identity is the opening's iid.
 *   sample   a pure working-set update: zero submissions, zero
 *            debits, zero log growth. The px invariant, kept —
 *            and statically enforced (the rebound gate's v5
 *            section bans submit/append/debit calls from this
 *            function's body; ADR-0027 Decision 6).
 *   commit   submits the outcome through the ordinary channel.
 *            The outcome is a CAUSAL SUCCESSOR of the opening by
 *            vector clock (same source, later slot) — the
 *            process's causal structure is structural, never
 *            asserted. if5_process_of recovers the opening from
 *            the outcome by query.
 *   cancel   IS a cut: the opening is undone (the ordinary
 *            view-exclusion path) and the process leaves causal
 *            history the way an undone change does — a STRONGER
 *            cancellation than the shipping callback, which had no
 *            history semantics at all.
 *
 * NOT CARRIED (ADR-0027's contract columns): the sample
 * trajectory's persistence (working-set, like the region name —
 * Known issue 1) and the cancel reason word (the 48-byte density
 * is registered; Known issue 2).
 *
 * Slots: static, bounded (IF5_MAX_PROCESSES), world-scoped. A
 * slot open at world free is simply gone — the OPENING record
 * stays in the log; the slot is not durable, and never pretended
 * to be. Lineage: R2; no slice ancestor.
 */
#include "if5_internal.h"

#include <string.h>

if5_process* if5_process_begin(if5_world* w, if5_sid src, uint16_t rid,
                               uint8_t ptype)
{
    if5_submit_opts o;
    uint32_t iid;
    uint32_t i;

    if (!w || src >= w->n_sources) return NULL;
    if (!if5__region(w, rid)) return NULL;

    /* A process opens through the ordinary channel — no privileged
     * path. observed_cut = the full log: the opener sees everything
     * (the process is opening ON the world as it stands). */
    memset(&o, 0, sizeof o);
    o.observed_cut = w->n_log;
    iid = if5_submit(w, src, rid, IF5_T_PROCESS, ptype, &o);
    if (!iid) return NULL;

    /* Find a slot (bounded, world-scoped; overflow is refusal, the
     * same discipline the cut registry carries). */
    for (i = 0; i < IF5_MAX_PROCESSES; i++) {
        if5_process* p = &w->procs[i];
        if (p->closed || p->w == NULL) {
            p->w         = w;
            p->src       = src;
            p->rid       = rid;
            p->ptype     = ptype;
            p->closed    = false;
            p->begin_iid = iid;
            p->latest    = 0;
            return p;
        }
    }
    /* No slot: the opening record stays (it happened), the slot
     * never materializes — the process is refused, not faked. */
    return NULL;
}

bool if5_process_sample(if5_process* p, uint64_t value)
{
    /* Purity is the contract: this body submits nothing, appends
     * nothing, debits nothing. The value lands in the slot for
     * readback (if5_process_latest) — the trajectory a cancelled
     * process leaves behind is nothing, by design (the px
     * invariant: the semantic world is touched only at phase
     * transitions). */
    if (!p || !p->w || p->closed) return false;
    p->latest = value;
    return true;
}

uint64_t if5_process_latest(const if5_process* p)
{
    return p ? p->latest : 0;
}

uint32_t if5_process_commit(if5_process* p, uint64_t value,
                            const if5_submit_opts* opts)
{
    if5_submit_opts o;
    uint32_t iid;

    if (!p || !p->w || p->closed) return 0;

    /* The outcome record: ordinary channel, the process's own
     * region and payload family. Default options are OMNISCIENT
     * (observed_cut = the full log): the process slot lives in the
     * submitting thread and its commit sees the world it is
     * landing in — the opening it declared (observed_cut = the
     * log at begin) is thereby a causal predecessor by the core's
     * own submission arithmetic, which is what if5_process_of
     * recovers. A caller who wants a BLIND outcome (a commit that
     * raced the world) passes opts with an older observed_cut, and
     * the outcome is then concurrent with whatever it did not see —
     * the ordinary adjudication, no process special case. */
    if (opts) o = *opts;
    else { memset(&o, 0, sizeof o); o.observed_cut = p->w->n_log; }
    iid = if5_submit(p->w, p->src, p->rid, p->ptype, value, &o);
    if (!iid) return 0;
    p->closed = true;
    return iid;
}

bool if5_process_cancel(if5_process* p)
{
    if (!p || !p->w || p->closed) return false;

    /* Cancel IS a cut: the opening is undone through the ordinary
     * sugar (CURSOR witness + view adoption), so the process
     * leaves causal history exactly the way an undone change does.
     * The slot closes either way — a failed undo (return 0) still
     * ends THIS handle's process (the caller sees false and the
     * slot is dead; a half-cancelled process is not re-enterable). */
    (void)if5_undo_nth(p->w, p->src, p->begin_iid);
    p->closed = true;
    return true;
}

bool if5_process_done(const if5_process* p)
{
    return !p || p->closed;
}

uint32_t if5_process_of(if5_world* w, const if5_view* v, uint32_t iid)
{
    const if5_intent* rec;
    if5_view def;
    uint32_t i;

    if (!w || iid == 0 || iid > w->n_log) return 0;
    rec = &w->log[iid - 1];
    if (!v) { def = if5_now(w); v = &def; }

    /* The outcome's process: the NEWEST live PROCESS record of the
     * SAME source that causally precedes (or equals) the outcome.
     * Newest-first; vc_le is the reachability this module's mapping
     * registered. The view's exclusion applies — at a rewound view,
     * a cancelled process's opening is gone and the outcome reads
     * as process-less (which is the truth at that address). */
    for (i = iid; i >= 1; i--) {
        const if5_intent* q = &w->log[i - 1];
        if (q->ptype != IF5_T_PROCESS) continue;
        if (q->src != rec->src) continue;
        if (!if5_vc_le(q->vc, rec->vc)) continue;
        if (if5__is_excluded(w, q, v)) continue;
        return i;
    }
    return 0;
}
