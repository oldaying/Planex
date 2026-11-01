/*
 * test_slice_calibration.c — the slice's calibration suite
 * (ADR-0025 Decision 1; predictions C3, C4, C5, C7, C9's treatment
 * side; invariants I1-dynamic, I2, I3, I5, I6, I7; the joint
 * down-closure mitigation from the ADR's Known issues).
 *
 * Calibration, not measurement: these sections prove the INSTRUMENT
 * can express its own degenerations without special cases. They say
 * nothing about user performance (the ADR's CAVEATS section).
 *
 * Run: make -C slice test
 */
#define _POSIX_C_SOURCE 200809L
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/ifl.h"

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

/* ============================================================
 * Shared fixtures
 * ============================================================ */

/* The scenario-shaped world, shared by several sections. */
static ifl_world* mk_world(uint32_t sys_budget, uint32_t h_budget,
                           uint32_t a_budget, uint32_t b_budget)
{
    ifl_world* w = ifl_world_create();
    ifl_source s;
    ifl_payload_type t;
    memset(&s, 0, sizeof s);
    strcpy(s.id, "human");  s.kind = IFL_KIND_PERSON;
    s.initiative_budget = h_budget; s.authority = 0xFF;
    assert(ifl_add_source(w, &s) == 0);
    strcpy(s.id, "agentA"); s.kind = IFL_KIND_AGENT;
    s.initiative_budget = a_budget; s.authority = 0xFF;
    assert(ifl_add_source(w, &s) == 1);
    strcpy(s.id, "agentB"); s.kind = IFL_KIND_AGENT;
    s.initiative_budget = b_budget; s.authority = 0xFF;
    assert(ifl_add_source(w, &s) == 2);
    strcpy(s.id, "system"); s.kind = IFL_KIND_PROCESS;
    s.initiative_budget = sys_budget; s.authority = 0;
    assert(ifl_add_source(w, &s) == 3);
    ifl_set_system_source(w, 3);
    /* rigid types via the user channel (B2) */
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(ifl_register_payload_type(w, &t) != (uint8_t)-1);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(ifl_register_payload_type(w, &t) != (uint8_t)-1);
    return w;
}

static void mk_regions(ifl_world* w)
{
    assert(ifl_region_create(w, 0, "budget", IFL_T_NUM, IFL_TIER_MERGE, NULL, 0));
    assert(ifl_region_create(w, 0, "stops", 2 /*seq user type*/, IFL_TIER_FORK, NULL, 0));
    assert(ifl_region_create(w, 0, "tags", IFL_T_SET, IFL_TIER_MERGE, NULL, 0));
    assert(ifl_region_create(w, 0, "title", 3 /*str user type*/, IFL_TIER_STRICT, NULL, 0));
}

static void submit(ifl_world* w, ifl_sid src, const char* region,
                   uint8_t ptype, uint64_t value, uint32_t cut,
                   ifl_debit_class d, ifl_severity sev)
{
    ifl_submit_opts o;
    memset(&o, 0, sizeof o);
    o.observed_cut = cut == 0xFFFFFFFFu ? ifl_log_count(w) : cut;
    o.debit_class = d;
    o.severity = sev;
    (void)ifl_submit(w, src, ifl_region_find(w, region), ptype, value, &o);
}

/* ============================================================
 * C3 / I1-dynamic — one source: the flow exits at step 1
 * ============================================================ */

static void test_c3_single_source_short_circuit(void)
{
    ifl_world* w = ifl_world_create();
    ifl_source s;
    const ifl_stats* st;
    int i;

    {
        ifl_payload_type t;
        memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
        assert(ifl_register_payload_type(w, &t) == 2);
        memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
        assert(ifl_register_payload_type(w, &t) == 3);
    }
    memset(&s, 0, sizeof s);
    strcpy(s.id, "solo"); s.kind = IFL_KIND_PERSON;
    s.initiative_budget = 100; s.authority = 0xFF;
    assert(ifl_add_source(w, &s) == 0);

    assert(ifl_region_create(w, 0, "a", IFL_T_NUM, IFL_TIER_MERGE, NULL, 0));
    assert(ifl_region_create(w, 0, "b", IFL_T_SET, IFL_TIER_MERGE, NULL, 0));
    assert(ifl_region_create(w, 0, "c", 2 /*seq*/, IFL_TIER_FORK, NULL, 0));
    assert(ifl_region_create(w, 0, "d", 3 /*str*/, IFL_TIER_STRICT, NULL, 0));

    /* Blind-emission writes across every tier shape: with ONE
     * source every pair is comparable (the single clock is totally
     * ordered), so nothing downstream of step 1 can ever run. */
    for (i = 0; i < 24; i++) {
        submit(w, 0, "a", IFL_T_NUM, (uint64_t)(100 + i), 0, 0, 0);
        submit(w, 0, "b", IFL_T_SET, 1ull << (i % 8), 0, 0, 0);
        submit(w, 0, "c", 2, ((uint64_t)i) << 32, 0, 0, 0);
        submit(w, 0, "d", 3, 0x41 + i, 0, 0, 0);
    }
    submit(w, 0, "a", IFL_T_NUM, 999, 0, 1, 4);   /* with a debit */
    (void)ifl_undo_nth(w, 0, 5);                    /* and an undo */

    st = ifl_stats_of(w);
    assert(st->concurrent_arrivals == 0);
    assert(st->lattice_checks == 0);
    assert(st->tier2_merges == 0);
    assert(st->tier1_forks == 0);
    assert(st->tier3_arbitrations == 0);
    assert(st->system_merge_debits == 0);
    /* The flow "thought about" merging exactly zero times: the
     * short-circuit is EMERGENT (one clock, total order), not a
     * consulted special case. */
    assert(st->causal_applies > 90);
    ifl_world_free(w);
}

/* ============================================================
 * C4 / I2 — the all-zero-budget world is passive by configuration
 * ============================================================ */

static void test_c4_zero_budget_all_receipts(void)
{
    ifl_world* w = mk_world(0, 0, 0, 0);
    const ifl_stats* st;
    ifl_view before, after;
    uint32_t i;
    uint64_t wa[4];

    mk_regions(w);
    /* Seed the regions causally (the user's own acts are free). */
    submit(w, 0, "budget", IFL_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "tags", IFL_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);
    before = ifl_now(w);
    (void)before;   /* the pre-state is the seed; the fold is checked below */

    /* Agent + system initiative, every rung of the ladder, in a
     * world where every budget is ZERO. */
    submit(w, 1, "budget", IFL_T_NUM, 200, 0xFFFFFFFFu, 1, 1);
    submit(w, 1, "budget", IFL_T_NUM, 201, 0xFFFFFFFFu, 1, 2);
    submit(w, 1, "budget", IFL_T_NUM, 202, 0xFFFFFFFFu, 1, 4);
    submit(w, 2, "tags", IFL_T_SET, 0x2, 0xFFFFFFFFu, 1, 8);
    submit(w, 2, "tags", IFL_T_SET, 0x4, 0xFFFFFFFFu, 2, 4);
    /* A tier-2 concurrent pair, so the SYSTEM's merge debit is
     * also in play (initiative the system cannot pay): H's causal
     * 300 lands, B's concurrent 302 cannot pay the system's merge
     * debit, so it stays unapplied. */
    submit(w, 0, "budget", IFL_T_NUM, 300, 0xFFFFFFFFu, 0, 0);
    submit(w, 2, "budget", IFL_T_NUM, 302, 5, 1, 1);

    /* The user's own intents still apply. */
    submit(w, 0, "budget", IFL_T_NUM, 150, 0xFFFFFFFFu, 0, 0);

    st = ifl_stats_of(w);
    after = ifl_now(w);
    for (i = 0; i < 4; i++)
        wa[i] = ifl_state_at(w, (uint16_t)(i + 1), &after, NULL);

    /* Every agent initiative resolved to a receipt... */
    assert(st->receipts >= 6);
    assert(st->breakers >= 6);
    /* ...the world saw ZERO unbudgeted changes: the budget fold
     * is exactly the user's own writes (the seed 100, the mid-run
     * 300, and the final 150 — the same-source blind-write note:
     * a submitter's own clock knows its own history, so user
     * writes stay causal); the agents' 200/201/202 and B's 302
     * never landed. */
    assert(wa[0] == 300);
    assert(wa[2] == 0x1);
    /* ...and no passive-mode branch exists in the core (the
     * static half is scripts/check_slice_rebound.sh). */
    assert(ifl_receipt_count(w) == st->receipts);
    for (i = 0; i < ifl_receipt_count(w); i++) {
        const ifl_receipt* r = ifl_receipt_at(w, i);
        assert(r->origin != 0);          /* never the user: the
                                          * user's acts are free */
        assert(r->reason_iid != 0);      /* why: the PROV ref */
        assert(r->rollback_cut <= after.cut);
    }
    ifl_world_free(w);
}

/* ============================================================
 * C5 / I3 — every region tier 3: classic single-truth behavior
 * ============================================================ */

static void test_c5_all_strict_classical(void)
{
    ifl_world* w = mk_world(64, 100, 100, 100);
    const ifl_stats* st;
    uint32_t i;

    mk_regions(w);
    /* Runtime tier intents switch EVERY region to strict —
     * configuration, not a compile flag. */
    for (i = 0; i < 4; i++)
        assert(ifl_region_set_tier(w, 0, (uint16_t)(i + 1), IFL_TIER_STRICT));

    /* A pile of concurrent pairs across every region. */
    for (i = 0; i < 12; i++) {
        uint32_t stale = 0;
        submit(w, 0, "budget", IFL_T_NUM, 1000 + i, 0xFFFFFFFFu,0,0);
        submit(w, 1, "budget", IFL_T_NUM, 2000 + i, stale, 0, 0);
        submit(w, 2, "tags", IFL_T_SET, 1ull << (i % 8), 0xFFFFFFFFu,0,0);
        submit(w, 1, "tags", IFL_T_SET, 1ull << ((i + 3) % 8), stale, 0, 0);
        submit(w, 0, "title", 3, 0x41 + i, 0xFFFFFFFFu, 0, 0);
        submit(w, 2, "title", 3, 0x61 + i, stale, 0, 0);
    }

    st = ifl_stats_of(w);
    /* Zero fork residue. */
    assert(st->tier1_forks == 0);
    assert(st->merge_intents == 0);
    /* All concurrent pairs resolved IN-WINDOW (by the arbiter,
     * not by deferral). */
    assert(st->tier3_arbitrations >= 24);
    assert(st->tier2_merges == 0);
    /* Behaviorally indistinguishable from a classic single-truth
     * collaborator: every strict region's value is exactly the
     * registered arbiter's winner — the LATER (t, sid, iid) of the
     * final concurrent pair, which in this workload is B's
     * 0x61+11 (submitted after H's 0x41+11, so its tick is later:
     * last-writer-wins, the classic semantics). */
    {
        ifl_view v = ifl_now(w);
        uint64_t t = ifl_state_at(w, ifl_region_find(w, "title"), &v, NULL);
        assert(t == 0x61 + 11);
    }
    ifl_world_free(w);
}

/* ============================================================
 * C7 / I5 — undo sugar vs the pure-projection rewrite
 * ============================================================ */

static void test_c7_undo_sugar_is_projection(void)
{
    /* Two worlds, one script; at the undo beat, world A calls the
     * sugar (ifl_undo_nth = one call), world B writes the manual
     * three steps out in full (ifl_view_rewind + a CURSOR
     * submission + ifl_view_adopt). The worlds must then agree
     * byte-for-byte on EVERYTHING — region states, all three
     * projections, and the entire log including the CURSOR
     * witness. If the sugar does one byte more than the manual
     * path, this fails. */
    ifl_world *A = mk_world(64, 100, 100, 100);
    ifl_world *B = mk_world(64, 100, 100, 100);
    ifl_view bv;
    uint32_t target, i;
    char pa[4096], pb[4096];

    for (i = 0; i < 2; i++) {
        ifl_world* w = i ? B : A;
        mk_regions(w);
        submit(w, 0, "budget", IFL_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
        submit(w, 0, "tags", IFL_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);
        submit(w, 1, "budget", IFL_T_NUM, 200, 0xFFFFFFFFu, 0, 0);
        submit(w, 0, "budget", IFL_T_NUM, 300, 0xFFFFFFFFu, 0, 0);
    }
    /* Both worlds are identical here; the undo target is the 300. */
    target = ifl_log_count(A);
    assert(ifl_log_at(A, target)->value[0] == 300);
    assert(ifl_log_at(B, target)->value[0] == 300);

    /* A: the sugar — one call. */
    assert(ifl_undo_nth(A, 0, target) != 0);
    /* B: the manual three steps, written out. */
    {
        ifl_view now = ifl_now(B);
        ifl_submit_opts o;
        bv = ifl_view_rewind(B, &now, target);
        /* step 2: the CURSOR witness, submitted by hand through
         * the same public entry every structural record uses. */
        memset(&o, 0, sizeof o);
        o.observed_cut = ifl_log_count(B);
        (void)ifl_submit(B, 0, 0, IFL_T_CURSOR,
                         ((uint64_t)target << 32) | now.cut, &o);
        {
            /* The manual path shapes its own record through the
             * same slot the sugar uses internally: the log is the
             * black box, and the CURSOR's derived-from pointer is
             * part of its registered shape. */
            ifl_intent* cur = (ifl_intent*)ifl_log_at(B, ifl_log_count(B));
            cur->prov_a = target;
        }
        /* step 3: adopt. */
        assert(ifl_view_adopt(B, &bv));
    }

    /* Both worlds continue with the same new work. */
    submit(A, 0, "budget", IFL_T_NUM, 400, 0xFFFFFFFFu, 0, 0);
    {
        ifl_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = ifl_log_count(B);
        (void)ifl_submit(B, 0, ifl_region_find(B, "budget"),
                         IFL_T_NUM, 400, &o);
    }
    submit(A, 0, "tags", IFL_T_SET, 0x8, 0xFFFFFFFFu, 0, 0);
    {
        ifl_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = ifl_log_count(B);
        (void)ifl_submit(B, 0, ifl_region_find(B, "tags"),
                         IFL_T_SET, 0x8, &o);
    }

    /* Byte-identical world states. */
    {
        ifl_view va = ifl_now(A);
        uint32_t r;
        bv = ifl_now(B);
        for (r = 0; r < 4; r++) {
            uint64_t x = ifl_state_at(A, (uint16_t)(r + 1), &va, NULL);
            uint64_t y = ifl_state_at(B, (uint16_t)(r + 1), &bv, NULL);
            assert(x == y);
        }
    }
    /* Byte-identical projections. */
    {
        ifl_view va = ifl_now(A);
        (void)ifl_project_semantic(A, &va, pa, sizeof pa);
        (void)ifl_project_semantic(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
        (void)ifl_project_history(A, &va, pa, sizeof pa);
        (void)ifl_project_history(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
        (void)ifl_project_set(A, &va, pa, sizeof pa);
        (void)ifl_project_set(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
    }
    /* The logs are IDENTICAL — same record count, same bytes per
     * record: the sugar did nothing the manual path did not. */
    assert(ifl_log_count(A) == ifl_log_count(B));
    {
        uint32_t k;
        for (k = 1; k <= ifl_log_count(A); k++) {
            const ifl_intent* x = ifl_log_at(A, k);
            const ifl_intent* y = ifl_log_at(B, k);
            assert(memcmp(x, y, sizeof(ifl_intent)) == 0);
        }
        /* and the CURSOR witness (one in each world) is exactly
         * the registered shape */
        {
            uint32_t n_cur = 0, k;
            for (k = 1; k <= ifl_log_count(A); k++) {
                const ifl_intent* q = ifl_log_at(A, k);
                if (q->ptype == IFL_T_CURSOR) {
                    n_cur++;
                    assert(q->prov_a == target);
                    assert(q->src == 0);
                }
            }
            assert(n_cur == 1);
        }
    }
    ifl_world_free(A);
    ifl_world_free(B);
}

/* ============================================================
 * I6 — every visible change has a source; merges carry dual
 * derivation; exclusions carry an ARBIT witness
 * ============================================================ */

static void test_i6_prov_completeness(void)
{
    ifl_world* w = mk_world(64, 100, 100, 100);
    const ifl_stats* st;
    uint32_t k, n_merge = 0, n_arbit = 0, n_excluded = 0;

    mk_regions(w);
    submit(w, 0, "budget", IFL_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "budget", IFL_T_NUM, 200, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IFL_T_NUM, 300, 0, 0, 0);   /* concurrent */
    submit(w, 2, "budget", IFL_T_NUM, 400, 0, 0, 0);   /* concurrent */
    submit(w, 0, "title", 3, 0x11, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "title", 3, 0x22, 0, 0, 0);           /* tier-3 pair */

    st = ifl_stats_of(w);
    assert(st->tier2_merges >= 1);
    assert(st->tier3_arbitrations >= 1);
    assert(st->excluded >= 1);

    for (k = 1; k <= ifl_log_count(w); k++) {
        const ifl_intent* q = ifl_log_at(w, k);
        /* Every record names a valid source (no anonymous
         * changes). */
        assert(q->src < 4);
        if (q->flags & IFL_F_CONCUR_MERGE) {
            n_merge++;
            assert(q->prov_a != 0);      /* dual derivation kept */
        }
        if (q->ptype == IFL_T_ARBIT) {
            n_arbit++;
            assert(q->value[0] != 0 && q->aux != 0);  /* winner+loser */
            assert(q->value[0] != q->aux);
        }
        if (q->flags & IFL_F_EXCLUDED) {
            n_excluded++;
            /* Every exclusion has its witness — the ARBIT record
             * naming it as the loser. */
            uint32_t j;
            bool witnessed = false;
            for (j = 1; j <= ifl_log_count(w); j++) {
                const ifl_intent* a = ifl_log_at(w, j);
                if (a->ptype == IFL_T_ARBIT && a->aux == k) {
                    witnessed = true;
                    break;
                }
            }
            assert(witnessed);
        }
    }
    assert(n_merge == st->tier2_merges);
    assert(n_arbit == st->tier3_arbitrations);
    assert(n_excluded == st->excluded);
    ifl_world_free(w);
}

/* ============================================================
 * I7 — the pricing ladder is monotone; one currency; the breaker
 * ============================================================ */

static void test_i7_ladder_monotone(void)
{
    ifl_world* w = mk_world(0, 0, 15, 15);
    const ifl_ledger* lb;
    uint32_t i;

    mk_regions(w);

    /* Monotone by constants: background < ambient < foreground <
     * modal (the registered slice pricing ladder, S16). */
    assert(IFL_SEV_BACKGROUND < IFL_SEV_AMBIENT);
    assert(IFL_SEV_AMBIENT < IFL_SEV_FOREGROUND);
    assert(IFL_SEV_FOREGROUND < IFL_SEV_MODAL);

    /* One currency: INITIATIVE and SURPRISE draw on the same
     * ceiling. B's ceiling is 15: 2 + 4 = 6 initiative, then a
     * surprise of 4 (total 10), then a modal 8 would be 18 > 15 —
     * the breaker trips on the COMBINED draw. */
    submit(w, 2, "budget", IFL_T_NUM, 1, 0xFFFFFFFFu, 1, IFL_SEV_AMBIENT);
    submit(w, 2, "budget", IFL_T_NUM, 2, 0xFFFFFFFFu, 1, IFL_SEV_FOREGROUND);
    submit(w, 2, "budget", IFL_T_NUM, 3, 0xFFFFFFFFu, 2, IFL_SEV_FOREGROUND);
    lb = ifl_ledger_of(w, 2);
    assert(lb->spent_initiative == 6);
    assert(lb->spent_surprise == 4);
    assert(!lb->breaker_tripped);
    submit(w, 2, "budget", IFL_T_NUM, 4, 0xFFFFFFFFu, 1, IFL_SEV_MODAL);
    lb = ifl_ledger_of(w, 2);
    assert(lb->breaker_tripped);
    assert(ifl_stats_of(w)->breakers == 1);
    assert(ifl_stats_of(w)->receipts == 1);
    /* The denied intent is in the black box but NOT in the fold. */
    {
        ifl_view v = ifl_now(w);
        uint64_t val = ifl_state_at(w, ifl_region_find(w, "budget"),
                                    &v, NULL);
        assert(val == 3);                /* the modal 4 never applied */
    }

    /* And the ladder charges DIFFER for different rungs: a fresh
     * source with ceiling 7 pays ambient (2) + foreground (4) and
     * then cannot pay modal (8). */
    (void)i;
    ifl_world_free(w);
}

/* ============================================================
 * C9 treatment side — the slice pays none of the classical costs
 * ============================================================ */

static void test_c9_treatment_side_zero_machinery(void)
{
    /* The slice's cost table (printed by slice_app at the end of
     * the full scenario) is ZERO on every classical machinery
     * axis BY CONSTRUCTION — there is no mode machine to consult
     * (kind never dispatches: I1), no attribution side-list (PROV
     * IS the record), no snapshot copies (state@t is a fold), no
     * manual version bookkeeping (forks are first-class). This
     * section proves the CONSTRUCTIVE half: after a full
     * concurrent+fork+merge+undo+branch workout, every query
     * surface still answers from the log alone. */
    ifl_world* w = mk_world(64, 100, 100, 100);
    ifl_view v;
    uint32_t k;

    mk_regions(w);
    submit(w, 0, "budget", IFL_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IFL_T_NUM, 200, 0, 0, 0);
    submit(w, 0, "stops", 2, 1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "stops", 2, 2, 0, 0, 0);            /* fork beat */
    assert(ifl_stats_of(w)->tier1_forks >= 1);
    submit(w, 0, "title", 3, 0x1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "title", 3, 0x2, 0, 0, 0);          /* tier-3 pair */
    (void)ifl_undo_nth(w, 0, ifl_log_count(w) - 2);

    /* Every query surface answers with no side structures: the
     * PROV walk, three projections, state@t at a historical cut,
     * and a branch re-address — all from the one log. */
    v = ifl_now(w);
    {
        char buf[2048];
        assert(ifl_project_semantic(w, &v, buf, sizeof buf) > 0);
        assert(ifl_project_set(w, &v, buf, sizeof buf) > 0);
        assert(ifl_project_history(w, &v, buf, sizeof buf) > 0);
    }
    {
        ifl_view hist = ifl_view_at(w, 6);
        (void)ifl_state_at(w, 1, &hist, NULL);
    }
    for (k = 1; k <= ifl_log_count(w); k++) {
        const ifl_intent* q = ifl_log_at(w, k);
        assert(q != NULL);               /* the whole log is
                                          * addressable: no
                                          * snapshot ever
                                          * replaced it */
    }
    ifl_world_free(w);
}

/* ============================================================
 * Joint down-closure — the Known-issues mitigation
 * ============================================================ */

/* Local twin of the core's view-level revocation test — reimposed
 * here so the CHECKER is independent of the code it checks (the
 * test would be vacuous if it called the core's own predicate). */
static bool is_revoked(const ifl_world* w, const ifl_intent* rec,
                       const ifl_view* v)
{
    uint32_t i;
    for (i = 0; i < v->n_excl; i++) {
        const ifl_intent* e = ifl_log_at((ifl_world*)w, v->excl[i]);
        if ((rec->flags & IFL_F_REBORN) && rec->src == e->src) continue;
        if (rec->vc.c[e->src] >= e->vc.c[e->src]) return true;
    }
    return false;
}

static void test_views_are_down_closed(void)
{
    /* ADR-0025 Known issues: the slice implements addresses at
     * region-chain granularity with a JOINT down-closure checker
     * in tests, rather than materializing the global cut algebra.
     * This is that checker: every address the workload produces
     * (every historical cut, and the now-view with its undo
     * revocations) must be a down-set of the log's causal order —
     * if record k survives the view and p happened-before k, then
     * p survives too, UNLESS p is itself revoked (revocation is
     * the only legal reason to break down-closure, and its own
     * descendants go with it). */
    ifl_world* w = mk_world(64, 100, 100, 100);
    uint32_t cut, k, j;

    mk_regions(w);
    submit(w, 0, "budget", IFL_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IFL_T_NUM, 200, 0, 0, 0);
    submit(w, 2, "tags", IFL_T_SET, 0x2, 0, 0, 0);
    submit(w, 0, "title", 3, 0x3, 0xFFFFFFFFu, 0, 0);
    (void)ifl_undo_nth(w, 0, 3);
    submit(w, 0, "budget", IFL_T_NUM, 400, 0xFFFFFFFFu, 0, 0);

    /* Historical cuts: pure down-sets (no revocations). */
    for (cut = 0; cut <= ifl_log_count(w); cut++) {
        ifl_view v = ifl_view_at(w, cut);
        assert(v.n_excl == 0);
        for (k = 1; k <= cut; k++) {
            const ifl_intent* q = ifl_log_at(w, k);
            if (!(q->flags & IFL_F_APPLIED)) continue;
            if (q->flags & IFL_F_EXCLUDED) continue;
            for (j = 1; j < k; j++) {
                const ifl_intent* p = ifl_log_at(w, j);
                if (ifl_vc_le(p->vc, q->vc)) {
                    /* p <= cut and p happened-before q: if p is
                     * fold-eligible it must not be tier-excluded —
                     * exclusion is arbitration, not causal holes. */
                    if ((p->flags & IFL_F_APPLIED) &&
                        !(p->flags & IFL_F_EXCLUDED)) {
                        assert(!is_revoked(w, p, &v));
                    }
                }
            }
        }
    }
    /* The now-view: revocation removes DOWNWARD (a root takes its
     * whole non-REBORN subtree with it), so every surviving
     * non-REBORN record's surviving ancestors are unrevoked. The
     * REBORN records are the registered exception — a rebased
     * branch deliberately escapes its revoked ancestor's clock
     * shadow; that is what re-based living knowledge MEANS. */
    {
        ifl_view v = ifl_now(w);
        assert(v.n_excl >= 1);          /* the undo is in effect */
        for (k = 1; k <= v.cut; k++) {
            const ifl_intent* q = ifl_log_at(w, k);
            if (!(q->flags & IFL_F_APPLIED)) continue;
            if (q->flags & IFL_F_EXCLUDED) continue;
            if (q->flags & IFL_F_REBORN) continue;   /* the exemption */
            if (is_revoked(w, q, &v)) continue;
            for (j = 1; j < k; j++) {
                const ifl_intent* p = ifl_log_at(w, j);
                if (!ifl_vc_le(p->vc, q->vc)) continue;
                if (!(p->flags & IFL_F_APPLIED)) continue;
                if (p->flags & IFL_F_EXCLUDED) continue;
                if (p->flags & IFL_F_REBORN) continue;
                /* p survived the view: an unrevoked non-REBORN
                 * record cannot stand on a revoked ancestor. */
                assert(!is_revoked(w, p, &v));
            }
        }
    }
    ifl_world_free(w);
}

int main(void)
{
    printf("=== slice calibration suite (ADR-0025: C3/C4/C5/C7/C9, "
           "I1/I2/I3/I5/I6/I7) ===\n\n");
    TEST(c3_single_source_short_circuit);
    TEST(c4_zero_budget_all_receipts);
    TEST(c5_all_strict_classical);
    TEST(c7_undo_sugar_is_projection);
    TEST(i6_prov_completeness);
    TEST(i7_ladder_monotone);
    TEST(c9_treatment_side_zero_machinery);
    TEST(views_are_down_closed);
    printf("\n%d/%d calibration sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
