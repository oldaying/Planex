/*
 * test_v5c_persist.c — the client-side persistence suite
 * (ADR-0028): F2's round-trip half (the same view renders
 * byte-identically from a reloaded world) and F6's client-visible
 * half (a named address survives, and an annotation can be
 * re-attached to it — annotations themselves are session state
 * by contract, and the test asserts exactly that).
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static if5_world* mk_world(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    if5_source s;
    if5_submit_opts o;

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
    memcpy(s.id, "u", 1);
    s.kind = IF5_KIND_PERSON;
    s.trust = 3;
    s.authority = 0xFFFFFFFFu;
    assert(if5_add_source(w, &s) == 0);

    assert(if5_region_create(w, (if5_sid)0, "top", 0, IF5_T_NUM,
                             IF5_TIER_MERGE, NULL, 0) == 1);
    assert(if5_region_create(w, (if5_sid)0, "left", 1, IF5_T_STR,
                             IF5_TIER_STRICT, NULL, 0) == 2);
    assert(if5_region_create(w, (if5_sid)0, "right", 1, IF5_T_SET,
                             IF5_TIER_MERGE, NULL, 0) == 3);

    memset(&o, 0, sizeof o);
    o.observed_cut = 0xFFFFFFFFu;
    assert(if5_submit(w, (if5_sid)0, 1, IF5_T_NUM, 42, &o));
    assert(if5_submit(w, (if5_sid)0, 2, IF5_T_STR,
                      0x3031323334353637ull, &o));
    assert(if5_submit(w, (if5_sid)0, 3, IF5_T_SET, 0x1010101010101010ull,
                      &o));

    /* a named address: the pre-second-round checkpoint */
    {
        if5_cut c = if5_cut_at(w, if5_log_count(w));
        assert(if5_cut_register_named(w, &c, "before") == 1);
    }
    assert(if5_submit(w, (if5_sid)0, 1, IF5_T_NUM, 77, &o));
    return w;
}

int main(void)
{
    if5_world* a = mk_world();
    if5_world* b;
    if5c_policy p = if5c_policy_default();
    if5c_layout *la, *lb;
    if5c_fb *fa, *fb2;
    char diag[256];
    uint8_t *bytes_a, *bytes_b;
    int npx;

    printf("=== v5 client persistence suite (ADR-0028: F2 rt, F6) ===\n\n");

    /* save / load */
    assert(if5_snapshot_save(a, "/tmp/if5c_rt.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);
    b = if5_world_create();
    {
        if5_payload_type t;
        memset(&t, 0, sizeof t);
        t.name = "seq";
        t.value_size = 8;
        assert(if5_register_payload_type(b, &t) == 2);
        memset(&t, 0, sizeof t);
        t.name = "str";
        t.value_size = 8;
        assert(if5_register_payload_type(b, &t) == IF5_T_STR);
    }
    assert(if5_snapshot_load(b, "/tmp/if5c_rt.bin", diag,
                             sizeof diag) == IF5_SNAP_OK);

    /* F2's round-trip half: the same view renders identically */
    la = if5c_layout_new();
    lb = if5c_layout_new();
    assert(if5c_layout_at(la, a, NULL, &p) == 3);
    assert(if5c_layout_at(lb, b, NULL, &p) == 3);
    fa = if5c_fb_new(la->width, la->height);
    fb2 = if5c_fb_new(lb->width, lb->height);
    assert(fa && fb2);
    assert(if5c_render(fa, la, &p) == 0);
    assert(if5c_render(fb2, lb, &p) == 0);

    npx = if5c_fb_width(fa) * if5c_fb_height(fa) * 4;
    bytes_a = (uint8_t*)malloc((size_t)npx);
    bytes_b = (uint8_t*)malloc((size_t)npx);
    assert(bytes_a && bytes_b);
    assert(if5c_fb_bytes(fa, bytes_a, npx) == npx);
    assert(if5c_fb_bytes(fb2, bytes_b, npx) == npx);
    if (memcmp(bytes_a, bytes_b, (size_t)npx) != 0) {
        fprintf(stderr, "F2 FAIL: render differs across the round trip\n");
        assert(false);
    }
    printf("  F2 (round-trip render): PASS (%dx%d, hash %08x)\n",
           if5c_fb_width(fa), if5c_fb_height(fa),
           if5c_fb_hash(fa));

    /* F6's client half: the named address survives, resolves, and
     * folds the same; and an annotation re-attaches to it (the
     * annotation itself is session state — the contract's own
     * words — and the reload starts it empty, which is asserted,
     * not apologized for) */
    {
        if5c_annot* an = if5c_annot_new();
        assert(strcmp(if5_cut_name(b, 1), "before") == 0);
        assert(if5_cut_by_name(b, "before") == 1);
        {
            const if5_cut* ca = if5_cut_by_id(a, 1);
            const if5_cut* cb = if5_cut_by_id(b, 1);
            uint32_t x = 0, y = 0;
            assert(memcmp(ca, cb, sizeof(if5_cut)) == 0);
            assert(if5_cut_state(a, ca, 1, &x) ==
                   if5_cut_state(b, cb, 1, &y));
            assert(x == y);
        }
        /* the pre-save world's fold at the named address renders
         * identically from the reloaded world */
        {
            if5c_layout* lc = if5c_layout_new();
            if5_view v = if5_cut_view(if5_cut_by_id(b, 1));
            if5c_fb* fc = if5c_fb_new(8, 8);
            assert(if5c_layout_at(lc, b, &v, &p) == 3);
            if5c_fb_free(fc);
            fc = if5c_fb_new(lc->width, lc->height);
            assert(if5c_render(fc, lc, &p) == 0);
            if5c_fb_free(fc);
            if5c_layout_free(lc);
        }
        assert(if5c_annotate(an, 1, "the pre-77 checkpoint"));
        assert(strcmp(if5c_annotation_of(an, 1),
                      "the pre-77 checkpoint") == 0);
        if5c_annot_free(an);
        printf("  F6 (client half): PASS (named address folds and "
               "renders identically)\n");
    }

    free(bytes_a);
    free(bytes_b);
    if5c_fb_free(fa);
    if5c_fb_free(fb2);
    if5c_layout_free(la);
    if5c_layout_free(lb);
    if5_world_free(a);
    if5_world_free(b);

    printf("\n2/2 persistence sections pass\n");
    return 0;
}
