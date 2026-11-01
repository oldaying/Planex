/*
 * test_v5c_hit.c — the hit suite (ADR-0028): F4 — the hit test
 * is the layout's inverse, over randomized tree shapes.
 *
 * Every region's cell answers its own rid from inside (center
 * and corners); every point outside all cells answers 0; and a
 * randomized sweep agrees with a brute-force containment scan on
 * every sampled point. The inverse of a pure function is a
 * function — this suite holds the client to it.
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/* A randomized world: a random tree of 3..9 regions across up to
 * 3 levels, values and tiers drawn randomly, then a few
 * submissions so the folds are non-trivial. */
static if5_world* mk_random_world(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    if5_source s;
    if5_submit_opts o;
    uint32_t n = 3 + (uint32_t)(rng() % 7);
    uint32_t i;
    uint16_t parent_of[10];

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

    for (i = 0; i < n; i++) {
        char name[8];
        uint16_t parent = 0;
        uint8_t ptype = (uint8_t)(rng() % 4);   /* NUM/SET/SEQ/STR */
        uint8_t tier = (uint8_t)(1 + rng() % 3);
        if (i > 0 && (rng() & 1))
            parent = (uint16_t)(1 + rng() % i); /* under an earlier one */
        parent_of[i] = parent;
        snprintf(name, sizeof name, "r%u", i);
        assert(if5_region_create(w, (if5_sid)0, name, parent, ptype,
                                 (if5_tier)tier, NULL, 0) == i + 1);
    }

    memset(&o, 0, sizeof o);
    o.observed_cut = 0xFFFFFFFFu;
    for (i = 0; i < 6; i++) {
        uint16_t rid = (uint16_t)(1 + rng() % n);
        assert(if5_submit(w, (if5_sid)0, rid,
                          if5_region_info(w, rid)->ptype,
                          rng() & 0xFFFFFFFFFFFFull, &o));
    }
    (void)parent_of;
    return w;
}

/* The brute-force answer: the first cell containing the point. */
static uint16_t brute(const if5c_layout* l, int x, int y)
{
    uint32_t i;
    for (i = 0; i < l->n; i++) {
        const if5c_cell* c = &l->cells[i];
        if (x >= c->x && x < c->x + c->w &&
            y >= c->y && y < c->y + c->h)
            return c->rid;
    }
    return 0;
}

int main(void)
{
    uint32_t round;
    if5c_policy p = if5c_policy_default();
    uint32_t total_cells = 0, total_points = 0;

    printf("=== v5 client hit suite (ADR-0028: F4) ===\n\n");

    for (round = 0; round < 200; round++) {
        if5_world* w = mk_random_world();
        if5c_layout* l = if5c_layout_new();
        uint32_t i;

        assert(if5c_layout_at(l, w, NULL, &p) > 0);
        total_cells += l->n;

        /* inside: center and the four corners-inward of every cell
         * answer that cell's rid */
        for (i = 0; i < l->n; i++) {
            const if5c_cell* c = &l->cells[i];
            int probes[5][2] = {
                { c->x + c->w / 2, c->y + c->h / 2 },
                { c->x + 1,        c->y + 1 },
                { c->x + c->w - 2, c->y + 1 },
                { c->x + 1,        c->y + c->h - 2 },
                { c->x + c->w - 2, c->y + c->h - 2 },
            };
            int k;
            for (k = 0; k < 5; k++) {
                uint16_t got = if5c_hit(l, probes[k][0], probes[k][1]);
                if (got != c->rid) {
                    fprintf(stderr, "F4 FAIL (round %u): cell %u rid "
                            "%u probed at (%d,%d) answered %u\n",
                            round, i, c->rid, probes[k][0],
                            probes[k][1], got);
                    assert(false);
                }
                total_points++;
            }
        }

        /* outside: the void and the margins answer 0 */
        {
            int probes[6][2] = {
                { -1, -1 }, { l->width, l->height },
                { l->width + 5, 0 }, { 0, l->height + 5 },
                { p.base_x - 1, p.base_y }, { l->width - 1, -3 },
            };
            int k;
            for (k = 0; k < 6; k++) {
                uint16_t got = if5c_hit(l, probes[k][0], probes[k][1]);
                if (got != 0) {
                    fprintf(stderr, "F4 FAIL (round %u): outside point "
                            "(%d,%d) answered %u\n", round,
                            probes[k][0], probes[k][1], got);
                    assert(false);
                }
                total_points++;
            }
        }

        /* randomized sweep: if5c_hit agrees with brute force */
        {
            int k;
            for (k = 0; k < 40; k++) {
                int x = (int)(rng() % (uint32_t)(l->width + 8)) - 4;
                int y = (int)(rng() % (uint32_t)(l->height + 8)) - 4;
                uint16_t a = if5c_hit(l, x, y);
                uint16_t b = brute(l, x, y);
                if (a != b) {
                    fprintf(stderr, "F4 FAIL (round %u): (%d,%d) "
                            "answered %u, brute says %u\n", round,
                            x, y, a, b);
                    assert(false);
                }
                total_points++;
            }
        }

        if5c_layout_free(l);
        if5_world_free(w);
    }

    printf("  F4: PASS (200 randomized trees, %u cells, %u points, "
           "inverse holds everywhere)\n", total_cells, total_points);
    printf("\n1/1 hit sections pass\n");
    return 0;
}
