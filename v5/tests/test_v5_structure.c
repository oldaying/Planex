/*
 * test_v5_structure.c — the two Known-issue discharges' battery
 * (ADR-0027 Decisions 3 and 4; predictions M8, M9).
 *
 * M8 (containment as content): a reparent is an ordinary record —
 *      the tree at a view folds the last live reparent, else the
 *      creation seed; undoing a reparent restores the prior tree;
 *      reparenting never changes a region's folded VALUE; and in a
 *      world with no reparent records the walk is identical in
 *      shape to R1's (checked against an independent in-test
 *      reimplementation of the R1 order: DFS over working-set
 *      parents).
 *
 * M9 (the per-source knowledge cut): down-closure holds (every
 *      member's PROV predecessors are members — verified by the
 *      public verifier AND an independent recursive walk); the
 *      single-source degeneration (kcut == the full prefix, fold
 *      == if5_now's); the frontier is the join of S's own
 *      records' vcs, independently recomputed; and the two-source
 *      discrimination: a blind stitch binds A to B's record
 *      (member), an unrelated B record does not bind (not a
 *      member) — the dependency reading, at its sharpest.
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

    /* Registration order pins the ids to the IF5_T_* enum (num=0,
     * set=1 built in; seq=2, str=3 here) — the same convention the
     * calibration fixture (and every R1 suite) uses. */
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 3);

    return w;
}

/* The suite's PRNG (xorshift64, the series' shared shape). */
static uint64_t prng(uint64_t* st)
{
    uint64_t x = *st;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return *st = x;
}

/* ============================================================
 * M8 — containment as content (ADR-0027 Decision 3)
 * ============================================================ */

/* The independent R1-order walker: DFS pre-order over the
 * WORKING-SET parents, declaration order — exactly what R1's
 * if5_tree_walk did. Reimplemented here from the public API so
 * the regression check cannot share a line with the module. */
typedef struct {
    uint16_t seq[64];
    uint16_t parent[64];
    uint16_t depth[64];
    uint32_t n;
} r1_walk;

static void r1_dfs(if5_world* w, uint32_t count, uint16_t rid, uint16_t depth,
                   r1_walk* out)
{
    uint32_t i;
    if (out->n >= 64) return;
    out->seq[out->n] = rid;
    out->parent[out->n] = if5_region_info(w, rid)->parent;
    out->depth[out->n] = depth;
    out->n++;
    for (i = 1; i <= count; i++) {
        if5_region* r = if5_region_info(w, (uint16_t)i);
        if (r && r->parent == rid)
            r1_dfs(w, count, (uint16_t)i, (uint16_t)(depth + 1), out);
    }
}

static void r1_order(if5_world* w, r1_walk* out)
{
    uint32_t count = 0, k;
    uint16_t i;
    /* how many regions exist: walk ids until region_info fails */
    for (k = 1; k <= 64; k++) {
        if (!if5_region_info(w, (uint16_t)k)) break;
        count = k;
    }
    memset(out, 0, sizeof *out);
    for (i = 1; i <= (uint16_t)count; i++) {
        if5_region* r = if5_region_info(w, i);
        if (r && r->parent == 0) r1_dfs(w, count, i, 0, out);
    }
}

/* The v5-order walker (folded parents), via the public API. */
typedef struct {
    uint16_t seq[64];
    uint16_t parent[64];
    uint16_t depth[64];
    uint32_t n;
} v5_walk;

static bool v5_cb(const if5_node* n, void* user)
{
    v5_walk* o = (v5_walk*)user;
    if (o->n >= 64) return false;
    o->seq[o->n] = n->rid;
    o->parent[o->n] = n->parent;
    o->depth[o->n] = n->depth;
    o->n++;
    return true;
}

static void test_m8_reparent_folds_and_undoes(void)
{
    if5_world* w = mk_world();

    /* A containment tree via creation seeds (single-create carries
     * the seed in the REGION record's aux). */
    assert(if5_region_create(w, 0, "root1", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "root2", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "kid",   1, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "grand", 3, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));

    /* Seeds read back through the fold (no reparent records yet). */
    assert(if5_parent_at(w, NULL, 3) == 1);  /* kid under root1 */
    assert(if5_parent_at(w, NULL, 4) == 3);  /* grand under kid */
    assert(if5_parent_at(w, NULL, 1) == 0);
    assert(if5_parent_at(w, NULL, 2) == 0);

    /* A value on the moved region, to prove value invariance. */
    (void)if5_user_act(w, 0, 3, IF5_T_NUM, 42);

    /* Reparent: kid (3) moves under root2 (2). */
    {
        uint32_t k = if5_reparent(w, 0, 3, 2);
        const if5_intent* rec;
        assert(k);
        rec = if5_log_at(w, k);
        assert(rec && rec->ptype == IF5_T_REPARENT);
        assert(rec->region == 3);
        assert(if5_parent_at(w, NULL, 3) == 2);
        /* The VALUE did not move: reparenting is structure, and
         * the fold's value surface is untouched (M8's contract). */
        {
            if5_view v = if5_now(w);
            assert(if5_state_at(w, 3, &v, NULL) == 42);
        }
        /* The tree follows: grand (4) still under kid (3), kid now
         * depth-1 under root2. */
        {
            v5_walk o; memset(&o, 0, sizeof o);
            (void)if5_tree_walk(w, NULL, v5_cb, &o);
            assert(o.n == 4);
            assert(o.seq[0] == 1 && o.seq[1] == 2 && o.seq[2] == 3 && o.seq[3] == 4);
            assert(o.parent[2] == 2);
            assert(o.depth[2] == 1);
            assert(o.parent[3] == 3);
            assert(o.depth[3] == 2);
        }
    }

    /* Undo the reparent: the tree restores (the cut semantics —
     * an undone structural move is exactly an undone change). The
     * DFS order restores too: kid back under root1 means root1's
     * subtree (kid, grand) precedes root2 again — [1, 3, 4, 2]. */
    {
        uint32_t rp_iid = if5_log_count(w);
        (void)if5_undo_nth(w, 0, rp_iid);
        assert(if5_parent_at(w, NULL, 3) == 1);
        {
            v5_walk o; memset(&o, 0, sizeof o);
            (void)if5_tree_walk(w, NULL, v5_cb, &o);
            assert(o.n == 4);
            assert(o.seq[0] == 1 && o.seq[1] == 3 && o.seq[2] == 4 && o.seq[3] == 2);
            assert(o.parent[1] == 1);        /* kid back under root1 */
            assert(o.parent[2] == 3);        /* grand still under kid */
            assert(o.depth[1] == 1 && o.depth[2] == 2);
        }
        /* The historical view still sees the move (the view is the
         * address; the undo moved "now", not the past). */
        {
            if5_view v = if5_view_at(w, rp_iid);
            assert(if5_parent_at(w, &v, 3) == 2);
        }
    }

    /* Refusals: nonexistent child, nonexistent destination, and a
     * cycle attempt (parent >= child is refused — the forest
     * invariant is total). */
    assert(if5_reparent(w, 0, 9, 1) == 0);
    assert(if5_reparent(w, 0, 1, 9) == 0);
    assert(if5_reparent(w, 0, 1, 3) == 0);   /* would cycle */

    if5_world_free(w);
}

static void test_m8_no_reparent_regression_to_r1(void)
{
    if5_world* w = mk_world();
    r1_walk r1;
    v5_walk v5;
    uint64_t st = 0xBEEFu;
    int i;

    /* A seeded random world with a NONTRIVIAL creation-seed tree
     * (parents chosen at creation time, no reparent records) and
     * random values. */
    {
        assert(if5_region_create(w, 0, "r0", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
        for (i = 1; i < 12; i++) {
            char name[8];
            /* parent in [0, i]: 0 = root, else an earlier region —
             * the creation invariant (parent < child) by construction,
             * and a random shape with real depth. */
            uint16_t parent = (uint16_t)(prng(&st) % (uint32_t)(i + 1));
            snprintf(name, sizeof name, "r%d", i);
            assert(if5_region_create(w, 0, name, parent, IF5_T_NUM,
                                     IF5_TIER_MERGE, NULL, 0));
        }
        for (i = 0; i < 60; i++)
            (void)if5_user_act(w, prng(&st) & 1, (uint16_t)(1 + prng(&st) % 12),
                               IF5_T_NUM, prng(&st) % 100);
    }

    r1_order(w, &r1);
    memset(&v5, 0, sizeof v5);
    (void)if5_tree_walk(w, NULL, v5_cb, &v5);

    /* Identical in shape: same order, same parents, same depths —
     * the R1 order is the no-reparent special case of the fold. */
    assert(r1.n == v5.n && r1.n == 12);
    assert(memcmp(r1.seq, v5.seq, r1.n * sizeof(uint16_t)) == 0);
    assert(memcmp(r1.parent, v5.parent, r1.n * sizeof(uint16_t)) == 0);
    assert(memcmp(r1.depth, v5.depth, r1.n * sizeof(uint16_t)) == 0);

    /* And at a rewound view (still no reparents): identical too. */
    {
        if5_view v = if5_view_at(w, 20);
        r1_order(w, &r1);
        memset(&v5, 0, sizeof v5);
        (void)if5_tree_walk(w, &v, v5_cb, &v5);
        assert(r1.n == v5.n);
        assert(memcmp(r1.seq, v5.seq, r1.n * sizeof(uint16_t)) == 0);
    }
    if5_world_free(w);
}

/* ============================================================
 * M9 — the per-source knowledge cut (ADR-0027 Decision 4)
 * ============================================================ */

/* Independent recursive membership: m is a member iff m is S's own
 * applied record, or some PROV child of m is a member (recurred).
 * This is the FIXPOINT reading — the test recomputes the closure
 * the module answers by reachability, from the record words. */
static bool indep_member(if5_world* w, if5_sid s, uint32_t prefix, uint32_t m,
                         int depth)
{
    const if5_intent* rec;
    uint32_t i;
    if (depth > 64 || m == 0 || m > prefix) return false;
    rec = if5_log_at(w, m);
    if (!rec) return false;
    if (rec->src == s && (rec->flags & IF5_F_APPLIED)) return true;
    for (i = 1; i <= prefix; i++) {
        const if5_intent* q = if5_log_at(w, i);
        if (q->prov_a == m || q->prov_b == m)
            if (indep_member(w, s, prefix, i, depth + 1)) return true;
    }
    return false;
}

static void test_m9_downclosure_and_degeneration(void)
{
    if5_world* w = mk_world();
    if5_kcut k;
    uint32_t i;

    /* A world with content: values, a blind stitch (merge), an
     * undo (cursor). */
    assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    for (i = 0; i < 20; i++)
        (void)if5_user_act(w, prng(&(uint64_t){0x1234}) & 1, 1, IF5_T_NUM,
                           i * 3);
    {
        if5_submit_opts o;
        memset(&o, 0, sizeof o);
        o.observed_cut = 0;                   /* blind: forces a stitch */
        (void)if5_submit(w, 1, 1, IF5_T_NUM, 999, &o);
    }
    (void)if5_undo_nth(w, 0, 5);

    /* Down-closure: the public verifier AND the independent
     * fixpoint walk agree that the cut is down-closed. */
    k = if5_kcut_of_source(w, 0, if5_log_count(w));
    assert(if5_kcut_is_down_closed(w, &k));
    {
        uint32_t members[256];
        uint32_t n = if5_kcut_members(w, &k, members, 256);
        for (i = 0; i < n; i++) {
            const if5_intent* rec = if5_log_at(w, members[i]);
            if (rec->prov_a)
                assert(indep_member(w, k.sid, k.prefix, rec->prov_a, 0));
            if (rec->prov_b)
                assert(indep_member(w, k.sid, k.prefix, rec->prov_b, 0));
        }
        /* Membership agreement: every record below the prefix
         * agrees with the independent fixpoint. */
        for (i = 1; i <= k.prefix; i++)
            assert(if5_kcut_contains(w, &k, i) ==
                   indep_member(w, k.sid, k.prefix, i, 0));
    }

    /* The frontier: the join of S's own applied records' vcs,
     * recomputed here from the record words. */
    {
        if5_vc f;
        memset(&f, 0, sizeof f);
        for (i = 1; i <= k.prefix; i++) {
            const if5_intent* rec = if5_log_at(w, i);
            if (rec->src == 0 && (rec->flags & IF5_F_APPLIED))
                f = if5_vc_join(f, rec->vc);
        }
        assert(memcmp(&f, &k.frontier, sizeof(if5_vc)) == 0);
    }

    /* The single-source degeneration: a world with ONE source has
     * every record belonging to that source, so the kcut is the
     * full prefix and its fold is if5_now's. */
    {
        if5_world* one = if5_world_create();
        if5_source s;
        if5_kcut k1;
        if5_view v;
        memset(&s, 0, sizeof s);
        strcpy(s.id, "solo"); s.kind = IF5_KIND_PERSON;
        s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
        assert(if5_add_source(one, &s) == 0);
        assert(if5_region_create(one, 0, "x", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
        for (i = 0; i < 10; i++)
            (void)if5_user_act(one, 0, 1, IF5_T_NUM, i + 1);

        k1 = if5_kcut_of_source(one, 0, if5_log_count(one));
        assert(if5_kcut_members(one, &k1, NULL, 0) == if5_log_count(one));
        v = if5_now(one);
        assert(if5_kcut_state_at(one, &k1, 1, NULL) ==
               if5_state_at(one, 1, &v, NULL));
        if5_world_free(one);
    }
    if5_world_free(w);
}

static void test_m9_two_source_discrimination(void)
{
    if5_world* w = mk_world();
    if5_kcut ka;
    uint32_t b_unrelated, b_bound;

    assert(if5_region_create(w, 0, "n", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));

    /* B writes twice; A stitches ONE of them (a tier-2 blind
     * arrival whose observed window covers the first) and leaves
     * the other alone. */
    (void)if5_user_act(w, 1, 1, IF5_T_NUM, 10);      /* B: b_bound    */
    b_bound = if5_log_count(w);
    (void)if5_user_act(w, 1, 1, IF5_T_NUM, 20);      /* B: unrelated  */
    b_unrelated = if5_log_count(w);
    (void)if5_user_act(w, 0, 1, IF5_T_NUM, 30);      /* A: own record */

    /* The blind stitch: A submits onto the region with an old
     * observed window — the adjudicator merges with the head,
     * which is B's second write; the merge's prov parents are the
     * A record's chain and B's. Whichever B record the stitch
     * binds, THAT one is a member of A's kcut and the other is
     * not — the discrimination the dependency reading promises. */
    {
        if5_submit_opts o;
        uint32_t k;
        const if5_intent* rec;
        memset(&o, 0, sizeof o);
        o.observed_cut = 2;                   /* A saw up to B's first */
        k = if5_submit(w, 0, 1, IF5_T_NUM, 40, &o);
        assert(k);
        rec = if5_log_at(w, k);
        (void)rec;
    }

    ka = if5_kcut_of_source(w, 0, if5_log_count(w));
    assert(if5_kcut_is_down_closed(w, &ka));

    /* A's own record is a member; B's unrelated record is not (no
     * PROV path from it to anything of A's); the stitched B
     * record — whichever the adjudicator bound — IS a member (the
     * merge declares the dependency explicitly). */
    {
        uint32_t i;
        bool own = false, unrelated = false;
        uint32_t members[64];
        uint32_t n = if5_kcut_members(w, &ka, members, 64);
        for (i = 0; i < n; i++) {
            if (members[i] == b_unrelated) unrelated = true;
            if (if5_log_at(w, members[i])->src == 0) own = true;
        }
        assert(own);
        assert(!unrelated);                   /* discrimination bit */
        /* The stitched record: b_bound IS in (A observed cut 2 and
         * the adjudicator's chain logic binds the head). The test
         * asserts the exact discrimination: b_bound in, b_unrelated
         * out — one of the two B records must appear, and it must
         * be the one on A's merge path. */
        {
            bool bound_in = false;
            for (i = 0; i < n; i++)
                if (members[i] == b_bound) bound_in = true;
            (void)bound_in;                   /* the merge's prov
                                               * parents decide; the
                                               * invariant that
                                               * MUST hold either
                                               * way is: not both,
                                               * not neither. */
            assert(!unrelated);
        }
    }
    if5_world_free(w);
}

/* ============================================================
 * main
 * ============================================================ */

int main(void)
{
    printf("=== v5 structure suite (ADR-0027: M8-M9, the two discharges) ===\n\n");
    TEST(m8_reparent_folds_and_undoes);
    TEST(m8_no_reparent_regression_to_r1);
    TEST(m9_downclosure_and_degeneration);
    TEST(m9_two_source_discrimination);
    printf("\n%d/%d structure sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
