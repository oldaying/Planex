/*
 * if5_cut.c — the cut algebra: vol. 5 §5.1's causal slice as a
 * first-class value (ADR-0026 Decision 3b).
 *
 * This module discharges the Phase-4-scale known issue ADR-0025
 * registered ("materializing vol. 5 §5.1's global cut algebra as a
 * first-class data structure") at prefix+prune granularity:
 *
 *   - an ADDRESS SPACE: an if5_cut addresses the represented set
 *         { iota_i : i <= prefix }  minus  the causal up-sets of
 *     the prune roots — the shape the slice's views already used,
 *     now a value with identity, not a test-private convention;
 *   - an ALGEBRA: the principal lattice (join = union on
 *     principal cuts, meet = intersection, total), the prune
 *     constructor (undo), and the subset order — every output
 *     down-closed by construction, every law tested;
 *   - a REGISTRY: named, world-level addresses — the "global-cut
 *     addresses beyond region granularity" ADR-0025's tracking
 *     clause anticipated for E1b's audit family.
 *
 * Why prefix+prune and not per-source frontiers: the REBORN clock
 * collision (ADR-0025's design fix #5) makes "vc componentwise <= F"
 * unsound as a membership test — a re-based record reuses its
 * source's clock slot value, so clock comparison cannot distinguish
 * "before F" from "instead of what F pruned". Sound per-source cuts
 * need record-identity sets or monotone re-basing: R2-scale
 * semantics, registered as ADR-0026's Known issue #1.
 *
 * Correctness bar (V3): if5_cut_state must agree with the fold at
 * the equivalent view — and the test suite checks it against an
 * INDEPENDENT membership-driven fold, so agreement is not vacuous.
 * Membership uses the fold's own exclusion test (the REBORN
 * exemption included): membership and folding must never disagree.
 */
#include "if5_internal.h"

#include <string.h>
#include <stdlib.h>

/* ============================================================
 * The frontier (the address's knowledge summary)
 * ============================================================ */

/*
 * The living-knowledge rule, parameterized by an arbitrary prune
 * list: the join over records <= prefix that the address contains,
 * skipping adjudication-dead records (a dead record's knowledge is
 * not knowledge the address represents). This is if5__vc_upto's
 * rule with the world's now replaced by the address's own prunes —
 * the slice computed this only for the world's live view; the cut
 * algebra computes it for ANY address.
 */
if5_vc if5__cut_frontier(if5_world* w, uint32_t prefix,
                         const uint32_t* prunes, uint32_t n_prunes)
{
    if5_view v;
    if5_vc r;
    uint32_t i;

    memset(&r, 0, sizeof r);
    if (!w || prefix == 0) return r;

    v.cut = prefix > w->n_log ? w->n_log : prefix;
    v.n_excl = n_prunes > IF5_MAX_SELECTIONS ? IF5_MAX_SELECTIONS
                                             : n_prunes;
    v.n_sel = 0;
    for (i = 0; i < v.n_excl; i++) v.excl[i] = prunes[i];

    for (i = 0; i < v.cut; i++) {
        const if5_intent* q = &w->log[i];
        if (q->flags & IF5_F_EXCLUDED) continue;   /* dead: no knowledge */
        if (if5__is_excluded(w, q, &v)) continue;  /* pruned: not ours */
        r = if5_vc_join(r, q->vc);
    }
    return r;
}

/* ============================================================
 * Constructors
 * ============================================================ */

if5_cut if5_cut_at(if5_world* w, uint32_t prefix)
{
    if5_cut c;
    memset(&c, 0, sizeof c);
    if (!w) return c;
    c.prefix = prefix > w->n_log ? w->n_log : prefix;
    c.n_prunes = 0;
    c.frontier = if5__cut_frontier(w, c.prefix, c.prunes, 0);
    return c;
}

if5_cut if5_cut_of_view(const if5_view* v)
{
    if5_cut c;
    uint32_t i;
    memset(&c, 0, sizeof c);
    if (!v) return c;
    c.prefix = v->cut;
    c.n_prunes = v->n_excl > IF5_MAX_SELECTIONS ? IF5_MAX_SELECTIONS
                                                : v->n_excl;
    for (i = 0; i < c.n_prunes; i++) c.prunes[i] = v->excl[i];
    return c;
}

bool if5_cut_prune(if5_world* w, if5_cut* c, uint32_t iid)
{
    if (!w || !c) return false;
    if (iid == 0 || iid > c->prefix || iid > w->n_log) return false;
    if (c->n_prunes >= IF5_MAX_SELECTIONS) return false;
    /* Already pruned (idempotence guard: pruning an up-set twice
     * would double-list the root; the represented set would not
     * change, but the address's bookkeeping would lie about it). */
    {
        if5_view v;
        uint32_t i;
        v.cut = c->prefix;
        v.n_excl = c->n_prunes;
        v.n_sel = 0;
        for (i = 0; i < c->n_prunes; i++) v.excl[i] = c->prunes[i];
        if (if5__is_excluded(w, &w->log[iid - 1], &v)) return true;
    }
    c->prunes[c->n_prunes++] = iid;
    /* Down-closure holds by construction: the represented set lost
     * a causal up-set, and removing an up-set from a down-closed
     * set leaves a down-closed set. The frontier must drop the
     * pruned records' knowledge — recompute it. */
    c->frontier = if5__cut_frontier(w, c->prefix, c->prunes, c->n_prunes);
    return true;
}

/* ============================================================
 * The lattice
 * ============================================================ */

/*
 * meet = intersection, TOTAL on all cuts:
 *   S'(a) ∩ S'(b) = S(min prefix) ∖ (↑Pa ∪ ↑Pb)
 * Derivation: r is in both iff r is in both prefixes (r <= min)
 * and pruned by neither. Prune roots above the min prefix prune
 * nothing inside it — kept in the list, harmless to the set, and
 * simpler than filtering (the list is capacity-bounded either way).
 */
bool if5_cut_meet(if5_world* w, const if5_cut* a, const if5_cut* b,
                  if5_cut* out)
{
    uint32_t i, j;
    if (!w || !a || !b || !out) return false;
    if (a->n_prunes + b->n_prunes > IF5_MAX_SELECTIONS) return false;

    memset(out, 0, sizeof *out);
    out->prefix = a->prefix < b->prefix ? a->prefix : b->prefix;
    for (i = 0; i < a->n_prunes; i++) out->prunes[out->n_prunes++] = a->prunes[i];
    for (j = 0; j < b->n_prunes; j++) {
        bool dup = false;
        for (i = 0; i < out->n_prunes; i++)
            if (out->prunes[i] == b->prunes[j]) { dup = true; break; }
        if (!dup) out->prunes[out->n_prunes++] = b->prunes[j];
    }
    out->frontier = if5__cut_frontier(w, out->prefix, out->prunes,
                                      out->n_prunes);
    return true;
}

/*
 * join = union, PRINCIPAL cuts only. The union of two pruned
 * addresses is not in general representable as prefix+prunes:
 * a record can survive in the union via EITHER side's non-pruning
 * while being inside the other side's pruned up-set, and the
 * surviving set's complement need not be an up-set of a small root
 * list. ADR-0026 Decision 3b registers this boundary; join refuses
 * pruned inputs rather than silently misrepresenting the union.
 */
bool if5_cut_join(if5_world* w, const if5_cut* a, const if5_cut* b,
                  if5_cut* out)
{
    if (!w || !a || !b || !out) return false;
    if (a->n_prunes != 0 || b->n_prunes != 0) return false;

    memset(out, 0, sizeof *out);
    out->prefix = a->prefix > b->prefix ? a->prefix : b->prefix;
    out->n_prunes = 0;
    out->frontier = if5__cut_frontier(w, out->prefix, out->prunes, 0);
    return true;
}

/* ============================================================
 * The order
 * ============================================================ */

/*
 * S'(a) ⊆ S'(b) iff:
 *   (1) prefix(a) <= prefix(b)  — a's records are all in b's prefix; and
 *   (2) every b-prune root pb with pb <= prefix(a) is already
 *       excluded from S'(a) — otherwise pb itself would be in
 *       S'(a) but pruned out of S'(b).
 * The "already excluded" test is the fold's own descendant test
 * (REBORN exemption included), evaluated against a's prunes.
 */
bool if5_cut_le(const if5_world* w, const if5_cut* a, const if5_cut* b)
{
    if5_view va;
    uint32_t i, j;
    if (!w || !a || !b) return false;
    if (a->prefix > b->prefix) return false;

    va.cut = a->prefix;
    va.n_excl = a->n_prunes;
    va.n_sel = 0;
    for (i = 0; i < a->n_prunes; i++) va.excl[i] = a->prunes[i];

    for (j = 0; j < b->n_prunes; j++) {
        uint32_t pb = b->prunes[j];
        if (pb == 0 || pb > a->prefix) continue;   /* not in S(a) */
        if (!if5__is_excluded(w, &w->log[pb - 1], &va)) return false;
    }
    return true;
}

/* ============================================================
 * Membership and the structural verifier
 * ============================================================ */

bool if5_cut_contains(const if5_world* w, const if5_cut* c, uint32_t iid)
{
    if5_view v;
    uint32_t i;
    if (!w || !c || iid == 0 || iid > c->prefix || iid > w->n_log)
        return false;
    v.cut = c->prefix;
    v.n_excl = c->n_prunes;
    v.n_sel = 0;
    for (i = 0; i < v.n_excl; i++) v.excl[i] = c->prunes[i];
    return !if5__is_excluded(w, &w->log[iid - 1], &v);
}

/*
 * The public down-closure verifier (vol. 5 5.1's definition,
 * operationalized): every causal predecessor of every contained
 * record is contained. The predecessor test is the core's own
 * "r knows q" test — r.vc.c[q.src] >= q.vc.c[q.src], q != r —
 * WITH the REBORN collision exemption the exclusion test applies:
 * a re-based record's own-source slot can numerically reach a
 * revoked record's slot without containment (ADR-0025's design
 * fix #5; the first run of this verifier caught exactly that
 * collision and is recorded here). Without the exemption, the
 * verifier would demand that every REBORN record's revoked
 * same-source ancestor be contained — which the prune semantics
 * deliberately deny.
 * O(n^2) by design: this is the audit instrument, not a hot path.
 */
bool if5_cut_is_down_closed(const if5_world* w, const if5_cut* c)
{
    uint32_t r, q;
    if (!w || !c) return false;
    for (r = 1; r <= c->prefix && r <= w->n_log; r++) {
        const if5_intent* rr;
        if (!if5_cut_contains(w, c, r)) continue;
        rr = &w->log[r - 1];
        for (q = 1; q < r; q++) {
            const if5_intent* qq = &w->log[q - 1];
            if ((rr->flags & IF5_F_REBORN) && rr->src == qq->src)
                continue;                 /* the registered collision
                                          * exemption — the numeric
                                          * containment is a rebased
                                          * slot reuse, not causality */
            if (rr->vc.c[qq->src] >= qq->vc.c[qq->src]) {
                if (!if5_cut_contains(w, c, q)) return false;
            }
        }
    }
    return true;
}

/* ============================================================
 * The registry (E1b's global addresses)
 * ============================================================ */

uint8_t if5_cut_register(if5_world* w, const if5_cut* c)
{
    return if5_cut_register_named(w, c, "");
}

/* The name column (ADR-0028 Decision 3): register with a name.
 * Duplicate names are refused (return 0) — an address book with
 * two entries answering one name is not an address book. */
uint8_t if5_cut_register_named(if5_world* w, const if5_cut* c,
                               const char* name)
{
    uint8_t id;
    size_t nl;
    if (!w || !c || !name) return 0;
    if (w->n_cuts >= IF5_MAX_CUTS) return 0;
    nl = strlen(name);
    if (nl >= IF5_NAME_MAX) return 0;
    if (nl && if5_cut_by_name(w, name)) return 0;   /* unique */
    w->cuts[w->n_cuts] = *c;
    memset(w->cut_names[w->n_cuts], 0, IF5_NAME_MAX);
    memcpy(w->cut_names[w->n_cuts], name, nl);
    w->n_cuts++;
    id = w->n_cuts;                     /* 1-based id */
    return id;
}

const char* if5_cut_name(if5_world* w, uint8_t id)
{
    if (!w || id == 0 || id > w->n_cuts) return "";
    return w->cut_names[id - 1];
}

uint8_t if5_cut_by_name(if5_world* w, const char* name)
{
    uint8_t i;
    if (!w || !name || !*name) return 0;
    for (i = 0; i < w->n_cuts; i++)
        if (strcmp(w->cut_names[i], name) == 0) return (uint8_t)(i + 1);
    return 0;
}

const if5_cut* if5_cut_by_id(if5_world* w, uint8_t id)
{
    if (!w || id == 0 || id > w->n_cuts) return NULL;
    return &w->cuts[id - 1];
}

uint32_t if5_cut_count(if5_world* w)
{
    return w ? w->n_cuts : 0;
}

/* ============================================================
 * The fold at a cut
 * ============================================================ */

if5_view if5_cut_view(const if5_cut* c)
{
    if5_view v;
    uint32_t i;
    memset(&v, 0, sizeof v);
    if (!c) return v;
    v.cut = c->prefix;
    v.n_excl = c->n_prunes;
    v.n_sel = 0;
    for (i = 0; i < v.n_excl; i++) v.excl[i] = c->prunes[i];
    return v;
}

uint64_t if5_cut_state(if5_world* w, const if5_cut* c, uint16_t rid,
                       uint32_t* contrib)
{
    if5_view v;
    if (!w || !c) { if (contrib) *contrib = 0; return 0; }
    v = if5_cut_view(c);
    return if5_state_at(w, rid, &v, contrib);
}

/* ============================================================
 * The per-source knowledge cut (ADR-0027 Decision 4 —
 * ADR-0026 Known issue 2 discharged)
 *
 * Record identity, not clock comparison: the REBORN collision
 * (a re-based record reusing its source's clock slot) makes
 * "vc componentwise <= F" unsound as a membership test, and this
 * cut does not use that test anywhere. Membership is a
 * REACHABILITY question over the PROV graph, answered forward in
 * causality:
 *
 *     iid is a member  iff  iid is S's own applied record (at or
 *     below the prefix), OR iid reaches one along PROV edges —
 *     iid is a PROV-declared ancestor of something S did.
 *
 * The represented set is "the world as S has STRUCTURALLY bound
 * itself to": S's records plus the dependencies the log
 * explicitly names (merge parents, cursor targets). The scope
 * is honestly narrower than "everything S's work stood on": the
 * implicit knowledge dependency (observed-cut declarations) is
 * not recoverable from the record word — the judgment mapping's
 * discovery (ADR-0027 Decision 2c) — and observational knowledge
 * ("seen but never bound to") has no causal consequence and is
 * out of scope by decision, not by omission.
 *
 * Down-closure is BY CONSTRUCTION: if iid is a member via a path
 * to S's record r, every PROV predecessor of iid is on a path to
 * r too (prepend it). if5_kcut_is_down_closed verifies this
 * independently (the recursive form, checked against the
 * constructor rather than shared with it — M9's demand).
 *
 * Storage is O(1) per cut (sid + prefix + cached frontier) — the
 * identity set is implicit in the log and never materialized, so
 * there is no capacity cliff (ADR-0027's answer to the storage
 * question ADR-0026's tracking clause left open).
 * ============================================================ */

if5_kcut if5_kcut_of_source(if5_world* w, if5_sid sid, uint32_t prefix)
{
    if5_kcut k;
    uint32_t i;

    memset(&k, 0, sizeof k);
    if (!w || sid >= w->n_sources) return k;
    k.sid    = sid;
    k.prefix = prefix > w->n_log ? w->n_log : prefix;

    /* The frontier: the join of S's OWN applied records' vcs. It
     * dominates every member's vc (vc is monotone along causality:
     * a PROV predecessor's vc is componentwise <= its successor's),
     * so one join summarizes the whole set — recomputed by the
     * membership test's own inputs, never by a cached path, so the
     * summary cannot disagree with the set it summarizes. M9's
     * independent recomputation checks exactly this claim. */
    for (i = 1; i <= k.prefix; i++) {
        const if5_intent* rec = &w->log[i - 1];
        if (rec->src == sid && (rec->flags & IF5_F_APPLIED))
            k.frontier = if5_vc_join(k.frontier, rec->vc);
    }
    return k;
}

/* Forward reachability: does `from` reach a record of S along PROV
 * edges, staying within the prefix? Iterative worklist; the log
 * is a DAG by construction (prov pointers reference earlier
 * records), so the walk terminates without a visited-set — but a
 * visited bitmap costs one bit per record and makes the worst case
 * linear instead of exponential, so it is carried. */
static bool if5__kcut_reaches(if5_world* w, uint32_t from, if5_sid sid,
                              uint32_t prefix)
{
    uint32_t stack[512];
    uint8_t* seen = NULL;
    uint32_t top = 0;
    bool answer = false;
    uint32_t i;

    if (from == 0 || from > prefix) return false;

    /* Fast path: S's own applied record is a member outright. */
    {
        const if5_intent* rec = &w->log[from - 1];
        if (rec->src == sid && (rec->flags & IF5_F_APPLIED)) return true;
    }

    seen = (uint8_t*)calloc(prefix + 1, 1);
    if (!seen) return false;

    stack[top++] = from;
    seen[from] = 1;
    while (top > 0 && !answer) {
        uint32_t cur = stack[--top];
        const if5_intent* rec = &w->log[cur - 1];

        if (rec->src == sid && (rec->flags & IF5_F_APPLIED)) {
            answer = true;
            break;
        }
        /* PROV forward: the records whose prov_a/prov_b IS cur.
         * The merge/cursor parents are the ONLY explicit edges —
         * exactly the evidence class if5_cause_edges enumerates
         * (the two modules share the edge definition by reading
         * the same fields; M2/M9 check them independently). */
        for (i = 1; i <= prefix; i++) {
            const if5_intent* q = &w->log[i - 1];
            if (seen[i]) continue;
            if (q->prov_a == cur || q->prov_b == cur) {
                seen[i] = 1;
                if (top < 512) stack[top++] = i;
                else { answer = false; goto out; }  /* bounded walk:
                                                     * refusal, not
                                                     * overflow */
            }
        }
    }
out:
    free(seen);
    return answer;
}

bool if5_kcut_contains(if5_world* w, const if5_kcut* k, uint32_t iid)
{
    if (!w || !k || k->sid >= w->n_sources) return false;
    if (iid == 0 || iid > k->prefix) return false;
    return if5__kcut_reaches(w, iid, k->sid, k->prefix);
}

bool if5_kcut_is_down_closed(if5_world* w, const if5_kcut* k)
{
    uint32_t i;

    if (!w || !k) return false;
    /* Independent verification: for every record in the prefix,
     * membership implies the PROV predecessors' membership — the
     * recursive form of the constructor's argument, checked
     * against the constructor rather than shared with it (M9's
     * demand). */
    for (i = 1; i <= k->prefix; i++) {
        const if5_intent* rec;
        if (!if5_kcut_contains(w, k, i)) continue;
        rec = &w->log[i - 1];
        if (rec->prov_a && !if5_kcut_contains(w, k, rec->prov_a))
            return false;
        if (rec->prov_b && !if5_kcut_contains(w, k, rec->prov_b))
            return false;
    }
    return true;
}

uint64_t if5_kcut_state_at(if5_world* w, const if5_kcut* k,
                           uint16_t rid, uint32_t* contrib)
{
    if5_region* r;
    const if5_payload_type* ty;
    bool lattice, forked;
    uint32_t head = 0, n = 0, i;
    uint64_t acc = 0;

    if (contrib) *contrib = 0;
    if (!w || !k) return 0;
    r = if5__region(w, rid);
    if (!r) return 0;

    /* The fold over EXACTLY the member records — the same
     * per-record filter the core's fold applies (applied, live,
     * value-type, selected chain), with the VIEW tests replaced by
     * the kcut's membership test. The chain-selection question for
     * forked regions is answered by the ordinary selected head at
     * the equivalent prefix view — the members decide the rest. */
    {
        if5_view pv;
        memset(&pv, 0, sizeof pv);
        pv.cut = k->prefix;
        ty = if5__type(w, r->ptype);
        lattice = ty && ty->sq;
        forked = r->head_b != 0 && !lattice;
        if (forked) head = if5__selected_head(w, r, &pv);
    }

    for (i = 1; i <= k->prefix; i++) {
        const if5_intent* rec = &w->log[i - 1];
        if (rec->region != rid) continue;
        if (!(rec->flags & IF5_F_APPLIED)) continue;
        if (rec->flags & IF5_F_EXCLUDED) continue;
        if (!if5__is_value_type(rec->ptype)) continue;
        if (!if5_kcut_contains(w, k, i)) continue;
        if (forked) {
            if5_view pv;
            memset(&pv, 0, sizeof pv);
            pv.cut = k->prefix;
            if (!if5__on_selected_chain(w, head, rec)) continue;
        }

        if (lattice && ty->sq) {
            uint64_t out;
            (void)ty->sq(acc, ((uint64_t)rec->value[1] << 32) | rec->value[0], &out);
            acc = out;
        } else {
            acc = ((uint64_t)rec->value[1] << 32) | rec->value[0];
        }
        n++;
    }
    if (contrib) *contrib = n;
    return acc;
}

uint32_t if5_kcut_members(if5_world* w, const if5_kcut* k,
                          uint32_t* iids, uint32_t cap)
{
    uint32_t n = 0, i;

    if (!w || !k) return 0;
    for (i = 1; i <= k->prefix; i++) {
        if (!if5_kcut_contains(w, k, i)) continue;
        if (iids && n < cap) iids[n] = i;
        n++;
    }
    return n;
}
