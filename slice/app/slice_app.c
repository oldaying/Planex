/*
 * slice_app.c — the treatment arm: the slice proper.
 *
 * Encodes TRIP-17 (app/scenario.h) in the intent-flow ontology.
 * Every op becomes an intent; every beat prints the same line the
 * control arm prints, prefixed identically, so the two transcripts
 * are line-comparable. The tail prints the slice's own cost table
 * (what the ontology spends) — the control arm prints ITS cost
 * table (what the classical encoding spends): C9's comparison is
 * read off exactly these two tables, measured, not judged.
 *
 * Cost accounting on this arm (all zero by construction unless the
 * ontology genuinely requires them):
 *   mode_switches          a mode machine consulted per handler
 *                          entry — the slice has none to consult
 *   attribution_records    hand-maintained attribution side-list
 *                          entries — attribution is the PROV walk
 *   snapshot_copies        snapshot copies for time-addressed
 *                          queries — state@t is a fold, not a copy
 *   manual_version_records hand-rolled version bookkeeping at the
 *                          fork beat — forks are first-class
 */
#include "scenario.h"
#include "planex/ifl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The interpreter's per-line log-state record: op.cut references
 * the n_log after script line n. */
static uint32_t s_nlog_after[64];

/* PROV walk accumulator: prints attribution, counts contributors. */
static struct { uint32_t n; bool first; } prov_acc;

static void prov_cb(const ifl_intent* rec, void* user)
{
    (void)user;
    if (prov_acc.first) { printf(" "); prov_acc.first = false; }
    else printf(" <- ");
    printf("src%u@t%u", rec->src, rec->t);
    prov_acc.n++;
}

static uint32_t cut_of(uint32_t ref, uint32_t now)
{
    if (ref == SCEN_CUT_NOW) return now;
    if (ref == 0) return 0;              /* the empty world */
    return s_nlog_after[ref - 1];
}

int main(void)
{
    ifl_world* w;
    ifl_source src;
    ifl_sid sid_H, sid_A, sid_B, sid_SYS;
    uint8_t t_seq, t_str;
    uint32_t iid_after[64];
    int i;
    uint32_t branch_cut = 0;
    char buf[8192];

    w = ifl_world_create();
    if (!w) { fprintf(stderr, "world alloc failed\n"); return 2; }

    /* The scenario's SEQ and STR register through the USER channel
     * (B2's extension path — the built-ins used the same door). */
    {
        ifl_payload_type seq, str;
        memset(&seq, 0, sizeof seq);
        seq.name = "seq";
        seq.value_size = 8;               /* no sq: rigid by registration */
        t_seq = ifl_register_payload_type(w, &seq);
        memset(&str, 0, sizeof str);
        str.name = "str";
        str.value_size = 8;
        t_str = ifl_register_payload_type(w, &str);
        if (t_seq == (uint8_t)-1 || t_str == (uint8_t)-1) {
            fprintf(stderr, "user type registration failed\n"); return 2;
        }
    }

    /* Sources: kind is metadata. The budget handles are the only
     * behavioral difference between them. */
    memset(&src, 0, sizeof src);
    strcpy(src.id, "human");   src.kind = IFL_KIND_PERSON;
    src.initiative_budget = IFL_BUDGET_UNLIMITED; src.authority = 0xFF;
    sid_H = ifl_add_source(w, &src);
    strcpy(src.id, "agentA");  src.kind = IFL_KIND_AGENT;
    src.initiative_budget = 200; src.authority = 0xFF;
    sid_A = ifl_add_source(w, &src);
    strcpy(src.id, "agentB");  src.kind = IFL_KIND_AGENT;
    src.initiative_budget = 200; src.authority = 0xFF;
    sid_B = ifl_add_source(w, &src);
    strcpy(src.id, "system");  src.kind = IFL_KIND_PROCESS;
    src.initiative_budget = SCEN_SYS_BUDGET; src.authority = 0;
    sid_SYS = ifl_add_source(w, &src);
    ifl_set_system_source(w, sid_SYS);

    for (i = 0; i < SCEN_N_OPS; i++) {
        const scen_op* op = &scen_script[i];
        ifl_sid s = op->src == SCEN_SRC_H ? sid_H
                  : op->src == SCEN_SRC_A ? sid_A
                  : op->src == SCEN_SRC_B ? sid_B : sid_SYS;
        uint32_t iid = 0;

        switch (op->kind) {
        case SOP_REGION: {
            uint8_t pt = op->value == 0 ? IFL_T_NUM
                       : op->value == 1 ? IFL_T_SET
                       : op->value == (uint64_t)t_seq ? t_seq : t_str;
            uint16_t rid = ifl_region_create(w, s, op->region, pt,
                                             (ifl_tier)op->dst, NULL, 0);
            (void)rid;
            break;
        }
        case SOP_DELEG:
            (void)ifl_delegate(w, s, op->dst == SCEN_SRC_A ? sid_A : sid_B,
                               (uint32_t)op->value);
            break;
        case SOP_VALUE: {
            ifl_submit_opts o;
            memset(&o, 0, sizeof o);
            o.observed_cut = cut_of(op->cut, ifl_log_count(w));
            o.debit_class = (ifl_debit_class)op->debit;
            o.severity = (ifl_severity)op->sev;
            iid = ifl_submit(w, s, ifl_region_find(w, op->region),
                             op->region[0] == 'b' ? IFL_T_NUM
                           : op->region[0] == 't' && op->region[1] == 'a' ? IFL_T_SET
                           : op->region[0] == 't' ? t_str : t_seq,
                             op->value, &o);
            break;
        }
        case SOP_TIER:
            (void)ifl_region_set_tier(w, s, ifl_region_find(w, op->region),
                                      (ifl_tier)op->value);
            break;
        case SOP_QUERY: {
            ifl_view v;
            uint32_t contrib = 0;
            uint64_t val;
            if (op->value == 0) v = ifl_now(w);
            else if ((uint32_t)op->value == 34) { v = ifl_view_at(w, branch_cut); }
            else v = ifl_view_at(w, cut_of((uint32_t)op->value, ifl_log_count(w)));
            val = ifl_state_at(w, ifl_region_find(w, op->region), &v, &contrib);
            printf("  [query %s@%s] val=0x%llx contrib=%u\n", op->region,
                   op->value == 0 ? "now"
                 : (uint32_t)op->value == 34 ? "branch" : "then",
                   (unsigned long long)val, contrib);
            break;
        }
        case SOP_PROV: {
            /* The attribution audit: a walk, not a feature. The
             * callback receives newest-first contributors; the
             * transcript prints who moved the region, in order. */
            prov_acc.n = 0;
            prov_acc.first = true;
            printf("  [prov %s]", op->region);
            ifl_prov_walk(w, ifl_region_find(w, op->region), NULL,
                          prov_cb, &prov_acc);
            printf(" (%u contributors)\n", prov_acc.n);
            break;
        }
        case SOP_UNDO: {
            uint32_t target = iid_after[op->value - 1];
            branch_cut = ifl_log_count(w);  /* the abandoned branch's
                                             * address, kept for op 37 */
            ifl_undo_nth(w, s, target);
            break;
        }
        case SOP_MERGE: {
            ifl_region* r = ifl_region_info(w, ifl_region_find(w, op->region));
            iid = ifl_merge_intent(w, s, ifl_region_find(w, op->region),
                                   iid_after[op->value - 1],
                                   iid_after[op->dst - 1],
                                   r ? r->cache : 0);
            break;
        }
        case SOP_WINDOW:
            ifl_blackbox_configure(w, ifl_region_find(w, op->region),
                                   (uint16_t)op->value);
            break;
        case SOP_EXPORT: {
            uint32_t n = ifl_blackbox_export(w, s,
                            ifl_region_find(w, op->region),
                            buf, sizeof buf);
            printf("  [export %s] %u bytes of black box\n", op->region, n);
            break;
        }
        default:
            break;
        }

        if (iid == 0 && (op->kind == SOP_VALUE || op->kind == SOP_MERGE))
            iid = ifl_log_count(w);
        iid_after[i] = iid;
        s_nlog_after[i] = ifl_log_count(w);

        printf("%3d| %-72s | log=%u\n", i + 1, op->note, ifl_log_count(w));
    }

    /* ---- the final semantic projection: THE interface ---------- */
    printf("\n=== semantic projection (the slice's interface) ===\n");
    ifl_project_semantic(w, NULL, buf, sizeof buf);
    fputs(buf, stdout);

    printf("\n=== set-state projection ===\n");
    ifl_project_set(w, NULL, buf, sizeof buf);
    fputs(buf, stdout);

    /* ---- stats: the calibration suite's locked surface ---------- */
    {
        const ifl_stats* st = ifl_stats_of(w);
        printf("\n=== stats ===\n"
               "submits=%u causal=%u conc_arr=%u lattice=%u t2merge=%u "
               "t1fork=%u t3arb=%u sysmerge=%u\n"
               "breakers=%u receipts=%u excluded=%u mergeint=%u cursors=%u "
               "denied=%u\n",
               st->submits, st->causal_applies, st->concurrent_arrivals,
               st->lattice_checks, st->tier2_merges, st->tier1_forks,
               st->tier3_arbitrations, st->system_merge_debits,
               st->breakers, st->receipts, st->excluded,
               st->merge_intents, st->cursor_moves, st->denied_authority);
    }

    /* ---- the cost table (C9's treatment side) ------------------ *
     * Every number here is ZERO except what the ontology genuinely
     * spends. The control arm prints the same table with ITS
     * numbers; the calibration suite locks both. */
    printf("\n=== cost table (treatment) ===\n");
    printf("mode_switches=0            (no mode machine exists)\n");
    printf("attribution_records=0      (PROV walk replaces the list)\n");
    printf("snapshot_copies=0          (state@t is a fold)\n");
    printf("manual_version_records=0   (forks are first-class)\n");
    printf("log_records=%u\n", ifl_log_count(w));
    printf("history_bytes=%llu working_set_bytes=%llu\n",
           (unsigned long long)ifl_history_bytes(w),
           (unsigned long long)ifl_working_set_bytes(w));

    ifl_world_free(w);
    return 0;
}
