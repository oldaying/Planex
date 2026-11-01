/*
 * test_v5_seats.c — the three seats' battery (ADR-0026 Decisions
 * 3c/3d/3e; predictions V4, V5, V6).
 *
 * V4 (S15): the semantic tree is total (every region, exactly
 * once, at every view), deterministic (two walks byte-identical),
 * ordered (depth-first pre-order, declaration order), and carries
 * the full semantic content (name, value, role, tier,
 * contributions, PROV anchor).
 *
 * V5 (S17): the surprise timetable is PROV-backed (every entry
 * resolves to a log record of the right family), itemizes the
 * surprise debits exactly (the debit word's class packing),
 * filters per source, and is view-addressable (an undone record
 * leaves the timetable at that view).
 *
 * V6 (S24): the two-layer boundary — every projection (the three
 * text projections, the tree, the schedule) leaves the durable
 * layer's byte image unchanged.
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
 * Fixtures
 * ============================================================ */

static if5_world* mk_world(uint32_t sys_budget, uint32_t b_budget)
{
    if5_world* w = if5_world_create();
    if5_source s;
    if5_payload_type t;
    memset(&s, 0, sizeof s);
    strcpy(s.id, "human");  s.kind = IF5_KIND_PERSON;
    s.initiative_budget = 0xFFFFFFFFu; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 0);
    strcpy(s.id, "agentA"); s.kind = IF5_KIND_AGENT;
    s.initiative_budget = b_budget; s.authority = 0xFF;
    assert(if5_add_source(w, &s) == 1);
    strcpy(s.id, "system"); s.kind = IF5_KIND_PROCESS;
    s.initiative_budget = sys_budget; s.authority = 0;
    assert(if5_add_source(w, &s) == 2);
    if5_set_system_source(w, 2);
    memset(&t, 0, sizeof t); t.name = "seq"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t); t.name = "str"; t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 3);
    return w;
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

/* A walk collector. */
typedef struct walk_acc {
    if5_node nodes[64];
    uint32_t n;
} walk_acc;

static bool collect(const if5_node* n, void* user)
{
    walk_acc* a = (walk_acc*)user;
    a->nodes[a->n++] = *n;
    return true;
}

/* ============================================================
 * V4 — totality, determinism, order, content
 * ============================================================ */

static void test_s15_tree_total_deterministic(void)
{
    if5_world* w = mk_world(64, 100);
    walk_acc a, b;
    if5_view v;
    uint32_t i;

    /* A containment hierarchy, exercising the S15 parent:
     *   trip (root)
     *   +- budget (NUM, tier 2)
     *   +- itinerary (root-level container)
     *      +- stops (rigid, tier 1)
     *      +- tags (SET, tier 2)
     *   title (root, rigid, tier 3)
     */
    assert(if5_region_create(w, 0, "trip", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "budget", if5_region_find(w, "trip"),
                             IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "itinerary", 0, IF5_T_NUM,
                             IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "stops", if5_region_find(w, "itinerary"),
                             3 /*str, rigid*/, IF5_TIER_FORK, NULL, 0));
    assert(if5_region_create(w, 0, "tags", if5_region_find(w, "itinerary"),
                             IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "title", 0, 3, IF5_TIER_STRICT, NULL, 0));

    submit(w, 0, "budget", IF5_T_NUM, 1200, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 1350, 0, 0, 0);      /* concurrent */
    submit(w, 0, "stops", 3, 0x41, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "stops", 3, 0x42, 0, 0, 0);               /* fork */
    submit(w, 0, "tags", IF5_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "tags", IF5_T_SET, 0x2, 0, 0, 0);         /* concurrent */
    submit(w, 0, "title", 3, 0x43, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "title", 3, 0x44, 0, 0, 0);               /* tier-3 pair */

    v = if5_now(w);
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    assert(if5_tree_walk(w, &v, collect, &a) == 6);
    assert(if5_tree_walk(w, &v, collect, &b) == 6);

    /* Totality: every region exactly once. */
    assert(a.n == 6);
    {
        bool seen[7] = { false };
        for (i = 0; i < a.n; i++) {
            assert(a.nodes[i].rid >= 1 && a.nodes[i].rid <= 6);
            assert(!seen[a.nodes[i].rid]);
            seen[a.nodes[i].rid] = true;
        }
        for (i = 1; i <= 6; i++) assert(seen[i]);
    }
    /* Determinism: two walks are byte-identical. */
    assert(memcmp(a.nodes, b.nodes, sizeof(if5_node) * a.n) == 0);

    /* Order: depth-first pre-order, declaration order:
     * trip, budget, itinerary, stops, tags, title. */
    assert(strcmp(a.nodes[0].name, "trip") == 0);
    assert(strcmp(a.nodes[1].name, "budget") == 0);
    assert(strcmp(a.nodes[2].name, "itinerary") == 0);
    assert(strcmp(a.nodes[3].name, "stops") == 0);
    assert(strcmp(a.nodes[4].name, "tags") == 0);
    assert(strcmp(a.nodes[5].name, "title") == 0);

    /* Containment and depth. */
    assert(a.nodes[0].parent == 0 && a.nodes[0].depth == 0);
    assert(a.nodes[1].parent == 1 && a.nodes[1].depth == 1);
    assert(a.nodes[2].parent == 0 && a.nodes[2].depth == 0);
    assert(a.nodes[3].parent == 3 && a.nodes[3].depth == 1);
    assert(a.nodes[4].parent == 3 && a.nodes[4].depth == 1);
    assert(a.nodes[5].parent == 0 && a.nodes[5].depth == 0);

    /* Content: values, roles, tiers, contributions, anchors. */
    for (i = 0; i < a.n; i++) {
        const if5_region* r = if5_region_info(w, a.nodes[i].rid);
        uint32_t contrib = 0;
        uint64_t val = if5_state_at(w, a.nodes[i].rid, &v, &contrib);
        assert(r != NULL);
        assert(strcmp(a.nodes[i].name, r->name) == 0);
        assert(a.nodes[i].value == val);
        assert(a.nodes[i].contrib == contrib);
        assert(a.nodes[i].ptype == r->ptype);
        assert(a.nodes[i].tier == r->tier);
        /* The PROV anchor resolves to an applied, value-type
         * record on this region — or the region is untouched. */
        if (a.nodes[i].last_iid != 0) {
            const if5_intent* q = if5_log_at(w, a.nodes[i].last_iid);
            assert(q && q->region == a.nodes[i].rid);
            assert(q->flags & IF5_F_APPLIED);
        } else {
            assert(contrib == 0);
        }
    }

    /* The count-only form agrees (cb == NULL). */
    assert(if5_tree_walk(w, &v, NULL, NULL) == 6);
    if5_world_free(w);
}

/* ============================================================
 * V4 — the tree is a live projection: views address it
 * ============================================================ */

static void test_s15_tree_live_at_views(void)
{
    if5_world* w = mk_world(64, 100);
    walk_acc now_a, then_a;
    if5_view now, then;
    uint32_t i;

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "tags", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "budget", IF5_T_NUM, 200, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "tags", IF5_T_SET, 0x1, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "budget", IF5_T_NUM, 300, 0xFFFFFFFFu, 0, 0);
    submit(w, 0, "tags", IF5_T_SET, 0x2, 0xFFFFFFFFu, 0, 0);

    now = if5_now(w);
    memset(&now_a, 0, sizeof now_a);
    assert(if5_tree_walk(w, &now, collect, &now_a) == 2);
    assert(now_a.nodes[0].value == 300);
    assert(now_a.nodes[1].value == 0x3);

    /* A historical view: the tree folds THEN. Cut 7 = after the
     * first three writes (the log carries the two region opens'
     * REGION+TIER records first). */
    then = if5_view_at(w, 7);
    memset(&then_a, 0, sizeof then_a);
    assert(if5_tree_walk(w, &then, collect, &then_a) == 2);
    assert(then_a.nodes[0].value == 200);
    assert(then_a.nodes[1].value == 0x1);

    /* A rewound view: undoing the 300 (record 8) takes its causal
     * descendant (the later 0x2 tag) with it — the tree changes
     * exactly as the fold changes. */
    {
        if5_view rewound = if5_view_rewind(w, &now, 8);
        walk_acc r_a;
        memset(&r_a, 0, sizeof r_a);
        assert(if5_tree_walk(w, &rewound, collect, &r_a) == 2);
        assert(r_a.nodes[0].value == 200);   /* the 300 is gone */
        assert(r_a.nodes[1].value == 0x1);   /* the 0x2 tag too */
        (void)i;
    }
    if5_world_free(w);
}

/* ============================================================
 * V5 — the surprise timetable
 * ============================================================ */

/* The test's own exclusion twin (independence from the core). */
static bool revoked5(const if5_world* w, const if5_intent* rec,
                     const if5_view* v)
{
    uint32_t i;
    for (i = 0; i < v->n_excl; i++) {
        const if5_intent* e = if5_log_at((if5_world*)w, v->excl[i]);
        if ((rec->flags & IF5_F_REBORN) && rec->src == e->src) continue;
        if (rec->vc.c[e->src] >= e->vc.c[e->src]) return true;
    }
    return false;
}

static void test_s17_prov_backed_and_recounted(void)
{
    if5_world* w = mk_world(64, 60);
    if5_sched_entry ent[128];
    if5_view v;
    uint32_t n, i;

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "tags", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 200, 0, 1, IF5_SEV_AMBIENT);    /* initiative */
    submit(w, 1, "budget", IF5_T_NUM, 300, 0, 2, IF5_SEV_FOREGROUND); /* surprise */
    submit(w, 1, "tags", IF5_T_SET, 0x4, 0, 2, IF5_SEV_MODAL);        /* surprise */
    assert(if5_region_set_tier(w, 0, if5_region_find(w, "tags"), IF5_TIER_STRICT));
    submit(w, 0, "tags", IF5_T_SET, 0x8, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "tags", IF5_T_SET, 0x10, 0, 0, 0);      /* tier-3 pair */
    submit(w, 0, "budget", IF5_T_NUM, 400, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 500, 0, 0, 0);      /* concurrent merge */
    submit(w, 1, "budget", IF5_T_NUM, 600, 0xFFFFFFFFu, 1, IF5_SEV_MODAL); /* breaker */

    v = if5_now(w);
    n = if5_schedule(w, &v, IF5_SID_NULL, ent, 128);
    assert(n > 0 && n <= 128);

    /* Every entry is PROV-backed and correctly classified. */
    for (i = 0; i < n; i++) {
        const if5_intent* q = if5_log_at(w, ent[i].iid);
        assert(q != NULL);
        assert(ent[i].sid == q->src);
        assert(ent[i].region == q->region);
        switch (ent[i].kind) {
        case IF5_SCHED_SURPRISE:
            assert(q->debit != 0);
            assert(IF5_DEBIT_CLASS(q->debit) == IF5_DEBIT_SURPRISE);
            assert(ent[i].magnitude == IF5_DEBIT_SEV(q->debit));
            break;
        case IF5_SCHED_BREAKER: assert(q->ptype == IF5_T_RECEIPT); break;
        case IF5_SCHED_TIER:    assert(q->ptype == IF5_T_TIER);    break;
        case IF5_SCHED_ARBIT:   assert(q->ptype == IF5_T_ARBIT);   break;
        case IF5_SCHED_MERGE:
            assert(q->ptype == IF5_T_MERGE ||
                   (q->flags & IF5_F_CONCUR_MERGE));
            break;
        default: assert(false);
        }
    }

    /* The independent recount: a test-side scan of the log under
     * the same visibility rule must find exactly the same count. */
    {
        uint32_t count = 0, k;
        for (k = 1; k <= if5_log_count(w); k++) {
            const if5_intent* q = if5_log_at(w, k);
            uint8_t kind = 0;
            if (revoked5(w, q, &v)) continue;
            if (q->debit != 0 && IF5_DEBIT_CLASS(q->debit) == IF5_DEBIT_SURPRISE)
                kind = IF5_SCHED_SURPRISE;
            else switch (q->ptype) {
            case IF5_T_RECEIPT: kind = IF5_SCHED_BREAKER; break;
            case IF5_T_TIER:    kind = IF5_SCHED_TIER;    break;
            case IF5_T_ARBIT:   kind = IF5_SCHED_ARBIT;   break;
            case IF5_T_MERGE:   kind = IF5_SCHED_MERGE;   break;
            default:
                if (q->flags & IF5_F_CONCUR_MERGE) kind = IF5_SCHED_MERGE;
                break;
            }
            if (kind != 0) count++;
        }
        assert(count == n);
    }

    /* The N5 discipline, checked from the outside: the schedule
     * added no payload family — the registry still holds exactly
     * the types the test registered (built-ins + str). */
    assert(if5_type_info(w, IF5_T_USER) == NULL);

    printf("(%u entries) ", n);
    if5_world_free(w);
}

static void test_s17_surprise_itemization(void)
{
    if5_world* w = mk_world(0, 200);
    if5_sched_entry ent[64];
    if5_view v;
    uint32_t n, i, n_surprise = 0;

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    submit(w, 0, "budget", IF5_T_NUM, 1, 0xFFFFFFFFu, 0, 0);
    /* Two surprise debits with known rungs, one initiative. */
    submit(w, 1, "budget", IF5_T_NUM, 10, 0xFFFFFFFFu, 2, IF5_SEV_BACKGROUND);
    submit(w, 1, "budget", IF5_T_NUM, 20, 0xFFFFFFFFu, 2, IF5_SEV_FOREGROUND);
    submit(w, 1, "budget", IF5_T_NUM, 30, 0xFFFFFFFFu, 1, IF5_SEV_MODAL);

    v = if5_now(w);
    n = if5_schedule(w, &v, IF5_SID_NULL, ent, 64);
    for (i = 0; i < n; i++) {
        if (ent[i].kind != IF5_SCHED_SURPRISE) continue;
        n_surprise++;
        /* The magnitudes are the posted rungs, in order: 1, then 4. */
        assert(ent[i].magnitude == 1 || ent[i].magnitude == 4);
        assert(ent[i].sid == 1);
    }
    assert(n_surprise == 2);
    /* The initiative debit is NOT on the surprise timetable. */
    for (i = 0; i < n; i++)
        assert(!(ent[i].kind == IF5_SCHED_SURPRISE && ent[i].magnitude == 8));

    /* Per-source filter: source 0's only timetable entry is the
     * creation-time TIER dial (constraint D1: the registered
     * default is itself a logged decision) — no surprises, no
     * receipts, no merges. */
    n = if5_schedule(w, &v, 0, ent, 64);
    for (i = 0; i < n; i++) {
        assert(ent[i].sid == 0);
        assert(ent[i].kind == IF5_SCHED_TIER);
    }
    assert(n == 1);
    /* Source 1 has exactly the two. */
    n = if5_schedule(w, &v, 1, ent, 64);
    assert(n == 2);

    printf("(%u surprise entries itemized) ", n_surprise);
    if5_world_free(w);
}

static void test_s17_view_addressing(void)
{
    if5_world* w = mk_world(0, 200);
    if5_sched_entry ent[64];
    if5_view v, rewound;
    uint32_t n_before, n_after, target;

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    submit(w, 0, "budget", IF5_T_NUM, 1, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 10, 0xFFFFFFFFu, 2, IF5_SEV_BACKGROUND);
    target = if5_log_count(w);            /* the surprise record */
    submit(w, 0, "budget", IF5_T_NUM, 2, 0xFFFFFFFFu, 0, 0);

    v = if5_now(w);
    n_before = if5_schedule(w, &v, IF5_SID_NULL, ent, 64);
    assert(n_before >= 1);

    /* Undo the surprise record: at the rewound view it leaves the
     * timetable — the schedule is a projection, not an archive. */
    rewound = if5_view_rewind(w, &v, target);
    n_after = if5_schedule(w, &rewound, IF5_SID_NULL, ent, 64);
    assert(n_after == n_before - 1);
    if5_world_free(w);
}

/* ============================================================
 * V6 — S24: projections never mutate the durable layer
 * ============================================================ */

/* The durable image: everything the two-layer boundary protects. */
typedef struct durable_image {
    uint32_t log_count;
    uint32_t receipts;
    uint64_t history_bytes;
    uint64_t working_bytes;
    if5_stats stats;
    if5_receipt receipt_bytes[32];
    if5_ledger ledgers[IF5_MAX_SOURCES];
} durable_image;

static void snapshot(if5_world* w, durable_image* img)
{
    uint32_t i;
    /* Zero first: absent sources' ledger slots must compare as
     * equal zeros, not as whatever the stack held (the first run
     * of this test memcmp'd uninitialized bytes and failed on
     * them — recorded here so the lesson stays). */
    memset(img, 0, sizeof *img);
    img->log_count = if5_log_count(w);
    img->receipts = if5_receipt_count(w);
    img->history_bytes = if5_history_bytes(w);
    img->working_bytes = if5_working_set_bytes(w);
    img->stats = *if5_stats_of(w);
    for (i = 0; i < img->receipts && i < 32; i++)
        img->receipt_bytes[i] = *if5_receipt_at(w, i);
    for (i = 0; i < IF5_MAX_SOURCES; i++) {
        const if5_ledger* l = if5_ledger_of(w, (if5_sid)i);
        if (l) img->ledgers[i] = *l;
    }
}

static void test_s24_projection_purity(void)
{
    if5_world* w = mk_world(64, 60);
    durable_image before, after;
    if5_sched_entry* ent;
    char buf[8192];
    uint32_t k;

    assert(if5_region_create(w, 0, "budget", 0, IF5_T_NUM, IF5_TIER_MERGE, NULL, 0));
    assert(if5_region_create(w, 0, "tags", 0, IF5_T_SET, IF5_TIER_MERGE, NULL, 0));
    submit(w, 0, "budget", IF5_T_NUM, 100, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 200, 0, 1, IF5_SEV_AMBIENT);
    submit(w, 1, "tags", IF5_T_SET, 0x2, 0, 2, IF5_SEV_MODAL);
    assert(if5_region_set_tier(w, 0, if5_region_find(w, "tags"), IF5_TIER_STRICT));
    submit(w, 0, "tags", IF5_T_SET, 0x4, 0xFFFFFFFFu, 0, 0);
    submit(w, 1, "tags", IF5_T_SET, 0x8, 0, 0, 0);
    submit(w, 1, "budget", IF5_T_NUM, 300, 0, 0, 0);      /* merge */

    ent = (if5_sched_entry*)malloc(256 * sizeof(if5_sched_entry));
    assert(ent);

    snapshot(w, &before);

    /* Fire EVERY projection at several views, repeatedly. */
    for (k = 0; k < 3; k++) {
        if5_view v = if5_now(w);
        if5_view hist = if5_view_at(w, if5_log_count(w) / 2);
        if5_view rewound = if5_view_rewind(w, &v, if5_log_count(w) - 1);
        if5_view views[3];
        views[0] = v; views[1] = hist; views[2] = rewound;
        {
            uint32_t i;
            for (i = 0; i < 3; i++) {
                (void)if5_project_semantic(w, &views[i], buf, sizeof buf);
                (void)if5_project_set(w, &views[i], buf, sizeof buf);
                (void)if5_project_history(w, &views[i], buf, sizeof buf);
                assert(if5_tree_walk(w, &views[i], NULL, NULL) == 2);
                (void)if5_schedule(w, &views[i], IF5_SID_NULL, ent, 256);
                (void)if5_schedule(w, &views[i], 1, ent, 256);
                /* Projections at a cut address, too. */
                {
                    if5_cut c = if5_cut_at(w, views[i].cut);
                    (void)if5_cut_state(w, &c, 1, NULL);
                    (void)if5_cut_is_down_closed(w, &c);
                }
            }
        }
        /* Registered cuts are working-set entries — registering
         * more is NOT a projection side effect on the durable
         * layer (the registry is address bookkeeping, part of the
         * working set; the snapshot's working_bytes covers the
         * struct wholesale, so no drift is possible). */
    }

    snapshot(w, &after);

    assert(before.log_count == after.log_count);
    assert(before.receipts == after.receipts);
    assert(before.history_bytes == after.history_bytes);
    assert(before.working_bytes == after.working_bytes);
    assert(memcmp(&before.stats, &after.stats, sizeof(if5_stats)) == 0);
    assert(memcmp(before.receipt_bytes, after.receipt_bytes,
                  sizeof(if5_receipt) * (before.receipts < 32 ? before.receipts : 32)) == 0);
    assert(memcmp(before.ledgers, after.ledgers,
                  sizeof(if5_ledger) * IF5_MAX_SOURCES) == 0);

    free(ent);
    if5_world_free(w);
}

int main(void)
{
    printf("=== v5 seats suite (ADR-0026 V4/V5/V6: S15 tree, S17 "
           "timetable, S24 purity) ===\n\n");
    TEST(s15_tree_total_deterministic);
    TEST(s15_tree_live_at_views);
    TEST(s17_prov_backed_and_recounted);
    TEST(s17_surprise_itemization);
    TEST(s17_view_addressing);
    TEST(s24_projection_purity);
    printf("\n%d/%d seat sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
