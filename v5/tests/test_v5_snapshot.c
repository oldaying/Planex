/*
 * test_v5_snapshot.c — the snapshot suite (ADR-0028): F6, F7,
 * F8, F9 — the named-address round trip, the fidelity of a
 * save/load, the canonicity of the format, and the exactness of
 * the one-pass rebuild.
 *
 * White-box by the differential's precedent (it too includes
 * both internal headers to compare caches the public API does
 * not expose): F9's "knowledge clocks, ledger balances, region
 * value caches" are world-internal fields, and comparing them is
 * exactly the test's job.
 *
 * The driver walks the FULL registered op alphabet — submits
 * with and without debits, single and batch region creation,
 * concurrent merges, forks, arbitrations, delegation, a breaker,
 * undo, an explicit stitch, a black-box export, a reparent, a
 * process commit and a process cancel — because the rebuild's
 * honesty is only as wide as the alphabet it has been held to.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5.h"
#include "../src/if5_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * The driver: a world over the full alphabet
 * ============================================================ */

static if5_world* build_world(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    if5_source s;
    if5_region_spec specs[3];
    if5_submit_opts o;
    uint16_t budget, stops, title, strict, seqr;
    uint32_t iid, cut_a, cut_b;
    if5_sid H, A, B;

    assert(w);

    /* the extension types through the same public channel every
     * test uses (B2: no privileged slots); seq lands at slot 2,
     * str at 3 == IF5_T_STR (test_v5_mech's established order) */
    memset(&t, 0, sizeof t);
    t.name = "seq";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t);
    t.name = "str";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == IF5_T_STR);

    /* three working sources + a paying system */
    memset(&s, 0, sizeof s);
    memcpy(s.id, "human", 5);
    s.kind = IF5_KIND_PERSON;
    s.trust = 3;
    s.authority = 0xFFFFFFFFu;
    s.initiative_budget = 100;
    H = if5_add_source(w, &s);

    memcpy(s.id, "agentA", 6);
    s.kind = IF5_KIND_AGENT;
    s.trust = 2;
    s.initiative_budget = 10;          /* small: the breaker fires */
    A = if5_add_source(w, &s);

    memcpy(s.id, "agentB", 6);
    s.kind = IF5_KIND_AGENT;
    s.trust = 2;
    s.initiative_budget = 40;
    B = if5_add_source(w, &s);

    memcpy(s.id, "system", 6);
    s.kind = IF5_KIND_PROCESS;
    s.trust = 1;
    s.initiative_budget = 200;
    {
        if5_sid SY = if5_add_source(w, &s);
        if5_set_system_source(w, SY);
    }

    /* single-created regions (rich-PROV path) */
    budget = if5_region_create(w, H, "budget", 0, IF5_T_NUM,
                               IF5_TIER_MERGE, NULL, 0);
    stops  = if5_region_create(w, H, "stops", 0, IF5_T_NUM,
                               IF5_TIER_FORK, NULL, 0);
    strict = if5_region_create(w, H, "strict", 0, IF5_T_NUM,
                               IF5_TIER_STRICT, NULL, 0);
    title  = if5_region_create(w, H, "title", 0, IF5_T_STR,
                               IF5_TIER_STRICT, NULL, 0);
    assert(budget && stops && strict && title);

    /* batch-created regions (the blob path — the seed derivation
     * the loader's rebuild must reproduce) */
    memset(specs, 0, sizeof specs);
    strcpy(specs[0].name, "b0"); specs[0].ptype = IF5_T_NUM;
    specs[0].tier = IF5_TIER_MERGE; specs[0].seed = 7;
    strcpy(specs[1].name, "b1"); specs[1].ptype = IF5_T_SET;
    specs[1].tier = IF5_TIER_MERGE; specs[1].seed = 0x11;
    strcpy(specs[2].name, "b2"); specs[2].ptype = IF5_T_NUM;
    specs[2].tier = IF5_TIER_FORK; specs[2].seed = 42;
    assert(if5_region_create_batch(w, H, specs, 3) == 3);
    seqr = if5_region_find(w, "b2");
    assert(seqr == 3 + 4);             /* 4 singles come first */

    /* plain causal submissions */
    memset(&o, 0, sizeof o);
    o.observed_cut = 0xFFFFFFFFu;      /* full knowledge */
    assert(if5_submit(w, H, budget, IF5_T_NUM, 100, &o));
    assert(if5_submit(w, H, budget, IF5_T_NUM, 150, &o));
    assert(if5_submit(w, H, title, IF5_T_STR, 0x3031323334353637ull, &o));

    /* an agent submission WITH an initiative debit (the word path) */
    o.debit_class = IF5_DEBIT_INITIATIVE;
    o.severity = IF5_SEV_AMBIENT;      /* cost 2 */
    assert(if5_submit(w, A, budget, IF5_T_NUM, 200, &o));

    /* the breaker: A's budget is 10; spent 2 + 4 + 4 = 10 exactly,
     * so the FOURTH debit (4 more) is the overdraft — the receipt
     * fires and the record lands unapplied */
    o.severity = IF5_SEV_FOREGROUND;   /* cost 4 */
    assert(if5_submit(w, A, budget, IF5_T_NUM, 300, &o));
    o.severity = IF5_SEV_FOREGROUND;
    iid = if5_submit(w, A, budget, IF5_T_NUM, 400, &o);
    assert(iid);
    iid = if5_submit(w, A, budget, IF5_T_NUM, 500, &o);
    assert(iid);
    assert(if5_receipt_count(w) == 1);
    assert(if5_receipt_at(w, 0) != NULL);

    /* concurrency at the merge tier: H and B race the budget */
    cut_a = if5_log_count(w);
    assert(if5_submit(w, H, budget, IF5_T_NUM, 500, &o) == 0 ||
           if5_log_count(w) > cut_a);
    {
        /* narrow both to the same older cut so the vcs collide */
        if5_submit_opts race;
        memset(&race, 0, sizeof race);
        race.observed_cut = cut_a;
        race.debit_class = IF5_DEBIT_NONE;
        assert(if5_submit(w, H, budget, IF5_T_NUM, 500, &race));
        assert(if5_submit(w, B, budget, IF5_T_NUM, 600, &race));
    }

    /* concurrency at the fork tier */
    cut_b = if5_log_count(w);
    {
        if5_submit_opts race;
        memset(&race, 0, sizeof race);
        race.observed_cut = cut_b;
        assert(if5_submit(w, H, stops, IF5_T_NUM, 1, &race));
        assert(if5_submit(w, B, stops, IF5_T_NUM, 2, &race));
    }

    /* concurrency at the strict tier (the ARBIT witness path) */
    {
        if5_submit_opts race;
        memset(&race, 0, sizeof race);
        race.observed_cut = cut_b;
        assert(if5_submit(w, H, strict, IF5_T_NUM, 10, &race));
        assert(if5_submit(w, B, strict, IF5_T_NUM, 20, &race));
    }

    /* delegation: H carves 30 out for B */
    assert(if5_delegate(w, H, B, 30));

    /* undo: H revokes the 400 attempt */
    iid = if5_log_at(w, 7) ? 7 : 7;    /* some earlier record */
    (void)iid;
    {
        /* revoke the record that set title — a clean, findable
         * target (the fold at older views must still see it) */
        uint32_t t_iid = 0, i;
        for (i = 1; i <= if5_log_count(w); i++)
            if (if5_log_at(w, i)->region == title) { t_iid = i; break; }
        assert(t_iid);
        assert(if5_undo_nth(w, H, t_iid) != 0);
    }

    /* the explicit stitch on the forked region */
    {
        if5_region* r = if5_region_info(w, stops);
        if (r->head_b) {
            assert(if5_merge_intent(w, H, stops, r->head_a, r->head_b,
                                    3) != 0);
        }
    }

    /* the black box: configure + export (the EXPORT record) */
    if5_blackbox_configure(w, budget, 64);
    {
        char bb[4096];
        assert(if5_blackbox_export(w, H, budget, bb, sizeof bb) > 0);
    }

    /* a reparent: b2 moves under the strict region */
    assert(if5_reparent(w, H, seqr, strict) != 0);

    /* a process: commit one, cancel one (the cancel is a cut) */
    {
        if5_process* p = if5_process_begin(w, B, title, IF5_T_STR);
        assert(p);
        assert(if5_process_sample(p, 0x4142434445464748ull));
        assert(if5_process_commit(p, 0x4142434445464748ull, NULL));

        p = if5_process_begin(w, A, title, IF5_T_STR);
        assert(p);
        assert(if5_process_sample(p, 99));
        assert(if5_process_cancel(p));
    }

    /* named addresses in the registry */
    {
        if5_cut c = if5_cut_at(w, if5_log_count(w));
        assert(if5_cut_register_named(w, &c, "now") == 1);
        assert(if5_cut_register_named(w, &c, "now") == 0);  /* dup refused */
        c = if5_cut_at(w, if5_log_count(w) / 2);
        assert(if5_cut_register_named(w, &c, "half") == 2);
        c = if5_cut_at(w, 1);
        assert(if5_cut_register(w, &c) == 3);               /* unnamed ok */
    }
    return w;
}

/* The loader-side world: fresh, with the same extension type
 * re-registered (the type-table check demands it). */
static if5_world* fresh_typed_world(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    memset(&t, 0, sizeof t);
    t.name = "seq";
    t.value_size = 8;
    assert(w && if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t);
    t.name = "str";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == IF5_T_STR);
    return w;
}

/* ============================================================
 * The comparison instruments
 * ============================================================ */

static void cmp_projections(const char* what, if5_world* a, if5_world* b,
                            const if5_view* v)
{
    char pa[4096], pb[4096];
    uint32_t na, nb;
    na = if5_project_semantic(a, v, pa, sizeof pa);
    nb = if5_project_semantic(b, v, pb, sizeof pb);
    if (na != nb || memcmp(pa, pb, na) != 0) {
        fprintf(stderr, "F7 FAIL: semantic projection differs (%s)\n",
                what);
        assert(false);
    }
    na = if5_project_set(a, v, pa, sizeof pa);
    nb = if5_project_set(b, v, pb, sizeof pb);
    if (na != nb || memcmp(pa, pb, na) != 0) {
        fprintf(stderr, "F7 FAIL: set projection differs (%s)\n", what);
        assert(false);
    }
    na = if5_project_history(a, v, pa, sizeof pa);
    nb = if5_project_history(b, v, pb, sizeof pb);
    if (na != nb || memcmp(pa, pb, na) != 0) {
        fprintf(stderr, "F7 FAIL: history projection differs (%s)\n",
                what);
        assert(false);
    }
}

static uint32_t region_count_of(if5_world* w) { return w->n_regions; }

typedef struct {
    if5_node nodes[256];
    uint32_t n;
} node_list;

static bool collect(const if5_node* n, void* user)
{
    node_list* l = (node_list*)user;
    if (l->n >= 256) return false;
    l->nodes[l->n++] = *n;
    return true;
}

static void cmp_tree(const char* what, if5_world* a, if5_world* b,
                     const if5_view* v)
{
    node_list la, lb;
    la.n = 0; lb.n = 0;
    if5_tree_walk(a, v, collect, &la);
    if5_tree_walk(b, v, collect, &lb);
    if (la.n != lb.n || memcmp(la.nodes, lb.nodes,
                               sizeof(if5_node) * la.n) != 0) {
        fprintf(stderr, "F7 FAIL: tree walk differs (%s): %u vs %u\n",
                what, la.n, lb.n);
        assert(false);
    }
}

static void cmp_schedule(const char* what, if5_world* a, if5_world* b,
                         const if5_view* v)
{
    if5_sched_entry ea[128], eb[128];
    uint32_t na, nb;
    na = if5_schedule(a, v, IF5_SID_NULL, ea, 128);
    nb = if5_schedule(b, v, IF5_SID_NULL, eb, 128);
    if (na != nb || memcmp(ea, eb, sizeof(if5_sched_entry) * na)) {
        fprintf(stderr, "F7 FAIL: schedule differs (%s): %u vs %u\n",
                what, na, nb);
        assert(false);
    }
}

static void cmp_blackbox(const char* what, if5_world* a, if5_world* b)
{
    char ba[8192], bb[8192];
    uint32_t na, nb;
    na = if5_blackbox_export(a, (if5_sid)0, if5_region_find(a, "budget"),
                             ba, sizeof ba);
    nb = if5_blackbox_export(b, (if5_sid)0, if5_region_find(b, "budget"),
                             bb, sizeof bb);
    if (na != nb || memcmp(ba, bb, na)) {
        fprintf(stderr, "F7 FAIL: black-box export differs (%s)\n", what);
        assert(false);
    }
}

/* F9: the white-box cache comparison — knowledge clocks, ledger
 * arithmetic, region caches, fork heads, the running join. */
static void cmp_caches(const char* what, if5_world* a, if5_world* b)
{
    uint32_t i, k;
    if (a->n_sources != b->n_sources || a->n_regions != b->n_regions ||
        a->n_log != b->n_log) {
        fprintf(stderr, "F9 FAIL: shape differs (%s)\n", what);
        assert(false);
    }
    for (k = 0; k < a->n_sources; k++) {
        if (memcmp(&a->svc[k], &b->svc[k], sizeof(if5_vc))) {
            fprintf(stderr, "F9 FAIL: svc[%u] differs (%s)\n", k, what);
            assert(false);
        }
        if (memcmp(&a->ledgers[k], &b->ledgers[k], sizeof(if5_ledger))) {
            fprintf(stderr, "F9 FAIL: ledger[%u] differs (%s): "
                    "spent %u/%u sub %u/%u carved %u/%u brk %d/%d\n",
                    k, what,
                    a->ledgers[k].spent_initiative,
                    b->ledgers[k].spent_initiative,
                    a->ledgers[k].sub_ceiling, b->ledgers[k].sub_ceiling,
                    a->ledgers[k].carved, b->ledgers[k].carved,
                    (int)a->ledgers[k].breaker_tripped,
                    (int)b->ledgers[k].breaker_tripped);
            assert(false);
        }
    }
    if (memcmp(&a->vc_now, &b->vc_now, sizeof(if5_vc))) {
        fprintf(stderr, "F9 FAIL: vc_now differs (%s)\n", what);
        assert(false);
    }
    for (i = 0; i < a->n_regions; i++) {
        const if5_region* ra = &a->regions[i];
        const if5_region* rb = &b->regions[i];
        if (memcmp(ra->name, rb->name, IF5_NAME_MAX) ||
            ra->ptype != rb->ptype || ra->tier != rb->tier ||
            ra->window != rb->window || ra->parent != rb->parent ||
            ra->cache != rb->cache || ra->last_iid != rb->last_iid ||
            ra->head_a != rb->head_a || ra->head_b != rb->head_b) {
            fprintf(stderr, "F9 FAIL: region[%u] differs (%s): "
                    "cache %llu/%llu last %u/%u heads %u,%u/%u,%u\n",
                    i, what,
                    (unsigned long long)ra->cache,
                    (unsigned long long)rb->cache,
                    ra->last_iid, rb->last_iid,
                    ra->head_a, ra->head_b, rb->head_a, rb->head_b);
            assert(false);
        }
    }
    if (a->tick != b->tick) {
        fprintf(stderr, "F9 FAIL: tick differs (%s): %u vs %u\n",
                what, a->tick, b->tick);
        assert(false);
    }
}

static void cmp_stats(const char* what, if5_world* a, if5_world* b)
{
    const if5_stats* sa = if5_stats_of(a);
    const if5_stats* sb = if5_stats_of(b);
    if (memcmp(sa, sb, sizeof(if5_stats))) {
        fprintf(stderr, "F9 FAIL: stats differ (%s): "
                "submits %u/%u causal %u/%u arrivals %u/%u forks %u/%u "
                "arbits %u/%u merges %u/%u\n", what,
                sa->submits, sb->submits, sa->causal_applies,
                sb->causal_applies, sa->concurrent_arrivals,
                sb->concurrent_arrivals, sa->tier1_forks,
                sb->tier1_forks, sa->tier3_arbitrations,
                sb->tier3_arbitrations, sa->tier2_merges,
                sb->tier2_merges);
        assert(false);
    }
}

/* ============================================================
 * The tests
 * ============================================================ */

static void test_f7_f9_round_trip(void)
{
    if5_world* a = build_world();
    if5_world* b;
    char diag[256];
    uint32_t i, r;

    printf("  [TEST] f7_f9_round_trip\n");

    assert(if5_snapshot_save(a, "if5_snap_a.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);

    b = fresh_typed_world();
    /* the extension type re-registered through the public channel,
     * exactly as the loader contract demands */
    {
        int rc = if5_snapshot_load(b, "if5_snap_a.bin", diag,
                                   sizeof diag);
        if (rc != IF5_SNAP_OK) {
            fprintf(stderr, "LOAD rc=%d diag: %s\n", rc, diag);
            assert(false);
        }
    }

    /* F7: the durable truth, verbatim */
    assert(a->n_log == b->n_log);
    assert(memcmp(a->log, b->log, (size_t)a->n_log * sizeof(if5_intent))
           == 0);
    assert(a->n_arena == b->n_arena);
    assert(memcmp(a->arena, b->arena, a->n_arena) == 0);

    /* F7: the receipts, one field at a time */
    assert(if5_receipt_count(a) == if5_receipt_count(b));
    for (i = 0; i < if5_receipt_count(a); i++) {
        const if5_receipt* ra = if5_receipt_at(a, i);
        const if5_receipt* rb = if5_receipt_at(b, i);
        assert(memcmp(ra, rb, sizeof(if5_receipt)) == 0);
    }

    /* F7: projections, tree, schedule, black box — at now, at a
     * mid cut, and at a rewound view */
    {
        if5_view v_now = if5_now(a);
        if5_view v_mid = if5_view_at(a, if5_log_count(a) / 2);
        if5_view v_old = if5_view_rewind(a, &v_mid, 3);
        const if5_view* vs[3] = { &v_now, &v_mid, &v_old };
        const char* names[3] = { "now", "mid", "rewound" };
        for (i = 0; i < 3; i++) {
            cmp_projections(names[i], a, b, vs[i]);
            cmp_tree(names[i], a, b, vs[i]);
            cmp_schedule(names[i], a, b, vs[i]);
            for (r = 1; r <= region_count_of(a); r++) {
                uint32_t ca = 0, cb = 0;
                uint64_t va = if5_state_at(a, (uint16_t)r, vs[i], &ca);
                uint64_t vb = if5_state_at(b, (uint16_t)r, vs[i], &cb);
                if (va != vb || ca != cb) {
                    fprintf(stderr, "F7 FAIL: region %u fold differs "
                            "(%s): %llu/%llu\n", r, names[i],
                            (unsigned long long)va,
                            (unsigned long long)vb);
                    assert(false);
                }
            }
        }
        cmp_blackbox("now", a, b);
    }

    /* F7: every registered cut's fold, at its own address */
    assert(if5_cut_count(a) == if5_cut_count(b));
    for (i = 1; i <= if5_cut_count(a); i++) {
        const if5_cut* ca = if5_cut_by_id(a, (uint8_t)i);
        const if5_cut* cb = if5_cut_by_id(b, (uint8_t)i);
        uint16_t rid;
        assert(memcmp(ca, cb, sizeof(if5_cut)) == 0);
        for (rid = 1; rid <= region_count_of(a); rid++) {
            uint32_t na = 0, nb = 0;
            uint64_t va = if5_cut_state(a, ca, rid, &na);
            uint64_t vb = if5_cut_state(b, cb, rid, &nb);
            assert(va == vb && na == nb);
        }
    }

    /* F9: the caches, white-box */
    cmp_caches("round-trip", a, b);
    cmp_stats("round-trip", a, b);

    /* the cursor itself */
    {
        if5_view va = if5_now(a), vb = if5_now(b);
        assert(va.cut == vb.cut && va.n_excl == vb.n_excl &&
               va.n_sel == vb.n_sel);
    }

    /* F7's strongest form: the reloaded world CONTINUES identically
     * — one more submission on each, byte-compared */
    {
        if5_submit_opts o;
        uint32_t ia, ib;
        memset(&o, 0, sizeof o);
        o.observed_cut = 0xFFFFFFFFu;
        ia = if5_submit(a, (if5_sid)0, if5_region_find(a, "budget"),
                        IF5_T_NUM, 777, &o);
        ib = if5_submit(b, (if5_sid)0, if5_region_find(b, "budget"),
                        IF5_T_NUM, 777, &o);
        assert(ia && ib);
        assert(memcmp(&a->log[ia - 1], &b->log[ib - 1],
                      sizeof(if5_intent)) == 0);
        cmp_projections("after-continue", a, b, NULL);
        cmp_caches("after-continue", a, b);
    }

    if5_world_free(a);
    if5_world_free(b);
    printf("        F7 + F9: PASS\n");
}

static void test_f6_named_addresses(void)
{
    if5_world* a = build_world();
    if5_world* b;
    char diag[256];
    uint8_t id;

    printf("  [TEST] f6_named_addresses\n");

    assert(if5_snapshot_save(a, "if5_snap_f6.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);
    b = fresh_typed_world();
    assert(if5_snapshot_load(b, "if5_snap_f6.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);

    /* names, ids, addresses — all stable across the round trip */
    assert(if5_cut_count(b) == 3);
    assert(strcmp(if5_cut_name(b, 1), "now") == 0);
    assert(strcmp(if5_cut_name(b, 2), "half") == 0);
    assert(strcmp(if5_cut_name(b, 3), "") == 0);
    id = if5_cut_by_name(b, "now");
    assert(id == 1);
    {
        const if5_cut* ca = if5_cut_by_id(a, 1);
        const if5_cut* cb = if5_cut_by_id(b, 1);
        assert(memcmp(ca, cb, sizeof(if5_cut)) == 0);
    }
    assert(if5_cut_by_name(b, "absent") == 0);

    /* duplicate names are refused on the reloaded world too */
    {
        if5_cut c = if5_cut_at(b, if5_log_count(b));
        assert(if5_cut_register_named(b, &c, "now") == 0);
        assert(if5_cut_register_named(b, &c, "fresh") == 4);
    }

    if5_world_free(a);
    if5_world_free(b);
    printf("        F6: PASS\n");
}

static void test_f8_canonicity(void)
{
    if5_world* a = build_world();
    if5_world* b;
    char diag[256];
    FILE* f1;
    FILE* f2;
    long la, lb;
    uint8_t *da, *db;

    printf("  [TEST] f8_canonicity\n");

    assert(if5_snapshot_save(a, "if5_f8_1.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);
    assert(if5_snapshot_save(a, "if5_f8_2.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);

    /* save twice: byte-identical */
    f1 = fopen("if5_f8_1.bin", "rb");
    f2 = fopen("if5_f8_2.bin", "rb");
    assert(f1 && f2);
    fseek(f1, 0, SEEK_END); la = ftell(f1); fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_END); lb = ftell(f2); fseek(f2, 0, SEEK_SET);
    assert(la == lb && la > 0);
    da = (uint8_t*)malloc((size_t)la);
    db = (uint8_t*)malloc((size_t)lb);
    assert(fread(da, 1, (size_t)la, f1) == (size_t)la);
    assert(fread(db, 1, (size_t)lb, f2) == (size_t)lb);
    fclose(f1); fclose(f2);
    assert(memcmp(da, db, (size_t)la) == 0);
    free(da); free(db);

    /* the fixpoint: load, re-save, byte-identical again */
    b = fresh_typed_world();
    assert(if5_snapshot_load(b, "if5_f8_1.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);
    assert(if5_snapshot_save(b, "if5_f8_3.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);
    f1 = fopen("if5_f8_1.bin", "rb");
    f2 = fopen("if5_f8_3.bin", "rb");
    assert(f1 && f2);
    fseek(f1, 0, SEEK_END); la = ftell(f1); fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_END); lb = ftell(f2); fseek(f2, 0, SEEK_SET);
    assert(la == lb);
    da = (uint8_t*)malloc((size_t)la);
    db = (uint8_t*)malloc((size_t)lb);
    assert(fread(da, 1, (size_t)la, f1) == (size_t)la);
    assert(fread(db, 1, (size_t)lb, f2) == (size_t)lb);
    fclose(f1); fclose(f2);
    if (memcmp(da, db, (size_t)la) != 0) {
        fprintf(stderr, "F8 FAIL: re-save is not a fixpoint\n");
        assert(false);
    }
    free(da); free(db);

    if5_world_free(a);
    if5_world_free(b);
    printf("        F8: PASS\n");
}

static void test_refusals(void)
{
    if5_world* a = build_world();
    if5_world* b;
    char diag[256];
    uint8_t* img;
    long sz;
    FILE* f;

    printf("  [TEST] refusals\n");

    assert(if5_snapshot_save(a, "if5_ref.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);

    /* not fresh */
    b = if5_world_create();
    {
        if5_source src;
        if5_region_spec s;
        memset(&src, 0, sizeof src);
        memcpy(src.id, "h", 1);
        src.kind = IF5_KIND_PERSON;
        src.authority = 0xFFFFFFFFu;
        assert(if5_add_source(b, &src) == 0);
        memset(&s, 0, sizeof s);
        strcpy(s.name, "x");
        s.ptype = IF5_T_NUM; s.tier = IF5_TIER_MERGE;
        assert(if5_region_create_batch(b, (if5_sid)0, &s, 1) == 1);
    }
    assert(if5_snapshot_load(b, "if5_ref.bin", diag,
                             sizeof diag) == IF5_SNAP_ERR_STATE);
    if5_world_free(b);

    /* bad magic */
    f = fopen("if5_ref.bin", "rb");
    assert(f);
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    img = (uint8_t*)malloc((size_t)sz);
    assert(fread(img, 1, (size_t)sz, f) == (size_t)sz);
    fclose(f);
    img[0] = 'X';
    f = fopen("if5_ref_magic.bin", "wb");
    fwrite(img, 1, (size_t)sz, f);
    fclose(f);
    img[0] = 'I';                      /* restore for the later cases */
    b = fresh_typed_world();
    assert(if5_snapshot_load(b, "if5_ref_magic.bin", diag,
                             sizeof diag) == IF5_SNAP_ERR_MAGIC);
    if5_world_free(b);

    /* truncation */
    f = fopen("if5_ref_trunc.bin", "wb");
    fwrite(img, 1, (size_t)(sz - 40), f);
    fclose(f);
    b = fresh_typed_world();
    {
        int rc = if5_snapshot_load(b, "if5_ref_trunc.bin", diag,
                                   sizeof diag);
        if (rc != IF5_SNAP_ERR_TRUNC) {
            fprintf(stderr, "TRUNC rc=%d diag: %s\n", rc, diag);
            assert(false);
        }
    }
    if5_world_free(b);

    /* trailing bytes */
    {
        uint8_t* big = (uint8_t*)malloc((size_t)sz + 4);
        memcpy(big, img, (size_t)sz);
        big[sz] = big[sz+1] = big[sz+2] = big[sz+3] = 0;
        f = fopen("if5_ref_trail.bin", "wb");
        fwrite(big, 1, (size_t)sz + 4, f);
        fclose(f);
        free(big);
    }
    b = fresh_typed_world();
    assert(if5_snapshot_load(b, "if5_ref_trail.bin", diag,
                             sizeof diag) == IF5_SNAP_ERR_BOUNDS);
    if5_world_free(b);

    /* type-table drift: fewer types registered than the file
     * carries */
    b = if5_world_create();
    {
        if5_payload_type t;
        memset(&t, 0, sizeof t);
        t.name = "seq";
        t.value_size = 8;
        assert(if5_register_payload_type(b, &t) == 2);
    }
    {
        int rc = if5_snapshot_load(b, "if5_ref.bin", diag,
                                   sizeof diag);
        if (rc != IF5_SNAP_ERR_TYPE) {
            fprintf(stderr, "REFUSAL FAIL: type drift returned %d "
                    "(diag: %s)\n", rc, diag);
            assert(false);
        }
    }
    if5_world_free(b);
    free(img);

    if5_world_free(a);
    printf("        refusals: PASS (state/magic/trunc/trailing/type)\n");
}

int main(void)
{
    printf("=== v5 snapshot suite (ADR-0028: F6, F7, F8, F9) ===\n\n");
    test_f7_f9_round_trip();
    test_f6_named_addresses();
    test_f8_canonicity();
    test_refusals();
    printf("\n5/5 snapshot sections pass\n");
    return 0;
}
