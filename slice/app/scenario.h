/*
 * scenario.h — TRIP-17: THE deterministic task script, shared by
 * both arms (ADR-0025 Decision 4a).
 *
 * One interface, one person, two agents, three intent flows,
 * time-addressable undo. The op table below is the TASK — the same
 * ops, the same order, the same beats for both arms. The treatment
 * arm (slice_app) encodes each op as an intent in the ifl_ core;
 * the control arm (classic_arm) encodes the same op on the frozen
 * shipping px_* body, honestly-optimally. The ONLY difference
 * between the arms is the ontology (vol. 6 §6.3) — which is the
 * entire experimental claim.
 *
 * Registered beats (each is asserted by the calibration suite):
 *   - op 13: a concurrent `stops` pair hard-forks under tier 1
 *   - op 15: an explicit merge-intent stitches the fork
 *   - op 17/18: concurrent `budget` pairs merge under tier 2
 *   - op 21: a rigid type inside a tier-2 region routes to fork
 *     ("no lattice" is an answer, not an error)
 *   - op 23: concurrent `tags` join by set union
 *   - op 25: a tier-3 concurrent pair arbitrates last-writer-wins
 *   - op 30: B's sub-budget overdrafts; the breaker trips; the
 *     receipt propagates with hop = 2 (the delegation chain depth)
 *   - op 35: undo by slice redefinition; op 36 extends the rewound
 *     point; op 37 re-addresses the abandoned branch
 *
 * Knowledge encoding: op.cut names the SCRIPT LINE whose resulting
 * log state the submitter has seen. SCEN_CUT_NOW = full knowledge.
 * There is no implicit global knowledge — concurrency is real
 * exactly where a source's declared knowledge is stale.
 *
 * sid assignment (both arms): H = 0, A = 1, B = 2, SYS = 3.
 */
#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>
#include <stddef.h>   /* NULL, for the op table below */

/* op kinds */
enum {
    SOP_REGION = 1,   /* value = ptype; note tier in `dst`          */
    SOP_DELEG  = 2,   /* src = delegator, dst = delegate, amount    */
    SOP_VALUE  = 3,   /* the world write                             */
    SOP_TIER   = 4,   /* value = tier (the S14 dial)                */
    SOP_QUERY  = 5,   /* value = script line to address (0 = now)   */
    SOP_PROV   = 6,   /* attribution walk                            */
    SOP_UNDO   = 7,   /* value = script line to rewind past         */
    SOP_MERGE  = 8,   /* value = head_a line, dst = head_b line     */
    SOP_WINDOW = 9,   /* value = window                              */
    SOP_EXPORT = 10   /* black-box export                            */
};

/* sources */
#define SCEN_SRC_H    0
#define SCEN_SRC_A    1
#define SCEN_SRC_B    2
#define SCEN_SRC_SYS  3

/* knowledge reference: full knowledge at submit time */
#define SCEN_CUT_NOW  0x7FFFFFFFu

/* payload type ids for the scenario's regions (the built-ins NUM
 * and SET; SEQ and STR register through the user channel — B2's
 * extension path, exercised on purpose) */
#define SCEN_T_SEQ    2   /* {u32 pos, u32 tag} ordered insert     */
#define SCEN_T_STR    3   /* 8-byte line                            */

/* packed SEQ insert: pos in the high word, tag in the low */
#define SCEN_INS(pos, tag) ((((uint64_t)(pos)) << 32) | (uint64_t)(tag))

typedef struct scen_op {
    uint8_t     kind;
    uint8_t     src;      /* who                                        */
    uint8_t     dst;      /* DELEG delegate sid / MERGE head_b line /
                           * REGION default tier                        */
    const char* region;   /* where (NULL for DELEG/UNDO)                */
    uint64_t    value;    /* what: kind-specific                        */
    uint32_t    cut;      /* knowledge: script line, or SCEN_CUT_NOW    */
    uint8_t     debit;    /* ifl_debit_class (0 = user's own action)    */
    uint8_t     sev;      /* ifl_severity rung                          */
    const char* note;     /* the shared beat, printed by BOTH arms      */
} scen_op;

/*
 * The script. Line numbers (1-based) are the op's index + 1 and are
 * load-bearing: `cut` fields reference them.
 */
static const scen_op scen_script[] = {
/*  1 */ { SOP_REGION, SCEN_SRC_H, 2, "budget", 0 /*NUM*/, SCEN_CUT_NOW, 0, 0,
           "H opens the trip plan: budget (bounded int, tier 2)" },
/*  2 */ { SOP_REGION, SCEN_SRC_H, 1, "stops",  SCEN_T_SEQ, SCEN_CUT_NOW, 0, 0,
           "H opens stops (ordered inserts, tier 1 default)" },
/*  3 */ { SOP_REGION, SCEN_SRC_H, 2, "tags",   1 /*SET*/, SCEN_CUT_NOW, 0, 0,
           "H opens tags (grow-only set, tier 2)" },
/*  4 */ { SOP_REGION, SCEN_SRC_H, 3, "title",  SCEN_T_STR, SCEN_CUT_NOW, 0, 0,
           "H opens title (single line, tier 3)" },
/*  5 */ { SOP_DELEG, SCEN_SRC_H, SCEN_SRC_A, NULL, 40, SCEN_CUT_NOW, 0, 0,
           "H delegates to A: book the trip (sub-budget 40)" },
/*  6 */ { SOP_DELEG, SCEN_SRC_A, SCEN_SRC_B, NULL, 15, SCEN_CUT_NOW, 0, 0,
           "A sub-delegates to B: verify prices (sub-budget 15)" },
/*  7 */ { SOP_VALUE, SCEN_SRC_H, 0, "budget", 1200, SCEN_CUT_NOW, 0, 0,
           "H sets the budget at 1200" },
/*  8 */ { SOP_VALUE, SCEN_SRC_H, 0, "stops", SCEN_INS(0, 1), SCEN_CUT_NOW, 0, 0,
           "H adds stop: hotel" },
/*  9 */ { SOP_VALUE, SCEN_SRC_H, 0, "tags", 0x1, SCEN_CUT_NOW, 0, 0,
           "H tags: beach" },
/* 10 */ { SOP_VALUE, SCEN_SRC_H, 0, "title", 0x53756D6D657220ULL, SCEN_CUT_NOW, 0, 0,
           "H titles it Summer Trip" },
/* 11 */ { SOP_VALUE, SCEN_SRC_A, 0, "budget", 1350, 10, 0, 0,
           "A, seeing the plan, raises the budget to 1350" },
/* 12 */ { SOP_VALUE, SCEN_SRC_A, 0, "stops", SCEN_INS(1, 2), 10, 0, 0,
           "A inserts stop: museum (after hotel)" },
/* 13 */ { SOP_VALUE, SCEN_SRC_B, 0, "stops", SCEN_INS(1, 3), 6, 0, 0,
           "B, from A's delegation-time knowledge, inserts cafe — CONCURRENT" },
/* 14 */ { SOP_VALUE, SCEN_SRC_H, 0, "stops", SCEN_INS(2, 4), SCEN_CUT_NOW, 0, 0,
           "H adds stop: park (on the selected chain)" },
/* 15 */ { SOP_MERGE, SCEN_SRC_H, 14, "stops", 12, SCEN_CUT_NOW, 0, 0,
           "H stitches the fork: hotel, cafe, park + museum" },
/* 16 */ { SOP_VALUE, SCEN_SRC_H, 0, "budget", 1500, SCEN_CUT_NOW, 0, 0,
           "H revises the budget to 1500" },
/* 17 */ { SOP_VALUE, SCEN_SRC_A, 0, "budget", 1400, 11, 0, 0,
           "A's stale-knowledge 1400 arrives — CONCURRENT with H's 1500" },
/* 18 */ { SOP_VALUE, SCEN_SRC_B, 0, "budget", 1600, 13, 0, 0,
           "B's 1600 also arrives — the concurrent triple joins under tier 2" },
/* 19 */ { SOP_TIER, SCEN_SRC_H, 0, "stops", 2 /*tier 2*/, SCEN_CUT_NOW, 0, 0,
           "H dials stops from tier 1 to tier 2 (S14)" },
/* 20 */ { SOP_VALUE, SCEN_SRC_H, 0, "stops", SCEN_INS(3, 5), SCEN_CUT_NOW, 0, 0,
           "H adds stop: bus" },
/* 21 */ { SOP_VALUE, SCEN_SRC_A, 0, "stops", SCEN_INS(4, 6), 18, 0, 0,
           "A's train insert is concurrent; SEQ is RIGID — routes to fork" },
/* 22 */ { SOP_VALUE, SCEN_SRC_H, 0, "tags", 0x2, SCEN_CUT_NOW, 0, 0,
           "H tags: food" },
/* 23 */ { SOP_VALUE, SCEN_SRC_A, 0, "tags", 0x4, 18, 0, 0,
           "A tags: relax — CONCURRENT, set union under tier 2" },
/* 24 */ { SOP_VALUE, SCEN_SRC_H, 0, "title", 0x53756D32303236ULL, SCEN_CUT_NOW, 0, 0,
           "H retitles: Summer 2026" },
/* 25 */ { SOP_VALUE, SCEN_SRC_A, 0, "title", 0x5472697000ULL, 18, 0, 0,
           "A's shorter title is concurrent — tier 3 arbitrates LWW" },
/* 26 */ { SOP_VALUE, SCEN_SRC_A, 0, "budget", 1700, 22, 1 /*INITIATIVE*/, 4 /*FG*/,
           "A flags a price change (surprise-class initiative)" },
/* 27 */ { SOP_VALUE, SCEN_SRC_B, 0, "budget", 1800, 22, 1, 2 /*AMBIENT*/,
           "B revises 1800 (ambient initiative)" },
/* 28 */ { SOP_VALUE, SCEN_SRC_B, 0, "budget", 1900, 22, 1, 4 /*FOREGROUND*/,
           "B revises 1900 (foreground)" },
/* 29 */ { SOP_VALUE, SCEN_SRC_B, 0, "budget", 2000, 22, 1, 8 /*MODAL*/,
           "B revises 2000 (modal)" },
/* 30 */ { SOP_VALUE, SCEN_SRC_B, 0, "budget", 2100, 22, 1, 8 /*MODAL*/,
           "B's next modal overdrafts the sub-budget — BREAKER + RECEIPT" },
/* 31 */ { SOP_QUERY, SCEN_SRC_H, 0, "budget", 17, 0, 0, 0,
           "H audits the budget as of beat 17" },
/* 32 */ { SOP_QUERY, SCEN_SRC_H, 0, "stops", 14, 0, 0, 0,
           "H audits the stops as of beat 14 (mid-fork)" },
/* 33 */ { SOP_QUERY, SCEN_SRC_H, 0, "tags", 9, 0, 0, 0,
           "H audits the tags as of beat 9" },
/* 34 */ { SOP_PROV, SCEN_SRC_H, 0, "budget", 0, 0, 0, 0,
           "H walks the budget's provenance" },
/* 35 */ { SOP_UNDO, SCEN_SRC_H, 0, NULL, 16, 0, 0, 0,
           "H undoes the 1500 revision (slice redefinition)" },
/* 36 */ { SOP_VALUE, SCEN_SRC_H, 0, "budget", 1450, SCEN_CUT_NOW, 0, 0,
           "H works at the rewound point: 1450" },
/* 37 */ { SOP_QUERY, SCEN_SRC_H, 0, "budget", 34, 0, 0, 0,
           "H re-addresses the abandoned branch (as of beat 34)" },
/* 38 */ { SOP_WINDOW, SCEN_SRC_H, 0, "stops", 8, 0, 0, 0,
           "H configures the black-box window on stops (S3)" },
/* 39 */ { SOP_EXPORT, SCEN_SRC_H, 0, "stops", 0, 0, 0, 0,
           "H exports the black box (S3)" },
/* 40 */ { SOP_VALUE, SCEN_SRC_H, 0, "tags", 0x8, SCEN_CUT_NOW, 0, 0,
           "H tags: family" },
/* 41 */ { SOP_VALUE, SCEN_SRC_A, 0, "stops", SCEN_INS(5, 7), SCEN_CUT_NOW, 0, 0,
           "A adds stop: airport (post-merge, causal)" },
/* 42 */ { SOP_QUERY, SCEN_SRC_H, 0, "stops", 0 /*now*/, 0, 0, 0,
           "H reads the stops at now" },
/* 43 */ { SOP_PROV, SCEN_SRC_H, 0, "stops", 0, 0, 0, 0,
           "H walks the stops' provenance (attribution audit)" },
/* 44 */ { SOP_VALUE, SCEN_SRC_H, 0, "title", 0x547269703220ULL, SCEN_CUT_NOW, 0, 0,
           "H retitles: Trip 2 (causal rewrite after the arbitration)" },
/* 45 */ { SOP_EXPORT, SCEN_SRC_H, 0, "tags", 0, 0, 0, 0,
           "H exports the tags black box" },
};

#define SCEN_N_OPS ((int)(sizeof(scen_script) / sizeof(scen_script[0])))

/* The registered names, shared by both arms' transcripts. */
#define SCEN_R_BUDGET "budget"
#define SCEN_R_STOPS  "stops"
#define SCEN_R_TAGS   "tags"
#define SCEN_R_TITLE  "title"

/* The registered delegation amounts (ADR-0025 4a). */
#define SCEN_A_SUB 40
#define SCEN_B_SUB 15

/* The system's initiative budget for the scenario run: enough for
 * the registered merges (3 x background) plus honest headroom — the
 * calibration suite's zero-budget world is a SEPARATE run (C4). */
#define SCEN_SYS_BUDGET 64

#endif /* SCENARIO_H */
