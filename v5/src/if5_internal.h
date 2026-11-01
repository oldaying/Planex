/*
 * if5_internal.h — shared internals of the v5 intent-flow core
 * (ADR-0026). Not installed, not part of the public contract.
 *
 * Lineage: promoted from slice/src/ifl_internal.h (ADR-0025); the
 * world gains the cut registry (ADR-0026 Decision 3b) and the
 * region gains the S15 containment parent (Decision 3c).
 */
#ifndef IF5_INTERNAL_H
#define IF5_INTERNAL_H

#include "planex/if5.h"
#include <assert.h>

struct if5_world {
    /* sources (N1) */
    if5_source  sources[IF5_MAX_SOURCES];
    if5_vc      svc[IF5_MAX_SOURCES];   /* each source's knowledge */
    if5_ledger  ledgers[IF5_MAX_SOURCES];
    uint8_t     n_sources;

    /* payload registry (B2 open channel) */
    if5_payload_type types[IF5_MAX_PAYLOAD_TYPES];
    uint8_t     n_types;

    /* regions */
    if5_region  regions[IF5_MAX_REGIONS];
    uint16_t    n_regions;

    /* the log — the ONLY history structure (N2) */
    if5_intent* log;
    uint32_t    n_log, cap_log;
    uint8_t*    arena;
    uint32_t    n_arena, cap_arena;
    if5_vc      vc_now;                 /* running join of all record vcs */

    /* receipts (N4 / T7) */
    if5_receipt* receipts;
    uint32_t     n_receipts, cap_receipts;

    /* the interface cursor */
    if5_view    now;

    /* The paying system source (explicit configuration via
     * if5_set_system_source — never derived from `kind`, which is
     * metadata and never dispatches; I1). IF5_SID_NULL = no one can
     * pay for system initiative, so none happens (I2's zero-budget
     * world is the general form of this). */
    if5_sid     sys_sid;

    /* The cut registry (ADR-0026 3b): named global addresses.
     * Registry storage is part of the working set, never of
     * history — the differential asserts the working-set delta
     * against the slice as the exact sizeof difference. */
    if5_cut     cuts[IF5_MAX_CUTS];
    uint8_t     n_cuts;

    /* --- the mechanism layer (ADR-0027): world-scoped registries
     * and process slots — working set, never history. The three
     * frozen R1 projections register through the public channel at
     * world creation (B2 equality of standing); the route table maps
     * ptype -> compile function the same way. Process slots are the
     * interaction surfaces: static, bounded, and gone at world free
     * (the OPENING records stay in the log — records are durable,
     * slots are not; ADR-0027 CAVEATS). */
    struct {
        char               name[IF5_PROJ_NAME_MAX];
        if5_projection_fn  fn;
    }           projections[IF5_MAX_PROJECTIONS];
    uint8_t     n_projections;
    uint8_t     reserved_pad[7];

    if5_compile_fn routes[IF5_MAX_PAYLOAD_TYPES];

    struct if5_process {
        if5_world*  w;
        if5_sid     src;
        uint16_t    rid;
        uint8_t     ptype;
        bool        closed;
        uint32_t    begin_iid;
        uint64_t    latest;
    }           procs[IF5_MAX_PROCESSES];
    uint8_t     n_procs;
    uint8_t     reserved_pad2[7];

    if5_stats   stats;
    uint32_t    tick;
};

/* --- log plumbing (if5_core.c) -------------------------------- */

uint32_t if5__append(if5_world* w, if5_intent* rec);

/* Knowledge: the pointwise-max vc over records 1..cut under the
 * living-knowledge rule (see if5_core.c — dead records do not
 * contribute knowledge). */
if5_vc if5__vc_upto(if5_world* w, uint32_t cut);

/* Arena (oversized payloads). Returns the packed {off, len}. */
uint64_t if5__arena_put(if5_world* w, const void* data, uint32_t len);
const void* if5__arena_get(if5_world* w, uint64_t packed, uint32_t* len);

/* Undo-revocation test (if5_core.c, view-level): is `rec` a causal
 * descendant of any root the view has revoked? */
bool if5__is_excluded(const if5_world* w, const if5_intent* rec,
                      const if5_view* v);

/* --- adjudication (if5_merge.c) -------------------------------- */

bool if5__adjudicate(if5_world* w, if5_intent* rec);
bool if5__region_concurrent(if5_world* w, uint16_t rid, if5_vc vc);
bool if5__arbiter_prefers(const if5_intent* newbie, const if5_intent* head);

/* --- budget (if5_budget.c) -------------------------------------- */

bool if5__post_debit(if5_world* w, if5_sid sid, if5_debit_class c,
                     if5_severity sev, uint32_t reason_iid);
void if5__emit_receipt(if5_world* w, if5_sid origin, uint32_t reason_iid,
                       uint32_t rollback_cut);

/* --- fold filters (if5_core.c, shared with if5_tree.c) ---------- */

/* Is ptype a fold participant? (Structural bookkeeping records are
 * not; REGION seeds and MERGE stitches are.) */
bool if5__is_value_type(uint8_t ptype);

/* The view's fork-head selection for a region, walked back to the
 * view's cut; and the chain-membership test. Both shared with
 * if5_tree.c so the tree's PROV anchor matches the fold exactly. */
uint32_t if5__selected_head(const if5_world* w, const if5_region* r,
                            const if5_view* v);
bool if5__on_selected_chain(const if5_world* w, uint32_t head,
                            const if5_intent* rec);

/* --- cut algebra (if5_cut.c) ------------------------------------ */

/* The frontier of a represented set, under the living-knowledge
 * rule: the join over records <= prefix that are contained (the
 * prunes' up-sets excluded), skipping adjudication-dead records —
 * the same rule if5__vc_upto applies, parameterized by an
 * arbitrary prune list instead of the world's now. */
if5_vc if5__cut_frontier(if5_world* w, uint32_t prefix,
                         const uint32_t* prunes, uint32_t n_prunes);

/* --- shared small helpers --------------------------------------- */

if5_region* if5__region(if5_world* w, uint16_t rid);
const if5_payload_type* if5__type(if5_world* w, uint8_t ptype);
uint32_t if5__n(char* dst, uint32_t cap, const char* fmt, ...);

/* Built-in sq implementations (registered through the same public
 * channel as user types — B2's equality-of-standing rule). */
bool if5__sq_num(uint64_t a, uint64_t b, uint64_t* out);
bool if5__le_num(uint64_t a, uint64_t b);
bool if5__sq_set(uint64_t a, uint64_t b, uint64_t* out);
bool if5__le_set(uint64_t a, uint64_t b);

/* --- mechanism-layer internals (ADR-0027) ---------------------- */

/* The mechanism-layer built-ins register at world creation, through
 * the same public channel applications use (B2 equality of
 * standing): the three frozen projections + the tree and schedule
 * text renderings (if5_projection.c), and the NUM/SET/STR compile
 * routes (if5_route.c). Called from if5_world_create. */
void if5__register_builtin_projections(if5_world* w);
void if5__register_builtin_routes(if5_world* w);

/* The reparent fold's seed read: the single-create REGION record's
 * aux carries the creation-time parent (batch-created regions seed
 * at root — the frozen blob layout's meaning). if5_core.c fills the
 * aux at single-region creation; if5_tree.c / if5_cause.c read it
 * through this helper. 0 = root. */
uint16_t if5__seed_parent(const if5_world* w, uint16_t rid);

#endif /* IF5_INTERNAL_H */
