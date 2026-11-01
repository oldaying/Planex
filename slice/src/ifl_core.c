/*
 * ifl_core.c — the intent-flow core: world, log, clocks, regions,
 * views, projections, PROV, black box.
 *
 * Program I's falsification instrument (ADR-0025). The semantics are
 * vol. 5's; this file operationalizes them at instrument scale.
 *
 * Determinism contract: the clock is a tick counter advanced once per
 * submitted record; every iteration is array order; every arbiter is
 * the total order on (t, sid, iid). Two runs of the same script on
 * the same build produce byte-identical transcripts — that is not a
 * nicety, it is what makes C7's differential test and E2's
 * "recomputable on any checkout" clause possible.
 */
#include "ifl_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Small helpers
 * ============================================================ */

/* Bounded sprintf: returns bytes written, never negative, always
 * NUL-terminates. Every transcript line in the core goes through
 * this — a truncated projection is still a deterministic one. */
uint32_t ifl__n(char* dst, uint32_t cap, const char* fmt, ...)
{
    va_list ap;
    int     n;
    if (cap == 0) return 0;
    va_start(ap, fmt);
    n = vsnprintf(dst, (size_t)cap, fmt, ap);
    va_end(ap);
    if (n < 0)        { dst[0] = '\0'; return 0; }
    if ((uint32_t)n >= cap) { dst[cap - 1] = '\0'; return cap - 1; }
    return (uint32_t)n;
}

ifl_region* ifl__region(ifl_world* w, uint16_t rid)
{
    if (rid == 0 || rid > w->n_regions) return NULL;
    return &w->regions[rid - 1];
}

const ifl_payload_type* ifl__type(ifl_world* w, uint8_t ptype)
{
    if (ptype >= w->n_types) return NULL;
    return &w->types[ptype];
}

/* ============================================================
 * Vector-clock algebra (Fidge 1988 / Mattern 1989)
 * ============================================================ */

ifl_vc ifl_vc_join(ifl_vc a, ifl_vc b)
{
    ifl_vc r;
    int    i;
    for (i = 0; i < IFL_MAX_SOURCES; i++)
        r.c[i] = a.c[i] > b.c[i] ? a.c[i] : b.c[i];
    return r;
}

bool ifl_vc_le(ifl_vc a, ifl_vc b)
{
    int i;
    for (i = 0; i < IFL_MAX_SOURCES; i++)
        if (a.c[i] > b.c[i]) return false;
    return true;
}

bool ifl_vc_concurrent(ifl_vc a, ifl_vc b)
{
    return !ifl_vc_le(a, b) && !ifl_vc_le(b, a);
}

/* ============================================================
 * Built-in joins — registered through the same public channel as
 * user types (B2's equality-of-standing rule). See ifl_world_create.
 * ============================================================ */

bool ifl__sq_num(uint64_t a, uint64_t b, uint64_t* out)
{
    *out = a > b ? a : b;              /* NUM: bounded value, join = max */
    return true;
}

bool ifl__le_num(uint64_t a, uint64_t b)
{
    return a <= b;                     /* inflationary: a <= max(a, b) */
}

bool ifl__sq_set(uint64_t a, uint64_t b, uint64_t* out)
{
    *out = a | b;                      /* SET: tag union */
    return true;
}

bool ifl__le_set(uint64_t a, uint64_t b)
{
    return (a & ~b) == 0;              /* inflationary: a's tags survive */
}

/* ============================================================
 * World lifecycle
 * ============================================================ */

ifl_world* ifl_world_create(void)
{
    ifl_world* w = (ifl_world*)calloc(1, sizeof(ifl_world));
    if (!w) return NULL;

    /* The built-ins arrive through the same registration channel
     * every user type uses — there is no privileged constructor
     * path (B2). Order fixes their ids: NUM=0, SET=1, matching the
     * IFL_T_* enum, which the _Static_assert-style check below
     * pins at run time. */
    {
        ifl_payload_type num;
        ifl_payload_type set;

        memset(&num, 0, sizeof num);
        num.name = "num";
        num.value_size = 4;
        num.sq = ifl__sq_num;
        num.le = ifl__le_num;
        if (ifl_register_payload_type(w, &num) != IFL_T_NUM)
            { free(w); return NULL; }

        memset(&set, 0, sizeof set);
        set.name = "set";
        set.value_size = 8;
        set.sq = ifl__sq_set;
        set.le = ifl__le_set;
        if (ifl_register_payload_type(w, &set) != IFL_T_SET)
            { free(w); return NULL; }
    }

    w->tick = 1;
    w->sys_sid = IFL_SID_NULL;          /* no payer until the scenario
                                         * declares one (I2's general
                                         * form: no one can pay, so no
                                         * system initiative happens) */
    return w;
}

void ifl_set_system_source(ifl_world* w, ifl_sid sid)
{
    if (!w) return;
    w->sys_sid = (sid < w->n_sources) ? sid : IFL_SID_NULL;
}

void ifl_world_free(ifl_world* w)
{
    if (!w) return;
    free(w->log);
    free(w->arena);
    free(w->receipts);
    free(w);
}

/* ============================================================
 * Sources (N1)
 * ============================================================ */

ifl_sid ifl_add_source(ifl_world* w, const ifl_source* s)
{
    ifl_sid sid;
    if (!w || !s || w->n_sources >= IFL_MAX_SOURCES) return IFL_SID_NULL;
    sid = w->n_sources++;
    w->sources[sid] = *s;
    /* A source CANNOT arrive already delegated — delegation is an
     * event that happens to an existing source (ifl_delegate). The
     * caller's struct is often reused; a stale delegated_by here
     * would silently poison the chain check (a copied 0 is a legal
     * sid). Force it: */
    w->sources[sid].delegated_by = IFL_SID_NULL;
    /* The budget handle is the ONLY behavioral difference between
     * sources — wire it into the ledger NOW (a source with budget 0
     * and no delegation is a quiet one: I2's configuration, not a
     * mode). */
    w->ledgers[sid].budget = s->initiative_budget;
    /* A source's knowledge starts empty (it has observed nothing);
     * knowledge flows only through explicit channels — submits it
     * can see, delegation, receipts, queries. There is no implicit
     * global knowledge, or concurrency would be erased by fiat. */
    memset(&w->svc[sid], 0, sizeof(ifl_vc));
    return sid;
}

const ifl_source* ifl_source_info(ifl_world* w, ifl_sid sid)
{
    if (!w || sid >= w->n_sources) return NULL;
    return &w->sources[sid];
}

ifl_ledger* ifl_ledger_of(ifl_world* w, ifl_sid sid)
{
    if (!w || sid >= w->n_sources) return NULL;
    return &w->ledgers[sid];
}

/* ============================================================
 * The payload-type registry (B2 — the open extension channel)
 * ============================================================ */

/*
 * Structural admission check. A join offered to the registry is
 * probed on a fixed, deterministic sample of triples — the probe
 * points are chosen so that every synthetic-non-associative join
 * the property suite plants (C6's "the checker must bite") is
 * caught HERE, at registration, before it can ever adjudicate a
 * world record. The full 1,000,000-group law battery is
 * test_slice_property.c's job; this is the turnstile.
 */
static bool ifl__laws_hold(const ifl_payload_type* t)
{
    static const uint64_t probe[] = {
        0, 1, 2, 3, 5, 7, 11, 13, 0xFF, 0x100, 0xABCD, 0xFFFF
    };
    size_t i, j, k, n = sizeof probe / sizeof probe[0];
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            for (k = 0; k < n; k++) {
                uint64_t a = probe[i], b = probe[j], c = probe[k];
                uint64_t ab, bc, labc, rabc, aab, abb;
                if (!t->sq(a, b, &ab)) return false;
                if (!t->sq(b, c, &bc)) return false;
                if (!t->sq(ab, c, &labc)) return false;
                if (!t->sq(a, bc, &rabc)) return false;
                if (labc != rabc) return false;            /* associative */
                if (!t->sq(b, a, &abb)) return false;
                if (ab != abb) return false;               /* commutative */
                if (!t->sq(a, a, &aab)) return false;
                if (aab != a) return false;                /* idempotent */
                if (t->le && !t->le(a, ab)) return false;  /* inflationary */
            }
        }
    }
    return true;
}

uint8_t ifl_register_payload_type(ifl_world* w, const ifl_payload_type* t)
{
    uint8_t id;
    if (!w || !t || !t->name || t->value_size > 8) return (uint8_t)-1;
    if (w->n_types >= IFL_MAX_PAYLOAD_TYPES) return (uint8_t)-1;
    for (id = 0; id < w->n_types; id++)
        if (strcmp(w->types[id].name, t->name) == 0) return (uint8_t)-1;
    if (t->sq) {
        if (!t->le) return (uint8_t)-1;         /* sq without le: no order */
        if (!ifl__laws_hold(t)) return (uint8_t)-1;   /* the turnstile bites */
    }
    w->types[w->n_types] = *t;
    return w->n_types++;
}

uint64_t ifl_payload_blob(ifl_world* w, const void* data, uint32_t len)
{
    return ifl__arena_put(w, data, len);
}

const ifl_payload_type* ifl_type_info(ifl_world* w, uint8_t ptype)
{
    return ifl__type(w, ptype);
}

/* ============================================================
 * The log — the ONLY history structure (N2)
 * ============================================================ */

uint32_t ifl__append(ifl_world* w, ifl_intent* rec)
{
    if (w->n_log == w->cap_log) {
        uint32_t ncap = w->cap_log ? w->cap_log * 2 : 256;
        ifl_intent* nl = (ifl_intent*)realloc(w->log,
            (size_t)ncap * sizeof(ifl_intent));
        if (!nl) return 0;              /* allocation failure: fatal by
                                         * discipline (instrument, not
                                         * service) — callers assert */
        w->log = nl;
        w->cap_log = ncap;
    }
    rec->t = ++w->tick;                 /* one tick per record: the
                                         * physical-time coordinate the
                                         * tier-3 arbiter totals on */
    w->log[w->n_log] = *rec;
    w->n_log++;
    w->vc_now = ifl_vc_join(w->vc_now, rec->vc);
    w->stats.submits++;
    return w->n_log;                    /* iid, 1-based */
}


bool ifl__is_excluded(const ifl_world* w, const ifl_intent* rec,
                      const ifl_view* v);

ifl_vc ifl__vc_upto(ifl_world* w, uint32_t cut)
{
    ifl_vc r;
    uint32_t i;
    memset(&r, 0, sizeof r);
    if (cut == 0) return r;

    /*
     * The living-knowledge rule (registered here): a submitter's
     * declared knowledge is the join over the records it has seen
     * MINUS the records the current view has revoked (now.excl and
     * their causal descendants). Without this, every record any
     * source writes after an undo would still carry the revoked
     * record's clock component and be folded out — the undo would
     * erase the future, not the past. With it, "extends the rewound
     * point" (vol. 5) is a knowledge statement the vc encodes.
     *
     * Historical cut references written BEFORE the undo are
     * unaffected: their cut is below the revoked iid, and a record
     * at or below that cut cannot carry a later record's component.
     */
    if (w->now.n_excl == 0 && cut >= w->n_log)
        return w->vc_now;                /* running fold fast path —
                                          * only valid in a world with
                                          * nothing revoked */
    for (i = 0; i < cut && i < w->n_log; i++) {
        const ifl_intent* q = &w->log[i];
        if (q->flags & IFL_F_EXCLUDED) continue;   /* dead: skip */
        if (ifl__is_excluded(w, q, &w->now)) continue;
        r = ifl_vc_join(r, q->vc);
    }
    return r;
}

/* ============================================================
 * Arena (oversized payloads: region-creation blobs)
 * ============================================================ */

uint64_t ifl__arena_put(ifl_world* w, const void* data, uint32_t len)
{
    uint64_t packed;
    uint32_t off;
    if (w->n_arena + len > w->cap_arena) {
        uint32_t ncap = w->cap_arena ? w->cap_arena * 2 : 1024;
        while (ncap < w->n_arena + len) ncap *= 2;
        uint8_t* na = (uint8_t*)realloc(w->arena, ncap);
        if (!na) { assert(!"arena growth failed"); return 0; }
        w->arena = na;
        w->cap_arena = ncap;
    }
    off = w->n_arena;
    if (len) memcpy(w->arena + off, data, len);
    w->n_arena += len;
    packed = ((uint64_t)off << 32) | len;   /* the packed {off, len} */
    return packed;
}

const void* ifl__arena_get(ifl_world* w, uint64_t packed, uint32_t* len)
{
    uint32_t off = (uint32_t)(packed >> 32);
    uint32_t l   = (uint32_t)(packed & 0xFFFFFFFFu);
    if (off + l > w->n_arena) { if (len) *len = 0; return NULL; }
    if (len) *len = l;
    return w->arena + off;
}

/* ============================================================
 * Regions
 * ============================================================ */

/* The shared tail of region creation: emits the REGION record and,
 * per constraint D1 (no silent defaults), immediately emits the
 * tier intent that puts the registered default ON THE RECORD. */
static uint16_t ifl__region_open(ifl_world* w, ifl_sid sid, uint16_t rid,
                                 ifl_tier tier, const void* blob,
                                 uint32_t len, ifl_vc knowledge)
{
    ifl_intent rec;
    ifl_region* r = &w->regions[rid - 1];
    (void)knowledge;

    memset(&rec, 0, sizeof rec);
    rec.src    = sid;
    rec.region = rid;
    rec.ptype  = IFL_T_REGION;
    rec.flags  = IFL_F_APPLIED;
    if (blob && len) {
        uint64_t packed = ifl__arena_put(w, blob, len);
        rec.value[0] = (uint32_t)(packed >> 32);
        rec.value[1] = (uint32_t)(packed & 0xFFFFFFFFu);
    }
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        vc.c[sid] += 1;
        rec.vc = vc;
    }
    rec.prov_a = 0;
    rec.prov_b = 0;
    if (!ifl__append(w, &rec)) return 0;
    r->last_iid = w->n_log;

    /* D1: the tier default is a logged, sourced, undoable decision. */
    if (!ifl_region_set_tier(w, sid, rid, tier)) return 0;
    return rid;
}

uint16_t ifl_region_create(ifl_world* w, ifl_sid src, const char* name,
                           uint8_t ptype, ifl_tier tier,
                           const void* blob, uint32_t len)
{
    uint16_t rid;
    ifl_region* r;
    size_t nl;
    if (!w || !name || src >= w->n_sources || w->n_regions >= IFL_MAX_REGIONS)
        return 0;
    if (!ifl__type(w, ptype)) return 0;
    if (ifl_region_find(w, name)) return 0;
    nl = strlen(name);
    if (nl >= IFL_NAME_MAX) return 0;

    rid = ++w->n_regions;
    r = &w->regions[rid - 1];
    memset(r, 0, sizeof *r);
    memcpy(r->name, name, nl);
    r->ptype = ptype;
    r->tier  = 1;                        /* placeholder; set properly below */
    return ifl__region_open(w, src, rid, tier, blob, len, w->svc[src]);
}

uint32_t ifl_region_create_batch(ifl_world* w, ifl_sid src,
                                 const ifl_region_spec* specs, uint32_t n)
{
    /*
     * The document-load intent (ADR-0025 4f): ONE log record plus
     * one arena blob carrying every region's seed. The E2 leg
     * materializes its 1000-object load this way; the per-region
     * form above is the rich-PROV path for scenario-scale creation.
     */
    uint32_t i, made = 0;
    uint16_t first = 0, last = 0;
    ifl_intent rec;
    uint64_t packed;
    uint8_t* blob;
    uint32_t blob_len;

    if (!w || !specs || n == 0 || src >= w->n_sources) return 0;
    if (w->n_regions + n > IFL_MAX_REGIONS) return 0;

    blob_len = n * 10u;                  /* ptype(1) tier(1) seed(8) per spec */
    blob = (uint8_t*)calloc(blob_len, 1);
    if (!blob) return 0;

    for (i = 0; i < n; i++) {
        ifl_region* r;
        uint16_t rid;
        if (!ifl__type(w, specs[i].ptype)) { free(blob); return made; }
        rid = ++w->n_regions;
        r = &w->regions[rid - 1];
        memset(r, 0, sizeof *r);
        {
            size_t nl = strlen(specs[i].name);
            if (nl >= IFL_NAME_MAX) { free(blob); return made; }
            memcpy(r->name, specs[i].name, nl);
        }
        r->ptype = specs[i].ptype;
        r->tier  = specs[i].tier;
        r->cache = specs[i].seed;        /* the seed IS the region's initial
                                          * folded value for batch loads */
        if (!first) first = rid;
        last = rid;
        blob[i * 10u + 0] = specs[i].ptype;
        blob[i * 10u + 1] = specs[i].tier;
        memcpy(blob + i * 10u + 2, &specs[i].seed, 8);
        made++;
    }

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = first;                  /* the batch's first region; the
                                          * blob carries the full span */
    rec.ptype  = IFL_T_REGION;
    rec.flags  = IFL_F_APPLIED;
    packed = ifl__arena_put(w, blob, blob_len);
    rec.value[0] = (uint32_t)(packed >> 32);
    rec.value[1] = (uint32_t)(packed & 0xFFFFFFFFu);
    rec.aux = last;                      /* span end, for the audit walk */
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        vc.c[src] += 1;
        rec.vc = vc;
    }
    if (!ifl__append(w, &rec)) { free(blob); return made ? made - 1 : 0; }
    free(blob);
    return made;
}

uint16_t ifl_region_find(ifl_world* w, const char* name)
{
    uint16_t i;
    if (!w || !name) return 0;
    for (i = 0; i < w->n_regions; i++)
        if (strcmp(w->regions[i].name, name) == 0) return (uint16_t)(i + 1);
    return 0;
}

ifl_region* ifl_region_info(ifl_world* w, uint16_t rid)
{
    return ifl__region(w, rid);
}

bool ifl_region_set_tier(ifl_world* w, ifl_sid src, uint16_t rid,
                         ifl_tier tier)
{
    ifl_intent rec;
    ifl_region* r = ifl__region(w, rid);
    if (!w || !r || src >= w->n_sources) return false;
    if (tier != IFL_TIER_FORK && tier != IFL_TIER_MERGE &&
        tier != IFL_TIER_STRICT) return false;

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = rid;
    rec.ptype  = IFL_T_TIER;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = (uint32_t)tier;
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        vc.c[src] += 1;
        rec.vc = vc;
    }
    if (!ifl__append(w, &rec)) return false;
    r->tier = tier;                      /* working-set cache of the fold */
    return true;
}

/* ============================================================
 * The main entry: submit
 * ============================================================ */

uint32_t ifl_submit(ifl_world* w, ifl_sid src, uint16_t rid,
                    uint8_t ptype, uint64_t value,
                    const ifl_submit_opts* opts)
{
    ifl_submit_opts o;
    ifl_intent rec;
    ifl_region* r;
    const ifl_payload_type* ty;
    bool structural;
    uint32_t iid;

    if (!w || src >= w->n_sources) return 0;

    if (opts) o = *opts;
    else { memset(&o, 0, sizeof o); o.observed_cut = 0; }

    structural = (ptype == IFL_T_TIER || ptype == IFL_T_DELEG ||
                  ptype == IFL_T_RECEIPT || ptype == IFL_T_CURSOR ||
                  ptype == IFL_T_EXPORT || ptype == IFL_T_ARBIT ||
                  ptype == IFL_T_MERGE || ptype == IFL_T_REGION);

    /*
     * Structural records are exempt from the region contract:
     * CURSOR/DELEG/RECEIPT are world-scoped (rid 0) and carry no
     * region; TIER/ARBIT/MERGE/EXPORT/REGION are region-scoped and
     * must name one. The split is on the payload family's
     * registered role — never on the source's kind (I1).
     */
    {
        bool global = (ptype == IFL_T_CURSOR || ptype == IFL_T_DELEG ||
                       ptype == IFL_T_RECEIPT);
        r = ifl__region(w, rid);
        if (!r && !(global && rid == 0)) return 0;
    }
    /* Only WORLD payloads need a registry entry; the structural
     * families (REGION/TIER/DELEG/RECEIPT/CURSOR/EXPORT/ARBIT/
     * MERGE) are the core's own records and register nothing —
     * the open registry governs extension types, not the core's
     * bookkeeping. */
    ty = ifl__type(w, ptype);
    if (!ty && !structural) return 0;

    (void)0; /* structural is computed above, before first use */

    /*
     * Structural families (TIER/DELEG/RECEIPT/CURSOR/EXPORT/ARBIT/
     * MERGE) are exempt from the world-payload gates — they ARE the
     * gates' records. World payloads (NUM/SET/SEQ/STR/REGION/USER)
     * pass the authority and budget checks below. This split is on
     * the payload REGISTRY's structural role, never on the source's
     * kind — kind never dispatches (I1). (The flag itself is
     * computed above, before its first use.)
     */

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = rid;
    rec.ptype  = ptype;
    rec.commit = o.commit;
    rec.value[0] = (uint32_t)(value & 0xFFFFFFFFu);
    rec.value[1] = (uint32_t)(value >> 32);

    /* Causal coordinate: the submitter's declared knowledge, plus
     * this record's own increment. Knowledge flows only through
     * explicit channels — o.observed_cut IS that channel. */
    {
        ifl_vc vc = ifl__vc_upto(w, o.observed_cut);
        uint32_t own = vc.c[src];
        if (w->svc[src].c[src] > own) own = w->svc[src].c[src];
        vc.c[src] = own + 1;
        rec.vc = vc;
        w->svc[src] = ifl_vc_join(w->svc[src], vc);
    }

    /* REBORN: a submission by a source that has undone something
     * runs on re-based living knowledge (ifl_undo_nth resets its
     * service clock below the revoked record's slot). Its own-source
     * slot value can numerically collide with the revoked record's
     * without containment — the exemption is decided HERE, at
     * submission, and recorded in the flags for every later fold. */
    if (w->now.n_excl) {
        uint32_t k;
        for (k = 0; k < w->now.n_excl; k++) {
            const ifl_intent* e = &w->log[w->now.excl[k] - 1];
            if (e->src == src) { rec.flags |= IFL_F_REBORN; break; }
        }
    }

    /* Authority: the payload's registered bit must be held. */
    if (!structural) {
        if (ty->authority_bit &&
            !(w->sources[src].authority & ty->authority_bit)) {
            w->stats.denied_authority++;
            /* The black box records the attempt (the log is the only
             * history structure); the fold never sees it. */
            rec.flags = 0;
            iid = ifl__append(w, &rec);
            return iid;
        }
    }

    /* Budget: initiative and surprise debits post BEFORE the effect.
     * Overdraft does not fail the call — the submission is logged,
     * the intent is left unapplied, the breaker trips, a receipt is
     * emitted (vol. 5 7.1). Failure and budget breach are the same
     * class of event. */
    if (o.debit_class != IFL_DEBIT_NONE) {
        if (!ifl__post_debit(w, src, o.debit_class, o.severity,
                             w->n_log + 1)) {
            rec.flags = 0;
            iid = ifl__append(w, &rec);
            w->now.cut = w->n_log;
            return iid;
        }
        rec.debit = (uint16_t)o.severity;
    }

    iid = ifl__append(w, &rec);
    if (!iid) return 0;

    if (!structural) {
        (void)ifl__adjudicate(w, &w->log[iid - 1]);
    } else {
        w->log[iid - 1].flags |= IFL_F_APPLIED;
        /* Structural records update the working-set caches they own. */
        if (r) {
            switch (ptype) {
            case IFL_T_TIER:
                r->tier = (ifl_tier)rec.value[0];
                break;
            case IFL_T_MERGE:
                /* Handled by ifl_merge_intent's dedicated path. */
                break;
            default:
                break;
            }
        }
    }

    w->now.cut = w->n_log;               /* "now" follows the black box;
                                          * excl/sel survive from the view */
    return iid;
}

uint32_t ifl_user_act(ifl_world* w, ifl_sid src, uint16_t rid,
                      uint8_t ptype, uint64_t value)
{
    ifl_submit_opts o;
    memset(&o, 0, sizeof o);
    o.observed_cut = w->n_log;           /* the user's own action is
                                          * submitted with full knowledge —
                                          * the script narrows this when it
                                          * wants concurrency */
    return ifl_submit(w, src, rid, ptype, value, &o);
}

/* ============================================================
 * Delegation (S22 / T7)
 * ============================================================ */

bool ifl_delegate(ifl_world* w, ifl_sid delegator, ifl_sid delegate,
                  uint32_t amount)
{
    ifl_ledger* dl;
    ifl_ledger* gl;
    ifl_intent rec;
    uint32_t remaining;

    if (!w || delegator >= w->n_sources || delegate >= w->n_sources)
        return false;
    if (delegator == delegate) return false;
    if (w->sources[delegate].delegated_by != IFL_SID_NULL &&
        w->sources[delegate].delegated_by != delegator)
        return false;                    /* re-delegation to a new chain
                                          * is out of the slice's scope */

    gl = &w->ledgers[delegator];
    dl = &w->ledgers[delegate];

    /* The delegate's ceiling is carved from the delegator's
     * REMAINING balance: spend + already-carved sub-budgets. */
    if (gl->budget != IFL_BUDGET_UNLIMITED) {
        uint64_t cap = gl->budget;
        uint64_t used = (uint64_t)gl->spent_initiative + gl->spent_surprise
                        + gl->carved;
        remaining = used >= cap ? 0 : (uint32_t)(cap - used);
    } else {
        remaining = amount;              /* unlimited delegator: the carve
                                          * is not rationed */
    }
    if (amount > remaining) return false;

    gl->carved += amount;                /* subtracts from the delegator's
                                          * available balance */
    dl->sub_ceiling += amount;           /* the delegate's spending ceiling */
    w->sources[delegate].delegated_by = delegator;

    /* The delegation record carries the delegator's current
     * knowledge — this is how causal knowledge propagates DOWN a
     * chain (the delegate now sees what the delegator saw). */
    w->svc[delegate] = ifl_vc_join(w->svc[delegate], w->svc[delegator]);

    memset(&rec, 0, sizeof rec);
    rec.src    = delegator;
    rec.region = 0;
    rec.ptype  = IFL_T_DELEG;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = amount;
    rec.value[1] = delegate;
    {
        ifl_vc vc = w->svc[delegator];
        uint32_t own = vc.c[delegator];
        vc.c[delegator] = own + 1;
        rec.vc = vc;
        w->svc[delegator] = vc;
    }
    return ifl__append(w, &rec) != 0;
}

/* ============================================================
 * Public N4 primitives (suite surface)
 * ============================================================ */

bool ifl_debit(ifl_world* w, ifl_sid sid, ifl_debit_class c,
               ifl_severity sev, uint32_t reason_iid)
{
    return ifl__post_debit(w, sid, c, sev, reason_iid);
}

uint32_t ifl_receipt_count(ifl_world* w)
{
    return w ? w->n_receipts : 0;
}

const ifl_receipt* ifl_receipt_at(ifl_world* w, uint32_t i)
{
    if (!w || i >= w->n_receipts) return NULL;
    return &w->receipts[i];
}

/* ============================================================
 * Views and the fold (N2)
 * ============================================================ */

ifl_view ifl_now(ifl_world* w)
{
    ifl_view v;
    memset(&v, 0, sizeof v);
    if (!w) return v;
    v = w->now;
    if (v.cut > w->n_log) v.cut = w->n_log;
    return v;
}

/* Is `rec` causally after any excluded root (an undo's dropped
 * subtree)? The descendant test is the vector-clock containment:
 * rec carries e's own increment.
 *
 * The REBORN exemption: a record submitted after ITS OWN undo by
 * the SAME source is re-based on living knowledge — its own-source
 * clock slot was reset below the revoked record's slot value, so
 * the numeric collision with e's slot cannot mean containment. A
 * REBORN record still dies if it carries a DIFFERENT revoked
 * record's component. (Two branches of one source share one clock
 * axis; without this exemption the re-based branch could never
 * escape the revoked one's shadow — the numbers alone cannot
 * tell "after e" from "instead of e".) */
bool ifl__is_excluded(const ifl_world* w, const ifl_intent* rec,
                      const ifl_view* v)
{
    uint32_t i;
    for (i = 0; i < v->n_excl; i++) {
        const ifl_intent* e = &w->log[v->excl[i] - 1];
        if ((rec->flags & IFL_F_REBORN) && rec->src == e->src) continue;
        if (rec->vc.c[e->src] >= e->vc.c[e->src]) return true;
    }
    return false;
}

/* Which fork head does the view address for `rid`? Default: the
 * region's current head (b if forked, else a), WALKED BACK to the
 * chain position at the view's cut — a historical view must see the
 * fork as it stood THEN, not as the region table stands now (a
 * later merge collapses the fork in the working set; the view's
 * fold is not allowed to notice). */
static uint32_t ifl__selected_head(const ifl_world* w, const ifl_region* r,
                                   const ifl_view* v)
{
    uint32_t i;
    uint32_t h;
    for (i = 0; i < v->n_sel; i++)
        if (v->sel[i].rid == (uint16_t)(r - w->regions + 1))
            return v->sel[i].head_iid;
    h = r->head_b ? r->head_b : r->head_a;
    while (h > v->cut && h) {
        const ifl_intent* p = &w->log[h - 1];
        h = p->prov_a;                   /* walk the spine back to the
                                          * chain's tail at this cut */
    }
    return h;
}

/* Is `rec` on the chain the view selected? Walks the prov_a spine
 * from the selected head — O(chain), registered instrument scale. */
static bool ifl__on_selected_chain(const ifl_world* w, uint32_t head,
                                   const ifl_intent* rec)
{
    uint32_t cur = head;
    while (cur) {
        const ifl_intent* h = &w->log[cur - 1];
        if (h == rec) return true;
        cur = h->prov_a;
    }
    return false;
}

static bool ifl__is_value_type(uint8_t ptype)
{
    return ptype != IFL_T_TIER && ptype != IFL_T_DELEG &&
           ptype != IFL_T_RECEIPT && ptype != IFL_T_CURSOR &&
           ptype != IFL_T_EXPORT && ptype != IFL_T_ARBIT;
    /* REGION participates (it seeds), MERGE participates (it is the
     * stitched value). */
}

uint64_t ifl_state_at(ifl_world* w, uint16_t rid, const ifl_view* v,
                      uint32_t* contrib)
{
    ifl_region* r = ifl__region(w, rid);
    const ifl_payload_type* ty;
    ifl_view def;
    uint64_t acc = 0;
    uint32_t n = 0, i;
    bool lattice;
    uint32_t head = 0;
    bool forked;

    if (!r) { if (contrib) *contrib = 0; return 0; }
    if (!v) { def = ifl_now(w); v = &def; }
    ty = ifl__type(w, r->ptype);
    lattice = ty && ty->sq;
    forked = r->head_b != 0 && !lattice;
    if (forked) head = ifl__selected_head(w, r, v);

    for (i = 0; i < v->cut && i < w->n_log; i++) {
        const ifl_intent* rec = &w->log[i];
        if (rec->region != rid) continue;
        if (!(rec->flags & IFL_F_APPLIED)) continue;
        if (rec->flags & IFL_F_EXCLUDED) continue;
        if (!ifl__is_value_type(rec->ptype)) continue;
        if (ifl__is_excluded(w, rec, v)) continue;
        if (forked && !ifl__on_selected_chain(w, head, rec)) continue;

        if (lattice && ty->sq) {
            uint64_t out;
            (void)ty->sq(acc, ((uint64_t)rec->value[1] << 32) | rec->value[0], &out);
            acc = out;
            n++;
        } else {
            /* Rigid region: last record on the selected chain wins. */
            acc = ((uint64_t)rec->value[1] << 32) | rec->value[0];
            n++;
        }
    }
    if (contrib) *contrib = n;
    return acc;
}

void ifl_prov_walk(ifl_world* w, uint16_t rid, const ifl_view* v,
                   ifl_prov_cb cb, void* user)
{
    ifl_view def;
    ifl_region* r = ifl__region(w, rid);
    uint32_t i;

    if (!w || !r || !cb) return;
    if (!v) { def = ifl_now(w); v = &def; }

    /* Newest first: the audit walk reads backwards — the newest
     * contributor is the one the attribution task asks about first. */
    for (i = v->cut < w->n_log ? v->cut : w->n_log; i > 0; i--) {
        const ifl_intent* rec = &w->log[i - 1];
        if (rec->region != rid) continue;
        if (!(rec->flags & IFL_F_APPLIED)) continue;
        if (rec->flags & IFL_F_EXCLUDED) continue;
        if (!ifl__is_value_type(rec->ptype)) continue;
        if (ifl__is_excluded(w, rec, v)) continue;
        cb(rec, user);
    }
}

/* ============================================================
 * Undo is sugar (N2 / I5)
 * ============================================================ */

ifl_view ifl_view_rewind(ifl_world* w, const ifl_view* v, uint32_t iid)
{
    ifl_view out;
    ifl_view def;
    if (!v) { def = ifl_now(w); v = &def; }
    out = *v;

    if (iid == 0 || iid > w->n_log) return out;   /* no-op address */
    if (out.n_excl < IFL_MAX_SELECTIONS)
        out.excl[out.n_excl++] = iid;

    /* Fork heads reselect to the latest surviving heads: a head that
     * was itself dropped hands the selection to its chain parent. */
    {
        uint32_t i;
        for (i = 0; i < w->n_regions; i++) {
            ifl_region* r = &w->regions[i];
            uint32_t sel = 0, s;
            for (s = 0; s < out.n_sel; s++)
                if (out.sel[s].rid == i + 1) sel = out.sel[s].head_iid;
            if (r->head_b && (!sel || ifl__is_excluded(w, &w->log[sel - 1], &out)))
                out.sel[out.n_sel ? out.n_sel - 1 : 0].rid = out.n_sel
                    ? out.sel[out.n_sel - 1].rid : (uint16_t)(i + 1);
        }
    }
    return out;
}

ifl_view ifl_view_at(ifl_world* w, uint32_t cut)
{
    ifl_view v;
    memset(&v, 0, sizeof v);
    if (!w) return v;
    v.cut = cut > w->n_log ? w->n_log : cut;
    /* A historical view folds without any undo exclusions: the
     * abandoned branch stays addressable exactly as it happened. */
    return v;
}

uint32_t ifl_undo_nth(ifl_world* w, ifl_sid src, uint32_t iid)
{
    ifl_view rewound;
    ifl_intent rec;
    uint32_t newiid;

    if (!w || src >= w->n_sources) return 0;
    if (iid == 0 || iid > w->n_log) return w->now.cut;

    /* THE SUGAR, in full: exactly the three manual steps of the
     * pure-projection path (ADR-0025 Decision 4d; C7's differential
     * demand), composed:
     *   1. compute the rewound slice (ifl_view_rewind)
     *   2. emit the CURSOR witness (who moved "now", from where)
     *   3. adopt the rewound view (ifl_view_adopt)
     * Nothing else happens here. The calibration suite drives the
     * manual three steps against this function and requires
     * byte-identical worlds. */
    rewound = ifl_view_rewind(w, &w->now, iid);

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = 0;
    rec.ptype  = IFL_T_CURSOR;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = iid;                  /* the moved-past record */
    rec.value[1] = w->now.cut;           /* the previous cut */
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        uint32_t own = vc.c[src];
        vc.c[src] = own + 1;
        rec.vc = vc;
        w->svc[src] = vc;
    }
    rec.prov_a = iid;
    newiid = ifl__append(w, &rec);
    if (!newiid) return 0;

    if (!ifl_view_adopt(w, &rewound)) return 0;
    w->stats.cursor_moves++;
    return w->now.cut;
}

bool ifl_view_adopt(ifl_world* w, const ifl_view* v)
{
    uint32_t i;

    if (!w || !v) return false;

    /* Adopt: the view becomes "now" (clamped to the log — a view
     * cannot see past the black box). */
    w->now = *v;
    if (w->now.cut > w->n_log) w->now.cut = w->n_log;

    /* Re-base every REVOKING source's service clock on the adopted
     * view's living knowledge: the source's NEXT submission extends
     * the rewound point, not the abandoned branch (vol. 5: "new
     * intents after a rewind extend the rewound point"). Without
     * this, the source's own clock slot would stay at the abandoned
     * branch's peak and every later submission would numerically
     * contain the revoked record. Other sources' svc is left alone:
     * their stale knowledge is real (they have not been told), and
     * the living-knowledge rule in ifl__vc_upto already keeps their
     * NEXT declared-knowledge join clean. */
    for (i = 0; i < w->now.n_excl; i++) {
        ifl_sid rs = w->log[w->now.excl[i] - 1].src;
        w->svc[rs] = ifl__vc_upto(w, w->n_log);
    }
    return true;
}

/* ============================================================
 * Tier-1 stitching (explicit merge-intent)
 * ============================================================ */

uint32_t ifl_merge_intent(ifl_world* w, ifl_sid src, uint16_t rid,
                          uint32_t head_a, uint32_t head_b,
                          uint64_t stitched)
{
    ifl_region* r = ifl__region(w, rid);
    ifl_intent rec;
    uint32_t iid;

    if (!w || !r || src >= w->n_sources) return 0;
    if (head_a == 0 || head_a > w->n_log) return 0;
    if (head_b == 0 || head_b > w->n_log) return 0;
    if (w->log[head_a - 1].region != rid || w->log[head_b - 1].region != rid)
        return 0;

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = rid;
    rec.ptype  = IFL_T_MERGE;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = (uint32_t)(stitched & 0xFFFFFFFFu);
    rec.value[1] = (uint32_t)(stitched >> 32);
    rec.aux = 1;                         /* policy tag: manual stitch */
    {
        ifl_vc vc = ifl_vc_join(w->log[head_a - 1].vc, w->log[head_b - 1].vc);
        uint32_t own = vc.c[src];
        vc.c[src] = own + 1;
        rec.vc = vc;
        w->svc[src] = vc;
    }
    rec.prov_a = head_a;                 /* dual derivation (I6) */
    rec.prov_b = head_b;
    iid = ifl__append(w, &rec);
    if (!iid) return 0;

    /* The fork collapses to the merged head; both parents stay in
     * the log and in the black box. */
    r->head_a = iid;
    r->head_b = 0;
    r->last_iid = iid;
    w->stats.merge_intents++;
    w->now.cut = w->n_log;
    return iid;
}

/* ============================================================
 * Black box (S3)
 * ============================================================ */

void ifl_blackbox_configure(ifl_world* w, uint16_t rid, uint16_t window)
{
    ifl_region* r = ifl__region(w, rid);
    if (r) r->window = window;
}

uint32_t ifl_blackbox_export(ifl_world* w, ifl_sid src, uint16_t rid,
                             char* buf, uint32_t cap)
{
    ifl_region* r = ifl__region(w, rid);
    uint32_t pos = 0, i;
    uint32_t from;
    ifl_intent rec;

    if (!w || !r || src >= w->n_sources || !buf || cap == 0) return 0;

    /* The window bounds what an EXPORT serializes — the log itself
     * is retained in full (E2's registered measurement mode). */
    from = r->window && r->window < w->n_log ? w->n_log - r->window : 0;

    for (i = from; i < w->n_log; i++) {
        const ifl_intent* q = &w->log[i];
        if (q->region != rid && q->ptype != IFL_T_REGION) continue;
        pos += ifl__n(buf + pos, cap - pos,
                      "iid=%u src=%u rg=%u ty=%u flags=0x%x "
                      "v=0x%x%08x prov=%u/%u\n",
                      i + 1, q->src, q->region, q->ptype, q->flags,
                      q->value[1], q->value[0], q->prov_a, q->prov_b);
        if (pos >= cap - 1) break;
    }

    memset(&rec, 0, sizeof rec);
    rec.src    = src;
    rec.region = rid;
    rec.ptype  = IFL_T_EXPORT;
    rec.flags  = IFL_F_APPLIED;
    rec.value[0] = from + 1;             /* serialized range start */
    rec.value[1] = w->n_log;             /* range end */
    {
        ifl_vc vc = ifl__vc_upto(w, w->n_log);
        uint32_t own = vc.c[src];
        vc.c[src] = own + 1;
        rec.vc = vc;
        w->svc[src] = vc;
    }
    (void)ifl__append(w, &rec);
    w->now.cut = w->n_log;
    return pos;
}

/* ============================================================
 * Projections — semantic-tree-first (ADR-0025 4g)
 * ============================================================ */

static const char* ifl__tier_name(uint8_t tier)
{
    switch (tier) {
    case IFL_TIER_FORK:   return "fork";
    case IFL_TIER_MERGE:  return "merge";
    case IFL_TIER_STRICT: return "strict";
    default:              return "?";
    }
}

static const char* ifl__ptype_name(const ifl_world* w, uint8_t ptype)
{
    if (ptype < w->n_types) return w->types[ptype].name;
    return "?";
}

uint32_t ifl_project_semantic(ifl_world* w, const ifl_view* v,
                              char* buf, uint32_t cap)
{
    ifl_view def;
    uint32_t pos = 0, i;

    if (!w || !buf || cap == 0) return 0;
    if (!v) { def = ifl_now(w); v = &def; }

    pos += ifl__n(buf + pos, cap - pos,
                  "semantic @cut=%u excl=%u\n", v->cut, v->n_excl);
    for (i = 0; i < w->n_regions; i++) {
        ifl_region* r = &w->regions[i];
        uint32_t contrib = 0;
        uint64_t val = ifl_state_at(w, (uint16_t)(i + 1), v, &contrib);
        if (r->head_b)
            pos += ifl__n(buf + pos, cap - pos,
                          "  %-10s tier=%s ptype=%s val=0x%x%08x "
                          "contrib=%u FORK(a=%u,b=%u)\n",
                          r->name, ifl__tier_name(r->tier),
                          ifl__ptype_name(w, r->ptype),
                          (uint32_t)(val >> 32), (uint32_t)val,
                          contrib, r->head_a, r->head_b);
        else
            pos += ifl__n(buf + pos, cap - pos,
                          "  %-10s tier=%s ptype=%s val=0x%x%08x contrib=%u\n",
                          r->name, ifl__tier_name(r->tier),
                          ifl__ptype_name(w, r->ptype),
                          (uint32_t)(val >> 32), (uint32_t)val, contrib);
    }
    return pos;
}

uint32_t ifl_project_set(ifl_world* w, const ifl_view* v,
                         char* buf, uint32_t cap)
{
    /* The set-state projection (S9): groups regions by semantic
     * proximity (their payload family) and ignores the total order —
     * order-free by construction, stable by iteration rule. */
    ifl_view def;
    uint32_t pos = 0, t, i;

    if (!w || !buf || cap == 0) return 0;
    if (!v) { def = ifl_now(w); v = &def; }

    pos += ifl__n(buf + pos, cap - pos, "set @cut=%u\n", v->cut);
    for (t = 0; t < w->n_types; t++) {
        bool any = false;
        uint32_t start = pos;
        for (i = 0; i < w->n_regions; i++) {
            ifl_region* r = &w->regions[i];
            uint32_t contrib = 0;
            uint64_t val;
            if (r->ptype != t) continue;
            val = ifl_state_at(w, (uint16_t)(i + 1), v, &contrib);
            pos += ifl__n(buf + pos, cap - pos, "  %-10s 0x%x%08x (%u)\n",
                          r->name, (uint32_t)(val >> 32), (uint32_t)val,
                          contrib);
            any = true;
        }
        if (!any) pos = start;           /* empty group: not printed */
        else {
            char hdr[64];
            uint32_t hl = ifl__n(hdr, sizeof hdr, "group %s\n",
                                 ifl__ptype_name(w, (uint8_t)t));
            if (pos + hl < cap) {
                /* Move the group body right, drop the header in. The
                 * body's NUL terminator moves WITH it only if we move
                 * pos-start+1 bytes — or we re-terminate here, which
                 * is the simpler guarantee. */
                memmove(buf + start + hl, buf + start, pos - start);
                memcpy(buf + start, hdr, hl);
                pos += hl;
                buf[pos] = '\0';
            }
        }
    }
    return pos;
}

uint32_t ifl_project_history(ifl_world* w, const ifl_view* v,
                             char* buf, uint32_t cap)
{
    /* The history projection (S9): the strict causal expansion —
     * every record up to the cut, in log order, with its causal
     * coordinate. */
    ifl_view def;
    uint32_t pos = 0, i;

    if (!w || !buf || cap == 0) return 0;
    if (!v) { def = ifl_now(w); v = &def; }

    pos += ifl__n(buf + pos, cap - pos, "history @cut=%u excl=%u\n",
                  v->cut, v->n_excl);
    for (i = 0; i < v->cut && i < w->n_log; i++) {
        const ifl_intent* q = &w->log[i];
        pos += ifl__n(buf + pos, cap - pos,
                      "  #%u src=%u rg=%u ty=%s t=%u vc=[%u,%u,%u,%u] "
                      "flags=0x%x prov=%u/%u\n",
                      i + 1, q->src, q->region,
                      ifl__ptype_name(w, q->ptype), q->t,
                      q->vc.c[0], q->vc.c[1], q->vc.c[2], q->vc.c[3],
                      q->flags, q->prov_a, q->prov_b);
        if (pos >= cap - 1) break;
    }
    return pos;
}

/* ============================================================
 * Instrumentation (E2 / I7)
 * ============================================================ */

const ifl_stats* ifl_stats_of(ifl_world* w) { return w ? &w->stats : NULL; }

const ifl_intent* ifl_log_at(ifl_world* w, uint32_t iid)
{
    if (!w || iid == 0 || iid > w->n_log) return NULL;
    return &w->log[iid - 1];
}

uint32_t ifl_log_count(ifl_world* w) { return w ? w->n_log : 0; }

uint64_t ifl_history_bytes(ifl_world* w)
{
    /*
     * Every byte retained solely because of N2 (ADR-0025 4f):
     * the intent records THEMSELVES (n_log × 48 — the registered
     * accounting unit), the payload arena's live bytes, and the
     * receipt store's live bytes.
     *
     * Adjudication note (registered here, at the first E2 run):
     * the growth policy's reserved CAPACITY (cap_log etc.) is the
     * allocator's contribution, not N2's — an exact-fit allocator
     * would hold cap == n, so counting caps would price the
     * realloc strategy, not the ontology. Records, arena bytes,
     * receipts: those are what history IS. Deterministic by
     * construction: no RSS, no allocator noise — any checkout
     * recomputes this.
     */
    if (!w) return 0;
    return (uint64_t)w->n_log * sizeof(ifl_intent)
         + w->n_arena
         + (uint64_t)w->n_receipts * sizeof(ifl_receipt);
}

uint64_t ifl_working_set_bytes(ifl_world* w)
{
    /*
     * The live state (ADR-0025 4f): the world structure itself —
     * whose dominant member is the region table at its registered
     * capacity (the 1000 rows in the E2 leg's case), plus the
     * source/ledger/registry tables every fold needs. The struct
     * IS the working set: no log bytes, no arena bytes — those are
     * history. (Counting n_regions × sizeof(ifl_region) on top of
     * sizeof(ifl_world) would double-count the table: the array
     * lives inside the struct.)
     */
    if (!w) return 0;
    return sizeof(ifl_world)
         + (uint64_t)w->n_types * sizeof(ifl_payload_type);
}
