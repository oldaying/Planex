/*
 * e2_engineering_leg.c — the E2 engineering leg: the research
 * series' first machine-measured number, run AT LANDING under the
 * pre-registered boundary (ADR-0025 Decision 4f, prediction C8).
 *
 * Registered load (ADR-0025 4f):
 *   - 1000 mutable objects, mutations NUM and SET payloads, from
 *     3 sources plus the system source
 *   - mixed 85% causal / 15% concurrent pairs
 *   - checkpoints at n = 500, 1000, 2000, 4000 changes
 *
 * Registered boundary (Reading B, adjudicated in ADR-0025 4f):
 *   - residency <= 3.0 x working set AT n = 2000
 *   - across all checkpoints: linear fit with R^2 >= 0.99 AND
 *     per-checkpoint incremental bytes-per-change within +/-15%
 *     of the session mean
 *
 * Residency is measured by instrumented byte accounting (the sum
 * of all bytes retained solely because of N2: intent records at
 * 48 bytes, the payload arena, the receipt store) — not by RSS,
 * so the number is deterministic and recomputable on any checkout.
 * The rolling window (S3) is explicitly DISABLED during
 * measurement (flattening the curve to pass linearity would be
 * boundary smuggling in the other direction).
 *
 * Self-judging (vol. 6 §2.4's recomputation rule): this binary
 * exits non-zero if either boundary breaks — the registered
 * boundaries re-verify on every clean checkout. Raw bytes at
 * every checkpoint are printed as CSV to stdout so a reviewer can
 * re-adjudicate under any stricter reading (e.g. ratio at all n)
 * without re-running anything.
 *
 * Row shape (the registered 40-byte row: 24-byte label, u32
 * value, u16 seq, u16 flags, u32 last-intent ref) maps onto the
 * slice's regions: the label lives in the region name (its unique
 * prefix at the slice's registered IFL_NAME_MAX), the value is
 * the NUM payload, flags are the SET payload of even rows, the
 * sequence and last-intent reference are the record's own t and
 * prov fields. Odd rows are NUM regions, even rows SET regions —
 * the mutation mix is registered as "NUM and SET payloads".
 */
#include "planex/ifl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- the registered load ------------------------------------- */

#define E2_N_OBJECTS    1000
#define E2_N_CHANGES    4000
#define E2_CONCUR_SHARE 15u          /* percent (85/15 registered) */

/* Deterministic PRNG (xorshift64) — seeded, so the published
 * numbers are recomputable on any checkout. */
static uint64_t e2_rng_state = 0x9E3779B97F4A7C15ull;

static uint64_t e2_rng(void)
{
    uint64_t x = e2_rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    e2_rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

typedef struct {
    uint32_t n_changes;        /* changes so far                 */
    uint32_t n_log;            /* log records                    */
    uint64_t history_bytes;    /* N2 residency                   */
    uint64_t working_bytes;    /* live state                     */
    double   ratio;            /* history / working              */
    double   incr_per_change;  /* bytes per change since prev ckpt */
} e2_checkpoint;

/* The log count after each change — the stale-knowledge lookup
 * for the concurrency beat ("the world as of k changes ago"). */
static uint32_t log_after_change[E2_N_CHANGES + 1];
/* The object each change landed on — the concurrency beat aims
 * stale knowledge at RECENTLY-WRITTEN objects (the hot-object
 * shape of a real workload), so a stale submission actually
 * races the record it cannot see. */
static uint32_t obj_of_change[E2_N_CHANGES + 1];

int main(void)
{
    ifl_world* w;
    ifl_source src;
    ifl_sid sid[4];
    ifl_region_spec specs[E2_N_OBJECTS];
    static const uint32_t ckpts[] = { 500, 1000, 2000, 4000 };
    e2_checkpoint cks[4];
    uint32_t n_cks = 0;
    uint32_t i, n = 0;
    uint32_t concurrent_pairs = 0;
    uint64_t prev_hist = 0;
    uint32_t prev_changes = 0;

    w = ifl_world_create();
    if (!w) { fprintf(stderr, "world alloc failed\n"); return 2; }

    /* 3 sources + the system: H, A, B, SYS — the system pays the
     * tier-2 merges (registered: "from 3 sources plus the system
     * source"). */
    memset(&src, 0, sizeof src);
    strcpy(src.id, "load");   src.kind = IFL_KIND_PERSON;
    src.initiative_budget = IFL_BUDGET_UNLIMITED; src.authority = 0xFF;
    sid[0] = ifl_add_source(w, &src);
    strcpy(src.id, "mut-a");  src.kind = IFL_KIND_AGENT;
    src.initiative_budget = 5000; src.authority = 0xFF;
    sid[1] = ifl_add_source(w, &src);
    strcpy(src.id, "mut-b");  src.kind = IFL_KIND_AGENT;
    src.initiative_budget = 5000; src.authority = 0xFF;
    sid[2] = ifl_add_source(w, &src);
    strcpy(src.id, "e2-sys"); src.kind = IFL_KIND_PROCESS;
    src.initiative_budget = 100000; src.authority = 0;
    sid[3] = ifl_add_source(w, &src);
    ifl_set_system_source(w, sid[3]);

    /* The 1000-object load, materialized as ONE document-load
     * intent (ADR-0025 4f: batch form). Odd rows NUM (u32 value),
     * even rows SET (u64 flags). */
    for (i = 0; i < E2_N_OBJECTS; i++) {
        snprintf(specs[i].name, sizeof specs[i].name, "o%03u", i);
        specs[i].ptype = (uint8_t)(i % 2 ? IFL_T_NUM : IFL_T_SET);
        specs[i].tier  = IFL_TIER_MERGE;   /* tolerant: the CRDT beat */
        specs[i].seed  = e2_rng() & 0xFFFFFFFFull;
    }
    if (ifl_region_create_batch(w, sid[0], specs, E2_N_OBJECTS)
        != E2_N_OBJECTS) {
        fprintf(stderr, "batch load failed\n"); return 2;
    }

    /* The incremental-cost baseline: the load blob (the 1000-object
     * creation) is part of the REGISTERED LOAD, not part of the
     * mutation stream's marginal cost — the per-change increment is
     * measured from "load complete" onward. (Adjudicated against
     * the alternative — amortizing the blob into the first window —
     * which would report 68 B/change for window 1 and 48 for the
     * rest: a one-time load cost smeared into a linearity verdict
     * it has nothing to say about. The blob's own bytes stay in
     * history_bytes at every checkpoint; only the INCREMENT starts
     * here.) */
    prev_hist = ifl_history_bytes(w);

    /* The mutation stream: 85% causal (full knowledge), 15%
     * concurrent (stale knowledge — the submitter saw the world
     * as of an older change). */
    while (n < E2_N_CHANGES) {
        uint32_t obj;
        uint32_t who = (uint32_t)(e2_rng() % 3);   /* H, A, B */
        bool concurrent = (e2_rng() % 100u) < E2_CONCUR_SHARE;
        ifl_submit_opts o;
        uint64_t value;
        uint16_t rid;

        /* The registered mix: 85% causal / 15% concurrent PAIRS.
         * A concurrent submission aims at a recently-written
         * object with stale knowledge — otherwise the stale cut
         * lands on a cold object, sees nothing to race, and the
         * pair never realizes (the first E2 run measured exactly
         * that failure mode: 607 stale submissions, 2 realized
         * pairs — recorded here so the load's shape decision is
         * on the record, not buried in the generator). */
        if (concurrent && n > 1) {
            obj = obj_of_change[n - 1 - (uint32_t)(e2_rng() % 2)];
        } else {
            obj = (uint32_t)(e2_rng() % E2_N_OBJECTS);
        }
        rid = (uint16_t)(obj + 1);

        memset(&o, 0, sizeof o);
        if (concurrent && n > 4) {
            /* Stale knowledge: the state a few changes back — the
             * submission is then concurrent with whatever landed
             * since (per-object; the fold decides). */
            o.observed_cut = log_after_change[n - 3 - (uint32_t)(e2_rng() % 3)];
            concurrent_pairs++;
        } else {
            o.observed_cut = ifl_log_count(w);
        }
        value = e2_rng();
        if (obj % 2) value &= 0xFFFFFFFFull;       /* NUM: u32 */
        (void)ifl_submit(w, sid[who], rid,
                         (uint8_t)(obj % 2 ? IFL_T_NUM : IFL_T_SET),
                         value, &o);
        n++;
        log_after_change[n] = ifl_log_count(w);
        obj_of_change[n] = obj;

        /* Checkpoints: the registered four. */
        for (i = 0; i < 4; i++) {
            if (n == ckpts[i]) {
                cks[n_cks].n_changes = n;
                cks[n_cks].n_log = ifl_log_count(w);
                cks[n_cks].history_bytes = ifl_history_bytes(w);
                cks[n_cks].working_bytes = ifl_working_set_bytes(w);
                cks[n_cks].ratio = (double)cks[n_cks].history_bytes
                                 / (double)cks[n_cks].working_bytes;
                if (n_cks > 0 || prev_changes > 0) {
                    cks[n_cks].incr_per_change =
                        (double)(cks[n_cks].history_bytes - prev_hist)
                        / (double)(n - prev_changes);
                } else {
                    /* First checkpoint after load: the increment
                     * from the load-complete baseline. */
                    cks[n_cks].incr_per_change =
                        (double)(cks[n_cks].history_bytes - prev_hist)
                        / (double)n;
                }
                prev_hist = cks[n_cks].history_bytes;
                prev_changes = n;
                n_cks++;
            }
        }
    }

    /* ---- publish: CSV first (the raw data), verdict after ----- */
    printf("# e2-engineering-leg raw data (recomputable: seeded "
           "xorshift64, seed 0x9E3779B97F4A7C15)\n");
    printf("checkpoint,n_changes,n_log,history_bytes,working_set_bytes,"
           "ratio,incr_bytes_per_change\n");
    for (i = 0; i < n_cks; i++)
        printf("%u,%u,%u,%llu,%llu,%.6f,%.3f\n", i + 1,
               cks[i].n_changes, cks[i].n_log,
               (unsigned long long)cks[i].history_bytes,
               (unsigned long long)cks[i].working_bytes,
               cks[i].ratio, cks[i].incr_per_change);

    printf("\nconcurrent_pairs=%u of %u changes (%.1f%%)\n",
           concurrent_pairs, E2_N_CHANGES,
           100.0 * concurrent_pairs / E2_N_CHANGES);
    {
        const ifl_stats* st = ifl_stats_of(w);
        printf("tier2_merges=%u tier1_forks=%u tier3=%u breakers=%u "
               "receipts=%u\n",
               st->tier2_merges, st->tier1_forks, st->tier3_arbitrations,
               st->breakers, st->receipts);
    }

    /* ---- self-judge the registered boundaries (C8) ----------- */

    /* Boundary 1: ratio <= 3.0 AT n = 2000 (the load floor). */
    {
        double r2000 = 0;
        bool found = false;
        for (i = 0; i < n_cks; i++)
            if (cks[i].n_changes == 2000) { r2000 = cks[i].ratio; found = true; }
        printf("\n[C8a] residency at n=2000: %.3f x working set "
               "(boundary <= 3.0)\n", r2000);
        if (!found || r2000 > 3.0) {
            printf("C8a FAILED — N2's engineering leg has fired; the "
                   "demotion recommendation goes to the maintainer "
                   "with this data\n");
            ifl_world_free(w);
            return 1;
        }
        printf("C8a PASS\n");
    }

    /* Boundary 2: linearity — R^2 >= 0.99 over the four
     * checkpoints, and incremental bytes-per-change within +/-15%
     * of the session mean. */
    {
        /* Least squares over (n_changes, history_bytes). */
        double sx = 0, sy = 0, sxx = 0, sxy = 0;
        double slope, intercept, sst = 0, sse = 0, r2;
        double mean_incr = 0, lo, hi;
        bool incr_ok = true;
        for (i = 0; i < n_cks; i++) {
            double x = (double)cks[i].n_changes;
            double y = (double)cks[i].history_bytes;
            sx += x; sy += y; sxx += x * x; sxy += x * y;
        }
        slope = (n_cks * sxy - sx * sy) / (n_cks * sxx - sx * sx);
        intercept = (sy - slope * sx) / n_cks;
        for (i = 0; i < n_cks; i++) {
            double x = (double)cks[i].n_changes;
            double y = (double)cks[i].history_bytes;
            double fit = slope * x + intercept;
            sst += (y - sy / n_cks) * (y - sy / n_cks);
            sse += (y - fit) * (y - fit);
        }
        r2 = sst > 0 ? 1.0 - sse / sst : 0.0;

        for (i = 0; i < n_cks; i++) mean_incr += cks[i].incr_per_change;
        mean_incr /= (double)n_cks;
        lo = mean_incr * 0.85;
        hi = mean_incr * 1.15;
        for (i = 0; i < n_cks; i++)
            if (cks[i].incr_per_change < lo || cks[i].incr_per_change > hi)
                incr_ok = false;

        printf("[C8b] linearity: R^2 = %.6f (boundary >= 0.99), "
               "slope = %.1f bytes/change\n", r2, slope);
        printf("[C8b] incremental cost: mean %.1f, window [%.1f, %.1f] "
               "(boundary +/-15%% of mean): %s\n",
               mean_incr, lo, hi, incr_ok ? "within" : "OUTSIDE");
        if (r2 < 0.99 || !incr_ok) {
            printf("C8b FAILED — the growth is not honestly linear; "
                   "recorded as measured, not patched\n");
            ifl_world_free(w);
            return 1;
        }
        printf("C8b PASS\n");
    }

    printf("\ne2_engineering_leg: ALL REGISTERED BOUNDARIES PASS\n");
    ifl_world_free(w);
    return 0;
}
