/*
 * test_v5_calibration.c — the v5 calibration suite: the slice's
 * battery (ADR-0025 Decision 1) re-derived on the promoted core
 * (ADR-0026 Decision 3a; predictions C3/C4/C5/C7/C9, invariants
 * I1/I2/I3/I5/I6/I7, and the down-closure checker — now backed by
 * the promoted public verifier if5_cut_is_down_closed).
 *
 * The V-series' claim (V7): the promotion changed none of the
 * instrument's calibration behavior. The DIFFERENTIAL
 * (test_v5_differential.c) is the stronger, cross-core form of the
 * same claim; this suite keeps v5 self-contained — a fresh checkout
 * can adjudicate v5's calibration without building the slice.
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

/* ============================================================
 * Shared fixtures (the slice's shapes, if5_ names, parent 0)
 * ============================================================ */

static if5_world* mk_world(uint32_t sys_budget, uint32_t h_budget,
                           uint32_t a_budget, uint32_t b_budget)
{
    if5_world* w = if5_world_create();
    if5_source s;
    if5_payload_type t;
    memset(&s, 0, sizeof s);
    strcpy(s.id, "human");  s.kind = IF5_KIND_PERSON;
    s.initiative_budget = h_budget; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);
    strcpy(s.id, "agentA"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = a_budget; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 1);
    strcpy(s.id, "agentB"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = b_budget; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 2);
    strcpy(s.id, "system"); s.kind = IF5_KIND_PROCESS;
    s.initiative_budget = sys_budget; s.authority = 0;
    assert(if5_add_source(w, &s) == 3);
    if5_set_system_source(w, 3);
    /* rigid types via the user channel (B2) */
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);
    return w;
}

static void mk_regions(if5_world* w)
{
    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "stops", 0, 2 /*seq user type*/, IF5_TIER_FORK, NULL, 0));
    assert(if5_region_create(w, 0, "tags", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "title", 0, 3 /*str user type*/, IF5_TIER_STRICT, NULL, 0));
}

static void submit(if5_world* w, if5_sid src, const char* region,
                   uint8_t ptype, uint64_t value, uint32_t cut,
                   if5_debit_class d, if5_severity sev)
{
    if5_submit_opts o;
    memset(&o, 0, sizeof o);
    o.observed_cut = cut == 0xFFFFFFFFu ? if5_log_count(w) : cut;
    o.debit_class = d;
    o.severity = sev;
    (void)if5_submit(w, src, if5_region_find(w, region), ptype, value, &o);
}

/* ============================================================
 * C3 / I1-dynamic — one source: the flow exits at step 1
 * ============================================================ */

static void test_c3_single_source_short_circuit(void)
{
    if5_world* w = if5_world_create();
    if5_source s;
    const if5_stats* st;
    int i;

    {
        if5_payload_type t;
        memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
        assert(if5_register_payload_type(w, &t) == 2);
        memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
        assert(if5_register_payload_type(w, &t) == 3);
    }
    memset(&s, 0, sizeof s);
    strcpy(s.id, "solo"); s.kind = IF5_KIND_PERSON;
    s.initiative_budget = 100; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);

    assert(if5_region_create(w, 0, "a", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "b", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "c", 0, 2 /*seq*/, IF5_TIER_FORK, NULL, 0));
    assert(if5_region_create(w, 0, "d", 0, 3 /*str*/, IF5_TIER_STRICT, NULL, 0));

    /* Blind-emission writes across every tier shape: with ONE
     * source every pair is comparable (the single clock is totally
     * ordered), so nothing downstream of step 1 can ever run. */
    for (i = 0; i < 24; i++) {
        submit(w, 0, "a", IF5_T_NUM, (uint64_t)(100 + i), 0, 0, 0);
        submit(w, 0, "b", IF5_T_SET, 1ull << (i % 8), 0, 0, 0);
        submit(w, 0, "c", 2, ((uint64_t)i) << 32, 0, 0, 0);
        submit(w, 0, "d", 3, (uint64_t)(0x41 + i), 0, 0, 0);
    }
    submit(w, 0, "a", IF5_T_NUM, 999, 0, 1, 4);   /* with a debit */
    (void)if5_undo_nth(w, 0, 5);                    /* and an undo */

    st = if5_stats_of(w);
    assert(st->concurrent_arrivals == 0);
    assert(st->lattice_checks == 0);
    assert(st->tier2_merges == 0);
    assert(st->tier1_forks == 0);
    assert(st->tier3_arbitrations == 0);
    assert(st->system_merge_debits == 0);
    assert(st->causal_applies > 90);
    if5_world_free(w);
}

/* ============================================================
 * C4 / I2 — the all-zero-budget world is passive by configuration
 * ============================================================ */

static void test_c4_zero_budget_all_receipts(void)
{
    if5_world* w = mk_world(0, 0, 0, 0);
    const if5_stats* st;
    if5_view after;
    uint32_t i;
    uint64_t wa[4];

    mk_regions(w);
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "tags", IF5_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);

    submit(w, 1, "budget", IF5_T_NUM, 200, 0xFFFFFFFFu, 1, 1);
    submit(w, 1, "budget", IF5_T_NUM, 201, 0xFFFFFFFFu, 1, 2);
    submit(w, 1, "budget", IF5_T_NUM, 202, 0xFFFFFFFFu, 1, 4);
    submit(w, 2, "tags", IF5_T_SET, 0x2, 0xFFFFFFFFu, 1, 8);
    submit(w, 2, "tags", IF5_T_SET, 0x4, 0xFFFFFFFFu, 2, 4);
    submit(w, 0, "budget", IF5_T_NUM, 300, 0xFFFFFFFFu, 0, 0);
    submit(w, 2, "budget", IF5_T_NUM, 302, 5, 1, 1);
    submit(w, 0, "budget", IF5_T_NUM, 150, 0xFFFFFFFFu, 0, 0);

    st = if5_stats_of(w);
    after = if5_now(w);
    for (i = 0; i < 4; i++)
        wa[i] = if5_state_at(w, (uint16_t)(i + 1), &after, NULL);

    assert(st->receipts >= 6);
    assert(st->breakers >= 6);
    assert(wa[0] == 300);
    assert(wa[2] == 0x1);
    assert(if5_receipt_count(w) == st->receipts);
    for (i = 0; i < if5_receipt_count(w); i++) {
        const if5_receipt* r = if5_receipt_at(w, i);
        assert(r->origin != 0);
        assert(r->reason_iid != 0);
        assert(r->rollback_cut <= after.cut);
    }
    if5_world_free(w);
}

/* ============================================================
 * C5 / I3 — every region tier 3: classic single-truth behavior
 * ============================================================ */

static void test_c5_all_strict_classical(void)
{
    if5_world* w = mk_world(64, 100, 100, 100);
    const if5_stats* st;
    uint32_t i;

    mk_regions(w);
    for (i = 0; i < 4; i++)
        assert(if5_region_set_tier(w, 0, (uint16_t)(i + 1), IF5_TIER_STRICT));

    for (i = 0; i < 12; i++) {
        uint32_t stale = 0;
        submit(w, 0, "budget", IF5_T_NUM, 1000 + i, 0xFFFFFFFFu,0,0);
        submit(w, 1, "budget", IF5_T_NUM, 2000 + i, stale, 0, 0);
        submit(w, 2, "tags", IF5_T_SET, 1ull << (i % 8), 0xFFFFFFFFu,0,0);
        submit(w, 1, "tags", IF5_T_SET, 1ull << ((i + 3) % 8), stale, 0, 0);
        submit(w, 0, "title", 3, 0x41 + i, 0xFFFFFFFFu, 0, 0);
        submit(w, 2, "title", 3, 0x61 + i, stale, 0, 0);
    }

    st = if5_stats_of(w);
    assert(st->tier1_forks == 0);
    assert(st->merge_intents == 0);
    assert(st->tier3_arbitrations >= 24);
    assert(st->tier2_merges == 0);
    {
        if5_view v = if5_now(w);
        uint64_t t = if5_state_at(w, if5_region_find(w, "title"), &v, NULL);
        assert(t == 0x61 + 11);
    }
    if5_world_free(w);
}

/* ============================================================
 * C7 / I5 — undo sugar vs the pure-projection rewrite
 * ============================================================ */

static void test_c7_undo_sugar_is_projection(void)
{
    if5_world *A = mk_world(64, 100, 100, 100);
    if5_world *B = mk_world(64, 100, 100, 100);
    if5_view bv;
    uint32_t target, i;
    char pa[4096], pb[4096];

    for (i = 0; i < 2; i++) {
        if5_world* w = i ? B : A;
        mk_regions(w);
        submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
        submit(w, 0, "tags", IF5_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);
        submit(w, 1, "budget", IF5_T_NUM, 200, 0xFFFFFFFFu, 0, 0);
        submit(w, 0, "budget", IF5_T_NUM, 300, 0xFFFFFFFFu, 0, 0);
    }
    target = if5_log_count(A);
    assert(if5_log_at(A, target)->value[0] == 300);
    assert(if5_log_at(B, target)->value[0] == 300);

    /* A: the sugar — one call. */
    assert(if5_undo_nth(A, 0, target) != 0);
    /* B: the manual three steps, written out. */
    {
        if5_view now = if5_now(B);
        if5_submit_opts o;
        bv = if5_view_rewind(B, &now, target);
        memset(&o, 0, sizeof o);
        o.observed_cut = if5_log_count(B);
        (void)if5_submit(B, 0, 0, IF5_T_CURSOR,
                         ((uint64_t)target << 32) | now.cut, &o);
        {
            if5_intent* cur = (if5_intent*)if5_log_at(B, if5_log_count(B));
            cur->prov_a = target;
        }
        assert(if5_view_adopt(B, &bv));
    }

    /* Both worlds continue with the same new work. */
    submit(A, 0, "budget", IF5_T_NUM, 400, 0xFFFFFFFFu, 0, 0);
    {
        if5_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = if5_log_count(B);
        (void)if5_submit(B, 0, if5_region_find(B, "budget"),
                         IF5_T_NUM, 400, &o);
    }
    submit(A, 0, "tags", IF5_T_SET, 0x8, 0xFFFFFFFFu, 0, 0);
    {
        if5_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = if5_log_count(B);
        (void)if5_submit(B, 0, if5_region_find(B, "tags"),
                         IF5_T_SET, 0x8, &o);
    }

    /* Byte-identical world states. */
    {
        if5_view va = if5_now(A);
        uint32_t r;
        bv = if5_now(B);
        for (r = 0; r < 4; r++) {
            uint64_t x = if5_state_at(A, (uint16_t)(r + 1), &va, NULL);
            uint64_t y = if5_state_at(B, (uint16_t)(r + 1), &bv, NULL);
            assert(x == y);
        }
    }
    /* Byte-identical projections. */
    {
        if5_view va = if5_now(A);
        (void)if5_project_semantic(A, &va, pa, sizeof pa);
        (void)if5_project_semantic(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
        (void)if5_project_history(A, &va, pa, sizeof pa);
        (void)if5_project_history(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
        (void)if5_project_set(A, &va, pa, sizeof pa);
        (void)if5_project_set(B, &bv, pb, sizeof pb);
        assert(strcmp(pa, pb) == 0);
    }
    /* The logs are IDENTICAL — the debit-word class packing makes
     * its one appearance here: A and B both went through if5_submit
     * for the CURSOR, so the packing is present in BOTH logs and
     * the byte comparison stays exact. */
    assert(if5_log_count(A) == if5_log_count(B));
    {
        uint32_t k;
        for (k = 1; k <= if5_log_count(A); k++) {
            const if5_intent* x = if5_log_at(A, k);
            const if5_intent* y = if5_log_at(B, k);
            assert(memcmp(x, y, sizeof(if5_intent)) == 0);
        }
        {
            uint32_t n_cur = 0, j;
            for (j = 1; j <= if5_log_count(A); j++) {
                const if5_intent* q = if5_log_at(A, j);
                if (q->ptype == IF5_T_CURSOR) {
                    n_cur++;
                    assert(q->prov_a == target);
                    assert(q->src == 0);
                }
            }
            assert(n_cur == 1);
        }
    }
    if5_world_free(A);
    if5_world_free(B);
}

/* ============================================================
 * I6 — provenance completeness
 * ============================================================ */

static void test_i6_prov_completeness(void)
{
    if5_world* w = mk_world(64, 100, 100, 100);
    const if5_stats* st;
    uint32_t k, n_merge = 0, n_arbit = 0, n_excluded = 0;

    mk_regions(w);
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "budget", IF5_T_NUM, 200, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 300, 0, 0, 0);   /* concurrent */
    submit(w, 2, "budget", IF5_T_NUM, 400, 0, 0, 0);   /* concurrent */
    submit(w, 0, "title", 3, 0x11, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "title", 3, 0x22, 0, 0, 0);           /* tier-3 pair */

    st = if5_stats_of(w);
    assert(st->tier2_merges >= 1);
    assert(st->tier3_arbitrations >= 1);
    assert(st->excluded >= 1);

    for (k = 1; k <= if5_log_count(w); k++) {
        const if5_intent* q = if5_log_at(w, k);
        assert(q->src < 4);
        if (q->flags & IF5_F_CONCUR_MERGE) {
            n_merge++;
            assert(q->prov_a != 0);
        }
        if (q->ptype == IF5_T_ARBIT) {
            n_arbit++;
            assert(q->value[0] != 0 && q->aux != 0);
            assert(q->value[0] != q->aux);
        }
        if (q->flags & IF5_F_EXCLUDED) {
            n_excluded++;
            {
                uint32_t j;
                bool witnessed = false;
                for (j = 1; j <= if5_log_count(w); j++) {
                    const if5_intent* a = if5_log_at(w, j);
                    if (a->ptype == IF5_T_ARBIT && a->aux == k) {
                        witnessed = true;
                        break;
                    }
                }
                assert(witnessed);
            }
        }
    }
    assert(n_merge == st->tier2_merges);
    assert(n_arbit == st->tier3_arbitrations);
    assert(n_excluded == st->excluded);
    if5_world_free(w);
}

/* ============================================================
 * I7 — the pricing ladder is monotone; one currency; the breaker
 * ============================================================ */

static void test_i7_ladder_monotone(void)
{
    if5_world* w = mk_world(0, 0, 15, 15);
    const if5_ledger* lb;

    mk_regions(w);

    assert(IF5_SEV_BACKGROUND < IF5_SEV_AMBIENT);
    assert(IF5_SEV_AMBIENT < IF5_SEV_FOREGROUND);
    assert(IF5_SEV_FOREGROUND < IF5_SEV_MODAL);

    /* One currency: B's ceiling 15 = 6 initiative + 4 surprise,
     * then the modal 8 would exceed it on the COMBINED draw. */
    submit(w, 2, "budget", IF5_T_NUM, 1, 0xFFFFFFFFu, 1, IF5_SEV_AMBIENT);
    submit(w, 2, "budget", IF5_T_NUM, 2, 0xFFFFFFFFu, 1, IF5_SEV_FOREGROUND);
    submit(w, 2, "budget", IF5_T_NUM, 3, 0xFFFFFFFFu, 2, IF5_SEV_FOREGROUND);
    lb = if5_ledger_of(w, 2);
    assert(lb->spent_initiative == 6);
    assert(lb->spent_surprise == 4);
    assert(!lb->breaker_tripped);
    /* The debit word carries the class (ADR-0026 3d): the third
     * submit's record must read back as surprise-class. */
    {
        const if5_intent* rec = if5_log_at(w, if5_log_count(w));
        assert(IF5_DEBIT_CLASS(rec->debit) == IF5_DEBIT_SURPRISE);
        assert(IF5_DEBIT_SEV(rec->debit) == IF5_SEV_FOREGROUND);
        assert(IF5_DEBIT_CLASS(if5_log_at(w, if5_log_count(w) - 1)->debit)
               == IF5_DEBIT_INITIATIVE);
    }
    submit(w, 2, "budget", IF5_T_NUM, 4, 0xFFFFFFFFu, 1, IF5_SEV_MODAL);
    lb = if5_ledger_of(w, 2);
    assert(lb->breaker_tripped);
    assert(if5_stats_of(w)->breakers == 1);
    assert(if5_stats_of(w)->receipts == 1);
    {
        if5_view v = if5_now(w);
        uint64_t val = if5_state_at(w, if5_region_find(w, "budget"),
                                    &v, NULL);
        assert(val == 3);
    }
    if5_world_free(w);
}

/* ============================================================
 * C9 treatment side — no classical machinery anywhere
 * ============================================================ */

static void test_c9_treatment_side_zero_machinery(void)
{
    if5_world* w = mk_world(64, 100, 100, 100);
    if5_view v;
    uint32_t k;

    mk_regions(w);
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 200, 0, 0, 0);
    submit(w, 0, "stops", 2, 1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "stops", 2, 2, 0, 0, 0);            /* fork beat */
    assert(if5_stats_of(w)->tier1_forks >= 1);
    submit(w, 0, "title", 3, 0x1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "title", 3, 0x2, 0, 0, 0);          /* tier-3 pair */
    (void)if5_undo_nth(w, 0, if5_log_count(w) - 2);

    /* Every query surface answers from the log alone — including
     * the v5 additions (tree, schedule, cuts): the seats add QUERY
     * surface, never side structures. */
    v = if5_now(w);
    {
        char buf[2048];
        assert(if5_project_semantic(w, &v, buf, sizeof buf) > 0);
        assert(if5_project_set(w, &v, buf, sizeof buf) > 0);
        assert(if5_project_history(w, &v, buf, sizeof buf) > 0);
        assert(if5_tree_walk(w, &v, NULL, NULL) == 4);
        assert(if5_schedule(w, &v, IF5_SID_NULL, NULL, 0) >= 1);
    }
    {
        if5_view hist = if5_view_at(w, 6);
        (void)if5_state_at(w, 1, &hist, NULL);
    }
    for (k = 1; k <= if5_log_count(w); k++) {
        assert(if5_log_at(w, k) != NULL);
    }
    if5_world_free(w);
}

/* ============================================================
 * Down-closure — now the promoted public verifier (V2's
 * constructor half is in test_v5_cut; this section runs the
 * verifier over the calibration workload's addresses)
 * ============================================================ */

static void test_views_are_down_closed(void)
{
    if5_world* w = mk_world(64, 100, 100, 100);
    uint32_t cut;

    mk_regions(w);
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 200, 0, 0, 0);
    submit(w, 2, "tags", IF5_T_SET, 0x2, 0, 0, 0);
    submit(w, 0, "title", 3, 0x3, 0xFFFFFFFFu, 0, 0);
    (void)if5_undo_nth(w, 0, 3);
    submit(w, 0, "budget", IF5_T_NUM, 400, 0xFFFFFFFFu, 0, 0);

    /* Every historical address is a down-closed set — checked by
     * the PUBLIC verifier the cut algebra exported (the slice kept
     * this checker test-private; ADR-0026 3b promoted it). */
    for (cut = 0; cut <= if5_log_count(w); cut++) {
        if5_cut c = if5_cut_at(w, cut);
        assert(if5_cut_is_down_closed(w, &c));
    }
    /* The now-view's address (prunes from the undo) too. */
    {
        if5_view v = if5_now(w);
        if5_cut c = if5_cut_of_view(&v);
        assert(c.n_prunes >= 1);
        assert(if5_cut_is_down_closed(w, &c));
        /* And it folds identically through the cut path (V3's
         * in-world spot check; the full battery is test_v5_cut). */
        {
            uint32_t cl = 0, cv = 0;
            uint64_t a = if5_cut_state(w, &c, 1, &cl);
            uint64_t b = if5_state_at(w, 1, &v, &cv);
            assert(a == b && cl == cv);
        }
    }
    if5_world_free(w);
}

int main(void)
{
    printf("=== v5 calibration suite (ADR-0026 V7: the promotion changed "
           "none of the instrument's calibration) ===\n\n");
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
