/*
 * ifl.h — the intent-flow core: the vertical slice's ontology (v0.1.0-slice)
 *
 * This is Program I's falsification instrument, registered by ADR-0025
 * before this file existed. It implements the research series' semantics
 * draft (vol. 5) at instrument scale:
 *
 *   N1  sources   — person / agent / process are the same type; the kind
 *                   field is metadata and NEVER dispatches (I1).
 *   N2  log+cuts  — the append-only intent log is the ONLY history
 *                   structure; every state is a per-query fold over a
 *                   causal cut. There is no undo primitive: undo is a
 *                   view move (I5).
 *   N3  tiers     — per-region, runtime, registered by logged tier
 *                   intents (constraint D1: no silent defaults);
 *                   tier-1 fork keeps both versions, tier-2 joins by
 *                   the registered semilattice, tier-3 arbitrates
 *                   last-writer-wins with fold-level exclusion of the
 *                   loser (the loser stays in the log — deletion would
 *                   weaken provenance, the stricter reading of vol. 5
 *                   §6.1; the ARBIT record witnesses every exclusion).
 *   N4  ledger    — one budget per source; INITIATIVE and SURPRISE
 *                   debits share one currency; overdraft trips the
 *                   breaker and yields a receipt, never a silent
 *                   failure (I2, I7); delegation opens a sub-ledger.
 *   PROV          — every record carries provenance; merges carry dual
 *                   derivation (I6).
 *
 * Zero dependencies, C17, integer-only (no float paths at all: the MSVC
 * C4244 class cannot exist here). Deterministic: the clock is a tick
 * counter, all iteration orders are array order, all arbiters are
 * total orders on (t, sid, iid).
 *
 * Registered capacities (ADR-0025 Decision 4a/4b): at most
 * IFL_MAX_SOURCES sources (the slice's three flows plus the system
 * source), 8-byte inline payloads, one value arena for oversized
 * payloads (region creation blobs), two fork heads per region.
 *
 * The seven invariants I1-I7 are enforced by slice/tests/; the rebound
 * signal-1 static gate is scripts/check_slice_rebound.sh.
 */
#ifndef PLANEX_IFL_H
#define PLANEX_IFL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * Version — slice verification artifact, does NOT bump Planex
 * ============================================================ */

#define IFL_VERSION_MAJOR 0
#define IFL_VERSION_MINOR 1
#define IFL_VERSION_PATCH 0
#define IFL_VERSION "0.1.0-slice"

/* ============================================================
 * Registered capacities (ADR-0025 Decision 4)
 * ============================================================ */

#define IFL_MAX_SOURCES   4   /* person + 2 agents + system (vol. 6 6.1) */
#define IFL_MAX_REGIONS   1024
#define IFL_MAX_PAYLOAD_TYPES 16
#define IFL_NAME_MAX      12  /* region names in the registered scenario */
#define IFL_SID_NULL      0xFFu

typedef uint8_t ifl_sid;   /* 0-based source index; IFL_SID_NULL = none */

/* ============================================================
 * N1 — sources (vol. 5 table 3-1: the six-field delegation table)
 *
 * kind is METADATA ONLY. No API path in this core branches on it
 * (that is I1's criterion; scripts/check_slice_rebound.sh enforces
 * the static half). A quiet system is the all-zero-budget
 * configuration (I2), not a mode.
 * ============================================================ */

typedef enum ifl_kind {
    IFL_KIND_PERSON  = 0,
    IFL_KIND_AGENT   = 1,
    IFL_KIND_PROCESS = 2
} ifl_kind;

typedef struct ifl_source {
    char     id[16];            /* stable identity */
    ifl_kind kind;              /* metadata, never dispatches (I1) */
    uint8_t  trust;             /* 0..3 — merge eligibility input (4.4) */
    uint32_t initiative_budget; /* N4 handle; 0 = quiet (I2) */
    uint32_t authority;         /* bitmask: may emit world payloads */
    ifl_sid  delegated_by;      /* IFL_SID_NULL = root, else delegator */
} ifl_source;

/* ============================================================
 * Causal blood — vector clocks (Fidge 1988 / Mattern 1989)
 * ============================================================ */

typedef struct ifl_vc {
    uint32_t c[IFL_MAX_SOURCES];
} ifl_vc;

/* vc algebra: pointwise max, partial order, concurrency test. */
ifl_vc ifl_vc_join(ifl_vc a, ifl_vc b);      /* pointwise max */
bool   ifl_vc_le(ifl_vc a, ifl_vc b);        /* a happened-before b */
bool   ifl_vc_concurrent(ifl_vc a, ifl_vc b);/* incomparable both ways */

/* ============================================================
 * Payloads — the open type registry (vol. 5 3.3: B2's ban on
 * closed input alphabets). A type is a value width plus an
 * OPTIONAL join. "No join exists" is a first-class answer
 * (rigid types route to tiers 1/3), never an error.
 *
 * Contract for sq, when present:
 *   sq(a, b) == sq(b, a)                     (commutative)
 *   sq(sq(a,b),c) == sq(a,sq(b,c))           (associative)
 *   sq(a, a) == a                            (idempotent)
 *   le(a, sq(a, b))                          (inflationary — vol. 5
 *                                             4.3 condition two)
 * Registration runs a fast structural validation; the full law
 * battery is slice/tests/test_slice_property.c (C6), including a
 * synthetic non-associative sq that the registry MUST reject.
 * ============================================================ */

typedef uint8_t ifl_ptype;

/* Built-in families (world payloads + structural records). */
enum {
    IFL_T_NUM    = 0,  /* u32 bounded value, join = max          (tier 2) */
    IFL_T_SET    = 1,  /* u64 tag set,     join = union          (tier 2) */
    IFL_T_SEQ    = 2,  /* {u32 pos, u32 tag} ordered insert — rigid       */
    IFL_T_STR    = 3,  /* 8-byte line — rigid                          */
    IFL_T_REGION = 4,  /* region creation (batch blob via arena)        */
    IFL_T_TIER   = 5,  /* tier dial move (S14)                         */
    IFL_T_DELEG  = 6,  /* delegation open (S22): sub-budget + delegate  */
    IFL_T_RECEIPT= 7,  /* breaker/receipt event (T7)                   */
    IFL_T_CURSOR = 8,  /* view move: undo/branch addressing (N2 sugar)  */
    IFL_T_EXPORT = 9,  /* black-box export (S3)                        */
    IFL_T_ARBIT  = 10, /* tier-3 arbitration witness                    */
    IFL_T_MERGE  = 11, /* explicit tier-1 merge-intent                  */
    IFL_T_USER   = 12  /* registered extension types start here        */
};

typedef struct ifl_payload_type {
    const char* name;
    uint8_t     value_size;            /* 1..8 inline (registered) */
    bool        (*sq)(uint64_t a, uint64_t b, uint64_t* out);
    bool        (*le)(uint64_t a, uint64_t b);   /* optional order */
    uint32_t    authority_bit;         /* required authority mask bit */
} ifl_payload_type;

/* Forward: the world (full definition in ifl_internal.h). */
typedef struct ifl_world ifl_world;

/* Arena-backed payload helper: stores an oversized value and returns
 * the packed {offset, length} encoding that ifl_submit accepts for
 * arena-backed types (value_size == 0 declares a blob type). */
uint64_t ifl_payload_blob(ifl_world* w, const void* data, uint32_t len);

/* Register through the SAME channel the built-ins used. Returns the
 * type id or (uint8_t)-1 on failure (duplicate name / bad contract /
 * registry full). The built-ins NUM and SET are registered at world
 * creation; the scenario's SEQ/STR register through this entry as
 * user types, exercising the extension path. */
uint8_t ifl_register_payload_type(ifl_world* w, const ifl_payload_type* t);

/* Read side of the registry: the type's registration (name, width,
 * join, order) — the SAME function pointers the adjudicator
 * dereferences. NULL for an unregistered id. */
const ifl_payload_type* ifl_type_info(ifl_world* w, uint8_t ptype);

/* ============================================================
 * The intent record — seven slots (vol. 5 table 3-2)
 *
 * One 44-byte record; the intent id (iid) is the 1-based log index,
 * implicit in position (registered in ADR-0025 Decision 4f). The
 * value is inline u32[2]; oversized payloads (region creation blobs)
 * live in the arena with value[0] = arena offset, value[1] = length.
 *
 * flags (adjudication state, not content — every flip is witnessed
 * by a structural record in the log, per ADR-0025 Decision 4c):
 *   IFL_F_APPLIED      this intent is in the fold
 *   IFL_F_CONCUR_MERGE this application joined a concurrent sibling
 *                      (prov_a/prov_b = dual derivation, I6)
 *   IFL_F_EXCLUDED     tier-3 loser: in the log, out of every fold
 * ============================================================ */

enum {
    IFL_F_APPLIED      = 0x01u,
    IFL_F_CONCUR_MERGE = 0x02u,
    IFL_F_EXCLUDED     = 0x04u,
    IFL_F_REBORN       = 0x08u  /* submitted after its own undo, on
                                 * re-based living knowledge — see
                                 * ifl_undo_nth */
};

typedef struct ifl_intent {
    ifl_vc   vc;          /* slot 3: causal coordinate */
    uint32_t value[2];    /* slot 5: payload (inline or arena ref) */
    uint32_t prov_a;      /* slot 6: derived-from (merge: first parent) */
    uint32_t prov_b;      /* slot 6: derived-from (merge: second parent) */
    uint32_t t;           /* slot 4: physical time (slice: tick) */
    uint32_t aux;         /* structural second word: ARBIT loser_iid,
                           * CURSOR previous cut, MERGE policy tag */
    uint16_t debit;       /* slot 7a: posted budget debit */
    uint16_t region;      /* addressing: region id */
    uint8_t  src;         /* slot 2: source sid */
    uint8_t  ptype;       /* payload family (open registry) */
    uint8_t  flags;       /* adjudication state */
    uint8_t  commit;      /* slot 7b: commitment strategy */
} ifl_intent;

/* 48 bytes exactly — the registered E2 residency accounting unit
 * (ADR-0025 Decision 4f: "intent records at 48 bytes"). No padding:
 * every field is load-bearing; `aux` carries each structural
 * family's second data word. */
_Static_assert(sizeof(ifl_intent) == 48, "ifl_intent density is registered (ADR-0025 4f)");

/* ============================================================
 * N3 — tiers (vol. 5 ch. 6). Per-region, runtime, logged.
 * ============================================================ */

typedef enum ifl_tier {
    IFL_TIER_FORK    = 1,   /* hard fork: both versions live       */
    IFL_TIER_MERGE   = 2,   /* tolerant merge: the registered sq   */
    IFL_TIER_STRICT  = 3    /* strict convergence: arbiter LWW     */
} ifl_tier;

/* ============================================================
 * N4 — the ledger (vol. 5 ch. 7). One per source.
 *
 * The registered slice pricing ladder (ADR-0025 Decision 4e — S16's
 * spectrum priced in integers; values are E4's to measure, the
 * monotonicity is ours to keep and is tested):
 *   background 1 < ambient 2 < foreground 4 < modal 8
 * ============================================================ */

typedef enum ifl_severity {
    IFL_SEV_BACKGROUND  = 1,
    IFL_SEV_AMBIENT     = 2,
    IFL_SEV_FOREGROUND  = 4,
    IFL_SEV_MODAL       = 8
} ifl_severity;

typedef enum ifl_debit_class {
    IFL_DEBIT_NONE       = 0,  /* the user's own action: no debit */
    IFL_DEBIT_INITIATIVE = 1,  /* system/agent initiative (B4)    */
    IFL_DEBIT_SURPRISE   = 2   /* prior violation (B5)            */
} ifl_debit_class;

#define IFL_BUDGET_UNLIMITED 0xFFFFFFFFu

typedef struct ifl_ledger {
    uint32_t budget;          /* own ceiling; 0 = quiet (I2) */
    uint32_t spent_initiative;
    uint32_t spent_surprise;
    uint32_t sub_ceiling;     /* received via delegation (the
                              * delegate's own spending ceiling) */
    uint32_t carved;          /* total carved out to delegates
                              * (subtracts from THIS ledger's
                              * available balance) */
    bool     breaker_tripped;
} ifl_ledger;

/* Receipt (vol. 5 table 7-1): who / why / cost / rollback / checkpoint */
typedef struct ifl_receipt {
    uint8_t  origin;          /* who (sid) */
    uint8_t  hop;             /* delegation depth of the origin */
    uint32_t reason_iid;      /* why (PROV activity reference) */
    uint32_t cost_initiative; /* itemized: initiative units */
    uint32_t cost_surprise;   /* itemized: surprise units */
    uint32_t rollback_cut;    /* undo-to address (causal cut) */
    uint32_t checkpoint;      /* commitment snapshot (cursor id) */
} ifl_receipt;

/* ============================================================
 * Regions — the confluence points (N1) and tier scope (N3)
 * ============================================================ */

typedef struct ifl_region {
    char     name[IFL_NAME_MAX];
    uint8_t  ptype;
    uint8_t  tier;
    uint16_t window;          /* S3 rolling-window parameter (0 = full) */
    uint32_t arena_off;       /* current value's arena slot */
    uint32_t arena_len;
    uint64_t cache;           /* current folded value (working set) */
    uint32_t last_iid;        /* last applied intent */
    uint32_t head_a;          /* fork head A (iid) — tier 1 */
    uint32_t head_b;          /* fork head B (iid) — 0 = no fork */
} ifl_region;

/* ============================================================
 * Views — time addresses (N2). A view is a causal cut plus, per
 * forked region, a head selection. "now" is just the interface's
 * current view; undo/branch/history-panel are three skins of the
 * same query (vol. 5 5.2). No view field ever enters the fold.
 * ============================================================ */

#define IFL_MAX_SELECTIONS 8

typedef struct ifl_selection {
    uint16_t rid;
    uint32_t head_iid;   /* which fork head this view addresses */
} ifl_selection;

typedef struct ifl_view {
    uint32_t cut;                          /* max iid in the fold */
    uint32_t n_excl;                       /* undone subtree roots */
    uint32_t excl[IFL_MAX_SELECTIONS];     /* undo = drop iid + its
                                              causal descendants */
    uint32_t n_sel;
    ifl_selection sel[IFL_MAX_SELECTIONS]; /* per-fork head choices */
} ifl_view;

/* ============================================================
 * Adjudication statistics (vol. 5 4.2's four steps, counted —
 * I1/I3's test surface)
 * ============================================================ */

typedef struct ifl_stats {
    uint32_t submits;            /* every logged record */
    uint32_t causal_applies;     /* step-1 exits (causal fold) */
    uint32_t concurrent_arrivals;/* arrivals concurrent with a sibling */
    uint32_t lattice_checks;     /* step-2: sq consulted */
    uint32_t tier2_merges;       /* step-2 outcome: sq joins executed */
    uint32_t tier1_forks;        /* step-3 outcome: forks opened */
    uint32_t tier3_arbitrations; /* step-3 outcome: LWW arbitrations */
    uint32_t system_merge_debits;/* N4: merges the system paid for */
    uint32_t breakers;           /* ledger overdrafts */
    uint32_t receipts;           /* receipt records */
    uint32_t excluded;           /* tier-3 losers kept-but-excluded */
    uint32_t merge_intents;      /* explicit tier-1 stitches */
    uint32_t cursor_moves;       /* view moves (undo/branch) */
    uint32_t denied_authority;   /* authority conflicts (4.4) */
} ifl_stats;

/* ============================================================
 * The world
 * ============================================================ */

ifl_world* ifl_world_create(void);
void       ifl_world_free(ifl_world* w);

/* Declare which source pays for system initiative (tier-2 merges,
 * ambient notices). Explicit configuration — the core NEVER derives
 * this from `kind`, which is metadata and never dispatches (I1);
 * pass IFL_SID_NULL for a world where no one can pay, so no system
 * initiative happens (the general form of I2's zero-budget world). */
void       ifl_set_system_source(ifl_world* w, ifl_sid sid);

/* --- sources ------------------------------------------------ */

ifl_sid ifl_add_source(ifl_world* w, const ifl_source* s);
const ifl_source* ifl_source_info(ifl_world* w, ifl_sid sid);
ifl_ledger* ifl_ledger_of(ifl_world* w, ifl_sid sid); /* null: bad sid */

/* --- regions ------------------------------------------------ */

/* Region creation is a logged, sourced intent (ptype IFL_T_REGION).
 * tier is the REGISTERED default — itself an N3 decision on the
 * record (constraint D1), emitted as a tier intent immediately
 * after creation. `blob`/`len` (optional, arena-backed) seeds the
 * initial value. */
uint16_t ifl_region_create(ifl_world* w, ifl_sid src, const char* name,
                           uint8_t ptype, ifl_tier tier,
                           const void* blob, uint32_t len);

/* Batch form — the document-load intent: ONE log record plus one
 * arena blob carrying every region's seed. The E2 engineering leg
 * materializes its 1000-object load this way (ADR-0025 4f); the
 * per-region form remains the rich-PROV path for scenario-scale
 * creation. Returns the number of regions created. */
typedef struct ifl_region_spec {
    char     name[IFL_NAME_MAX];
    uint8_t  ptype;
    uint8_t  tier;
    uint64_t seed;
} ifl_region_spec;

uint32_t ifl_region_create_batch(ifl_world* w, ifl_sid src,
                                 const ifl_region_spec* specs, uint32_t n);

uint16_t ifl_region_find(ifl_world* w, const char* name);
ifl_region* ifl_region_info(ifl_world* w, uint16_t rid);

/* The tier dial (S14): a logged, sourced, undoable tier move. */
bool ifl_region_set_tier(ifl_world* w, ifl_sid src, uint16_t rid, ifl_tier tier);

/* --- the main entry: submit an intent ----------------------- */

typedef struct ifl_submit_opts {
    /* Knowledge: "I have already seen everything <= observed_cut".
     * 0 = blind emit (maximally concurrent). Knowledge flows only
     * through explicit channels (delegation, receipts, queries) —
     * there is no implicit global knowledge, or concurrency would
     * be erased by fiat. */
    uint32_t       observed_cut;
    ifl_debit_class debit_class;  /* agent/system initiative pays */
    ifl_severity   severity;      /* the S16 ladder rung */
    uint8_t        commit;        /* commitment strategy tag */
} ifl_submit_opts;

/* Returns the new iid, or 0 on hard failure (bad sid/rid/ptype).
 * Budget overdraft does NOT fail the call: the submission is logged
 * (the black box records the attempt), the intent is left unapplied,
 * the breaker trips, and a receipt is emitted (vol. 5 7.1). */
uint32_t ifl_submit(ifl_world* w, ifl_sid src, uint16_t rid,
                    uint8_t ptype, uint64_t value,
                    const ifl_submit_opts* opts);

/* Convenience for the common no-debit user action. */
uint32_t ifl_user_act(ifl_world* w, ifl_sid src, uint16_t rid,
                      uint8_t ptype, uint64_t value);

/* --- delegation (S22 / T7) ----------------------------------- */

/* Opens a sub-ledger: the delegate's ceiling is carved from the
 * delegator's REMAINING balance; the delegation itself is a logged
 * intent carrying the delegator's current knowledge (the world as
 * the delegator knows it), which is how causal knowledge propagates
 * down a chain. Returns false when the delegator cannot afford it. */
bool ifl_delegate(ifl_world* w, ifl_sid delegator, ifl_sid delegate,
                  uint32_t amount);

/* --- N4 primitives (exposed for the suites) ------------------ */

/* Posts a debit; on overdraft trips the breaker, emits a receipt,
 * and returns false (the caller must not apply its effect). */
bool ifl_debit(ifl_world* w, ifl_sid sid, ifl_debit_class c,
               ifl_severity sev, uint32_t reason_iid);
uint32_t ifl_receipt_count(ifl_world* w);
const ifl_receipt* ifl_receipt_at(ifl_world* w, uint32_t i);

/* --- queries: the fold (N2) ---------------------------------- */

/* The world's "now" view (the interface's cursor). */
ifl_view ifl_now(ifl_world* w);

/* Region value at a view: the per-query fold. For semilattice
 * regions the fold is the sq-join of every applied intent <= cut;
 * for rigid regions it is the last applied, non-excluded intent
 * <= cut on the selected head chain. Sets *contrib to the count of
 * contributing intents (I6's audit surface) when non-null. */
uint64_t ifl_state_at(ifl_world* w, uint16_t rid, const ifl_view* v,
                      uint32_t* contrib);

/* PROV walk (I6): calls cb for every contributing intent of the
 * region's current value at the view, newest first. */
typedef void (*ifl_prov_cb)(const ifl_intent* rec, void* user);
void ifl_prov_walk(ifl_world* w, uint16_t rid, const ifl_view* v,
                   ifl_prov_cb cb, void* user);

/* --- undo is sugar (N2 / I5) --------------------------------- */

/* Rewinds the world's now-view past iid: the new cut drops iid and
 * everything causally after it, fork heads reselect to the latest
 * surviving heads, and a CURSOR intent records who moved "now" and
 * why (I6). This function is THE sugar: behavioral identity with
 * the pure projection rewrite is enforced by test_slice_calibration
 * (C7). Returns the new cut. */
uint32_t ifl_undo_nth(ifl_world* w, ifl_sid src, uint32_t iid);

/* Pure-projection twin (no sugar): computes the rewound view for
 * the same iid. The differential test drives both and compares. */
ifl_view ifl_view_rewind(ifl_world* w, const ifl_view* v, uint32_t iid);

/* The third manual step of the pure-projection undo path: adopt
 * `v` as the world's "now" (cut clamped to the log), and re-base
 * the undoing sources' service clocks on the adopted view's
 * living knowledge — so their NEXT submissions extend the
 * rewound point, not the abandoned branch. `ifl_undo_nth` is
 * exactly { view_rewind + a CURSOR submission + view_adopt };
 * test_slice_calibration's C7 drives both paths and demands they
 * stay byte-identical — the sugar may do NOTHING the manual three
 * steps do not. */
bool ifl_view_adopt(ifl_world* w, const ifl_view* v);

/* Branch = select a different successor set: after a rewind, new
 * submits extend the rewound point; the abandoned branch remains
 * addressable through explicit views (see ifl_view_at). */
ifl_view ifl_view_at(ifl_world* w, uint32_t cut);

/* --- tier-1 stitching (explicit merge-intent) ----------------- */

/* payload: {head_a_iid, head_b_iid}; value: the stitched payload.
 * The merge-intent itself runs the full adjudication flow (vol. 5
 * 6.1 tier 1) and collapses the fork to a single head with dual
 * provenance. */
uint32_t ifl_merge_intent(ifl_world* w, ifl_sid src, uint16_t rid,
                          uint32_t head_a, uint32_t head_b,
                          uint64_t stitched);

/* --- black box (S3) ------------------------------------------ */

/* Window is a per-region N2 parameter (vol. 5 5.4), NOT a global
 * constant. The slice retains the full log (the E2 leg's registered
 * measurement mode); the window bounds what an EXPORT serializes. */
void ifl_blackbox_configure(ifl_world* w, uint16_t rid, uint16_t window);

/* Serializes the window's records into buf (<= cap) and logs an
 * EXPORT intent carrying who/when/what-range (PROV). Returns the
 * serialized byte count. */
uint32_t ifl_blackbox_export(ifl_world* w, ifl_sid src, uint16_t rid,
                             char* buf, uint32_t cap);

/* --- projections (semantic-tree-first, ADR-0025 4g) ----------- */

/* THE primary projection: the semantic structure at a view —
 * regions, values, tiers, contributing sources. Writes a stable,
 * deterministic text form (the slice's interface IS this query
 * surface; visual/a11y/generative are same-rank future projections,
 * none privileged above it). Returns bytes written. */
uint32_t ifl_project_semantic(ifl_world* w, const ifl_view* v,
                              char* buf, uint32_t cap);

/* Dual address (S9, type-level per vol. 5 5.3): the set-state
 * projection groups by semantic proximity and ignores the total
 * order; the history projection is the strict causal expansion. */
uint32_t ifl_project_set(ifl_world* w, const ifl_view* v,
                         char* buf, uint32_t cap);
uint32_t ifl_project_history(ifl_world* w, const ifl_view* v,
                             char* buf, uint32_t cap);

/* --- instrumentation (E2 / I7) -------------------------------- */

const ifl_stats* ifl_stats_of(ifl_world* w);
const ifl_intent* ifl_log_at(ifl_world* w, uint32_t iid); /* 1-based */
uint32_t ifl_log_count(ifl_world* w);

/* E2 residency accounting: every byte retained solely because of
 * N2 — intent records, the arena, receipts — NOT the region table
 * (that is the working set). Deterministic by construction, so the
 * published numbers are recomputable on any checkout. */
uint64_t ifl_history_bytes(ifl_world* w);
uint64_t ifl_working_set_bytes(ifl_world* w);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_IFL_H */
