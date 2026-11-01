/*
 * test_v5_property.c — the I4 semilattice property suite, re-derived on the promoted core
 * (ADR-0025 prediction C6, re-registered as ADR-0026 V7's property
 * half; the method is Claessen & Hughes 2000: random testing with
 * shrinking, per the ADRs' References).
 *
 * What runs here:
 *   - 1,000,000 random concurrent groups per round, per registered
 *     lattice type (NUM max, SET union) and mixed: associativity,
 *     commutativity, idempotence, inflationarity
 *   - a synthetic NON-ASSOCIATIVE sq offered to the registry is
 *     REJECTED at the door (the turnstile must bite)
 *   - a planted non-associative join, tested directly, is caught,
 *     and the counterexample SHRINKS to a minimal triple
 *
 * A pass is an existence proof for the registered lattices under
 * the tested generators — not a proof that every future payload
 * type admits a lattice (E3c's acceptance question is untouched;
 * see the ADRs' CAVEATS).
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

#define C6_ROUNDS 1000000u

/* Deterministic PRNG — seeded, so a failure is reproducible. */
static uint64_t rng_state = 0x243F6A8885A308D3ull;

static uint64_t rng(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

/* The four laws, checked on one triple against a raw join. */
static bool laws_hold(bool (*sq)(uint64_t, uint64_t, uint64_t*),
                      bool (*le)(uint64_t, uint64_t),
                      uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t ab, bc, labc, rabc, ba, aa;
    if (!sq(a, b, &ab)) return false;
    if (!sq(b, c, &bc)) return false;
    if (!sq(ab, c, &labc)) return false;
    if (!sq(a, bc, &rabc)) return false;
    if (labc != rabc) return false;                 /* associative */
    if (!sq(b, a, &ba)) return false;
    if (ab != ba) return false;                     /* commutative */
    if (!sq(a, a, &aa)) return false;
    if (aa != a) return false;                      /* idempotent */
    if (le && !le(a, ab)) return false;             /* inflationary */
    return true;
}

/* The core's built-in joins, reached the only public way there
 * is: through a world's payload registry's READ SIDE
 * (if5_type_info — the same channel every user type uses, B2).
 * The battery dereferences THE SAME function pointers the
 * adjudicator does; a reimplementation would test nothing. */
static bool (*g_sq_num)(uint64_t, uint64_t, uint64_t*);
static bool (*g_le_num)(uint64_t, uint64_t);
static bool (*g_sq_set)(uint64_t, uint64_t, uint64_t*);
static bool (*g_le_set)(uint64_t, uint64_t);

static bool sq_num(uint64_t a, uint64_t b, uint64_t* out)
{
    return g_sq_num(a, b, out);
}
static bool le_num(uint64_t a, uint64_t b)
{
    return g_le_num(a, b);
}
static bool sq_set(uint64_t a, uint64_t b, uint64_t* out)
{
    return g_sq_set(a, b, out);
}
static bool le_set(uint64_t a, uint64_t b)
{
    return g_le_set(a, b);
}

static void bind_joins(void)
{
    if5_world* w = if5_world_create();
    const if5_payload_type* n = if5_type_info(w, IF5_T_NUM);
    const if5_payload_type* s = if5_type_info(w, IF5_T_SET);
    assert(n && n->sq && n->le);
    assert(s && s->sq && s->le);
    g_sq_num = n->sq; g_le_num = n->le;
    g_sq_set = s->sq; g_le_set = s->le;
    if5_world_free(w);
}

/* ============================================================
 * The registered lattices hold under a million random groups
 * ============================================================ */

static void test_p6_num_lattice_one_million(void)
{
    uint32_t i;
    for (i = 0; i < C6_ROUNDS; i++) {
        uint64_t a = rng() & 0xFFFFFFFFu;   /* NUM is u32-bounded */
        uint64_t b = rng() & 0xFFFFFFFFu;
        uint64_t c = rng() & 0xFFFFFFFFu;
        if (!laws_hold(sq_num, le_num, a, b, c)) {
            fprintf(stderr, "\nC6 COUNTEREXAMPLE (NUM): "
                    "a=%llu b=%llu c=%llu\n",
                    (unsigned long long)a, (unsigned long long)b,
                    (unsigned long long)c);
            assert(false);
        }
    }
    printf("(NUM: %u groups) ", C6_ROUNDS);
}

static void test_p6_set_lattice_one_million(void)
{
    uint32_t i;
    for (i = 0; i < C6_ROUNDS; i++) {
        uint64_t a = rng(), b = rng(), c = rng();
        if (!laws_hold(sq_set, le_set, a, b, c)) {
            fprintf(stderr, "\nC6 COUNTEREXAMPLE (SET): "
                    "a=%llx b=%llx c=%llx\n",
                    (unsigned long long)a, (unsigned long long)b,
                    (unsigned long long)c);
            assert(false);
        }
    }
    printf("(SET: %u groups) ", C6_ROUNDS);
}

static void test_p6_registry_admits_the_builtins(void)
{
    /* NUM and SET registered fine at world creation (their ids are
     * the enum's): the turnstile sampled them and found the laws.
     * Cross-check by re-offering clones under new names. */
    if5_world* w = if5_world_create();
    if5_payload_type t;
    memset(&t, 0, sizeof t);
    t.name = "num2"; t.value_size = 4;
    t.sq = sq_num; t.le = le_num;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);
    memset(&t, 0, sizeof t);
    t.name = "set2"; t.value_size = 8;
    t.sq = sq_set; t.le = le_set;
    assert(if5_register_payload_type(w, &t) != (uint8_t)-1);
    if5_world_free(w);
    printf("(clones admitted) ");
}

/* ============================================================
 * The turnstile bites: a synthetic non-associative join is
 * REJECTED at registration
 * ============================================================ */

/* sq(a,b) = a/2 + b/2 + (a&b&1): commutative, idempotent,
 * NOT associative — the classic planted counterexample. */
static bool sq_mean(uint64_t a, uint64_t b, uint64_t* out)
{
    *out = (a >> 1) + (b >> 1) + (a & b & 1u);
    return true;
}
static bool le_mean(uint64_t a, uint64_t b)
{
    (void)a; (void)b;
    return true;                          /* claim inflationarity */
}

static void test_p6_registry_rejects_nonassociative(void)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    uint8_t id;

    memset(&t, 0, sizeof t);
    t.name = "mean"; t.value_size = 8;
    t.sq = sq_mean; t.le = le_mean;
    id = if5_register_payload_type(w, &t);
    if (id != (uint8_t)-1) {
        fprintf(stderr, "\nC6 FAILED THE BITE: the registry admitted "
                "a non-associative join (id %u) — the turnstile is "
                "toothless\n", id);
        assert(false);
    }
    if5_world_free(w);
    printf("(the checker bit) ");
}

/* ============================================================
 * Shrinking: a planted counterexample is caught and reduced
 * ============================================================ */

static void test_p6_counterexample_shrinks(void)
{
    uint64_t a = 0, b = 0, c = 0;
    uint32_t i;
    bool found = false;

    /* Find a counterexample for the planted join. */
    for (i = 0; i < C6_ROUNDS && !found; i++) {
        a = rng() & 0xFF; b = rng() & 0xFF; c = rng() & 0xFF;
        if (!laws_hold(sq_mean, le_mean, a, b, c)) found = true;
    }
    assert(found);
    {
        uint64_t orig_a = a, orig_b = b, orig_c = c;
        /* Shrink: drive each coordinate down (halve, then to the
         * small constants) while the violation survives. */
        for (;;) {
            bool moved = false;
            uint64_t cand;
            cand = a >> 1;
            if (cand < a && !laws_hold(sq_mean, le_mean, cand, b, c)) {
                a = cand; moved = true;
            }
            cand = b >> 1;
            if (cand < b && !laws_hold(sq_mean, le_mean, a, cand, c)) {
                b = cand; moved = true;
            }
            cand = c >> 1;
            if (cand < c && !laws_hold(sq_mean, le_mean, a, b, cand)) {
                c = cand; moved = true;
            }
            if (!moved) break;
        }
        /* The shrunk triple is still a counterexample... */
        assert(!laws_hold(sq_mean, le_mean, a, b, c));
        /* ...and it is minimal-ish: the planted join's smallest
         * witnesses live at small constants (e.g. (0,1,2):
         * sq(sq(0,1),2) = sq(1,2) = 1, but
         * sq(0,sq(1,2)) = sq(0,1) = 0). */
        assert(a <= 3 && b <= 3 && c <= 3);
        printf("(shrunk %llx/%llx/%llx -> %llx/%llx/%llx) ",
               (unsigned long long)orig_a, (unsigned long long)orig_b,
               (unsigned long long)orig_c,
               (unsigned long long)a, (unsigned long long)b,
               (unsigned long long)c);
    }
}

/* ============================================================
 * Mixed-type rounds: the laws hold across interleaved types
 * ============================================================ */

static void test_p6_mixed_rounds(void)
{
    uint32_t i;
    for (i = 0; i < C6_ROUNDS; i++) {
        bool ok;
        switch (rng() & 1) {
        case 0:
            ok = laws_hold(sq_num, le_num,
                           rng() & 0xFFFFFFFFu, rng() & 0xFFFFFFFFu,
                           rng() & 0xFFFFFFFFu);
            break;
        default:
            ok = laws_hold(sq_set, le_set,
                           rng(), rng(), rng());
            break;
        }
        if (!ok) {
            fprintf(stderr, "\nC6 COUNTEREXAMPLE (mixed round %u)\n", i);
            assert(false);
        }
    }
    printf("(mixed: %u groups) ", C6_ROUNDS);
}

int main(void)
{
    bind_joins();
    printf("=== v5 property suite (ADR-0026 V7: C6/I4 on if5 — the "
           "semilattice laws, %u groups/round) ===\n\n", C6_ROUNDS);
    TEST(p6_num_lattice_one_million);
    TEST(p6_set_lattice_one_million);
    TEST(p6_registry_admits_the_builtins);
    TEST(p6_registry_rejects_nonassociative);
    TEST(p6_counterexample_shrinks);
    TEST(p6_mixed_rounds);
    printf("\n%d/%d property sections pass\n", g_pass, g_run);
    return g_pass == g_run ? 0 : 1;
}
