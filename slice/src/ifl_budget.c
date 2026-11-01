/*
 * ifl_budget.c — the attention ledger (N4), the pricing ladder,
 * the circuit breaker, sub-budgets, and receipts.
 *
 * One ledger per source. INITIATIVE and SURPRISE share one currency
 * (vol. 5 ch. 7): both are attention the system spent that the
 * human did not ask for — the difference is only whether a prior
 * prediction was violated (B5) or not (B4).
 *
 * The registered slice pricing ladder (ADR-0025 4e — S16's
 * readiness/rupture spectrum priced in integers; the VALUES are
 * E4's to measure, the MONOTONICITY is ours to keep and is tested
 * by test_slice_property.c): background 1 < ambient 2 < foreground
 * 4 < modal 8.
 *
 * Overdraft trips the breaker INSTEAD OF the action (S1's algebra:
 * failure and budget breach are the same class of event) and
 * produces a receipt — origin, reason (PROV activity ref),
 * itemized cost, rollback address, checkpoint — never a silent
 * failure.
 */
#include "ifl_internal.h"

#include <stdlib.h>
#include <string.h>

/* ============================================================
 * The posted cost of a severity rung. The ladder is monotone BY
 * CONSTRUCTION (the registered constants) and monotone BY TEST
 * (the property suite walks every pair). Nothing in the core
 * reads these as anything but integers.
 * ============================================================ */

static uint32_t ifl__cost(ifl_severity sev)
{
    switch (sev) {
    case IFL_SEV_BACKGROUND:  return 1;
    case IFL_SEV_AMBIENT:     return 2;
    case IFL_SEV_FOREGROUND:  return 4;
    case IFL_SEV_MODAL:       return 8;
    default:                  return 0;  /* no rung: no cost (the
                                          * user's own action) */
    }
}

/* ============================================================
 * The sub-ledger ceiling (S22 delegation)
 * ============================================================ */

/*
 * A delegated source spends against its sub-ceiling (carved from
 * the delegator's remaining balance); a root source spends against
 * its own budget. Either way, what has been CARVED OUT to further
 * delegates subtracts from the available balance — the chain's
 * arithmetic is one ledger rule, not two.
 */
static uint32_t ifl__ceiling_of(const ifl_ledger* l)
{
    return l->sub_ceiling > 0 ? l->sub_ceiling : l->budget;
}

/* ============================================================
 * Receipts (T7) — the receipted failure channel
 * ============================================================ */

void ifl__emit_receipt(ifl_world* w, ifl_sid origin, uint32_t reason_iid,
                       uint32_t rollback_cut)
{
    ifl_receipt rc;
    ifl_intent rec;
    const ifl_ledger* l;

    if (w->n_receipts == w->cap_receipts) {
        uint32_t ncap = w->cap_receipts ? w->cap_receipts * 2 : 16;
        ifl_receipt* nr = (ifl_receipt*)realloc(w->receipts,
            (size_t)ncap * sizeof(ifl_receipt));
        if (!nr) { assert(!"receipt store growth failed"); return; }
        w->receipts = nr;
        w->cap_receipts = ncap;
    }

    l = &w->ledgers[origin];

    memset(&rc, 0, sizeof rc);
    rc.origin          = origin;
    rc.hop             = 0;
    {
        /* Delegation depth of the origin: walk up the chain. The
         * chain is bounded by IFL_MAX_SOURCES, so this is O(1) at
         * instrument scale and needs no separate bookkeeping. */
        ifl_sid cur = w->sources[origin].delegated_by;
        while (cur != IFL_SID_NULL) {
            rc.hop++;
            cur = w->sources[cur].delegated_by;
        }
    }
    rc.reason_iid      = reason_iid;      /* why: PROV activity ref */
    rc.cost_initiative = l->spent_initiative;
    rc.cost_surprise   = l->spent_surprise;
    rc.rollback_cut    = rollback_cut;    /* undo-to address */
    rc.checkpoint      = w->now.cut;      /* commitment snapshot */

    w->receipts[w->n_receipts++] = rc;
    w->stats.receipts++;

    /* The receipt is ALSO a logged record — the black box carries
     * it (S3), which is what makes E5b's deployment measurement
     * ("breaker triggers", queryable) possible without a second
     * channel. */
    memset(&rec, 0, sizeof rec);
    rec.src    = origin;
    rec.region = 0;
    rec.ptype  = IFL_T_RECEIPT;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = w->n_receipts;         /* receipt index */
    rec.value[1] = reason_iid;
    rec.aux = rollback_cut;
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        uint32_t own = vc.c[origin];
        vc.c[origin] = own + 1;
        rec.vc = vc;
        w->svc[origin] = vc;
    }
    (void)ifl__append(w, &rec);
}

/* ============================================================
 * The debit turnstile
 * ============================================================ */

bool ifl__post_debit(ifl_world* w, ifl_sid sid, ifl_debit_class c,
                     ifl_severity sev, uint32_t reason_iid)
{
    ifl_ledger* l;
    uint32_t cost, ceiling, spent;

    if (!w || sid >= w->n_sources) return false;
    if (c != IFL_DEBIT_INITIATIVE && c != IFL_DEBIT_SURPRISE) return false;

    l = &w->ledgers[sid];
    cost = ifl__cost(sev);
    if (cost == 0) return true;           /* no rung: nothing to post
                                           * (the free default) */

    ceiling = ifl__ceiling_of(l);
    spent = l->spent_initiative + l->spent_surprise + l->carved;

    if (ceiling != IFL_BUDGET_UNLIMITED && spent + cost > ceiling) {
        /* Overdraft: the breaker trips INSTEAD OF the action. The
         * receipt (not a silent failure, not an exception) is the
         * caller's only consequence — and the action must not run. */
        l->breaker_tripped = true;
        w->stats.breakers++;
        ifl__emit_receipt(w, sid, reason_iid, w->now.cut);
        return false;
    }

    if (c == IFL_DEBIT_INITIATIVE) l->spent_initiative += cost;
    else                           l->spent_surprise += cost;
    return true;
}
