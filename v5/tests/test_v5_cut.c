/*
 * test_v5_cut.c — the cut algebra's law battery and address checks
 * (ADR-0026 Decision 3b; predictions V2 and V3).
 *
 * V2: the principal lattice's laws hold (commutativity,
 * associativity, idempotence, absorption), meet is total on pruned
 * cuts, join refuses them (the registered boundary), and EVERY
 * algebra output is down-closed under the PUBLIC verifier.
 *
 * V3: if5_cut_state agrees with an INDEPENDENT membership-driven
 * fold (a test-side reimplementation sharing no code with the
 * core's fold), the prune constructor's addressed set is exactly
 * the sugar's rewound view, and registered addresses fold
 * identically to their values.
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

#define CUT_ROUNDS 4000u

static uint64_t rng_state = 0x9E3779B97F4A7C15ull;

static uint64_t rng(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

/* A scenario-shaped world with concurrent traffic and an undo, so
 * cuts have real causal structure to address. */
static if5_world* mk_world(void)
{
    if5_world* w = if5_world_create();
    if5_source s;
    if5_payload_type t;
    memset(&s, 0, sizeof s);
    strcpy(s.id, "human");  s.kind = IF5_KIND_PERSON;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);
    strcpy(s.id, "agentA"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = 200; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 1);
    strcpy(s.id, "agentB"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = 200; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 2);
    strcpy(s.id, "system"); s.kind = IF5_KIND_PROCESS;
    s.initiative_budget = 64; s.authority = 0;
    assert(if5_add_source(w, &s) == 3);
    if5_set_system_source(w, 3);
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "stops", 0, 2, IF5_TIER_FORK, NULL, 0));
    assert(if5_region_create(w, 0, "tags", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "title", 0, 3, IF5_TIER_STRICT, NULL, 0));
    return w;
}

static void submit(if5_world* w, if5_sid src, const char* region,
                   uint8_t ptype, uint64_t value, uint32_t cut)
{
    if5_submit_opts o;
    memset(&o, 0, sizeof o);
    o.observed_cut = cut == 0xFFFFFFFFu ? if5_log_count(w) : cut;
    (void)if5_submit(w, src, if5_region_find(w, region), ptype, value, &o);
}

static void fill(if5_world* w)
{
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu);
    submit(w, 1, "budget", IF5_T_NUM, 200, 0);            /* concurrent */
    submit(w, 2, "budget", IF5_T_NUM, 300, 0);            /* concurrent */
    submit(w, 0, "stops", 2, ((uint64_t)1) << 32, 0xFFFFFFFFu);
    submit(w, 1, "stops", 2, ((uint64_t)2) << 32, 0);     /* fork beat */
    submit(w, 0, "tags", IF5_T_SET, 0x1, 0xFFFFFFFFu);
    submit(w, 1, "tags", IF5_T_SET, 0x2, 0);              /* concurrent */
    submit(w, 0, "title", 3, 0x11, 0xFFFFFFFFu);
    submit(w, 1, "title", 3, 0x22, 0);                    /* tier-3 pair */
    submit(w, 0, "budget", IF5_T_NUM, 400, 0xFFFFFFFFu);
}

/* Random principal cut. */
static if5_cut rnd_cut(if5_world* w)
{
    return if5_cut_at(w, (uint32_t)(rng() % (if5_log_count(w) + 1)));
}

/* Random pruned cut: principal at a prefix, then random prunes of
 * addressable records. */
static if5_cut rnd_pruned_cut(if5_world* w)
{
    if5_cut c = rnd_cut(w);
    uint32_t k, n = (uint32_t)(rng() % 3);
    for (k = 0; k < n; k++) {
        if (c.prefix == 0) break;
        (void)if5_cut_prune(w, &c, 1 + (uint32_t)(rng() % c.prefix));
    }
    return c;
}

/* ============================================================
 * V2 — the principal lattice's laws
 * ============================================================ */

static void test_lattice_laws_hold(void)
{
    if5_world* w = mk_world();
    uint32_t i;

    fill(w);

    for (i = 0; i < CUT_ROUNDS; i++) {
        if5_cut a = rnd_cut(w), b = rnd_cut(w), c = rnd_cut(w);
        if5_cut ab, ba, ab_c, a_bc, bc, ac, ac_b, a_bc2;
        if5_cut aa, meet_ab, join_am;

        /* commutativity */
        assert(if5_cut_join(w, &a, &b, &ab));
        assert(if5_cut_join(w, &b, &a, &ba));
        assert(ab.prefix == ba.prefix && ab.n_prunes == ba.n_prunes);
        assert(if5_cut_is_down_closed(w, &ab));

        /* associativity */
        assert(if5_cut_join(w, &b, &c, &bc));
        assert(if5_cut_join(w, &a, &bc, &a_bc));
        assert(if5_cut_join(w, &ab, &c, &ab_c));
        assert(a_bc.prefix == ab_c.prefix);
        assert(if5_cut_is_down_closed(w, &a_bc));

        /* idempotence */
        assert(if5_cut_join(w, &a, &a, &aa));
        assert(aa.prefix == a.prefix && aa.n_prunes == 0);

        /* absorption: join(a, meet(a,b)) == a */
        assert(if5_cut_meet(w, &a, &b, &meet_ab));
        assert(if5_cut_join(w, &a, &meet_ab, &join_am));
        assert(join_am.prefix == a.prefix && join_am.n_prunes == 0);
        assert(if5_cut_is_down_closed(w, &meet_ab));

        /* meet's laws on principal cuts: commutative, associative
         * (note: mbc is the MEET of b and c — the first draft of
         * this check accidentally reused `bc`, the JOIN, and the
         * assertion correctly caught the mistake: min(a, max(b,c))
         * is not min(a, min(b,c)). The battery bites on test bugs
         * too, which is the point.) */
        assert(if5_cut_meet(w, &b, &a, &ba));
        assert(meet_ab.prefix == ba.prefix);
        assert(if5_cut_meet(w, &a, &c, &ac));
        assert(if5_cut_meet(w, &ac, &b, &ac_b));
        {
            if5_cut mbc;
            assert(if5_cut_meet(w, &b, &c, &mbc));
            assert(if5_cut_meet(w, &a, &mbc, &a_bc2));
        }
        assert(ac_b.prefix == a_bc2.prefix);

        /* order consistency: a <= join(a,b); meet(a,b) <= a */
        assert(if5_cut_le(w, &a, &ab));
        assert(if5_cut_le(w, &meet_ab, &a));
        /* reflexivity, antisymmetry on principal cuts */
        assert(if5_cut_le(w, &a, &a));
        if (if5_cut_le(w, &a, &b) && if5_cut_le(w, &b, &a))
            assert(a.prefix == b.prefix);
    }
    printf("(%u rounds) ", CUT_ROUNDS);
    if5_world_free(w);
}

/* ============================================================
 * V2 — join's registered boundary: pruned inputs are refused
 * ============================================================ */

static void test_join_refuses_pruned(void)
{
    if5_world* w = mk_world();
    if5_cut p, q, out;

    fill(w);
    p = if5_cut_at(w, if5_log_count(w));
    q = p;
    assert(if5_cut_prune(w, &p, 3));

    /* join must refuse the pruned input (the union of pruned
     * addresses is not prefix+prune representable — ADR-0026 3b). */
    assert(!if5_cut_join(w, &p, &q, &out));
    assert(!if5_cut_join(w, &q, &p, &out));

    /* meet stays total: the intersection is representable. */
    assert(if5_cut_meet(w, &p, &q, &out));
    assert(out.prefix == q.prefix && out.n_prunes == 1);
    assert(if5_cut_is_down_closed(w, &out));
    if5_world_free(w);
}

/* ============================================================
 * V2 — every constructor output is down-closed (the public
 * verifier, run over randomized prunes and meets)
 * ============================================================ */

static void test_algebra_outputs_down_closed(void)
{
    if5_world* w = mk_world();
    uint32_t i;

    fill(w);
    for (i = 0; i < CUT_ROUNDS; i++) {
        if5_cut a = rnd_pruned_cut(w), b = rnd_pruned_cut(w), m;
        assert(if5_cut_is_down_closed(w, &a));
        assert(if5_cut_is_down_closed(w, &b));
        assert(if5_cut_meet(w, &a, &b, &m));
        assert(if5_cut_is_down_closed(w, &m));
    }
    printf("(%u rounds) ", CUT_ROUNDS);
    if5_world_free(w);
}

/* ============================================================
 * V3 — the independent membership fold
 * ============================================================ */

/* The test's OWN exclusion twin (independent of the core's
 * predicate — the check would be vacuous otherwise). */
static bool revoked(const if5_world* w, const if5_intent* rec,
                    const if5_cut* c)
{
    uint32_t i;
    for (i = 0; i < c->n_prunes; i++) {
        const if5_intent* e = if5_log_at((if5_world*)w, c->prunes[i]);
        if ((rec->flags & IF5_F_REBORN) && rec->src == e->src) continue;
        if (rec->vc.c[e->src] >= e->vc.c[e->src]) return true;
    }
    return false;
}

/* Membership, reimplemented test-side. */
static bool member(const if5_world* w, const if5_cut* c, uint32_t iid)
{
    if (iid == 0 || iid > c->prefix) return false;
    return !revoked(w, if5_log_at((if5_world*)w, iid), c);
}

/* The independent fold for a LATTICE region (NUM): the max over
 * every contained, applied, value-type record on the region —
 * sharing no code with the core's fold. */
static uint64_t indep_fold_num(const if5_world* w, const if5_cut* c,
                               uint16_t rid, uint32_t* contrib)
{
    uint64_t acc = 0;
    uint32_t i, n = 0;
    for (i = 1; i <= c->prefix; i++) {
        const if5_intent* q = if5_log_at((if5_world*)w, i);
        if (q->region != rid) continue;
        if (!(q->flags & IF5_F_APPLIED)) continue;
        if (q->flags & IF5_F_EXCLUDED) continue;
        if (q->ptype == IF5_T_TIER || q->ptype == IF5_T_DELEG ||
            q->ptype == IF5_T_RECEIPT || q->ptype == IF5_T_CURSOR ||
            q->ptype == IF5_T_EXPORT || q->ptype == IF5_T_ARBIT)
            continue;
        if (revoked(w, q, c)) continue;
        {
            uint64_t v = ((uint64_t)q->value[1] << 32) | q->value[0];
            if (v > acc) acc = v;
            n++;
        }
    }
    if (contrib) *contrib = n;
    return acc;
}

static void test_cut_state_matches_independent_fold(void)
{
    if5_world* w = mk_world();
    uint16_t rb = 0, rt = 0;
    uint32_t i;

    fill(w);
    (void)if5_undo_nth(w, 0, 4);          /* put a prune in the world */
    rb = if5_region_find(w, "budget");
    rt = if5_region_find(w, "tags");

    /* Principal cuts across the whole log. */
    for (i = 0; i <= if5_log_count(w); i++) {
        if5_cut c = if5_cut_at(w, i);
        uint32_t c1 = 0, c2 = 0;
        uint64_t got = if5_cut_state(w, &c, rb, &c1);
        uint64_t want = indep_fold_num(w, &c, rb, &c2);
        assert(got == want);
        assert(c1 == c2);
    }
    /* Pruned cuts: the undo's own address, plus randomized prunes. */
    {
        if5_view v = if5_now(w);
        if5_cut c = if5_cut_of_view(&v);
        uint32_t c1 = 0, c2 = 0;
        assert(if5_cut_state(w, &c, rb, &c1)
               == indep_fold_num(w, &c, rb, &c2));
        assert(c1 == c2);
    }
    for (i = 0; i < CUT_ROUNDS; i++) {
        if5_cut c = rnd_pruned_cut(w);
        uint32_t c1 = 0, c2 = 0;
        uint64_t got = if5_cut_state(w, &c, rb, &c1);
        uint64_t want = indep_fold_num(w, &c, rb, &c2);
        if (got != want || c1 != c2) {
            fprintf(stderr, "\nV3 MISMATCH: cut prefix=%u prunes=%u "
                    "got=%llu want=%llu contrib %u/%u\n",
                    c.prefix, c.n_prunes,
                    (unsigned long long)got, (unsigned long long)want,
                    c1, c2);
            assert(false);
        }
        /* The SET region too (union lattice): independent fold as
         * bitwise OR of the low tags. */
        got = if5_cut_state(w, &c, rt, &c1);
        {
            uint64_t acc = 0;
            uint32_t k, n = 0;
            for (k = 1; k <= c.prefix; k++) {
                const if5_intent* q = if5_log_at(w, k);
                if (q->region != rt) continue;
                if (!(q->flags & IF5_F_APPLIED)) continue;
                if (q->flags & IF5_F_EXCLUDED) continue;
                if (q->ptype == IF5_T_TIER) continue;
                if (revoked(w, q, &c)) continue;
                acc |= ((uint64_t)q->value[1] << 32) | q->value[0];
                n++;
            }
            assert(got == acc);
            assert(c1 == n);
        }
    }
    printf("(%u pruned rounds) ", CUT_ROUNDS);
    if5_world_free(w);
}

/* ============================================================
 * V3 — the prune constructor's addressed set is the sugar's
 * rewound view (the address algebra and the undo agree)
 * ============================================================ */

static void test_prune_is_the_undo_address(void)
{
    if5_world* w = mk_world();
    if5_view rewound;
    if5_cut c;
    uint32_t target, k, n_log;

    fill(w);
    n_log = if5_log_count(w);
    target = n_log - 1;                   /* undo the last budget write */
    rewound = if5_view_rewind(w, NULL, target);

    c = if5_cut_at(w, n_log);
    assert(if5_cut_prune(w, &c, target));

    /* The represented sets agree record-by-record: membership in
     * the pruned cut == non-revocation under the rewound view
     * (both evaluated by the test's own independent twins). */
    for (k = 1; k <= n_log; k++) {
        const if5_intent* q = if5_log_at(w, k);
        bool in_cut = member(w, &c, k);
        bool in_view = true;
        uint32_t i;
        for (i = 0; i < rewound.n_excl; i++) {
            const if5_intent* e = if5_log_at(w, rewound.excl[i]);
            if ((q->flags & IF5_F_REBORN) && q->src == e->src) continue;
            if (q->vc.c[e->src] >= e->vc.c[e->src]) { in_view = false; break; }
        }
        assert(in_cut == in_view);
    }
    /* And every region folds identically through both addresses. */
    {
        uint16_t r;
        for (r = 1; r <= 4; r++) {
            uint32_t c1 = 0, c2 = 0;
            uint64_t a = if5_cut_state(w, &c, r, &c1);
            uint64_t b = if5_state_at(w, r, &rewound, &c2);
            assert(a == b);
            assert(c1 == c2);
        }
    }
    if5_world_free(w);
}

/* ============================================================
 * The registry: named addresses fold identically to their values
 * ============================================================ */

static void test_registry_addresses(void)
{
    if5_world* w = mk_world();
    uint8_t id1, id2;
    const if5_cut* q;
    if5_cut c;

    fill(w);
    c = if5_cut_at(w, if5_log_count(w) / 2);
    id1 = if5_cut_register(w, &c);
    assert(id1 == 1);
    assert(if5_cut_count(w) == 1);
    q = if5_cut_by_id(w, id1);
    assert(q && q->prefix == c.prefix);

    /* A pruned, registered address. */
    assert(if5_cut_prune(w, &c, 2));
    id2 = if5_cut_register(w, &c);
    assert(id2 == 2 && if5_cut_count(w) == 2);

    /* Fold by id == fold by value. */
    {
        uint16_t r;
        for (r = 1; r <= 4; r++) {
            uint32_t c1 = 0, c2 = 0;
            uint64_t a = if5_cut_state(w, if5_cut_by_id(w, id2), r, &c1);
            uint64_t b = if5_cut_state(w, &c, r, &c2);
            assert(a == b && c1 == c2);
        }
    }
    /* Invalid ids read as absent. */
    assert(if5_cut_by_id(w, 0) == NULL);
    assert(if5_cut_by_id(w, 3) == NULL);
    /* The registry refuses to exceed capacity (fill it). */
    {
        uint8_t k;
        for (k = if5_cut_count(w); k < IF5_MAX_CUTS; k++)
            assert(if5_cut_register(w, &c) != 0);
        assert(if5_cut_register(w, &c) == 0);
    }
    if5_world_free(w);
}

int main(void)
{
    printf("=== v5 cut suite (ADR-0026 V2/V3: the algebra's laws and "
           "the address checks) ===\n\n");
    TEST(lattice_laws_hold);
    TEST(join_refuses_pruned);
    TEST(algebra_outputs_down_closed);
    TEST(cut_state_matches_independent_fold);
    TEST(prune_is_the_undo_address);
    TEST(registry_addresses);
    printf("\n%d/%d cut sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
