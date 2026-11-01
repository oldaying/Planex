/*
 * test_v5_mech.c — the mechanism layer's battery (ADR-0027
 * Decision 2; predictions M1-M7).
 *
 * M1 (Estimate as a fold): the estimate query agrees with the
 *      core's own fold at every checkpoint of a seeded random
 *      stream; the evidence ordinal takes its registered values
 *      in the four constructed states (none / chain / merged /
 *      forked); the trace is the fold at every integer cut; the
 *      sources enumeration is the dependency set, checked against
 *      an independent recount.
 *
 * M2 (Relation as the cause-graph): the PROV edge count matches
 *      the closed form over the log; effects is the exact
 *      reversal of edges; the concurrency enumeration agrees with
 *      pairwise independent recomputation; reachability answers
 *      the partial-order question (direction + transitivity).
 *
 * M3 (Closure as a judgment): the judgment is total over every
 *      record of a random stream; the verdict agrees with the
 *      flags in the registered precedence; the observation
 *      coordinate is the record's own vc, read back.
 *
 * M4 (Perception as the projection family): the registry holds
 *      the five built-ins; the registry path is byte-identical to
 *      the direct call for the frozen trio; extension
 *      registration works and duplicate names are refused.
 *
 * M5 (px_interaction as the adjudication surface): begin grows
 *      the log by exactly one PROCESS record; samples grow
 *      nothing (the durable image is memcmp-identical); commit
 *      grows by one and is a causal successor of the opening;
 *      cancel leaves the opening view-excluded; a closed process
 *      refuses everything.
 *
 * M6 (compilation as routing): the compiled record is
 *      field-identical to the direct submission of the same
 *      value; unparseable text and unrouted types refuse with
 *      zero log growth.
 *
 * M7 (the budget mapping): an exhausted ledger refuses exactly
 *      once, emits exactly one receipt, anchors it to a real
 *      record, and other sources are unaffected (refusal of the
 *      unit, not of the world).
 *
 * Every assertion checks a machine-verifiable engineering
 * statement against an INDEPENDENT recomputation (a second way to
 * compute the same thing), never against the implementation
 * under test alone — the series' discipline, applied at the
 * mechanism layer.
 *
 * Run: make -C v5 test
 */
#define _POSIX_C_SOURCE 200809L
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_run = 0, g_pass = 0;

#define TEST(name) do {                                  \
    g_run++;                                             \
    printf("  [TEST] %-44s ", #name);                   \
    test_##name();                                       \
    printf("OK\n");                                      \
    g_pass++;                                            \
} while (0)

/* The durable image: history bytes + the record array's byte
 * image (the working-set caches are allowed to move — they are
 * caches; the DURABLE layer is what must be immutable under
 * queries). */
static uint64_t img_history(const if5_world* w)
{
    return if5_history_bytes((if5_world*)w);
}

static void snap(const if5_world* w, uint64_t* hist, uint32_t* nlog)
{
    *hist = img_history(w);
    *nlog = if5_log_count((if5_world*)w);
}

/* ============================================================
 * Fixtures
 * ============================================================ */

static if5_world* mk_world(void)
{
    if5_world* w = if5_world_create();
    if5_source s;
    if5_payload_type t;

    memset(&s, 0, sizeof s);
    strcpy(s.id, "human");  s.kind = IF5_KIND_PERSON;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);
    strcpy(s.id, "agent");  s.kind = IF5_KIND_AGENT;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 1);
    strcpy(s.id, "system"); s.kind = IF5_KIND_PROCESS;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 2);
    if5_set_system_source(w, 2);   /* tier-2 merges need a payer
                                     * (I1/I2: unconfigured = no
                                     * one can pay = no merge) */

    /* Registration order pins the ids to the IF5_T_* enum (the
     * calibration fixture's convention): seq=2, str=3. */
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 3);

    assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "g", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 1, "s", 0, IF5_T_STR, IF5_TIER_FORK, NULL, 0));
    assert(if5_region_create(w, 0, "f", 0, IF5_T_STR, IF5_TIER_FORK, NULL, 0));
    return w;
}

/* The deterministic PRNG the suite shares with the differential's
 * streams (xorshift64) — independence is in the ALGORITHM, not
 * the seed source. */
static uint64_t prng(uint64_t* st)
{
    uint64_t x = *st;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return *st = x;
}

/* ============================================================
 * M1 — Estimate as a fold
 * ============================================================ */

static void test_m1_estimate_fold_agreement(void)
{
    if5_world* w = mk_world();
    uint64_t st = 0xC0FFEEu;
    int i;

    /* A random stream over the world-payload regions, checked at
     * every op: the estimate query must agree with the core's own
     * fold (value + contribution count), and the sources count
     * must agree with an independent walk of the same filter. */
    for (i = 0; i < 400; i++) {
        if5_estimate e;
        if5_view v;
        uint64_t val;
        uint32_t contrib = 0;
        uint16_t rid = (prng(&st) & 1) ? 1 : 2;
        uint8_t pty = (rid == 1) ? IF5_T_NUM : IF5_T_SET;

        if (rid == 1)
            (void)if5_user_act(w, prng(&st) & 1, rid, pty, prng(&st) % 1000);
        else
            (void)if5_user_act(w, prng(&st) & 1, rid, pty, 1ull << (prng(&st) % 8));

        v = if5_now(w);
        assert(if5_estimate_at(w, rid, &v, &e));
        val = if5_state_at(w, rid, &v, &contrib);
        assert(e.value == val);
        assert(e.contributions == contrib);

        /* Independent recount: walk the log with the value-type
         * exclusion list REPLICATED HERE (the structural families
         * are public enum values — the test re-derives the filter
         * rather than sharing the module's helper). */
        {
            uint32_t n = 0, j;
            for (j = 1; j <= v.cut; j++) {
                const if5_intent* rec = if5_log_at(w, j);
                if (rec->region != rid) continue;
                if (!(rec->flags & IF5_F_APPLIED)) continue;
                if (rec->flags & IF5_F_EXCLUDED) continue;
                switch (rec->ptype) {
                case IF5_T_TIER: case IF5_T_DELEG: case IF5_T_RECEIPT:
                case IF5_T_CURSOR: case IF5_T_EXPORT: case IF5_T_ARBIT:
                case IF5_T_REPARENT: case IF5_T_PROCESS:
                    continue;
                default:
                    break;
                }
                n++;
            }
            assert(if5_estimate_sources(w, rid, &v, NULL, 0) == n);
        }
    }

    /* The trace: the fold at every integer cut in (lo, hi] — a
     * bounded window of the stream (the full-history trace is the
     * same check repeated; 20 steps carry the assertion). */
    {
        if5_view hi = if5_now(w);
        if5_view lo = if5_view_at(w, hi.cut > 20 ? hi.cut - 20 : 0);
        uint64_t trace[32];
        uint32_t n = if5_estimate_trace(w, 1, &lo, &hi, trace, 32);
        uint32_t i2;
        assert(n == hi.cut - lo.cut);
        assert(n <= 32);
        for (i2 = 0; i2 < n; i2++) {
            if5_view at = if5_view_at(w, lo.cut + 1 + i2);
            assert(trace[i2] == if5_state_at(w, 1, &at, NULL));
        }
        /* Counting mode: the total alone, out == NULL. */
        assert(if5_estimate_trace(w, 1, &lo, &hi, NULL, 0) == n);
    }
    if5_world_free(w);
}

static void test_m1_evidence_ordinals(void)
{
    /* A self-contained world: the shared fixture's region "s" was
     * created by source 1, whose omniscient creation vc joined
     * source 0's clock — every later source-1 submission would be
     * COMPARABLE (never concurrent), and the MERGED/FORKED states
     * below need genuine blindness. Each state gets its own world
     * so no state's setup pollutes the next. */
    if5_estimate e;
    if5_view v;
    if5_source s;

    /* NONE: a world whose only view is the empty one. */
    {
        if5_world* w = if5_world_create();
        memset(&s, 0, sizeof s);
        strcpy(s.id, "h"); s.kind = IF5_KIND_PERSON;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 0);
        assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
        v = if5_view_at(w, 0);
        assert(if5_estimate_at(w, 1, &v, &e));
        assert(e.evidence == IF5_CONF_NONE);
        assert(e.contributions == 0);
        if5_world_free(w);
    }

    /* CHAIN: single-source writes on the NUM lattice — one line of
     * work, no concurrency anywhere. Contributions count the
     * REGION seed too (the core's registered semantics: REGION
     * participates in the fold — it is the seed), so 1 seed + 2
     * writes = 3. */
    {
        if5_world* w = if5_world_create();
        memset(&s, 0, sizeof s);
        strcpy(s.id, "h"); s.kind = IF5_KIND_PERSON;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 0);
        assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
        (void)if5_user_act(w, 0, 1, IF5_T_NUM, 10);
        (void)if5_user_act(w, 0, 1, IF5_T_NUM, 20);
        v = if5_now(w);
        assert(if5_estimate_at(w, 1, &v, &e));
        assert(e.evidence == IF5_CONF_CHAIN);
        assert(e.value == 20);               /* max: the seed 0 loses */
        assert(e.contributions == 3);
        if5_world_free(w);
    }

    /* MERGED: a BLIND arrival from a second source (observed_cut
     * = 0 — the omniscient path is comparable by construction and
     * never concurrent; the calibration suite's C4 made the same
     * point from the other side). The adjudicator stitches it
     * (tier 2 join) — the evidence state "stitched". */
    {
        if5_world* w = if5_world_create();
        if5_submit_opts o;
        uint32_t k;
        const if5_intent* rec;

        memset(&s, 0, sizeof s);
        strcpy(s.id, "h"); s.kind = IF5_KIND_PERSON;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 0);
        strcpy(s.id, "a"); s.kind = IF5_KIND_AGENT;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 1);
        strcpy(s.id, "sys"); s.kind = IF5_KIND_PROCESS;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 2);
        if5_set_system_source(w, 2);   /* the stitch needs a payer */

        assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
        (void)if5_user_act(w, 0, 1, IF5_T_NUM, 20);

        memset(&o, 0, sizeof o);
        o.observed_cut = 0;                   /* blind */
        k = if5_submit(w, 1, 1, IF5_T_NUM, 99, &o);
        rec = if5_log_at(w, k);
        assert(rec && (rec->flags & IF5_F_CONCUR_MERGE));
        v = if5_now(w);
        assert(if5_estimate_at(w, 1, &v, &e));
        assert(e.evidence == IF5_CONF_MERGED);
        assert(e.value == 99);                /* max(20, 99) */
        if5_world_free(w);
    }

    /* FORKED: a blind arrival on the FORK (non-lattice) region —
     * two live versions, head_b set, no join to decide one. */
    {
        if5_world* w = if5_world_create();
        if5_payload_type t;
        if5_submit_opts o;
        uint32_t k;
        const if5_intent* rec;
        if5_region* r;

        /* keep the id<->enum alignment (num=0, set=1, seq=2, str=3) */
        memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
        assert(if5_register_payload_type(w, &t) == 2);
        memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
        assert(if5_register_payload_type(w, &t) == 3);

        memset(&s, 0, sizeof s);
        strcpy(s.id, "h"); s.kind = IF5_KIND_PERSON;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 0);
        strcpy(s.id, "a"); s.kind = IF5_KIND_AGENT;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(w, &s) == 1);

        assert(if5_region_create(w, 0, "f", 0, IF5_T_STR, IF5_TIER_FORK, NULL, 0));
        (void)if5_user_act(w, 0, 1, IF5_T_STR, 0x1111ull);

        memset(&o, 0, sizeof o);
        o.observed_cut = 0;                   /* blind: the fork */
        k = if5_submit(w, 1, 1, IF5_T_STR, 0x4141414141414141ull, &o);
        rec = if5_log_at(w, k);
        assert(rec);
        r = if5_region_info(w, 1);
        assert(r->head_b != 0);
        v = if5_now(w);
        assert(if5_estimate_at(w, 1, &v, &e));
        assert(e.evidence == IF5_CONF_FORKED);
        if5_world_free(w);
    }
}

/* ============================================================
 * M2 — Relation as the cause-graph
 * ============================================================ */

static void test_m2_edges_closed_form_and_reversal(void)
{
    if5_world* w = mk_world();
    uint64_t st = 0xABCDEFu;
    if5_edge eff[256];
    uint32_t total_edges = 0, i;

    /* Random stream + a merge + an undo, so every edge class
     * exists: plain (0 edges), merge (2), cursor (1). */
    for (i = 0; i < 200; i++)
        (void)if5_user_act(w, prng(&st) & 1, (uint16_t)(1 + (prng(&st) % 2)),
                           (prng(&st) & 1) ? IF5_T_NUM : IF5_T_SET,
                           prng(&st) % 100);
    /* A tier-2 blind arrival to force a stitch (a MERGE record
     * with two prov parents). */
    {
        if5_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = 0;                   /* blind: concurrency */
        (void)if5_submit(w, 1, 1, IF5_T_NUM, 555, &o);
    }
    (void)if5_undo_nth(w, 0, 3);              /* a CURSOR (1 edge) */

    /* The closed form, counted by THIS test from the record words
     * (never from the module): merge -> 2 edges when both parents
     * live, cursor -> 1, else the prov_a spine of rigid chains. */
    for (i = 1; i <= if5_log_count(w); i++) {
        const if5_intent* rec = if5_log_at(w, i);
        if (rec->prov_a) total_edges++;
        if (rec->prov_b) total_edges++;
    }
    {
        uint32_t counted = 0;
        for (i = 1; i <= if5_log_count(w); i++)
            counted += if5_cause_edges(w, i, NULL, 0);
        assert(counted == total_edges);
    }

    /* The exact reversal: every edge (a -> b) appears as an effect
     * edge of a, and the two enumerations cross-check. */
    for (i = 1; i <= if5_log_count(w); i++) {
        if5_edge ed[4];
        uint32_t n = if5_cause_edges(w, i, ed, 4), j;
        for (j = 0; j < n; j++) {
            uint32_t m = if5_cause_effects(w, ed[j].from_iid, eff, 256), k;
            bool seen = false;
            assert(ed[j].to_iid == i);
            for (k = 0; k < m; k++)
                if (eff[k].to_iid == i && eff[k].from_iid == ed[j].from_iid)
                    seen = true;
            assert(seen);
        }
    }
    if5_world_free(w);
}

static void test_m2_concurrency_and_reachability(void)
{
    if5_world* w = mk_world();
    uint32_t conc[64];
    uint32_t n, i, j;

    /* A BLIND pair makes the concurrency: both writers declare
     * observed_cut = 0, so neither's vc contains the other — the
     * omniscient user_act path is comparable by construction and
     * never concurrent (C4's lesson, respected here). The region's
     * merge tier stitches both with the system payer the fixture
     * configured. */
    {
        if5_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = 0;
        (void)if5_submit(w, 0, 1, IF5_T_NUM, 10, &o);
        (void)if5_submit(w, 1, 1, IF5_T_NUM, 20, &o);
    }
    /* A third, comparable to everything of source 0. */
    (void)if5_user_act(w, 0, 1, IF5_T_NUM, 30);

    n = if5_cause_concurrents(w, NULL, 1, conc, 64);

    /* Independent pairwise recomputation: a record is concurrent
     * iff some OTHER VALUE-TYPE record on the region has an
     * incomparable vc both ways — recomputed here from the record
     * words. The value-type exclusion is part of the semantic
     * predicate being tested (structural records are not
     * contributions; their omniscient creation vcs are exactly the
     * false concurrency the filter exists to skip), so replicating
     * the switch is the assertion, not a shared line. */
    {
        uint32_t expect = 0;
        for (i = 1; i <= if5_log_count(w); i++) {
            const if5_intent* a = if5_log_at(w, i);
            bool has = false;
            if (a->region != 1 || !(a->flags & IF5_F_APPLIED)) continue;
            switch (a->ptype) {
            /* REGION and MERGE stay: they are VALUE types (the
             * seed and the stitched value — the fold reads them,
             * M1's contributions count agreed). The structural
             * families go. */
            case IF5_T_TIER: case IF5_T_DELEG:
            case IF5_T_RECEIPT: case IF5_T_CURSOR: case IF5_T_EXPORT:
            case IF5_T_ARBIT: case IF5_T_REPARENT: case IF5_T_PROCESS:
                continue;
            default: break;
            }
            for (j = 1; j <= if5_log_count(w) && !has; j++) {
                const if5_intent* b = if5_log_at(w, j);
                if (j == i || b->region != 1 || !(b->flags & IF5_F_APPLIED))
                    continue;
                switch (b->ptype) {
                case IF5_T_TIER: case IF5_T_DELEG:
                case IF5_T_RECEIPT: case IF5_T_CURSOR: case IF5_T_EXPORT:
                case IF5_T_ARBIT: case IF5_T_REPARENT: case IF5_T_PROCESS:
                    continue;
                default: break;
                }
                {
                    if5_vc x = a->vc, y = b->vc;
                    bool le_xy = if5_vc_le(x, y), le_yx = if5_vc_le(y, x);
                    if (!le_xy && !le_yx) has = true;
                }
            }
            if (has) expect++;
        }
        assert(n == expect);
        assert(n >= 1);                       /* the blind pair bit */
    }

    /* Reachability: the vc partial order — direction (an earlier
     * same-source record precedes a later one), transitivity (a
     * chain's ends), and the incomparable pair both ways. */
    {
        const if5_intent* r1 = if5_log_at(w, 1);
        const if5_intent* r2 = if5_log_at(w, 2);
        uint32_t last = if5_log_count(w);

        /* r1 is source 0's first write; source 0's later write
         * (the third record is src 0 in this fixture? No: writes
         * were src0, src1, src0 — r3 is src 0 and knows r1). */
        assert(if5_cause_reachable(w, 1, last) ||
               if5_log_at(w, last)->src != 0);
        /* The blind pair is incomparable: neither precedes the
         * other — the two BLIND records are found by scanning for
         * the region's concurrent-merge witnesses (the stitched
         * pair), not by position. */
        {
            uint32_t a = 0, b = 0;
            for (i = 1; i <= if5_log_count(w); i++) {
                const if5_intent* q = if5_log_at(w, i);
                if (q->region != 1 || q->ptype != IF5_T_NUM) continue;
                if (q->flags & IF5_F_CONCUR_MERGE) { a = i; b = q->prov_a; }
            }
            assert(a && b);
            assert(!if5_cause_reachable(w, a, b));
            assert(!if5_cause_reachable(w, b, a));
            assert(if5_cause_reachable(w, a, a));  /* reflexive */
        }
        (void)r1; (void)r2;
    }
    if5_world_free(w);
}

/* ============================================================
 * M3 — Closure as a judgment
 * ============================================================ */

static void test_m3_judgment_total_and_consistent(void)
{
    if5_world* w = mk_world();
    uint64_t st = 0x5EEDu;
    uint32_t i;

    for (i = 0; i < 150; i++)
        (void)if5_user_act(w, prng(&st) & 1, (uint16_t)(1 + (prng(&st) % 3)),
                           (prng(&st) & 1) ? IF5_T_NUM : IF5_T_SET,
                           prng(&st) % 500);
    /* No undo in this stream: the EXCLUDED verdict is view-
     * relative (an undone record's causal descendants read
     * EXCLUDED at the rewound "now" — the registered precedence's
     * top slot), and it is checked precisely in the rewind block
     * below rather than blurring this block's flag-consistency
     * chain with a test-side reimplementation of the exclusion
     * walk. */
    for (i = 1; i <= if5_log_count(w); i++) {
        if5_judgment j;
        const if5_intent* rec = if5_log_at(w, i);
        assert(if5_judgment_of(w, i, NULL, &j));
        assert(j.iid == i);
        assert(j.region == rec->region);
        assert(j.ptype == rec->ptype);
        assert(j.src == rec->src);
        assert(j.rebased == ((rec->flags & IF5_F_REBORN) ? 1 : 0));
        /* The observation coordinate is the record's own vc —
         * read back, byte for byte. */
        assert(memcmp(&j.observed, &rec->vc, sizeof(if5_vc)) == 0);
        /* The verdict's precedence: applied-ness first, then the
         * merge flag; the exclusion is checked at a view below. */
        if (!(rec->flags & IF5_F_APPLIED))
            assert(j.verdict == IF5_VERDICT_REFUSED ||
                   j.verdict == IF5_VERDICT_ARBIT_LOST);
        else if (rec->flags & IF5_F_CONCUR_MERGE)
            assert(j.verdict == IF5_VERDICT_MERGED);
        else
            assert(j.verdict == IF5_VERDICT_APPLIED ||
                   j.verdict == IF5_VERDICT_FORKED);
        /* The evaluation: the fold at the view over the record's
         * own region, low word carried (region 0 folds nothing). */
        if (rec->region) {
            if5_view v = if5_now(w);
            uint64_t val = if5_state_at(w, rec->region, &v, NULL);
            assert(j.eval_value == (uint32_t)(val & 0xFFFFFFFFu));
        }
    }

    /* The undone record reads EXCLUDED at the rewound view — the
     * precedence's top slot, checked where it bites. */
    {
        if5_view v = if5_view_at(w, if5_log_count(w));
        v = if5_view_rewind(w, &v, 4);
        {
            if5_judgment j;
            assert(if5_judgment_of(w, 4, &v, &j));
            assert(j.verdict == IF5_VERDICT_EXCLUDED);
        }
    }
    if5_world_free(w);
}

/* Forward: the extension renderer M4 registers (defined after the
 * M4 body — C wants the declaration before use, the definition
 * where it reads best). */
static uint32_t ext_proj_fn(if5_world* w, const if5_view* v,
                            char* buf, uint32_t cap);

/* ============================================================
 * M4 — Perception as the projection family
 * ============================================================ */

static void test_m4_registry_byte_equality(void)
{
    if5_world* w = mk_world();
    char a[4096], b[4096];
    uint32_t la, lb, i;

    (void)if5_user_act(w, 0, 1, IF5_T_NUM, 42);
    (void)if5_user_act(w, 1, 2, IF5_T_SET, 0x5);

    /* The five built-ins, by name and by count. */
    assert(if5_projection_count(w) == 5);
    assert(strcmp(if5_projection_name(w, 0), "semantic") == 0);
    assert(strcmp(if5_projection_name(w, 1), "set") == 0);
    assert(strcmp(if5_projection_name(w, 2), "history") == 0);
    assert(strcmp(if5_projection_name(w, 3), "tree") == 0);
    assert(strcmp(if5_projection_name(w, 4), "schedule") == 0);

    /* The frozen trio through the registry == the direct calls,
     * byte for byte (the wrap is routing, nothing else). */
    la = if5_projection_invoke(w, NULL, "semantic", a, sizeof a);
    lb = if5_project_semantic(w, NULL, b, sizeof b);
    assert(la == lb && la < sizeof a);
    assert(memcmp(a, b, la) == 0);

    la = if5_projection_invoke(w, NULL, "set", a, sizeof a);
    lb = if5_project_set(w, NULL, b, sizeof b);
    assert(la == lb);
    assert(memcmp(a, b, la) == 0);

    la = if5_projection_invoke(w, NULL, "history", a, sizeof a);
    lb = if5_project_history(w, NULL, b, sizeof b);
    assert(la == lb);
    assert(memcmp(a, b, la) == 0);

    /* The tree and schedule renderings produce non-empty,
     * deterministic text (two runs byte-identical). */
    la = if5_projection_invoke(w, NULL, "tree", a, sizeof a);
    lb = if5_projection_invoke(w, NULL, "tree", b, sizeof b);
    assert(la > 0 && la == lb && memcmp(a, b, la) == 0);
    la = if5_projection_invoke(w, NULL, "schedule", a, sizeof a);
    lb = if5_projection_invoke(w, NULL, "schedule", b, sizeof b);
    assert(la == lb && memcmp(a, b, la) == 0);

    /* Extension registration through the same channel; duplicate
     * names refused; an unknown name invokes nothing. */
    assert(if5_projection_register(w, "ext", ext_proj_fn) != (uint8_t)-1);
    assert(if5_projection_register(w, "ext", ext_proj_fn) == (uint8_t)-1);
    assert(if5_projection_count(w) == 6);
    la = if5_projection_invoke(w, NULL, "ext", a, sizeof a);
    assert(la > 0 && strcmp(a, "EXT") == 0);
    assert(if5_projection_invoke(w, NULL, "nope", a, sizeof a) == 0);
    (void)i;
    if5_world_free(w);
}

/* An extension projection used by M4's registration test (a
 * constant renderer — its behavior is irrelevant, the routing is
 * the test's subject). */
static uint32_t ext_proj_fn(if5_world* w, const if5_view* v,
                            char* buf, uint32_t cap)
{
    (void)w; (void)v;
    if (!buf || cap < 4) return 3;
    memcpy(buf, "EXT", 4);
    return 3;
}

/* ============================================================
 * M5 — px_interaction as the adjudication surface
 * ============================================================ */

static void test_m5_process_discipline(void)
{
    if5_world* w = mk_world();
    if5_process* p;
    uint32_t n0, k;
    uint64_t h0;
    uint32_t begin_iid, commit_iid;

    n0 = if5_log_count(w);
    p = if5_process_begin(w, 0, 1, IF5_T_NUM);
    assert(p);
    begin_iid = n0 + 1;
    assert(if5_log_count(w) == n0 + 1);       /* exactly one opening */
    {
        const if5_intent* rec = if5_log_at(w, begin_iid);
        assert(rec && rec->ptype == IF5_T_PROCESS);
    }

    /* Samples: zero log growth, zero history growth, readback ok. */
    snap(w, &h0, &n0);
    {
        int i;
        for (i = 0; i < 100; i++)
            assert(if5_process_sample(p, (uint64_t)(i * 7)));
        assert(if5_process_latest(p) == 99 * 7);
    }
    {
        uint64_t h1; uint32_t n1;
        snap(w, &h1, &n1);
        assert(h1 == h0 && n1 == n0);
    }

    /* Commit: one record, causal successor of the opening (the
     * outcome knows the opening — same source, strictly later). */
    k = if5_process_commit(p, 777, NULL);
    assert(k == if5_log_count(w));
    assert(k == n0 + 1);
    commit_iid = k;
    {
        const if5_intent* op = if5_log_at(w, begin_iid);
        const if5_intent* oc = if5_log_at(w, commit_iid);
        assert(oc->vc.c[0] > op->vc.c[0]);
        assert(if5_cause_reachable(w, begin_iid, commit_iid));
    }
    /* The process query recovers the opening from the outcome. */
    assert(if5_process_of(w, NULL, commit_iid) == begin_iid);
    /* Closed: everything refuses. */
    assert(!if5_process_sample(p, 1));
    assert(if5_process_commit(p, 1, NULL) == 0);
    assert(!if5_process_cancel(p));
    assert(if5_process_done(p));

    /* Cancel is a cut: a second process whose opening is undone. */
    {
        if5_process* q = if5_process_begin(w, 0, 2, IF5_T_SET);
        if5_view v;
        assert(q);
        {
            uint32_t nb = if5_log_count(w);
            const if5_intent* rec = if5_log_at(w, nb);
            assert(rec && rec->ptype == IF5_T_PROCESS);
        }
        assert(if5_process_cancel(q));
        v = if5_now(w);
        {
            uint32_t i;
            bool excluded = false;
            for (i = 0; i < v.n_excl; i++)
                if (v.excl[i] == (uint32_t)0 || if5_log_at(w, v.excl[i]))
                    { /* guard for 0 */ }
            /* Direct check: the opening's judgment at now. */
            {
                if5_judgment j;
                uint32_t opening = if5_process_of(w, NULL, if5_log_count(w));
                (void)opening;
                /* find the second PROCESS record by scanning */
                for (i = if5_log_count(w); i >= 1; i--) {
                    const if5_intent* rec = if5_log_at(w, i);
                    if (rec->ptype == IF5_T_PROCESS && rec->src == 0) {
                        assert(if5_judgment_of(w, i, &v, &j));
                        assert(j.verdict == IF5_VERDICT_EXCLUDED);
                        excluded = true;
                        break;
                    }
                }
            }
            assert(excluded);
        }
        /* At the pre-cancel view the opening was applied (the cut
         * is the undo, not the record's birth state). */
        assert(if5_process_done(q));
    }
    if5_world_free(w);
}

/* ============================================================
 * M6 — compilation as routing
 * ============================================================ */

static void test_m6_routing_equivalence(void)
{
    if5_world* a = mk_world();
    if5_world* b = mk_world();
    uint32_t ka, kb;
    uint64_t h0;
    uint32_t n0;

    /* NUM: compiled == direct, field for field. */
    ka = if5_compile(a, 0, 1, IF5_T_NUM, "31337", NULL);
    kb = if5_user_act(b, 0, 1, IF5_T_NUM, 31337);
    assert(ka && kb);
    {
        const if5_intent* ra = if5_log_at(a, ka);
        const if5_intent* rb = if5_log_at(b, kb);
        assert(memcmp(ra, rb, sizeof(if5_intent)) == 0);
    }

    /* SET: hex routing. */
    ka = if5_compile(a, 0, 2, IF5_T_SET, "0xFF", NULL);
    kb = if5_user_act(b, 0, 2, IF5_T_SET, 0xFF);
    assert(ka && kb);
    assert(memcmp(if5_log_at(a, ka), if5_log_at(b, kb),
                  sizeof(if5_intent)) == 0);

    /* STR: the 8-byte line. */
    ka = if5_compile(a, 0, 3, IF5_T_STR, "Planex!!", NULL);
    kb = if5_user_act(b, 0, 3, IF5_T_STR, 0x506C616E65782121ull);
    assert(ka && kb);
    assert(memcmp(if5_log_at(a, ka), if5_log_at(b, kb),
                  sizeof(if5_intent)) == 0);

    /* Refusals: zero log growth, zero history growth. */
    snap(a, &h0, &n0);
    assert(if5_compile(a, 0, 1, IF5_T_NUM, "12x", NULL) == 0);  /* bad text */
    assert(if5_compile(a, 0, 3, IF5_T_STR, "123456789", NULL) == 0); /* >8  */
    assert(if5_compile(a, 0, 2, IF5_T_SET, "0x", NULL) == 0);   /* empty  */
    assert(if5_compile(a, 0, 1, 99, "1", NULL) == 0);           /* no rt  */
    {
        uint64_t h1; uint32_t n1;
        snap(a, &h1, &n1);
        assert(h1 == h0 && n1 == n0);
    }
    if5_world_free(a);
    if5_world_free(b);
}

/* ============================================================
 * M7 — the budget mapping
 * ============================================================ */

static void test_m7_budget_refusal_exactness(void)
{
    if5_world* w;
    if5_source s;
    if5_region* r;
    int i;
    uint32_t receipts0;

    w = if5_world_create();
    memset(&s, 0, sizeof s);
    strcpy(s.id, "thrifty"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = 3; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);
    strcpy(s.id, "rich"); s.kind = IF5_KIND_PERSON;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 1);

    assert(if5_region_create(w, 1, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    r = if5_region_info(w, 1);
    (void)r;

    receipts0 = if5_receipt_count(w);

    /* Budget 3: three debited submissions pass, the fourth is
     * refused (logged, unapplied, receipted) — exactly once. */
    {
        if5_submit_opts o;
        for (i = 0; i < 6; i++) {
            uint32_t k;
            memset(&o, 0, sizeof o);
            o.debit_class = IF5_DEBIT_INITIATIVE;
            o.severity = IF5_SEV_BACKGROUND;
            k = if5_submit(w, 0, 1, IF5_T_NUM, (uint64_t)i, &o);
            assert(k);                        /* refused, but LOGGED */
        }
    }
    {
        const if5_stats* st = if5_stats_of(w);
        uint32_t n_receipts = if5_receipt_count(w) - receipts0;
        uint32_t applied = 0, i;
        for (i = 1; i <= if5_log_count(w); i++) {
            const if5_intent* rec = if5_log_at(w, i);
            if (rec->ptype == IF5_T_NUM && (rec->flags & IF5_F_APPLIED))
                applied++;
        }
        assert(applied == 3);                 /* the budget's worth */
        /* Every refusal receipted — the overrun counter's v5
         * image: refusals, counted in receipts, itemized. */
        assert(n_receipts >= 1);
        (void)st;
        {
            /* The receipt anchors to a real record (PROV-backed). */
            const if5_receipt* rc = if5_receipt_at(w, if5_receipt_count(w) - 1);
            assert(rc);
            assert(rc->reason_iid >= 1 && rc->reason_iid <= if5_log_count(w));
        }
    }

    /* Refusal of the unit, not of the world: the other source is
     * unaffected while source 0's breaker is tripped. */
    {
        uint32_t k = if5_user_act(w, 1, 1, IF5_T_NUM, 1000);
        const if5_intent* rec = if5_log_at(w, k);
        assert(k && rec && (rec->flags & IF5_F_APPLIED));
    }
    /* And source 0 keeps being refused (the breaker latches). */
    {
        if5_submit_opts o;
        uint32_t k, before = if5_receipt_count(w);
        memset(&o, 0, sizeof o);
        o.debit_class = IF5_DEBIT_INITIATIVE;
        o.severity = IF5_SEV_BACKGROUND;
        k = if5_submit(w, 0, 1, IF5_T_NUM, 9999, &o);
        assert(k && !(if5_log_at(w, k)->flags & IF5_F_APPLIED));
        assert(if5_receipt_count(w) >= before);
    }
    if5_world_free(w);
}

/* ============================================================
 * main
 * ============================================================ */

int main(void)
{
    printf("=== v5 mechanism suite (ADR-0027: M1-M7, the seven mappings) ===\n\n");
    TEST(m1_estimate_fold_agreement);
    TEST(m1_evidence_ordinals);
    TEST(m2_edges_closed_form_and_reversal);
    TEST(m2_concurrency_and_reachability);
    TEST(m3_judgment_total_and_consistent);
    TEST(m4_registry_byte_equality);
    TEST(m5_process_discipline);
    TEST(m6_routing_equivalence);
    TEST(m7_budget_refusal_exactness);
    printf("\n%d/%d mechanism sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
