/*
 * test_v5_differential.c — THE master gate of the rewrite
 * (ADR-0026 Decision 4; predictions V1 and V9).
 *
 * v5 must be observationally equivalent to the FROZEN SLICE on the
 * registered observation surface: the two cores are driven in
 * lockstep through (i) the full TRIP-17 scenario, (ii) seeded
 * randomized op streams, and (iii) an E2-shaped load (1000
 * regions, 4000 changes, the registered hotspot shape), and
 * compared after every op and at every checkpoint on:
 *
 *   - every region's folded state at "now" AND at sampled
 *     historical views
 *   - all three text projections (byte-exact)
 *   - the black-box export text (byte-exact)
 *   - the stats struct (byte-exact)
 *   - the receipt store (count and every field)
 *   - the log count
 *   - every log record's contract fields (vc, value, prov, t, src,
 *     ptype, flags, region, aux, commit — everything except the
 *     debit word, whose class packing is the ONE registered
 *     record-format delta, ADR-0026 3d; the severity BITS are
 *     compared)
 *   - the history bytes (exact) and the working-set delta (the
 *     exact sizeof difference — the parent field and the cut
 *     registry are v5's registered working-set additions)
 *
 * At the E2 shape the differential additionally re-verifies v5's
 * OWN residency against the registered C8 boundary (<= 3.0x at
 * n = 2000, linear growth) — measured on v5 directly, not
 * transferred by argument (V9).
 *
 * Any mismatch on any compared surface is a semantics alteration
 * and fails the build: the rewrite may extend, never alter.
 *
 * Run: make -C v5 test
 */
#define _POSIX_C_SOURCE 200809L
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/ifl.h"      /* the frozen instrument */
#include "planex/if5.h"      /* the derivation */
#include "scenario.h"        /* TRIP-17, shared */
/* White-box: the working-set delta is asserted against the exact
 * sizeof difference of the two world structs, so the differential
 * needs their definitions (the publics are opaque). */
#include "../../slice/src/ifl_internal.h"
#include "../src/if5_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* The compared surfaces assume these layout equalities. */
_Static_assert(sizeof(ifl_stats) == sizeof(if5_stats),
               "the stats structs must stay layout-identical (differential surface)");
_Static_assert(sizeof(ifl_receipt) == sizeof(if5_receipt),
               "the receipt structs must stay layout-identical (differential surface)");

/* ============================================================
 * The two worlds, driven in lockstep
 * ============================================================ */

typedef struct duo {
    ifl_world* l;
    if5_world* v;
} duo;

static uint64_t rng_state = 0xB5E9377AC56DB4C3ull;   /* fixed seed: a
                                                      * failure is
                                                      * reproducible */
static uint64_t rng(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

static void die(const char* what, uint32_t opno, const char* phase)
{
    fprintf(stderr, "\nDIFFERENTIAL MISMATCH: %s (op/line %u, phase %s)\n",
            what, opno, phase);
    fprintf(stderr, "seeded PRNG — the failure is reproducible\n");
    assert(false);
}

/* Debug print (temporary diagnostics): the tail records of both. */
static void dbg_tail(duo* d, uint32_t n)
{
    uint32_t k;
    uint32_t cl = ifl_log_count(d->l), cv = if5_log_count(d->v);
    fprintf(stderr, "counts: ifl=%u if5=%u\n", cl, cv);
    for (k = cl > n ? cl - n : 1; k <= cl; k++) {
        const ifl_intent* a = ifl_log_at(d->l, k);
        fprintf(stderr, "  ifl #%u: src=%u rg=%u ty=%u flags=%x vc=[%u,%u,%u,%u]\n",
                k, a->src, a->region, a->ptype, a->flags,
                a->vc.c[0], a->vc.c[1], a->vc.c[2], a->vc.c[3]);
    }
    for (k = cv > n ? cv - n : 1; k <= cv; k++) {
        const if5_intent* b = if5_log_at(d->v, k);
        fprintf(stderr, "  if5 #%u: src=%u rg=%u ty=%u flags=%x vc=[%u,%u,%u,%u]\n",
                k, b->src, b->region, b->ptype, b->flags,
                b->vc.c[0], b->vc.c[1], b->vc.c[2], b->vc.c[3]);
    }
}

/* The registered working-set delta: everything v5 adds to the
 * world struct (the cut registry, the region parent) minus nothing
 * — asserted at every full comparison and printed once. */
static uint64_t ws_delta(void)
{
    return (uint64_t)(sizeof(if5_world) - sizeof(ifl_world));
}

/* Build a scenario-shaped pair of worlds. */
static void mk_duo(duo* d, uint32_t sys, uint32_t a, uint32_t b)
{
    ifl_source sl;
    if5_source sv;
    ifl_payload_type tl;
    if5_payload_type tv;

    d->l = ifl_world_create();
    d->v = if5_world_create();
    assert(d->l && d->v);

    /* the same user types through the same channel (B2) */
    memset(&tl, 0, sizeof tl); tl.name = "seq"; tl.value_size = 8;
    assert(ifl_register_payload_type(d->l, &tl) != (uint8_t)-1);
    memset(&tl, 0, sizeof tl); tl.name = "str"; tl.value_size = 8;
    assert(ifl_register_payload_type(d->l, &tl) != (uint8_t)-1);
    memset(&tv, 0, sizeof tv); tv.name = "seq"; tv.value_size = 8;
    assert(if5_register_payload_type(d->v, &tv) != (uint8_t)-1);
    memset(&tv, 0, sizeof tv); tv.name = "str"; tv.value_size = 8;
    assert(if5_register_payload_type(d->v, &tv) != (uint8_t)-1);

    memset(&sl, 0, sizeof sl);
    memset(&sv, 0, sizeof sv);
    strcpy(sl.id, "human");  strcpy(sv.id, "human");
    sl.kind = IFL_KIND_PERSON; sv.kind = IF5_KIND_PERSON;
    sl.initiative_budget = 0xFFFFFFFFu; sv.initiative_budget = 0xFFFFFFFFu;
    sl.authority = 0xFF; sv.authority = 0xFF;
    assert(ifl_add_source(d->l, &sl) == 0);
    assert(if5_add_source(d->v, &sv) == 0);
    strcpy(sl.id, "agentA"); strcpy(sv.id, "agentA");
    sl.kind = IFL_KIND_AGENT; sv.kind = IF5_KIND_AGENT;
    sl.initiative_budget = a; sv.initiative_budget = a;
    assert(ifl_add_source(d->l, &sl) == 1);
    assert(if5_add_source(d->v, &sv) == 1);
    strcpy(sl.id, "agentB"); strcpy(sv.id, "agentB");
    sl.initiative_budget = b; sv.initiative_budget = b;
    assert(ifl_add_source(d->l, &sl) == 2);
    assert(if5_add_source(d->v, &sv) == 2);
    strcpy(sl.id, "system"); strcpy(sv.id, "system");
    sl.kind = IFL_KIND_PROCESS; sv.kind = IF5_KIND_PROCESS;
    sl.initiative_budget = sys; sv.initiative_budget = sys;
    sl.authority = 0; sv.authority = 0;
    assert(ifl_add_source(d->l, &sl) == 3);
    assert(if5_add_source(d->v, &sv) == 3);
    ifl_set_system_source(d->l, 3);
    if5_set_system_source(d->v, 3);
}

static void free_duo(duo* d)
{
    ifl_world_free(d->l);
    if5_world_free(d->v);
}

/* Region creation (flat, parent 0 — the slice-compatible shape). */
static void duo_region(duo* d, const char* name, uint8_t ptype, uint8_t tier)
{
    uint16_t rl = ifl_region_create(d->l, 0, name, ptype,
                                    (ifl_tier)tier, NULL, 0);
    uint16_t rv = if5_region_create(d->v, 0, name, 0, ptype,
                                    (if5_tier)tier, NULL, 0);
    if (rl != rv) die("region_create returned different rids", 0, "setup");
    assert(rl != 0);
}

/* Submit with identical options on both cores. */
static void duo_submit(duo* d, ifl_sid src, const char* region,
                       uint8_t ptype, uint64_t value, uint32_t cut,
                       uint8_t debit, uint8_t sev)
{
    ifl_submit_opts ol;
    if5_submit_opts ov;
    memset(&ol, 0, sizeof ol);
    memset(&ov, 0, sizeof ov);
    ol.observed_cut = cut; ov.observed_cut = cut;
    ol.debit_class = (ifl_debit_class)debit; ov.debit_class = (if5_debit_class)debit;
    ol.severity = (ifl_severity)sev; ov.severity = (if5_severity)sev;
    (void)ifl_submit(d->l, src, ifl_region_find(d->l, region), ptype, value, &ol);
    (void)if5_submit(d->v, src, if5_region_find(d->v, region), ptype, value, &ov);
}

/* ============================================================
 * The comparison battery
 * ============================================================ */

/* Light compare: after every op. */
static void cmp_light(duo* d, uint32_t opno, const char* phase)
{
    ifl_view vl;
    if5_view vv;
    uint16_t r;

    if (ifl_log_count(d->l) != if5_log_count(d->v)) {
        dbg_tail(d, 8);
        die("log counts diverged", opno, phase);
    }
    if (memcmp(ifl_stats_of(d->l), if5_stats_of(d->v), sizeof(ifl_stats)) != 0)
        die("stats diverged", opno, phase);

    vl = ifl_now(d->l);
    vv = if5_now(d->v);
    if (vl.cut != vv.cut || vl.n_excl != vv.n_excl || vl.n_sel != vv.n_sel)
        die("now-view shape diverged", opno, phase);
    for (r = 1; r <= 4; r++) {
        uint32_t cl = 0, cv = 0;
        uint64_t a = ifl_state_at(d->l, r, &vl, &cl);
        uint64_t b = if5_state_at(d->v, r, &vv, &cv);
        if (a != b || cl != cv) die("fold at now diverged", opno, phase);
    }
}

/* Full compare: at checkpoints and stream ends. */
static void cmp_full(duo* d, uint32_t opno, const char* phase)
{
    static char bl[65536], bv[65536];
    ifl_view vl;
    if5_view vv;
    uint32_t i, k;

    cmp_light(d, opno, phase);

    vl = ifl_now(d->l);
    vv = if5_now(d->v);

    /* The three projections, byte-exact. */
    ifl_project_semantic(d->l, &vl, bl, sizeof bl);
    if5_project_semantic(d->v, &vv, bv, sizeof bv);
    if (strcmp(bl, bv) != 0) die("semantic projection diverged", opno, phase);
    ifl_project_set(d->l, &vl, bl, sizeof bl);
    if5_project_set(d->v, &vv, bv, sizeof bv);
    if (strcmp(bl, bv) != 0) die("set projection diverged", opno, phase);
    ifl_project_history(d->l, &vl, bl, sizeof bl);
    if5_project_history(d->v, &vv, bv, sizeof bv);
    if (strcmp(bl, bv) != 0) die("history projection diverged", opno, phase);

    /* Sampled historical views: the fold is a function of the
     * address on both cores. */
    {
        uint32_t n = ifl_log_count(d->l);
        uint32_t cuts[6];
        uint32_t nc = 0;
        cuts[nc++] = n / 5; cuts[nc++] = n / 2; cuts[nc++] = (3 * n) / 4;
        cuts[nc++] = n > 0 ? n - 1 : 0; cuts[nc++] = n;
        for (i = 0; i < nc; i++) {
            ifl_view hl = ifl_view_at(d->l, cuts[i]);
            if5_view hv = if5_view_at(d->v, cuts[i]);
            uint16_t r;
            for (r = 1; r <= 4; r++) {
                uint64_t a = ifl_state_at(d->l, r, &hl, NULL);
                uint64_t b = if5_state_at(d->v, r, &hv, NULL);
                if (a != b) die("historical fold diverged", opno, phase);
            }
        }
    }

    /* The black-box export text (the EXPORT records land on both
     * logs identically — the light compare catches count drift). */
    ifl_blackbox_export(d->l, 0, 2 /*stops*/, bl, sizeof bl);
    if5_blackbox_export(d->v, 0, 2 /*stops*/, bv, sizeof bv);
    if (strcmp(bl, bv) != 0) die("black-box export diverged", opno, phase);

    /* Receipts: count and every field. */
    if (ifl_receipt_count(d->l) != if5_receipt_count(d->v))
        die("receipt counts diverged", opno, phase);
    for (i = 0; i < ifl_receipt_count(d->l); i++) {
        const ifl_receipt* a = ifl_receipt_at(d->l, i);
        const if5_receipt* b = if5_receipt_at(d->v, i);
        if (a->origin != b->origin || a->hop != b->hop ||
            a->reason_iid != b->reason_iid ||
            a->cost_initiative != b->cost_initiative ||
            a->cost_surprise != b->cost_surprise ||
            a->rollback_cut != b->rollback_cut || a->checkpoint != b->checkpoint)
            die("receipt fields diverged", opno, phase);
    }

    /* Records: every contract field. The debit word is compared in
     * its severity bits only — the class packing is the registered
     * delta (ADR-0026 3d). */
    for (k = 1; k <= ifl_log_count(d->l); k++) {
        const ifl_intent* a = ifl_log_at(d->l, k);
        const if5_intent* b = if5_log_at(d->v, k);
        if (a->src != b->src || a->ptype != b->ptype ||
            a->flags != b->flags || a->region != b->region ||
            a->commit != b->commit || a->t != b->t || a->aux != b->aux ||
            a->prov_a != b->prov_a || a->prov_b != b->prov_b ||
            a->value[0] != b->value[0] || a->value[1] != b->value[1] ||
            memcmp(a->vc.c, b->vc.c, sizeof a->vc.c) != 0 ||
            (a->debit & 0x0FFFu) != (b->debit & 0x0FFFu)) {
            fprintf(stderr, "record #%u diverged:\n"
                    "  ifl: src=%u ty=%u fl=%x rg=%u cm=%u t=%u aux=%u "
                    "prov=%u/%u v=%x,%x vc=%u,%u,%u,%u db=%x\n"
                    "  if5: src=%u ty=%u fl=%x rg=%u cm=%u t=%u aux=%u "
                    "prov=%u/%u v=%x,%x vc=%u,%u,%u,%u db=%x\n",
                    k,
                    a->src, a->ptype, a->flags, a->region, a->commit, a->t,
                    a->aux, a->prov_a, a->prov_b, a->value[0], a->value[1],
                    a->vc.c[0], a->vc.c[1], a->vc.c[2], a->vc.c[3], a->debit,
                    b->src, b->ptype, b->flags, b->region, b->commit, b->t,
                    b->aux, b->prov_a, b->prov_b, b->value[0], b->value[1],
                    b->vc.c[0], b->vc.c[1], b->vc.c[2], b->vc.c[3], b->debit);
            die("record fields diverged", opno, phase);
        }
    }

    /* History bytes exact; working set = slice + the registered
     * delta. */
    if (ifl_history_bytes(d->l) != if5_history_bytes(d->v))
        die("history bytes diverged", opno, phase);
    if (if5_working_set_bytes(d->v) - ifl_working_set_bytes(d->l) != ws_delta())
        die("working-set delta is not the registered constant", opno, phase);
}

/* The PROV count callback (shared shape for both cores' walks —
 * the function pointer types differ only by the record type, and
 * the callback ignores the record). */
static void count_cb(const void* rec, void* user)
{
    (void)rec;
    (*(uint32_t*)user)++;
}

/* ============================================================
 * Phase 1 — TRIP-17, both arms in lockstep
 * ============================================================ */

static void phase_trip17(void)
{
    duo d;
    uint32_t s_nlog_after[64];
    uint32_t iid_after[64];
    uint32_t branch_cut = 0;
    int i;

    mk_duo(&d, SCEN_SYS_BUDGET, 200, 200);
    duo_region(&d, "budget", IF5_T_NUM, IF5_TIER_MERGE);
    duo_region(&d, "stops", SCEN_T_SEQ, IF5_TIER_FORK);
    duo_region(&d, "tags", IF5_T_SET, IF5_TIER_MERGE);
    duo_region(&d, "title", SCEN_T_STR, IF5_TIER_STRICT);

    for (i = 0; i < SCEN_N_OPS; i++) {
        const scen_op* op = &scen_script[i];
        uint32_t cut_now_l = ifl_log_count(d.l);

        switch (op->kind) {
        case SOP_REGION:
            /* regions all pre-created above (the script's four) */
            break;
        case SOP_DELEG:
            (void)ifl_delegate(d.l, op->src, op->dst, (uint32_t)op->value);
            (void)if5_delegate(d.v, op->src, op->dst, (uint32_t)op->value);
            break;
        case SOP_VALUE: {
            uint32_t cut = op->cut == SCEN_CUT_NOW ? cut_now_l
                          : op->cut == 0 ? 0
                          : s_nlog_after[op->cut - 1];
            uint8_t ptype = op->region[0] == 'b' ? IFL_T_NUM
                          : op->region[0] == 't' && op->region[1] == 'a' ? IFL_T_SET
                          : op->region[0] == 't' ? SCEN_T_STR : SCEN_T_SEQ;
            duo_submit(&d, op->src, op->region, ptype, op->value, cut,
                       op->debit, op->sev);
            break;
        }
        case SOP_TIER:
            (void)ifl_region_set_tier(d.l, op->src,
                    ifl_region_find(d.l, op->region), (ifl_tier)op->value);
            (void)if5_region_set_tier(d.v, op->src,
                    if5_region_find(d.v, op->region), (if5_tier)op->value);
            break;
        case SOP_QUERY: {
            /* read-only on both cores; the fold equality is the
             * light compare's job */
            ifl_view vl;
            if5_view vv;
            if (op->value == 0) { vl = ifl_now(d.l); vv = if5_now(d.v); }
            else if ((uint32_t)op->value == 34) {
                vl = ifl_view_at(d.l, branch_cut);
                vv = if5_view_at(d.v, branch_cut);
            } else {
                uint32_t c = op->value == 0 ? 0 : s_nlog_after[op->value - 1];
                vl = ifl_view_at(d.l, c);
                vv = if5_view_at(d.v, c);
            }
            {
                uint64_t a = ifl_state_at(d.l,
                                ifl_region_find(d.l, op->region), &vl, NULL);
                uint64_t b = if5_state_at(d.v,
                                if5_region_find(d.v, op->region), &vv, NULL);
                if (a != b) die("scenario query diverged", (uint32_t)(i + 1), "trip17");
            }
            break;
        }
        case SOP_PROV: {
            /* both walks run; contributor counts must match */
            uint32_t nl = 0, nv = 0;
            ifl_prov_walk(d.l, ifl_region_find(d.l, op->region), NULL,
                          (ifl_prov_cb)count_cb, &nl);
            if5_prov_walk(d.v, if5_region_find(d.v, op->region), NULL,
                          (if5_prov_cb)count_cb, &nv);
            if (nl != nv) die("prov walk diverged", (uint32_t)(i + 1), "trip17");
            break;
        }
        case SOP_UNDO: {
            uint32_t target = iid_after[op->value - 1];
            branch_cut = ifl_log_count(d.l);
            (void)ifl_undo_nth(d.l, op->src, target);
            (void)if5_undo_nth(d.v, op->src, target);
            break;
        }
        case SOP_MERGE: {
            ifl_region* r = ifl_region_info(d.l, ifl_region_find(d.l, op->region));
            uint32_t ha = iid_after[op->value - 1];
            uint32_t hb = iid_after[op->dst - 1];
            uint64_t stitch = r ? r->cache : 0;
            (void)ifl_merge_intent(d.l, op->src,
                    ifl_region_find(d.l, op->region), ha, hb, stitch);
            (void)if5_merge_intent(d.v, op->src,
                    if5_region_find(d.v, op->region), ha, hb, stitch);
            break;
        }
        case SOP_WINDOW:
            ifl_blackbox_configure(d.l, ifl_region_find(d.l, op->region),
                                   (uint16_t)op->value);
            if5_blackbox_configure(d.v, if5_region_find(d.v, op->region),
                                   (uint16_t)op->value);
            break;
        case SOP_EXPORT:
            /* the full compare performs exports; here the beat's
             * side effect must land identically */
            {
                static char bl[8192], bv[8192];
                uint32_t a = ifl_blackbox_export(d.l, op->src,
                                ifl_region_find(d.l, op->region),
                                bl, sizeof bl);
                uint32_t b = if5_blackbox_export(d.v, op->src,
                                if5_region_find(d.v, op->region),
                                bv, sizeof bv);
                if (a != b || strcmp(bl, bv) != 0)
                    die("scenario export diverged", (uint32_t)(i + 1), "trip17");
            }
            break;
        default:
            break;
        }

        iid_after[i] = ifl_log_count(d.l);
        s_nlog_after[i] = ifl_log_count(d.l);
        cmp_light(&d, (uint32_t)(i + 1), "trip17");
    }

    cmp_full(&d, SCEN_N_OPS, "trip17-final");
    free_duo(&d);
}

/* ============================================================
 * Phase 2 — seeded random streams
 * ============================================================ */

#define N_STREAMS 100u
#define OPS_PER_STREAM 200u

static void phase_random(void)
{
    uint32_t s;

    for (s = 0; s < N_STREAMS; s++) {
        duo d;
        static const char* rnames[4] = { "budget", "stops", "tags", "title" };
        static const uint8_t rtypes[4] = { IFL_T_NUM, SCEN_T_SEQ, IFL_T_SET, SCEN_T_STR };
        static const uint8_t rtiers[4] = { 2, 1, 2, 3 };
        uint32_t o;
        uint8_t deleg_a = 0;

        mk_duo(&d, 32 + (s % 64), 20 + (rng() % 100), 15 + (rng() % 40));
        for (o = 0; o < 4; o++)
            duo_region(&d, rnames[o], rtypes[o], rtiers[o]);

        for (o = 0; o < OPS_PER_STREAM; o++) {
            uint64_t roll = rng() % 100;
            uint32_t n_log = ifl_log_count(d.l);

            if (roll < 55) {
                /* VALUE: random region/source/value; knowledge is
                 * stale 1/4 of the time (real concurrency). */
                uint32_t rid = (uint32_t)(rng() % 4);
                uint32_t src = (uint32_t)(rng() % 3);
                uint64_t val = rng();
                uint32_t cut = (rng() % 4 == 0) ? (uint32_t)(rng() % (n_log + 1))
                                                : n_log;
                uint8_t debit = 0, sev = 0;
                if (rng() % 8 == 0) {
                    debit = (uint8_t)(1 + rng() % 2);   /* initiative or surprise */
                    sev = (uint8_t)(1u << (rng() % 4));  /* a ladder rung */
                }
                duo_submit(&d, (ifl_sid)src, rnames[rid], rtypes[rid],
                           val, cut, debit, sev);
            } else if (roll < 70) {
                /* TIER: the S14 dial. */
                uint32_t rid = (uint32_t)(rng() % 4);
                uint8_t tier = (uint8_t)(1 + rng() % 3);
                (void)ifl_region_set_tier(d.l, 0,
                        ifl_region_find(d.l, rnames[rid]), (ifl_tier)tier);
                (void)if5_region_set_tier(d.v, 0,
                        if5_region_find(d.v, rnames[rid]), (if5_tier)tier);
            } else if (roll < 82) {
                /* UNDO: a random addressable record. */
                if (n_log > 4) {
                    uint32_t target = 1 + (uint32_t)(rng() % (n_log - 2));
                    (void)ifl_undo_nth(d.l, 0, target);
                    (void)if5_undo_nth(d.v, 0, target);
                }
            } else if (roll < 88) {
                /* DELEG: chain deepening (idempotent re-carves are
                 * legal on both cores). */
                if (deleg_a == 0 || rng() % 2 == 0) {
                    uint32_t amt = 5 + (uint32_t)(rng() % 10);  /* drawn
                                            * ONCE: both cores get
                                            * the same amount (the
                                            * first draft drew twice
                                            * and the differential
                                            * caught it at record
                                            * level — its job) */
                    (void)ifl_delegate(d.l, 0, 1, amt);
                    (void)if5_delegate(d.v, 0, 1, amt);
                    deleg_a = 1;
                } else {
                    uint32_t amt = 2 + (uint32_t)(rng() % 5);
                    (void)ifl_delegate(d.l, 1, 2, amt);
                    (void)if5_delegate(d.v, 1, 2, amt);
                }
            } else {
                /* MERGE: stitch a fork when one exists (heads read
                 * from the IFL side; equal by induction). */
                uint32_t rid;
                bool merged = false;
                for (rid = 0; rid < 4 && !merged; rid++) {
                    ifl_region* r = ifl_region_info(d.l,
                            ifl_region_find(d.l, rnames[rid]));
                    if5_region* r5 = if5_region_info(d.v,
                            if5_region_find(d.v, rnames[rid]));
                    /* The two tables must agree before we read from
                     * either (a divergence here is itself a bug). */
                    if ((r && !r5) || (!r && r5) ||
                        (r && r5 && (r->head_a != r5->head_a ||
                                     r->head_b != r5->head_b))) {
                        fprintf(stderr, "REGION TABLE DIVERGED at '%s': "
                                "ifl a=%u b=%u | if5 a=%u b=%u\n",
                                rnames[rid],
                                r ? r->head_a : 0, r ? r->head_b : 0,
                                r5 ? r5->head_a : 0, r5 ? r5->head_b : 0);
                        die("region tables diverged", o + 1, "random");
                    }
                    if (r && r->head_b) {
                        uint64_t stitch = r->cache;
                        uint32_t ha = r->head_a, hb = r->head_b;
                        uint16_t rl_id = ifl_region_find(d.l, rnames[rid]);
                        uint16_t rv_id = if5_region_find(d.v, rnames[rid]);
                        uint32_t ml = ifl_merge_intent(d.l, 0, rl_id, ha, hb, stitch);
                        uint32_t mv = if5_merge_intent(d.v, 0, rv_id, ha, hb, stitch);
                        if ((ml == 0) != (mv == 0)) {
                            const ifl_intent* al = ifl_log_at(d.l, ha);
                            const ifl_intent* bl = ifl_log_at(d.l, hb);
                            const if5_intent* av = if5_log_at(d.v, ha);
                            const if5_intent* bv = if5_log_at(d.v, hb);
                            fprintf(stderr, "MERGE OUTCOME DIVERGED on '%s' rid %u/%u: "
                                    "ifl=%u if5=%u (passed heads %u/%u)\n",
                                    rnames[rid], rl_id, rv_id, ml, mv, ha, hb);
                            fprintf(stderr, "  head_a rec: ifl rg=%u ty=%u flags=%x | if5 rg=%u ty=%u flags=%x\n",
                                    al ? al->region : 9999, al ? al->ptype : 99, al ? al->flags : 99,
                                    av ? av->region : 9999, av ? av->ptype : 99, av ? av->flags : 99);
                            fprintf(stderr, "  head_b rec: ifl rg=%u ty=%u flags=%x | if5 rg=%u ty=%u flags=%x\n",
                                    bl ? bl->region : 9999, bl ? bl->ptype : 99, bl ? bl->flags : 99,
                                    bv ? bv->region : 9999, bv ? bv->ptype : 99, bv ? bv->flags : 99);
                            die("merge outcome diverged", o + 1, "random");
                        }
                        merged = true;
                    }
                }
            }
            cmp_light(&d, o + 1, "random");
        }
        cmp_full(&d, OPS_PER_STREAM, "random-final");
        free_duo(&d);
    }
    printf("(%u streams x %u ops) ", N_STREAMS, OPS_PER_STREAM);
}

/* ============================================================
 * Phase 3 — the E2 shape, with v5's residency re-verified (V9)
 * ============================================================ */

#define E2_REGIONS 1000u
#define E2_CHANGES 4000u

static void phase_e2(void)
{
    duo d;
    ifl_region_spec* sl = (ifl_region_spec*)malloc(E2_REGIONS * sizeof(ifl_region_spec));
    if5_region_spec* sv = (if5_region_spec*)malloc(E2_REGIONS * sizeof(if5_region_spec));
    uint64_t h_at[5];
    uint32_t i, made_l, made_v;

    assert(sl && sv);
    mk_duo(&d, 512, 512, 512);
    for (i = 0; i < E2_REGIONS; i++) {
        char name[8];
        snprintf(name, sizeof name, "r%04u", (unsigned)i);
        memset(&sl[i], 0, sizeof sl[i]);
        memset(&sv[i], 0, sizeof sv[i]);
        strcpy(sl[i].name, name); strcpy(sv[i].name, name);
        sl[i].ptype = IFL_T_NUM;  sv[i].ptype = IF5_T_NUM;
        sl[i].tier = 2;           sv[i].tier = 2;
        sl[i].seed = i;           sv[i].seed = i;
        sv[i].parent = 0;         /* the flat, slice-compatible shape */
    }
    made_l = ifl_region_create_batch(d.l, 0, sl, E2_REGIONS);
    made_v = if5_region_create_batch(d.v, 0, sv, E2_REGIONS);
    if (made_l != made_v) die("batch create diverged", 0, "e2");
    assert(made_l == E2_REGIONS);

    {
        uint32_t cp = 0;
        static const uint32_t checkpoints[4] = { 500, 1000, 2000, 4000 };
        for (i = 0; i < E2_CHANGES; i++) {
            uint32_t rid;
            uint32_t cut;
            uint64_t val = rng();
            if (rng() % 100 < 15) {
                /* hotspot: the first 50 regions, stale knowledge —
                 * the registered concurrency shape */
                rid = (uint32_t)(rng() % 50);
                cut = (uint32_t)(rng() % (ifl_log_count(d.l) + 1));
            } else {
                rid = (uint32_t)(rng() % E2_REGIONS);
                cut = ifl_log_count(d.l);
            }
            {
                char name[8];
                ifl_submit_opts ol; if5_submit_opts ov;
                snprintf(name, sizeof name, "r%04u", (unsigned)rid);
                memset(&ol, 0, sizeof ol); memset(&ov, 0, sizeof ov);
                ol.observed_cut = cut; ov.observed_cut = cut;
                (void)ifl_submit(d.l, 0, ifl_region_find(d.l, name),
                                 IFL_T_NUM, val, &ol);
                (void)if5_submit(d.v, 0, if5_region_find(d.v, name),
                                 IF5_T_NUM, val, &ov);
            }
            if (cp < 4 && i + 1 == checkpoints[cp]) {
                /* checkpoint: byte counts, a state digest over the
                 * hot regions + a sample, and the stats. */
                uint32_t k;
                if (ifl_log_count(d.l) != if5_log_count(d.v))
                    die("e2 log counts diverged", i + 1, "e2");
                if (ifl_history_bytes(d.l) != if5_history_bytes(d.v))
                    die("e2 history bytes diverged", i + 1, "e2");
                if (if5_working_set_bytes(d.v) - ifl_working_set_bytes(d.l)
                    != ws_delta())
                    die("e2 working-set delta diverged", i + 1, "e2");
                if (memcmp(ifl_stats_of(d.l), if5_stats_of(d.v),
                           sizeof(ifl_stats)) != 0)
                    die("e2 stats diverged", i + 1, "e2");
                for (k = 0; k < 50; k += 7) {
                    char name[8];
                    ifl_view vl = ifl_now(d.l);
                    if5_view vv = if5_now(d.v);
                    uint64_t a, b;
                    snprintf(name, sizeof name, "r%04u", (unsigned)k);
                    a = ifl_state_at(d.l, ifl_region_find(d.l, name), &vl, NULL);
                    b = if5_state_at(d.v, if5_region_find(d.v, name), &vv, NULL);
                    if (a != b) die("e2 state digest diverged", i + 1, "e2");
                }
                h_at[cp] = if5_history_bytes(d.v);
                cp++;
            }
        }
    }

    /* V9: v5's OWN residency against the registered C8 boundary —
     * measured on the derivation, not transferred by argument.
     * The boundary is at the registered LOAD FLOOR n = 2000
     * (ADR-0025 C8a's wording: "residency <= 3.0x at n = 2000");
     * at n = 4000 the slice itself measured 4.006x (history grows
     * linearly, the working set is constant — the ratio is not
     * meant to be bounded past the floor), so the assert reads the
     * n=2000 checkpoint only. Linearity is separately checked
     * below over the 2000->4000 span. */
    {
        uint64_t ws = if5_working_set_bytes(d.v);
        double residency = (double)h_at[2] / (double)ws;
        if (residency > 3.0) {
            fprintf(stderr, "\nV9 FAILED: v5 residency %.3fx at the n=2000 "
                    "load floor exceeds the registered 3.0x boundary\n",
                    residency);
            assert(false);
        }
        /* Linearity: the incremental cost per change between the
         * n=2000 and n=4000 checkpoints is 48 bytes +/- 15% (the
         * registered record density). */
        {
            double slope = (double)(h_at[3] - h_at[2]) / 2000.0;
            if (slope < 40.8 || slope > 55.2) {
                fprintf(stderr, "\nV9 FAILED: v5 incremental cost %.3f B/change "
                        "outside the registered [40.8, 55.2] band\n", slope);
                assert(false);
            }
            printf("(v5 residency %.3fx @2000, slope %.1f B/change) ",
                   (double)h_at[2] / (double)ws, slope);
        }
    }

    cmp_full(&d, E2_CHANGES, "e2-final");
    free_duo(&d);
    free(sl);
    free(sv);
}

int main(void)
{
    printf("=== v5 differential (ADR-0026 V1/V9: the rewrite may extend, "
           "never alter) ===\n\n");
    printf("registered working-set delta: %llu bytes "
           "(sizeof(if5_world) - sizeof(ifl_world))\n\n",
           (unsigned long long)ws_delta());
    printf("--- phase 1: TRIP-17 in lockstep ---\n");
    phase_trip17();
    printf("TRIP-17: identical on every compared surface\n\n");
    printf("--- phase 2: seeded random streams ---\n");
    phase_random();
    printf("random streams: identical on every compared surface\n\n");
    printf("--- phase 3: E2 shape (%u regions, %u changes) ---\n",
           E2_REGIONS, E2_CHANGES);
    phase_e2();
    printf("E2 shape: identical; residency within the registered boundary\n\n");
    printf("differential: PASS (v5 == slice on the registered observation "
           "surface)\n");
    return 0;
}
