/*
 * test_v5c_render.c — the render suite (ADR-0028): F1 (client
 * purity), F2 (renderer determinism), F3 (layout totality), F5
 * (easing endpoints and monotonicity).
 *
 * The F-series' client half: everything here drives the world
 * through the ordinary channel and the client through its own,
 * and demands that the two never meet — the durable image is
 * byte-compared around every client call, and the submit
 * counter must not move.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * The driver world (a small tree with depth, tiers, a schedule)
 * ============================================================ */

static if5_world* mk_world(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    if5_source s;
    if5_submit_opts o;
    uint16_t calc, disp, keys, k1, k2;

    assert(w);
    memset(&t, 0, sizeof t);
    t.name = "seq";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t);
    t.name = "str";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == IF5_T_STR);

    memset(&s, 0, sizeof s);
    memcpy(s.id, "user", 4);
    s.kind = IF5_KIND_PERSON;
    s.trust = 3;
    s.authority = 0xFFFFFFFFu;
    assert(if5_add_source(w, &s) == 0);
    memcpy(s.id, "sys", 3);
    s.kind = IF5_KIND_PROCESS;
    s.initiative_budget = 50;
    {
        if5_sid sy = if5_add_source(w, &s);
        if5_set_system_source(w, sy);
    }

    /* a two-level tree: calc -> {disp, keys -> {k1, k2}} */
    calc = if5_region_create(w, (if5_sid)0, "calc", 0, IF5_T_NUM,
                             IF5_TIER_MERGE, NULL, 0);
    disp = if5_region_create(w, (if5_sid)0, "disp", calc, IF5_T_STR,
                             IF5_TIER_STRICT, NULL, 0);
    keys = if5_region_create(w, (if5_sid)0, "keys", calc, IF5_T_NUM,
                             IF5_TIER_MERGE, NULL, 0);
    k1   = if5_region_create(w, (if5_sid)0, "k1", keys, IF5_T_STR,
                             IF5_TIER_STRICT, NULL, 0);
    k2   = if5_region_create(w, (if5_sid)0, "k2", keys, IF5_T_STR,
                             IF5_TIER_STRICT, NULL, 0);
    assert(calc == 1 && disp && keys && k1 && k2);

    memset(&o, 0, sizeof o);
    o.observed_cut = 0xFFFFFFFFu;
    assert(if5_submit(w, (if5_sid)0, disp, IF5_T_STR,
                      0x3031323334353637ull, &o));
    assert(if5_submit(w, (if5_sid)0, k1, IF5_T_STR,
                      0x3700000000000000ull, &o));
    o.debit_class = IF5_DEBIT_INITIATIVE;   /* a schedule entry */
    o.severity = IF5_SEV_FOREGROUND;
    assert(if5_submit(w, (if5_sid)1, k2, IF5_T_STR,
                      0x3800000000000000ull, &o));
    return w;
}

/* ============================================================
 * F1 — client purity: the durable image never moves
 * ============================================================ */

typedef struct {
    uint8_t* log_img;
    uint8_t* arena_img;
    uint8_t* rcpt_img;
    uint32_t n_log, n_arena, n_rcpt;
    if5_stats stats;
} durable;

static void snap(if5_world* w, durable* d)
{
    uint32_t i;
    d->n_log = if5_log_count(w);
    d->log_img = (uint8_t*)malloc((size_t)d->n_log * 48 + 1);
    for (i = 0; i < d->n_log; i++)
        memcpy(d->log_img + (size_t)i * 48, if5_log_at(w, i + 1), 48);
    d->n_arena = 0;
    d->arena_img = (uint8_t*)malloc(1);
    d->n_rcpt = if5_receipt_count(w);
    d->rcpt_img = (uint8_t*)malloc((size_t)d->n_rcpt * 24 + 1);
    for (i = 0; i < d->n_rcpt; i++)
        memcpy(d->rcpt_img + (size_t)i * 24, if5_receipt_at(w, i),
               sizeof(if5_receipt));
    d->stats = *if5_stats_of(w);
}

static void snap_free(durable* d)
{
    free(d->log_img);
    free(d->arena_img);
    free(d->rcpt_img);
}

static void test_f1_purity(void)
{
    if5_world* w = mk_world();
    if5c_policy p = if5c_policy_default();
    if5c_layout* l = if5c_layout_new();
    if5c_fb* fb;
    durable before, after;
    uint32_t submits0 = if5_stats_of(w)->submits;

    printf("  [TEST] f1_client_purity\n");

    snap(w, &before);
    assert(if5c_layout_at(l, w, NULL, &p) == 5);
    fb = if5c_fb_new(l->width, l->height);
    assert(fb);
    assert(if5c_render(fb, l, &p) == 0);
    {
        /* the full client surface: hit, ease, dump, hash */
        (void)if5c_hit(l, l->width / 2, l->height / 2);
        {
            if5c_fb* f2 = if5c_fb_new(l->width, l->height);
            if5c_fb* f3 = if5c_fb_new(l->width, l->height);
            assert(if5c_ease_frame(f3, fb, f2, 1, 2,
                                   IF5C_EASE_SMOOTHSTEP) == 0);
            if5c_fb_free(f2);
            if5c_fb_free(f3);
        }
        {
            char dump[65536];
            assert(if5c_fb_dump_ascii(fb, dump, sizeof dump) > 0);
        }
        (void)if5c_fb_hash(fb);
    }
    snap(w, &after);

    /* the durable image: log, receipts, stats — byte for byte */
    assert(after.n_log == before.n_log &&
           memcmp(before.log_img, after.log_img,
                  (size_t)before.n_log * 48) == 0);
    assert(after.n_rcpt == before.n_rcpt &&
           memcmp(before.rcpt_img, after.rcpt_img,
                  (size_t)before.n_rcpt * 24) == 0);
    assert(memcmp(&before.stats, &after.stats,
                  sizeof(if5_stats)) == 0);
    /* and the static half: zero submissions happened */
    assert(if5_stats_of(w)->submits == submits0);

    if5c_fb_free(fb);
    if5c_layout_free(l);
    snap_free(&before);
    snap_free(&after);
    if5_world_free(w);
    printf("        F1: PASS (durable image + zero submits)\n");
}

/* ============================================================
 * F2 — renderer determinism
 * ============================================================ */

static void test_f2_determinism(void)
{
    if5_world* w = mk_world();
    if5c_policy p = if5c_policy_default();
    if5c_layout* l1 = if5c_layout_new();
    if5c_layout* l2 = if5c_layout_new();
    if5c_fb *fa, *fb;
    uint32_t h1, h2;

    printf("  [TEST] f2_renderer_determinism\n");

    assert(if5c_layout_at(l1, w, NULL, &p) == 5);
    assert(if5c_layout_at(l2, w, NULL, &p) == 5);
    fa = if5c_fb_new(l1->width, l1->height);
    fb = if5c_fb_new(l2->width, l2->height);
    assert(fa && fb);
    assert(if5c_render(fa, l1, &p) == 0);
    assert(if5c_render(fb, l2, &p) == 0);

    h1 = if5c_fb_hash(fa);
    h2 = if5c_fb_hash(fb);
    if (h1 != h2) {
        fprintf(stderr, "F2 FAIL: two renders differ (%08x vs %08x)\n",
                h1, h2);
        assert(false);
    }

    /* re-render over the same surface: identical again */
    assert(if5c_render(fb, l2, &p) == 0);
    assert(if5c_fb_hash(fb) == h1);

    /* the layout itself is total order: cells in tree order */
    assert(l1->n == l2->n);
    assert(memcmp(l1->cells, l2->cells,
                  sizeof(if5c_cell) * l1->n) == 0);

    if5c_fb_free(fa);
    if5c_fb_free(fb);
    if5c_layout_free(l1);
    if5c_layout_free(l2);
    if5_world_free(w);
    printf("        F2: PASS (hash %08x stable)\n", h1);
}

/* ============================================================
 * F3 — layout totality: the layout tree IS the semantic tree
 * ============================================================ */

typedef struct {
    if5_node nodes[64];
    uint32_t n;
} nl;

static bool grab(const if5_node* n, void* user)
{
    nl* l = (nl*)user;
    l->nodes[l->n++] = *n;
    return true;
}

static void test_f3_totality(void)
{
    if5_world* w = mk_world();
    if5c_policy p = if5c_policy_default();
    if5c_layout* l = if5c_layout_new();
    nl tree;
    uint32_t i;
    int seen[8];

    printf("  [TEST] f3_layout_totality\n");

    tree.n = 0;
    if5_tree_walk(w, NULL, grab, &tree);
    assert(if5c_layout_at(l, w, NULL, &p) == tree.n);
    assert(l->n == tree.n);

    memset(seen, 0, sizeof seen);
    for (i = 0; i < l->n; i++) {
        const if5c_cell* c = &l->cells[i];
        const if5_node* n = &tree.nodes[i];
        /* same node, same order, same shape */
        if (memcmp(c->name, n->name, IF5_NAME_MAX) != 0 ||
            c->rid != n->rid || c->parent != n->parent ||
            c->depth != n->depth || c->value != n->value ||
            c->contrib != n->contrib || c->ptype != n->ptype ||
            c->tier != n->tier) {
            fprintf(stderr, "F3 FAIL: cell %u is not node %u\n", i, i);
            assert(false);
        }
        /* every cell has real geometry, inside the surface */
        assert(c->w == p.cell_w && c->h == p.cell_h);
        assert(c->x >= p.base_x && c->y >= p.base_y);
        assert(c->x + c->w <= l->width && c->y + c->h <= l->height);
        /* exactly one cell per region (totality: no doubles) */
        assert(c->rid < 8);
        assert(seen[c->rid] == 0);
        seen[c->rid] = 1;
        /* the depth-row rule */
        assert(c->y == p.base_y + (int)n->depth * p.row_h);
        /* the pre-order column rule: columns strictly increase */
        if (i) assert(c->x > l->cells[i - 1].x);
    }

    if5c_layout_free(l);
    if5_world_free(w);
    printf("        F3: PASS (%u cells, one per region, shape kept)\n",
           tree.n);
}

/* ============================================================
 * F5 — easing endpoints and per-channel monotonicity
 * ============================================================ */

static void test_f5_easing(void)
{
    if5_world* w = mk_world();
    if5c_policy p = if5c_policy_default();
    if5c_layout* la = if5c_layout_new();
    if5c_layout* lb = if5c_layout_new();
    if5c_fb *fa, *fb, *mid;
    if5_submit_opts o;
    int fam;

    printf("  [TEST] f5_easing_endpoints_monotone\n");

    /* view A: now; view B: one more submission later */
    assert(if5c_layout_at(la, w, NULL, &p) == 5);
    memset(&o, 0, sizeof o);
    o.observed_cut = 0xFFFFFFFFu;
    assert(if5_submit(w, (if5_sid)0, if5_region_find(w, "disp"),
                      IF5_T_STR, 0x4142434445464748ull, &o));
    assert(if5c_layout_at(lb, w, NULL, &p) == 5);

    fa = if5c_fb_new(la->width, la->height);
    fb = if5c_fb_new(lb->width, lb->height);
    mid = if5c_fb_new(lb->width, lb->height);
    assert(fa && fb && mid);
    /* the frames must share dimensions for easing to be defined:
     * same world shape, same policy -> same surface */
    assert(la->width == lb->width && la->height == lb->height);
    assert(if5c_render(fa, la, &p) == 0);
    assert(if5c_render(fb, lb, &p) == 0);

    for (fam = 0; fam <= 1; fam++) {
        if5c_ease_family f = fam ? IF5C_EASE_SMOOTHSTEP
                                 : IF5C_EASE_LINEAR;
        uint32_t N = 16, t;
        uint8_t *va, *vb, *prev;
        int npx = if5c_fb_width(fa) * if5c_fb_height(fa) * 4;

        va = (uint8_t*)malloc((size_t)npx);
        vb = (uint8_t*)malloc((size_t)npx);
        prev = (uint8_t*)malloc((size_t)npx);
        assert(va && vb && prev);
        assert(if5c_fb_bytes(fa, va, npx) == npx);
        assert(if5c_fb_bytes(fb, vb, npx) == npx);

        /* t = 0 and t = 1: byte-exact endpoints */
        assert(if5c_ease_frame(mid, fa, fb, 0, N, f) == 0);
        assert(if5c_fb_bytes(mid, prev, npx) == npx);
        if (memcmp(prev, va, (size_t)npx) != 0) {
            fprintf(stderr, "F5 FAIL: t=0 is not view A\n");
            assert(false);
        }
        assert(if5c_ease_frame(mid, fa, fb, N, N, f) == 0);
        assert(if5c_fb_bytes(mid, prev, npx) == npx);
        if (memcmp(prev, vb, (size_t)npx) != 0) {
            fprintf(stderr, "F5 FAIL: t=1 is not view B\n");
            assert(false);
        }

        /* monotone: the distance to B never increases */
        for (t = 0; t <= N; t++) {
            uint8_t* cur = (uint8_t*)malloc((size_t)npx);
            int i;
            assert(cur);
            assert(if5c_ease_frame(mid, fa, fb, t, N, f) == 0);
            assert(if5c_fb_bytes(mid, cur, npx) == npx);
            for (i = 0; i < npx; i += 4) {
                int ch;
                for (ch = 0; ch < 3; ch++) {
                    int before = (t == 0) ? va[i + ch] : prev[i + ch];
                    int now = cur[i + ch];
                    int db = vb[i + ch] - before;
                    int dn = vb[i + ch] - now;
                    if (db < 0) db = -db;
                    if (dn < 0) dn = -dn;
                    if (dn > db) {
                        fprintf(stderr, "F5 FAIL: channel moved away "
                                "from B (t=%u/%u, px %d ch %d)\n",
                                t, N, i / 4, ch);
                        assert(false);
                    }
                }
            }
            memcpy(prev, cur, (size_t)npx);
            free(cur);
        }
        free(va); free(vb); free(prev);
    }

    if5c_fb_free(fa);
    if5c_fb_free(fb);
    if5c_fb_free(mid);
    if5c_layout_free(la);
    if5c_layout_free(lb);
    if5_world_free(w);
    printf("        F5: PASS (both families, endpoints exact, "
           "monotone)\n");
}

/* ============================================================
 * The annotation table (2e): the cancel-reason discharge
 * ============================================================ */

static void test_annotate(void)
{
    if5c_annot* an = if5c_annot_new();
    printf("  [TEST] annotate_table\n");
    assert(an);
    assert(if5c_annotate(an, 3, "user dismissed the dialog"));
    assert(if5c_annotate(an, 7, "system rollback"));
    assert(strcmp(if5c_annotation_of(an, 3),
                  "user dismissed the dialog") == 0);
    assert(strcmp(if5c_annotation_of(an, 7), "system rollback") == 0);
    assert(if5c_annotation_of(an, 4) == NULL);
    assert(if5c_annotation_count(an) == 2);
    /* replace, not grow */
    assert(if5c_annotate(an, 3, "changed my mind"));
    assert(strcmp(if5c_annotation_of(an, 3), "changed my mind") == 0);
    assert(if5c_annotation_count(an) == 2);
    if5c_annot_free(an);
    printf("        annotate: PASS (replace semantics, bounded)\n");
}

int main(void)
{
    printf("=== v5 client render suite (ADR-0028: F1, F2, F3, F5) ===\n\n");
    test_f1_purity();
    test_f2_determinism();
    test_f3_totality();
    test_f5_easing();
    test_annotate();
    printf("\n5/5 render sections pass\n");
    return 0;
}
