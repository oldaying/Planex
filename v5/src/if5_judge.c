/*
 * if5_judge.c — Closure as a judgment (ADR-0027 Decision 2c, the
 * mechanism layer's third mapping).
 *
 * The shipping Closure (src/closure.c) is the five execution-side
 * stages as a RUNNABLE object: goal string, typed intent value,
 * action callback, runtime invocation, evaluation function. This
 * module re-derives the closure as a READ of the adjudicated
 * record — the five stages, mapped:
 *
 *   Goal          the region + payload family: the semantic
 *                 address the S15 node already carries (name,
 *                 role) — the goal is an ADDRESS, not prose
 *   Intent        the record itself (the value word)
 *   Action        the type-registered semilattice join — mutation
 *                 is TYPED, not arbitrary: the deepest change of
 *                 the mapping, and it is the axiom table's own
 *                 content decision (vol. 5: intent is a value,
 *                 not a function), not an omission
 *   Execution     the adjudicator's verdict, read off the flags
 *                 it already wrote (APPLIED / CONCUR_MERGE /
 *                 EXCLUDED / REBORN)
 *   Evaluation    the fold at the view — the judgment carries it
 *                 so "was the goal achieved" is answerable AT the
 *                 judgment, not just at the world
 *
 * The observation point (observed_cut) is carried because the
 * Winograd/Flores conversation-for-action reading the shipping
 * file cites needs it: what did the asker know when asking.
 *
 * Purity: a pure query module (S24). Lineage: R2; no slice
 * ancestor.
 */
#include "if5_internal.h"

#include <string.h>

bool if5_judgment_of(if5_world* w, uint32_t iid, const if5_view* v,
                     if5_judgment* out)
{
    const if5_intent* rec;
    if5_region* r;
    if5_view def;

    if (!out) return false;
    memset(out, 0, sizeof *out);
    if (!w || iid == 0 || iid > w->n_log) return false;
    rec = &w->log[iid - 1];
    if (!v) { def = if5_now(w); v = &def; }

    out->iid          = iid;
    out->region       = rec->region;
    out->ptype        = rec->ptype;
    out->src          = rec->src;
    out->rebased      = (rec->flags & IF5_F_REBORN) ? 1 : 0;
    out->debit_class  = (uint8_t)IF5_DEBIT_CLASS(rec->debit);
    out->severity     = (uint8_t)IF5_DEBIT_SEV(rec->debit);
    out->fork_heads   = 0;

    /* The observation point: the asker's causal coordinate — the
     * record's own vc (declared knowledge joined with this record's
     * increment). The scalar prefix the submitter handed to the
     * channel is NOT recoverable from the record (many prefixes
     * yield the same joined knowledge), so this struct carries the
     * coordinate and does not fake the prefix. */
    out->observed     = rec->vc;

    /* The goal's region context (fork state is the goal's, not the
     * record's — a record submitted onto a region that LATER forks
     * is judged at the view's region state). */
    r = if5__region(w, rec->region);
    if (r && r->head_b) {
        const if5_payload_type* ty = if5__type(w, r->ptype);
        bool lattice = ty && ty->sq;
        out->fork_heads = (uint8_t)(lattice ? 1 : 2);
    }

    /* The verdict, most-specific-first. Order matters and is
     * contract: an undone ARBIT loser reads EXCLUDED (what
     * happened to it at THIS view dominates what the arbiter
     * thought earlier); a refused submission reads REFUSED even
     * though it is in the log (the log records the attempt — the
     * black box records everything, the fold sees only truth). */
    if (if5__is_excluded(w, rec, v)) {
        out->verdict = IF5_VERDICT_EXCLUDED;
    } else if (!(rec->flags & IF5_F_APPLIED)) {
        if (rec->ptype == IF5_T_ARBIT) out->verdict = IF5_VERDICT_ARBIT_LOST;
        else                           out->verdict = IF5_VERDICT_REFUSED;
    } else if (rec->flags & IF5_F_CONCUR_MERGE) {
        out->verdict = IF5_VERDICT_MERGED;
    } else if (out->fork_heads == 2 && r && r->head_b) {
        /* A live non-lattice fork: the record sits on one of two
         * chains — the fold addresses the view's selection, so
         * "which chain" is a VIEW question and the judgment marks
         * the region's unresolved state rather than pretending a
         * single outcome. */
        out->verdict = IF5_VERDICT_FORKED;
    } else {
        out->verdict = IF5_VERDICT_APPLIED;
    }

    /* The evaluation: the fold at the view, over the record's own
     * region (world-scoped records fold nothing — region 0). */
    out->eval_value = rec->region
        ? (uint32_t)(if5_state_at(w, rec->region, v, NULL) & 0xFFFFFFFFu)
        : 0;

    return true;
}
