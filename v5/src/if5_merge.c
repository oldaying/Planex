/*
 * if5_merge.c — the four-step adjudication flow (vol. 5 §4.2),
 * the concurrency decider, and the tier-3 arbiter.
 *
 * Lineage (ADR-0026 Decision 3a): promoted unchanged from
 * slice/src/ifl_merge.c (ADR-0025, commit abad971) — namespace
 * only. The differential holds this file to the frozen slice.
 *
 * The flow, per record:
 *   step 1  causal?      -> apply, done (I1: in a one-source world
 *                           the flow exits HERE everywhere — the
 *                           counters must read zero past step 1)
 *   step 2  tier 2       -> the registered sq joins the pair; the
 *                           system posts a background debit; dual
 *                           derivation is recorded (I6)
 *   step 3  tier 1 / 3   -> fork keeps both versions; strict
 *                           arbitrates by the total order on
 *                           (t, sid, iid) and the loser is excluded
 *                           from the fold by an ARBIT witness
 *   step 4  merge-intent -> the explicit stitch (if5_merge_intent,
 *                           in if5_core.c — the only out-of-band
 *                           adjudicator, because it is itself a
 *                           world decision, not a system reaction)
 *
 * No branch in this file consults a source's kind (I1), any
 * quiet-system flag (I2 — a quiet system is the all-zero budget
 * configuration), or
 * the number of sources (I1's short-circuit is emergent: with one
 * source every pair is comparable, so step 1 always wins). The
 * static half of that claim is enforced by
 * scripts/check_slice_rebound.sh.
 */
#include "if5_internal.h"

#include <string.h>

/* ============================================================
 * The concurrency decider
 * ============================================================ */

/*
 * Is `vc` concurrent with any APPLIED sibling on `rid`?
 *
 * The scan is O(log) by registered design (ADR-0025 4f): no
 * per-region frontier is retained — frontier storage would tax the
 * working set 16 B/region, and at instrument scale the scan is
 * free. The cost is history-side, which is exactly the side E2
 * prices.
 */
bool if5__region_concurrent(if5_world* w, uint16_t rid, if5_vc vc)
{
    return if5__region_concurrent_upto(w, rid, vc, w->n_log);
}

/* The bounded form (ADR-0028 Decision 4): the scan restricted to
 * records 1..upto — the prefix that existed when an installed
 * record was adjudicated. The adjudicator's own call path is
 * unchanged (the wrapper above); the snapshot's rebuild uses this
 * to re-derive path counters over history without re-deciding
 * anything (the outcomes are the installed flags). */
bool if5__region_concurrent_upto(if5_world* w, uint16_t rid, if5_vc vc,
                                  uint32_t upto)
{
    uint32_t i, n;
    if (!w) return false;
    n = upto < w->n_log ? upto : w->n_log;
    for (i = 0; i < n; i++) {
        const if5_intent* q = &w->log[i];
        if (q->region != rid) continue;
        if (!(q->flags & IF5_F_APPLIED)) continue;
        if (q->flags & IF5_F_EXCLUDED) continue;
        if (q->ptype == IF5_T_REGION || q->ptype == IF5_T_TIER) continue;
        /* A record the current view has revoked is not a live
         * sibling: adjudicating against the abandoned branch would
         * let dead knowledge shape the living fold. */
        if (if5__is_excluded(w, q, &w->now)) continue;
        if (if5_vc_concurrent(vc, q->vc)) return true;
    }
    return false;
}

/* ============================================================
 * The tier-3 arbiter (N3)
 * ============================================================ */

/*
 * The registered arbiter: the total order on (t, sid, iid).
 * Submission order breaks ties, and the newcomer is always the
 * LATER append (its iid is strictly larger by construction), so a
 * full (t, sid) tie resolves in the newcomer's favor. Deterministic
 * by construction. CONTRACT: `newbie` must have been appended after
 * `head` — every call site in this file satisfies it.
 */
bool if5__arbiter_prefers(const if5_intent* newbie, const if5_intent* head)
{
    if (newbie->t != head->t) return newbie->t > head->t;
    if (newbie->src != head->src) return newbie->src > head->src;
    return true;                         /* iid tie-break: newcomer wins */
}

/* ============================================================
 * Step 2 — the tolerant merge (tier 2)
 * ============================================================ */

/*
 * Joins the newcomer into the region under the registered sq.
 * The SYSTEM posts the background debit (vol. 5 §4.2 step 2: the
 * merge is system initiative — if the system cannot pay, the merge
 * does not happen and the world sees no unbudgeted change, which
 * is I2/C4's entire test surface).
 */
static bool if5__tier2_join(if5_world* w, if5_intent* rec, if5_region* r)
{
    const if5_payload_type* ty = if5__type(w, r->ptype);
    uint32_t i;

    /* Find the sibling this record is concurrent with (the first
     * applied one — the join is over the region's folded value, so
     * any applied sibling witnesses the concurrency). */
    for (i = 0; i < w->n_log - 1; i++) {
        const if5_intent* q = &w->log[i];
        if (q->region != rec->region) continue;
        if (!(q->flags & IF5_F_APPLIED)) continue;
        if (if5__is_excluded(w, q, &w->now)) continue;
        if (!if5_vc_concurrent(rec->vc, q->vc)) continue;

        /* The system pays for executing the merge. The system's
         * identity is EXPLICIT CONFIGURATION (if5_set_system_source)
         * — never a kind dispatch (I1). An unconfigured system is a
         * world where nobody can pay: the merge does not happen. */
        w->stats.concurrent_arrivals++;
        w->stats.lattice_checks++;
        if (w->sys_sid == IF5_SID_NULL ||
            !if5__post_debit(w, w->sys_sid, IF5_DEBIT_INITIATIVE,
                             IF5_SEV_BACKGROUND,
                             (uint32_t)(rec - w->log + 1))) {
            /* Overdrafted (or absent) system: the merge is NOT
             * executed. Both records stay in the log; the newcomer
             * is left unapplied. The receipt is the visible
             * consequence. NOTE: post_debit may have appended a
             * RECEIPT record, which can realloc w->log — `rec` is
             * from this call's own append and MUST NOT be touched
             * again on this path. Return immediately. */
            return false;
        }
        w->stats.system_merge_debits++;

        /* Dual derivation (I6): the merged record carries both
         * parents. */
        rec->prov_a = i + 1;
        rec->prov_b = 0;                 /* second parent is the folded
                                          * state's last contributor —
                                          * prov_a witnesses the join */
        rec->flags |= IF5_F_APPLIED | IF5_F_CONCUR_MERGE;
        {
            uint64_t a = r->cache;
            uint64_t b = ((uint64_t)rec->value[1] << 32) | rec->value[0];
            uint64_t out = b;
            if (ty && ty->sq) (void)ty->sq(a, b, &out);
            r->cache = out;
            rec->value[0] = (uint32_t)(out & 0xFFFFFFFFu);
            rec->value[1] = (uint32_t)(out >> 32);
        }
        w->stats.tier2_merges++;
        r->last_iid = (uint32_t)(rec - w->log + 1);
        return true;
    }

    /* No concurrent sibling found after all (race between the scan
     * and the flags): fall through to a plain causal apply. */
    rec->flags |= IF5_F_APPLIED;
    {
        uint64_t b = ((uint64_t)rec->value[1] << 32) | rec->value[0];
        if (ty && ty->sq) {
            uint64_t out;
            (void)ty->sq(r->cache, b, &out);
            r->cache = out;
        } else {
            r->cache = b;
        }
    }
    r->last_iid = (uint32_t)(rec - w->log + 1);
    return true;
}

/* ============================================================
 * Step 3a — the hard fork (tier 1)
 * ============================================================ */

/*
 * Registered model (two fork heads, ADR-0025's registered
 * capacity): the existing chain keeps head A; the newcomer becomes
 * head B, its prov_a spine linking back to the fork point. A THIRD
 * concurrent arrival (beyond the registered two-head capacity)
 * extends chain B — the newest working line — which is the
 * documented operationalization at instrument scale.
 */
static bool if5__tier1_fork(if5_world* w, if5_intent* rec, if5_region* r)
{
    uint32_t me = (uint32_t)(rec - w->log + 1);

    w->stats.concurrent_arrivals++;
    if (r->head_b == 0) {
        /* First fork: the current chain's last applied record is
         * head A, the newcomer is head B. */
        r->head_a = r->last_iid;
        r->head_b = me;
        rec->prov_a = r->last_iid;
    } else {
        /* Chain B extends: the newcomer's spine continues from the
         * current head B. */
        rec->prov_a = r->head_b;
        r->head_b = me;
    }
    rec->flags |= IF5_F_APPLIED;
    r->last_iid = me;
    w->stats.tier1_forks++;
    return true;
}

/* ============================================================
 * Step 3b — strict convergence (tier 3)
 * ============================================================ */

/*
 * Arbitrates by (t, sid, iid). The loser is excluded from the fold
 * BY THE ARBITRATION RECORD — it stays in the log and the black
 * box (deleting it would weaken I6's provenance completeness; the
 * exclusion-not-deletion reading of vol. 5 §6.1 is the stricter
 * one, registered in ADR-0025 4c).
 */
static bool if5__tier3_arbitrate(if5_world* w, if5_intent* rec, if5_region* r)
{
    uint32_t me = (uint32_t)(rec - w->log + 1);
    uint32_t i;

    w->stats.concurrent_arrivals++;

    for (i = 0; i < w->n_log - 1; i++) {
        if5_intent* q = &w->log[i];
        if (q->region != rec->region) continue;
        if (!(q->flags & IF5_F_APPLIED)) continue;
        if (q->flags & IF5_F_EXCLUDED) continue;
        if (if5__is_excluded(w, q, &w->now)) continue;
        if (!if5_vc_concurrent(rec->vc, q->vc)) continue;

        w->stats.tier3_arbitrations++;

        /* The registered total order on (t, sid, iid): the
         * newcomer's iid is the larger (later append), so a full
         * (t, sid) tie resolves in its favor. */
        if (if5__arbiter_prefers(rec, q)) {
            /* Newcomer wins: the head is excluded, the ARBIT
             * record witnesses, the newcomer applies. */
            if5_intent wit;
            if5_vc jvc;
            uint16_t rg = rec->region;
            uint8_t wsrc = rec->src;
            uint32_t win = (uint32_t)(rec - w->log + 1);
            uint64_t nval = ((uint64_t)rec->value[1] << 32) | rec->value[0];

            q->flags |= IF5_F_EXCLUDED;
            w->stats.excluded++;

            jvc = if5_vc_join(rec->vc, q->vc);
            memset(&wit, 0, sizeof wit);
            wit.src    = wsrc;
            wit.region = rg;
            wit.ptype  = IF5_T_ARBIT;
            wit.flags  = IF5_F_APPLIED;
            wit.value[0] = win;         /* winner */
            wit.aux = i + 1;             /* loser */
            wit.vc = jvc;
            (void)if5__append(w, &wit);  /* may realloc w->log — reacquire
                                          * the record by iid below */

            rec = &w->log[win - 1];
            rec->flags |= IF5_F_APPLIED;
            r->cache = nval;
            r->last_iid = win;
            return true;
        } else {
            /* Head wins: the newcomer is excluded, the ARBIT
             * record witnesses, the head keeps the fold. */
            if5_intent wit;
            if5_vc jvc;
            uint16_t rg = rec->region;
            uint8_t wsrc = q->src;
            uint32_t win = i + 1;
            uint32_t lose = (uint32_t)(rec - w->log + 1);

            jvc = if5_vc_join(rec->vc, q->vc);
            memset(&wit, 0, sizeof wit);
            wit.src    = wsrc;
            wit.region = rg;
            wit.ptype  = IF5_T_ARBIT;
            wit.flags  = IF5_F_APPLIED;
            wit.value[0] = win;         /* winner */
            wit.aux = lose;              /* loser */
            wit.vc = jvc;
            (void)if5__append(w, &wit);  /* may realloc — reacquire */

            rec = &w->log[lose - 1];
            rec->flags |= IF5_F_EXCLUDED;
            w->stats.excluded++;
            return false;                /* not in the fold — but in
                                          * the log and the black box */
        }
    }

    /* No live concurrent sibling: plain apply. */
    rec->flags |= IF5_F_APPLIED;
    r->cache = ((uint64_t)rec->value[1] << 32) | rec->value[0];
    r->last_iid = me;
    return true;
}

/* ============================================================
 * The four-step flow
 * ============================================================ */

bool if5__adjudicate(if5_world* w, if5_intent* rec)
{
    if5_region* r = if5__region(w, rec->region);
    const if5_payload_type* ty;
    bool concurrent;

    if (!r) return false;

    /*
     * Step 1 — the causal fast path. In a one-source world every
     * pair of records is comparable (the single clock is totally
     * ordered), so the flow exits here EVERYWHERE: zero concurrent
     * arrivals, zero lattice checks, zero tier outcomes. That
     * emergent short-circuit is I1's dynamic half — no special
     * case consulted the source count to achieve it.
     */
    concurrent = if5__region_concurrent(w, rec->region, rec->vc);
    if (!concurrent) {
        uint32_t me = (uint32_t)(rec - w->log + 1);
        ty = if5__type(w, r->ptype);
        rec->flags |= IF5_F_APPLIED;
        {
            uint64_t b = ((uint64_t)rec->value[1] << 32) | rec->value[0];
            if (ty && ty->sq) {
                uint64_t out;
                (void)ty->sq(r->cache, b, &out);
                r->cache = out;
            } else {
                r->cache = b;
            }
        }
        /* Rigid regions track their chain spine through prov_a. A
         * causal arrival at a FORKED region extends the selected
         * chain (head B, the current working line) — that is what
         * "the view selects a head" means for subsequent work. */
        if (!(ty && ty->sq) && rec->ptype != IF5_T_REGION) {
            if (r->head_b) {
                rec->prov_a = r->head_b;
                r->head_b = me;
            } else {
                rec->prov_a = r->last_iid;
            }
        }
        r->last_iid = me;
        w->stats.causal_applies++;
        return true;
    }

    /*
     * Steps 2-3: the tier decides. The tier itself was set by a
     * logged, sourced, undoable TIER intent (D1) — this switch is
     * on the REGION's registered configuration, never on the
     * source's kind.
     */
    switch (r->tier) {
    case IF5_TIER_MERGE:
        ty = if5__type(w, r->ptype);
        if (ty && ty->sq)
            return if5__tier2_join(w, rec, r);
        /* A rigid type in a tier-2 region: "no lattice exists" is
         * an answer, not an error (vol. 5 3.3) — it routes to the
         * fork semantics, exactly as an unjoinable tier-1 pair. */
        return if5__tier1_fork(w, rec, r);
    case IF5_TIER_STRICT:
        return if5__tier3_arbitrate(w, rec, r);
    case IF5_TIER_FORK:
    default:
        return if5__tier1_fork(w, rec, r);
    }
}
