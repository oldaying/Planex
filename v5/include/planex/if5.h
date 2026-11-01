/*
 * if5.h — the intent-flow core, library grade: the v5 derivation
 * (v0.1.0-v5, ADR-0026).
 *
 * Lineage: this API is the PROMOTION of the slice's ifl core
 * (slice/, ADR-0025) to library grade, per ADR-0026 Decision 3a.
 * The semantics are unchanged by contract: test_v5_differential
 * drives this core and the frozen slice in lockstep and demands
 * byte-identical behavior on the registered observation surface
 * (states at views, the three projections, black-box exports,
 * stats, receipts, log counts, history bytes). v5 may EXTEND the
 * slice, never ALTER it — that is the rewrite's master discipline.
 *
 * What v5 adds over the slice (ADR-0026 Decision 3b-3e):
 *   if5_cut    — vol. 5 5.1's causal slice as a first-class value:
 *                a validated address algebra (principal cuts, the
 *                prune constructor, meet/join, the subset order) and
 *                a world-level address registry. Discharges ADR-0025's
 *                registered Phase-4-scale known issue at prefix+prune
 *                granularity.
 *   if5_tree   — S15 seated: the semantic tree, first class. A
 *                structured, deterministic, total walk over region
 *                containment — name, value, role, tier, contributions,
 *                PROV anchor per node; no visual field exists
 *                (statically gated). Vision is one projection; the
 *                tree is the work.
 *   if5_schedule — S17 seated: the surprise timetable, as a PURE
 *                QUERY over records the core already logs (N5's
 *                no-new-slots discipline). Onboarding, diffs,
 *                changelogs and AI explanations are projections of
 *                this one object.
 *   S24        — the two-layer boundary (consumable projections over
 *                the durable intent flow), enforced as a purity
 *                invariant by test_v5_seats, not asserted here.
 *
 * The one record-format delta over the slice (ADR-0026 Decision 3d):
 * the debit word packs the debit CLASS in bits 12-13 alongside the
 * severity rung in bits 0-3. No registered observation surface reads
 * the word (the black-box export, all projections, receipts and
 * stats do not print it — verified by the differential); the packing
 * is what makes "when was this user surprised" answerable from the
 * black box alone.
 *
 * N1-N5 as in the slice (ifl.h's documentation carries over in
 * full): sources whose kind is metadata and never dispatches; the
 * append-only log as the only history structure; per-region runtime
 * tiers; one attention ledger per source with receipted overdraft;
 * PROV on every record. Zero dependencies, C17, integer-only,
 * deterministic.
 */
#ifndef PLANEX_IF5_H
#define PLANEX_IF5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * Version — derivation artifact, does NOT bump Planex
 * ============================================================ */

#define IF5_VERSION_MAJOR 0
#define IF5_VERSION_MINOR 1
#define IF5_VERSION_PATCH 0
#define IF5_VERSION "0.1.0-v5"

/* ============================================================
 * Registered capacities (promoted from ADR-0025 Decision 4;
 * ADR-0026 Decision 3 adds the cut registry)
 * ============================================================ */

#define IF5_MAX_SOURCES   4   /* person + 2 agents + system (vol. 6 6.1) */
#define IF5_MAX_REGIONS   1024
#define IF5_MAX_PAYLOAD_TYPES 16
#define IF5_NAME_MAX      12  /* region names in the registered scenario */
#define IF5_SID_NULL      0xFFu
#define IF5_MAX_CUTS      64  /* named global addresses (ADR-0026 3b) */

typedef uint8_t if5_sid;   /* 0-based source index; IF5_SID_NULL = none */

/* ============================================================
 * N1 — sources (vol. 5 table 3-1). kind is METADATA ONLY; no API
 * path in this core branches on it (I1;
 * scripts/check_slice_rebound.sh's v5 section enforces the static
 * half, plus the S15 visual-field ban).
 * ============================================================ */

typedef enum if5_kind {
    IF5_KIND_PERSON  = 0,
    IF5_KIND_AGENT   = 1,
    IF5_KIND_PROCESS = 2
} if5_kind;

typedef struct if5_source {
    char     id[16];            /* stable identity */
    if5_kind kind;              /* metadata, never dispatches (I1) */
    uint8_t  trust;             /* 0..3 — merge eligibility input (4.4) */
    uint32_t initiative_budget; /* N4 handle; 0 = quiet (I2) */
    uint32_t authority;         /* bitmask: may emit world payloads */
    if5_sid  delegated_by;      /* IF5_SID_NULL = root, else delegator */
} if5_source;

/* ============================================================
 * Causal blood — vector clocks (Fidge 1988 / Mattern 1989)
 * ============================================================ */

typedef struct if5_vc {
    uint32_t c[IF5_MAX_SOURCES];
} if5_vc;

if5_vc if5_vc_join(if5_vc a, if5_vc b);      /* pointwise max */
bool   if5_vc_le(if5_vc a, if5_vc b);        /* a happened-before b */
bool   if5_vc_concurrent(if5_vc a, if5_vc b);/* incomparable both ways */

/* ============================================================
 * Payloads — the open type registry (vol. 5 3.3: B2's ban on
 * closed input alphabets), promoted unchanged.
 * ============================================================ */

typedef uint8_t if5_ptype;

enum {
    IF5_T_NUM    = 0,  /* u32 bounded value, join = max          (tier 2) */
    IF5_T_SET    = 1,  /* u64 tag set,     join = union          (tier 2) */
    IF5_T_SEQ    = 2,  /* {u32 pos, u32 tag} ordered insert — rigid       */
    IF5_T_STR    = 3,  /* 8-byte line — rigid                          */
    IF5_T_REGION = 4,  /* region creation (batch blob via arena)        */
    IF5_T_TIER   = 5,  /* tier dial move (S14)                         */
    IF5_T_DELEG  = 6,  /* delegation open (S22): sub-budget + delegate  */
    IF5_T_RECEIPT= 7,  /* breaker/receipt event (T7)                   */
    IF5_T_CURSOR = 8,  /* view move: undo/branch addressing (N2 sugar)  */
    IF5_T_EXPORT = 9,  /* black-box export (S3)                        */
    IF5_T_ARBIT  = 10, /* tier-3 arbitration witness                    */
    IF5_T_MERGE  = 11, /* explicit tier-1 merge-intent                  */
    /* ADR-0027 Decision 3/2g: the two mechanism-layer structural
     * families. Both are REGION-SCOPED structural records; both
     * are excluded from the value fold (if5__is_value_type);
     * neither existed in any R1 stream, so every historical byte
     * is unchanged. The user-extension start moves 12 -> 14,
     * once, here, in a tree with no external users (ADR-0027
     * Decision 3's surface-list amendment). */
    IF5_T_REPARENT = 12, /* containment move (S15 content; ADR-0027 3)  */
    IF5_T_PROCESS  = 13, /* interaction-process opening (ADR-0027 2g)   */
    IF5_T_USER   = 14  /* registered extension types start here        */
};

typedef struct if5_payload_type {
    const char* name;
    uint8_t     value_size;            /* 1..8 inline (registered) */
    bool        (*sq)(uint64_t a, uint64_t b, uint64_t* out);
    bool        (*le)(uint64_t a, uint64_t b);   /* optional order */
    uint32_t    authority_bit;         /* required authority mask bit */
} if5_payload_type;

typedef struct if5_world if5_world;

uint64_t if5_payload_blob(if5_world* w, const void* data, uint32_t len);
uint8_t  if5_register_payload_type(if5_world* w, const if5_payload_type* t);
const if5_payload_type* if5_type_info(if5_world* w, uint8_t ptype);

/* ============================================================
 * The intent record — seven slots (vol. 5 table 3-2), promoted
 * byte-for-byte: 48 bytes exactly, the registered E2 residency
 * accounting unit (ADR-0025 Decision 4f). The debit word's class
 * packing is the ONE delta over the slice (see file header).
 * ============================================================ */

enum {
    IF5_F_APPLIED      = 0x01u,
    IF5_F_CONCUR_MERGE = 0x02u,
    IF5_F_EXCLUDED     = 0x04u,
    IF5_F_REBORN       = 0x08u
};

typedef struct if5_intent {
    if5_vc   vc;          /* slot 3: causal coordinate */
    uint32_t value[2];    /* slot 5: payload (inline or arena ref) */
    uint32_t prov_a;      /* slot 6: derived-from (merge: first parent) */
    uint32_t prov_b;      /* slot 6: derived-from (merge: second parent) */
    uint32_t t;           /* slot 4: physical time (slice: tick) */
    uint32_t aux;         /* structural second word: ARBIT loser_iid,
                           * CURSOR previous cut, MERGE policy tag */
    uint16_t debit;       /* slot 7a: posted debit — severity rung in
                           * bits 0-3, debit CLASS in bits 12-13
                           * (ADR-0026 3d; the slice carried the
                           * rung only) */
    uint16_t region;      /* addressing: region id */
    uint8_t  src;         /* slot 2: source sid */
    uint8_t  ptype;       /* payload family (open registry) */
    uint8_t  flags;       /* adjudication state */
    uint8_t  commit;      /* slot 7b: commitment strategy */
} if5_intent;

_Static_assert(sizeof(if5_intent) == 48, "if5_intent density is registered (ADR-0025 4f; ADR-0026 V8)");

/* The debit word's accessors (the class packing of ADR-0026 3d). */
#define IF5_DEBIT_SEV(word)   ((uint16_t)((word) & 0x0FFFu))
#define IF5_DEBIT_CLASS(word) ((uint8_t)(((word) >> 12) & 0x3u))

/* ============================================================
 * N3 — tiers (vol. 5 ch. 6). Per-region, runtime, logged.
 * ============================================================ */

typedef enum if5_tier {
    IF5_TIER_FORK    = 1,   /* hard fork: both versions live       */
    IF5_TIER_MERGE   = 2,   /* tolerant merge: the registered sq   */
    IF5_TIER_STRICT  = 3    /* strict convergence: arbiter LWW     */
} if5_tier;

/* ============================================================
 * N4 — the ledger (vol. 5 ch. 7). One per source. The pricing
 * ladder is the registered slice default (ADR-0025 4e): the
 * VALUES are E4's to measure, the MONOTONICITY is ours.
 * ============================================================ */

typedef enum if5_severity {
    IF5_SEV_BACKGROUND  = 1,
    IF5_SEV_AMBIENT     = 2,
    IF5_SEV_FOREGROUND  = 4,
    IF5_SEV_MODAL       = 8
} if5_severity;

typedef enum if5_debit_class {
    IF5_DEBIT_NONE       = 0,  /* the user's own action: no debit */
    IF5_DEBIT_INITIATIVE = 1,  /* system/agent initiative (B4)    */
    IF5_DEBIT_SURPRISE   = 2   /* prior violation (B5)            */
} if5_debit_class;

#define IF5_BUDGET_UNLIMITED 0xFFFFFFFFu

typedef struct if5_ledger {
    uint32_t budget;          /* own ceiling; 0 = quiet (I2) */
    uint32_t spent_initiative;
    uint32_t spent_surprise;
    uint32_t sub_ceiling;     /* received via delegation */
    uint32_t carved;          /* total carved out to delegates */
    bool     breaker_tripped;
} if5_ledger;

typedef struct if5_receipt {
    uint8_t  origin;          /* who (sid) */
    uint8_t  hop;             /* delegation depth of the origin */
    uint32_t reason_iid;      /* why (PROV activity reference) */
    uint32_t cost_initiative; /* itemized: initiative units */
    uint32_t cost_surprise;   /* itemized: surprise units */
    uint32_t rollback_cut;    /* undo-to address (causal cut) */
    uint32_t checkpoint;      /* commitment snapshot (cursor id) */
} if5_receipt;

/* ============================================================
 * Regions — the confluence points (N1), tier scope (N3), and
 * semantic-tree containment (S15, ADR-0026 3c). `parent` is the
 * ONE structural addition over the slice's region: 0 = root
 * level (the flat, slice-compatible shape the differential
 * drives); otherwise the containing region's id.
 * ============================================================ */

typedef struct if5_region {
    char     name[IF5_NAME_MAX];
    uint8_t  ptype;
    uint8_t  tier;
    uint16_t window;          /* S3 rolling-window parameter (0 = full) */
    uint16_t parent;          /* S15 containment: 0 = root level */
    uint32_t arena_off;       /* current value's arena slot */
    uint32_t arena_len;
    uint64_t cache;           /* current folded value (working set) */
    uint32_t last_iid;        /* last applied intent */
    uint32_t head_a;          /* fork head A (iid) — tier 1 */
    uint32_t head_b;          /* fork head B (iid) — 0 = no fork */
} if5_region;

/* ============================================================
 * Views — time addresses (N2). A view is a causal cut plus, per
 * forked region, a head selection. "now" is just the interface's
 * current view (vol. 5 5.2). Promoted unchanged.
 * ============================================================ */

#define IF5_MAX_SELECTIONS 8

typedef struct if5_selection {
    uint16_t rid;
    uint32_t head_iid;   /* which fork head this view addresses */
} if5_selection;

typedef struct if5_view {
    uint32_t cut;                          /* max iid in the fold */
    uint32_t n_excl;                       /* undone subtree roots */
    uint32_t excl[IF5_MAX_SELECTIONS];     /* undo = drop iid + its
                                              causal descendants */
    uint32_t n_sel;
    if5_selection sel[IF5_MAX_SELECTIONS]; /* per-fork head choices */
} if5_view;

/* ============================================================
 * N2, first class — the cut algebra (vol. 5 5.1, materialized
 * per ADR-0026 Decision 3b)
 *
 * An if5_cut ADDRESSES the represented set
 *     { iota_i : i <= prefix }  minus  the causal up-sets of the
 * prune roots — a down-closed subset of the log by construction:
 * a log prefix is down-closed (causal-before implies earlier
 * submission in this core: knowledge only flows through channels
 * bounded by the log length), and removing a causal up-set from a
 * down-closed set leaves a down-closed set. if5_cut_is_down_closed
 * is the public structural verifier; every constructor below
 * preserves the invariant provably, and the test suite checks it
 * on every algebra output anyway.
 *
 * `frontier` is the address's cached knowledge summary: the
 * componentwise join of the vcs of the records in the represented
 * set (computed under the same living-knowledge rule as the core's
 * service clocks — dead records do not contribute knowledge).
 *
 * Scope honesty (ADR-0026 Known issue #1): cuts here are PREFIX
 * bounds plus prunes. The per-source cut ("the world as source A
 * has seen it") is NOT expressible — the REBORN clock collision
 * makes clock comparison unsound as a membership test, and sound
 * per-source cuts are R2-scale semantics with their own
 * registration.
 * ============================================================ */

typedef struct if5_cut {
    if5_vc   frontier;                    /* cached knowledge join */
    uint32_t prefix;                      /* log prefix bound */
    uint32_t prunes[IF5_MAX_SELECTIONS];  /* undo roots (iids) */
    uint32_t n_prunes;
} if5_cut;

/* The principal cut at a prefix bound (prefix is clamped to the
 * log; the empty world addresses prefix 0). */
if5_cut if5_cut_at(if5_world* w, uint32_t prefix);

/* The cut a view addresses (the view's global part: prefix =
 * v->cut, prunes = v->excl; head selections are region-level and
 * default — see if5_cut_view). */
if5_cut if5_cut_of_view(const if5_view* v);

/* The undo constructor (vol. 5 5.2): prune iota_iid's causal
 * subtree. Fails (returns false, *c untouched) when iid is not
 * addressable in c. The result is down-closed by construction. */
bool if5_cut_prune(if5_world* w, if5_cut* c, uint32_t iid);

/* The intersection — TOTAL on all cuts: the represented set is
 * S(min prefix) minus both prune sets' up-sets. Fails only on
 * prune-list overflow. */
bool if5_cut_meet(if5_world* w, const if5_cut* a, const if5_cut* b,
                  if5_cut* out);

/* The union — defined on PRINCIPAL cuts only (the vol. 5 5.1
 * lattice): the union of two pruned addresses is not in general
 * representable as prefix+prunes (ADR-0026 3b registers the
 * boundary), so join refuses pruned inputs. */
bool if5_cut_join(if5_world* w, const if5_cut* a, const if5_cut* b,
                  if5_cut* out);

/* The subset order on represented sets: S'(a) <= S'(b) iff
 * prefix(a) <= prefix(b) and every b-prune root addressable in a
 * is already a-pruned (the covering test — full derivation in
 * if5_cut.c). */
bool if5_cut_le(const if5_world* w, const if5_cut* a, const if5_cut* b);

/* Membership, under the fold's own test (the REBORN exemption
 * included — membership and folding must never disagree). */
bool if5_cut_contains(const if5_world* w, const if5_cut* c, uint32_t iid);

/* The structural verifier, promoted from the slice's test-private
 * checker: every causal predecessor of every contained record is
 * contained. O(n^2) at test scale by design. */
bool if5_cut_is_down_closed(const if5_world* w, const if5_cut* c);

/* The address registry (E1b's global addresses, ADR-0026 3b):
 * enter a cut by value; ids are 1-based, 0 = registry full. */
uint8_t      if5_cut_register(if5_world* w, const if5_cut* c);
const if5_cut* if5_cut_by_id(if5_world* w, uint8_t id);
uint32_t     if5_cut_count(if5_world* w);

/* The fold at a cut — observationally the view path (V3's
 * equivalence is tested against an independent membership fold,
 * not against itself). */
uint64_t if5_cut_state(if5_world* w, const if5_cut* c, uint16_t rid,
                       uint32_t* contrib);

/* The view a cut addresses (head selections default to the
 * regions' own current heads, walked back to the cut — the same
 * default the fold uses). */
if5_view if5_cut_view(const if5_cut* c);

/* ============================================================
 * Adjudication statistics (vol. 5 4.2's four steps, counted)
 * ============================================================ */

typedef struct if5_stats {
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
} if5_stats;

/* ============================================================
 * The world
 * ============================================================ */

if5_world* if5_world_create(void);
void       if5_world_free(if5_world* w);

void       if5_set_system_source(if5_world* w, if5_sid sid);

/* --- sources ------------------------------------------------ */

if5_sid if5_add_source(if5_world* w, const if5_source* s);
const if5_source* if5_source_info(if5_world* w, if5_sid sid);
if5_ledger* if5_ledger_of(if5_world* w, if5_sid sid);

/* --- regions ------------------------------------------------ */

/* Region creation with S15 containment: parent 0 = root level.
 * Otherwise promoted unchanged from the slice (logged, sourced,
 * tier default on the record per constraint D1). */
uint16_t if5_region_create(if5_world* w, if5_sid src, const char* name,
                           uint16_t parent, uint8_t ptype, if5_tier tier,
                           const void* blob, uint32_t len);

typedef struct if5_region_spec {
    char     name[IF5_NAME_MAX];
    uint8_t  ptype;
    uint8_t  tier;
    uint16_t parent;          /* S15 containment; 0 = root */
    uint64_t seed;
} if5_region_spec;

uint32_t if5_region_create_batch(if5_world* w, if5_sid src,
                                 const if5_region_spec* specs, uint32_t n);

uint16_t if5_region_find(if5_world* w, const char* name);
if5_region* if5_region_info(if5_world* w, uint16_t rid);

bool if5_region_set_tier(if5_world* w, if5_sid src, uint16_t rid, if5_tier tier);

/* --- the main entry: submit an intent ----------------------- */

typedef struct if5_submit_opts {
    uint32_t       observed_cut;
    if5_debit_class debit_class;  /* agent/system initiative pays */
    if5_severity   severity;      /* the S16 ladder rung */
    uint8_t        commit;        /* commitment strategy tag */
} if5_submit_opts;

uint32_t if5_submit(if5_world* w, if5_sid src, uint16_t rid,
                    uint8_t ptype, uint64_t value,
                    const if5_submit_opts* opts);

uint32_t if5_user_act(if5_world* w, if5_sid src, uint16_t rid,
                      uint8_t ptype, uint64_t value);

/* --- delegation (S22 / T7) ----------------------------------- */

bool if5_delegate(if5_world* w, if5_sid delegator, if5_sid delegate,
                  uint32_t amount);

/* --- N4 primitives (suite surface) ---------------------------- */

bool if5_debit(if5_world* w, if5_sid sid, if5_debit_class c,
               if5_severity sev, uint32_t reason_iid);
uint32_t if5_receipt_count(if5_world* w);
const if5_receipt* if5_receipt_at(if5_world* w, uint32_t i);

/* --- queries: the fold (N2) ---------------------------------- */

if5_view if5_now(if5_world* w);

uint64_t if5_state_at(if5_world* w, uint16_t rid, const if5_view* v,
                      uint32_t* contrib);

typedef void (*if5_prov_cb)(const if5_intent* rec, void* user);
void if5_prov_walk(if5_world* w, uint16_t rid, const if5_view* v,
                   if5_prov_cb cb, void* user);

/* --- undo is sugar (N2 / I5) --------------------------------- */

uint32_t if5_undo_nth(if5_world* w, if5_sid src, uint32_t iid);
if5_view if5_view_rewind(if5_world* w, const if5_view* v, uint32_t iid);
bool     if5_view_adopt(if5_world* w, const if5_view* v);
if5_view if5_view_at(if5_world* w, uint32_t cut);

/* --- tier-1 stitching (explicit merge-intent) ----------------- */

uint32_t if5_merge_intent(if5_world* w, if5_sid src, uint16_t rid,
                          uint32_t head_a, uint32_t head_b,
                          uint64_t stitched);

/* --- black box (S3) ------------------------------------------ */

void if5_blackbox_configure(if5_world* w, uint16_t rid, uint16_t window);
uint32_t if5_blackbox_export(if5_world* w, if5_sid src, uint16_t rid,
                             char* buf, uint32_t cap);

/* --- projections (promoted unchanged: the differential's
 *     byte-exact comparison surface) --------------------------- */

uint32_t if5_project_semantic(if5_world* w, const if5_view* v,
                              char* buf, uint32_t cap);
uint32_t if5_project_set(if5_world* w, const if5_view* v,
                         char* buf, uint32_t cap);
uint32_t if5_project_history(if5_world* w, const if5_view* v,
                             char* buf, uint32_t cap);

/* ============================================================
 * S15 seated — the semantic tree, first class (ADR-0026 3c)
 *
 * Vol. 4 S15's contract, operationalized: semantic structure is
 * the work, vision is one projection, screen-reader users are
 * first-class testers. The walk is depth-first pre-order over
 * region containment, children in declaration order, roots in
 * declaration order — deterministic and total: every region
 * appears exactly once at every view. Each node carries exactly
 * what an accessibility tree carries: name, value, semantic role
 * (the payload family), tier, contribution count, and the PROV
 * anchor of the newest contributor. NO visual field exists in
 * this structure or anywhere in the v5 core (the rebound gate's
 * v5 section bans the vocabulary).
 * ============================================================ */

typedef struct if5_node {
    char     name[IF5_NAME_MAX];  /* NUL-terminated copy */
    uint64_t value;               /* the fold at the view */
    uint32_t contrib;             /* contributing intents (I6) */
    uint32_t last_iid;            /* PROV anchor: newest contributor
                                   * at THIS view (0 = untouched) */
    uint16_t rid;
    uint16_t parent;              /* containment parent (0 = root) */
    uint16_t depth;               /* 0 = root level */
    uint8_t  ptype;               /* the semantic role */
    uint8_t  tier;
} if5_node;

/* Returns false from the callback to stop the walk. Returns the
 * number of nodes visited. */
typedef bool (*if5_tree_cb)(const if5_node* n, void* user);
uint32_t if5_tree_walk(if5_world* w, const if5_view* v,
                       if5_tree_cb cb, void* user);

/* ============================================================
 * S17 seated — the surprise timetable (ADR-0026 3d)
 *
 * Vol. 4 S17: the design object is not the picture but the
 * timetable of controlled prediction errors; onboarding, diffs,
 * changelogs and AI explanations are projections of this one
 * object. N5's no-new-slots discipline forbids a new log family:
 * the schedule is a PURE QUERY over records the core already
 * logs, visible at a view exactly as the fold is. Every entry is
 * PROV-backed — its iid resolves through if5_log_at to the record
 * that witnessed it.
 *
 * Entry kinds (what the black box can determine):
 *   SURPRISE  a surprise-class debit posted on a record (B5: a
 *             prior prediction was violated — the v5 debit word's
 *             class packing is what makes these itemizable)
 *   BREAKER   a breaker receipt (an overdraft the user must see)
 *   TIER      a tier dial move (S14: the merge semantics changed)
 *   ARBIT     a tier-3 exclusion (your write lost the arbitration)
 *   MERGE     a concurrent merge or explicit stitch (the fold
 *             changed by joining unseen work)
 * ============================================================ */

enum {
    IF5_SCHED_SURPRISE = 1,
    IF5_SCHED_BREAKER  = 2,
    IF5_SCHED_TIER     = 3,
    IF5_SCHED_ARBIT    = 4,
    IF5_SCHED_MERGE    = 5
};

typedef struct if5_sched_entry {
    uint32_t iid;       /* PROV anchor — resolves via if5_log_at */
    uint32_t magnitude; /* posted units (the severity rung) or 0 */
    uint16_t region;    /* the region it concerns (0 = world-scoped) */
    uint8_t  sid;       /* whose attention it concerns */
    uint8_t  kind;      /* IF5_SCHED_* */
} if5_sched_entry;

/* Filling mode (out != NULL): writes up to cap entries and returns
 * the TOTAL match count at the view — a return value greater than
 * cap signals truncation (the caller reallocs and asks again).
 * Counting mode (out == NULL) returns the total alone. src ==
 * IF5_SID_NULL selects all sources. A pure query: the durable
 * layer's byte image is unchanged (test_v5_seats enforces it). */
uint32_t if5_schedule(if5_world* w, const if5_view* v, if5_sid src,
                      if5_sched_entry* out, uint32_t cap);

/* ============================================================
 * STAGE R2 — the mechanism layer (ADR-0027): the seven shipping
 * abstractions re-derived as projections, queries, and judgments
 * over the durable core. Five of the six modules are PURE
 * queries (S24 purity extends to them: test-enforced by
 * durable-image memcmp); the two write paths (process phases,
 * compiled submission) go through the ordinary submit channel.
 * The NOT-CARRIED columns of ADR-0027 Decision 2 are contract:
 * no confidence prior, no easing curve, no asserted-by-convention
 * relation, no action callback, no screen raster, no
 * wall-clock budget, no geometric hit-test lives here.
 * ============================================================ */

/* --- (a) Estimate as a fold -----------------------------------
 * An estimate is not a cell: it is the region's fold READ at an
 * address. `evidence` is an ORDINAL, not a probability — the log
 * can support an evidence state, not a caller's prior, and any
 * consumer treating IF5_CONF_* as Bayesian confidence is
 * misusing this API (ADR-0027 CAVEATS).
 */
enum {
    IF5_CONF_NONE   = 0,   /* no contributing record at the view  */
    IF5_CONF_FORKED = 1,   /* fork unresolved: two versions live  */
    IF5_CONF_MERGED = 2,   /* concurrent work present, stitched   */
    IF5_CONF_CHAIN  = 3    /* single chain, fully applied         */
};

typedef struct if5_estimate {
    uint64_t value;         /* the fold at the view               */
    uint32_t contributions; /* applied live contributors          */
    uint32_t last_iid;      /* newest contributor (PROV anchor)   */
    uint8_t  evidence;      /* IF5_CONF_*                         */
    uint8_t  reserved[3];
} if5_estimate;

bool if5_estimate_at(if5_world* w, uint16_t rid, const if5_view* v,
                     if5_estimate* out);

/* The trajectory: the fold evaluated at every integer cut in
 * (from.cut, to.cut] — the shipping animation is the two-sample
 * special case. Counting mode (out == NULL) returns the total. */
uint32_t if5_estimate_trace(if5_world* w, uint16_t rid,
                            const if5_view* from, const if5_view* to,
                            uint64_t* out, uint32_t cap);

/* The dependency set, made explicit (the derived estimates'
 * automatic dependency tracking is structural here): the view-
 * live contributing records, oldest first. */
uint32_t if5_estimate_sources(if5_world* w, uint16_t rid,
                              const if5_view* v, uint32_t* iids,
                              uint32_t cap);

/* --- (b) Relation as the cause-graph over the log --------------
 * DEPENDS_ON is the PROV edge; TRIGGERS is the causal-successor
 * query; VARIES_WITH is the concurrency enumeration; CONTAINS is
 * the folded containment (if5_parent_at). BESIDE/AFFORDS/
 * WITHDRAWS_FOR/PRESENTS_FOR/INTERPRETS_AS have no causal
 * semantics and are NOT carried (ADR-0027 Decision 2b).
 */
typedef struct if5_edge {
    uint32_t from_iid;      /* the cause (a PROV parent)          */
    uint32_t to_iid;        /* the effect (the record itself)     */
} if5_edge;

/* The record's PROV edges (merge carries two, an undo cursor
 * carries one; a plain submission carries none). */
uint32_t if5_cause_edges(if5_world* w, uint32_t iid, if5_edge* out,
                         uint32_t cap);

/* The exact reversal: records whose prov_a/prov_b IS `iid`. */
uint32_t if5_cause_effects(if5_world* w, uint32_t iid, if5_edge* out,
                           uint32_t cap);

/* View-live records on the region that have at least one
 * concurrent partner (vc incomparable both ways). */
uint32_t if5_cause_concurrents(if5_world* w, const if5_view* v,
                               uint16_t rid, uint32_t* iids,
                               uint32_t cap);

/* The happened-before reachability (the vc partial order,
 * transitive): did `from` causally precede `to`? O(1). */
bool if5_cause_reachable(if5_world* w, uint32_t from, uint32_t to);

/* --- (c) Closure as a judgment ---------------------------------
 * The five execution-side stages, READ off the adjudicated
 * record: goal (the region's semantic address), intent (the
 * value word), adjudication (the verdict), observation point
 * (what the submitter knew), evaluation (the fold at the view).
 * The Action stage is the type-registered semilattice join —
 * mutation is typed, not arbitrary (ADR-0027 Decision 2c).
 */
typedef enum if5_verdict {
    IF5_VERDICT_APPLIED    = 1,  /* clean application              */
    IF5_VERDICT_MERGED     = 2,  /* concurrent, stitched into lattice */
    IF5_VERDICT_FORKED     = 3,  /* live on a forked region chain  */
    IF5_VERDICT_ARBIT_LOST = 4,  /* tier-3 arbitration loser       */
    IF5_VERDICT_REFUSED    = 5,  /* breaker/authority: logged, unapplied */
    IF5_VERDICT_EXCLUDED   = 6   /* undone at this view            */
} if5_verdict;

typedef struct if5_judgment {
    uint32_t iid;           /* the record being read             */
    if5_vc   observed;      /* the asker's causal coordinate at
                             * ask time (the record's own vc:
                             * declared knowledge + this record's
                             * increment). The SCALAR prefix the
                             * submitter passed is a channel
                             * property, not a record property —
                             * it is not recoverable, and this
                             * struct does not fake it            */
    uint32_t eval_value;    /* the fold at the view (evaluation) */
    uint16_t region;        /* the goal's address                */
    uint8_t  ptype;         /* the intent's payload family       */
    uint8_t  src;           /* who asked                         */
    uint8_t  verdict;       /* if5_verdict                       */
    uint8_t  fork_heads;    /* 0, 1, or 2 (region's live forks)  */
    uint8_t  rebased;       /* REBORN mirror                     */
    uint8_t  debit_class;   /* who paid (0 = nobody)             */
    uint8_t  severity;      /* the rung posted                   */
} if5_judgment;

bool if5_judgment_of(if5_world* w, uint32_t iid, const if5_view* v,
                     if5_judgment* out);

/* --- (d) Perception as the projection family -------------------
 * A WORLD-scoped registry (not process-global). The three frozen
 * R1 projections are registered through this same channel at
 * world creation and are WRAPPED, not modified — M4 asserts the
 * registry path is byte-identical to the direct call. Extension
 * projections (the tree, the schedule, text renderings) register
 * through the same public channel (B2's equality of standing).
 */
#define IF5_MAX_PROJECTIONS 16
#define IF5_PROJ_NAME_MAX   16

typedef uint32_t (*if5_projection_fn)(if5_world* w, const if5_view* v,
                                      char* buf, uint32_t cap);

uint8_t  if5_projection_register(if5_world* w, const char* name,
                                 if5_projection_fn fn);
uint32_t if5_projection_count(if5_world* w);
const char* if5_projection_name(if5_world* w, uint32_t index);
uint32_t if5_projection_invoke(if5_world* w, const if5_view* v,
                               const char* name, char* buf, uint32_t cap);

/* --- (e) px_loop's budget as N4's ledger -----------------------
 * No new API: the mapping is an argument (ADR-0027 Decision 2e)
 * plus M7's behavioral pin. The budget lives on the ledger, is
 * debited BEFORE the effect, and refuses (with a receipt) when
 * exhausted — the overrun counter's question is answered by the
 * receipt stream instead.
 */

/* --- (f) Intent compilation as the type registry's routing ------
 * Compilation is ROUTING THROUGH THE OPEN REGISTRY: a compile
 * function (text -> value word + submit options) attached to a
 * registered payload type. Built-in routes (NUM/SET/STR) register
 * through the same public channel. The compiled record is
 * byte-equal to the direct submission of the same value (M6).
 */
typedef bool (*if5_compile_fn)(const char* text, uint64_t* value,
                               if5_submit_opts* opts);

bool if5_route_register(if5_world* w, uint8_t ptype, if5_compile_fn fn);
uint32_t if5_compile(if5_world* w, if5_sid src, uint16_t rid,
                     uint8_t ptype, const char* text,
                     const if5_submit_opts* opts);

/* --- (g) px_interaction as the adjudication surface --------------
 * begin opens the process IN THE LOG (identity is an iid: the
 * opening is itself adjudicated, undoable, provable); samples
 * are pure working-set updates (zero submissions, zero debits —
 * the px invariant, statically enforced by the rebound gate);
 * commit submits through the ordinary channel (the outcome is a
 * causal successor of the opening by vector clock); cancel IS a
 * cut: the opening is undone and the process leaves causal
 * history the way an undone change does. Sample trajectories
 * are working-set by the accountization that keeps region names
 * working-set (ADR-0027 Known issue 1).
 */
#define IF5_MAX_PROCESSES 8

typedef struct if5_process if5_process;

if5_process* if5_process_begin(if5_world* w, if5_sid src, uint16_t rid,
                               uint8_t ptype);
bool     if5_process_sample(if5_process* p, uint64_t value);
uint32_t if5_process_commit(if5_process* p, uint64_t value,
                            const if5_submit_opts* opts);
bool     if5_process_cancel(if5_process* p);
uint64_t if5_process_latest(const if5_process* p);  /* sample readback */
uint32_t if5_process_of(if5_world* w, const if5_view* v, uint32_t iid);
bool     if5_process_done(const if5_process* p);

/* --- Known issue 1 discharged: containment becomes content -----
 * A reparent is an ORDINARY record (IF5_T_REPARENT, region =
 * child, value word = new parent) excluded from the value fold.
 * The tree at a view folds: the last live reparent of a region,
 * else the creation seed (the single-create REGION record's aux;
 * batch-created regions seed at root, which is what the frozen
 * blob layout always meant). The working-set parent field is a
 * CACHE of the live fold (M8: no reparent -> identical to R1).
 */
uint32_t if5_reparent(if5_world* w, if5_sid src, uint16_t child,
                      uint16_t new_parent);
uint16_t if5_parent_at(if5_world* w, const if5_view* v, uint16_t rid);

/* --- Known issue 2 discharged: the per-source knowledge cut -----
 * Record identity, not clock comparison (the REBORN collision
 * cannot bite). Addresses "source S's own records up to prefix,
 * together with their PROV-declared ancestors" — the world as S
 * has STRUCTURALLY bound itself to. Scope honesty (ADR-0027
 * Decision 4, registered twice over): the implicit knowledge
 * dependency of observed-cut declarations is NOT recoverable
 * (the record word stores no declared prefix — the judgment
 * mapping's discovery), and observational knowledge is out of
 * scope. Membership is a REACHABILITY test, never a stored list
 * and never a clock comparison; O(1) storage per cut.
 */
typedef struct if5_kcut {
    if5_vc   frontier;      /* the join of S's records' vcs (cached) */
    uint32_t prefix;        /* the log bound                      */
    uint8_t  sid;           /* whose knowledge this addresses     */
    uint8_t  reserved[3];
} if5_kcut;

if5_kcut if5_kcut_of_source(if5_world* w, if5_sid sid, uint32_t prefix);
bool     if5_kcut_contains(if5_world* w, const if5_kcut* k, uint32_t iid);
bool     if5_kcut_is_down_closed(if5_world* w, const if5_kcut* k);
uint64_t if5_kcut_state_at(if5_world* w, const if5_kcut* k,
                           uint16_t rid, uint32_t* contrib);
uint32_t if5_kcut_members(if5_world* w, const if5_kcut* k,
                          uint32_t* iids, uint32_t cap);

/* --- instrumentation (E2 / I7) -------------------------------- */

const if5_stats* if5_stats_of(if5_world* w);
const if5_intent* if5_log_at(if5_world* w, uint32_t iid); /* 1-based */
uint32_t if5_log_count(if5_world* w);
uint64_t if5_history_bytes(if5_world* w);
uint64_t if5_working_set_bytes(if5_world* w);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_IF5_H */
