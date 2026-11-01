/*
 * ifl_internal.h — shared internals of the intent-flow core.
 * Not installed, not part of the public contract.
 */
#ifndef IFL_INTERNAL_H
#define IFL_INTERNAL_H

#include "planex/ifl.h"
#include <assert.h>

struct ifl_world {
    /* sources (N1) */
    ifl_source  sources[IFL_MAX_SOURCES];
    ifl_vc      svc[IFL_MAX_SOURCES];   /* each source's knowledge */
    ifl_ledger  ledgers[IFL_MAX_SOURCES];
    uint8_t     n_sources;

    /* payload registry (B2 open channel) */
    ifl_payload_type types[IFL_MAX_PAYLOAD_TYPES];
    uint8_t     n_types;

    /* regions */
    ifl_region  regions[IFL_MAX_REGIONS];
    uint16_t    n_regions;

    /* the log — the ONLY history structure (N2) */
    ifl_intent* log;
    uint32_t    n_log, cap_log;
    uint8_t*    arena;
    uint32_t    n_arena, cap_arena;
    ifl_vc      vc_now;                 /* running join of all record vcs */

    /* receipts (N4 / T7) */
    ifl_receipt* receipts;
    uint32_t     n_receipts, cap_receipts;

    /* the interface cursor */
    ifl_view    now;

    /* The paying system source (explicit configuration via
     * ifl_set_system_source — never derived from `kind`, which is
     * metadata and never dispatches; I1). IFL_SID_NULL = no one can
     * pay for system initiative, so none happens (I2's zero-budget
     * world is the general form of this). */
    ifl_sid     sys_sid;

    ifl_stats   stats;
    uint32_t    tick;
};

/* --- log plumbing (ifl_core.c) -------------------------------- */

/* Appends a record; returns its iid (1-based). Sets iid-implied
 * fields (t) and returns 0 only on allocation failure (treated as
 * fatal by callers via assert — the slice is an instrument, not a
 * service). */
uint32_t ifl__append(ifl_world* w, ifl_intent* rec);

/* Knowledge: the pointwise-max vc over records 1..cut. cut == 0
 * yields the zero clock. Uses the running fold when cut == n_log. */
ifl_vc ifl__vc_upto(ifl_world* w, uint32_t cut);

/* Arena (oversized payloads). Returns the packed {off, len}. */
uint64_t ifl__arena_put(ifl_world* w, const void* data, uint32_t len);
const void* ifl__arena_get(ifl_world* w, uint64_t packed, uint32_t* len);

/* Undo-revocation test (ifl_core.c, view-level): is `rec` a causal
 * descendant of any root the view has revoked? */
bool ifl__is_excluded(const ifl_world* w, const ifl_intent* rec,
                      const ifl_view* v);

/* --- adjudication (ifl_merge.c) -------------------------------- */

/* The four-step flow of vol. 5 4.2, applied to a freshly appended
 * world-payload record. Returns true when the record ends up in the
 * fold. Assumes budget/authority checks already ran. */
bool ifl__adjudicate(ifl_world* w, ifl_intent* rec);

/* Is `vc` concurrent with any applied record on `rid`? (The scan is
 * O(log) by registered design: no per-region frontier is retained —
 * frontier storage would tax the working set 16B/region, and the
 * slice's scale makes the scan free. See ADR-0025 Decision 4f.) */
bool ifl__region_concurrent(ifl_world* w, uint16_t rid, ifl_vc vc);

/* The registered arbiter for tier 3: the total order on (t, sid,
 * iid) — submission order. Newcomer wins ties by construction. */
bool ifl__arbiter_prefers(const ifl_intent* newbie, const ifl_intent* head);

/* --- budget (ifl_budget.c) -------------------------------------- */

/* Posts a debit against sid's ledger; trips the breaker, emits a
 * receipt and returns false on overdraft. */
bool ifl__post_debit(ifl_world* w, ifl_sid sid, ifl_debit_class c,
                     ifl_severity sev, uint32_t reason_iid);

/* Emits a receipt record (logged, sourced) and stores it. */
void ifl__emit_receipt(ifl_world* w, ifl_sid origin, uint32_t reason_iid,
                       uint32_t rollback_cut);

/* --- shared small helpers --------------------------------------- */

ifl_region* ifl__region(ifl_world* w, uint16_t rid);
const ifl_payload_type* ifl__type(ifl_world* w, uint8_t ptype);
uint32_t ifl__n(char* dst, uint32_t cap, const char* fmt, ...);

/* Built-in sq implementations (registered through the same public
 * channel as user types — B2's equality-of-standing rule). */
bool ifl__sq_num(uint64_t a, uint64_t b, uint64_t* out);
bool ifl__le_num(uint64_t a, uint64_t b);
bool ifl__sq_set(uint64_t a, uint64_t b, uint64_t* out);
bool ifl__le_set(uint64_t a, uint64_t b);

#endif /* IFL_INTERNAL_H */
