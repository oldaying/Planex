/*
 * if5_snapshot.c — the persistence of addresses (ADR-0028
 * Decision 4): a canonical, versioned byte format for a world,
 * and the ONE rebuild pass that turns an installed log back into
 * a living world.
 *
 * The design line this module lives on:
 *
 *   SAVE  = the durable truth VERBATIM (the log and the arena —
 *           the same 48-byte words the differential freezes) +
 *           the seeds the log cannot carry (source identities,
 *           the type table as names, region names/config) +
 *           the audit stream (receipts) + the address book (the
 *           cut registry, with names) + the cursor. Canonical:
 *           the same world saves to the same file every time —
 *           no timestamps, no allocator-dependent bytes (F8).
 *
 *   LOAD  = verbatim install + if5__rebuild_caches. The records
 *           are installed, never re-adjudicated: their flags,
 *           PROV edges, and clocks ARE the decisions. The
 *           rebuild replays the BOOKKEEPING in one ordered pass
 *           — knowledge clocks, ledger arithmetic, region
 *           caches, fork heads, path counters — reading each
 *           record's own bytes and writing no decision byte.
 *           Every rebuildable cache is then cross-checked
 *           against the persisted values; a divergence is a
 *           REFUSED load, never a wrong world (the loader's one
 *           answer to corruption and to out-of-alphabet worlds
 *           alike — see the Known issues in ADR-0028).
 *
 * The black-box export format is a registered surface and is NOT
 * touched by any of this (F7 asserts its bytes across a round
 * trip); the snapshot is a new artifact, not a revision.
 *
 * Not a trust boundary (ADR-0028 CAVEATS): the loader validates
 * structure — bounds, arena offsets, prune ranges, type-table
 * identity — and refuses the malformed; a well-formed but lying
 * file can still install a lying log. The writer's honesty is
 * the F-series' job; the reader's honesty is refusal.
 *
 * Lineage: R3 (ADR-0028); no slice ancestor — persistence was
 * the registered deferral ADR-0026 CAVEATS handed down and
 * ADR-0027 §7 re-registered for this stage.
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "if5_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

/* ============================================================
 * The format (version 1, little-endian, fixed layout)
 * ============================================================
 *
 *   [8]   "IF5SNAP1"                     magic, no NUL
 *   u32   version (= 1)
 *   u32   tick                           (cross-check: n_log + 1)
 *   u32   n_log
 *   u32   n_arena
 *   u32   n_receipts
 *   u16   n_regions
 *   u8    n_sources
 *   u8    n_cuts
 *   u8    sys_sid                        (IF5_SID_NULL = none)
 *   u8    n_types
 *   u8    pad[3]
 *   -- the cursor ------------------------------------------------
 *   u32   cut; u32 n_excl; u32 excl[8]
 *   u32   n_sel; 8 x (u16 rid; u16 pad; u32 head_iid)
 *   -- the stats cross-check (13 x u32) ---------------------------
 *   -- sources: n_sources x ---------------------------------------
 *         id[16]; u8 kind; u8 trust; u8 pad[2];
 *         u32 initiative_budget; u32 authority;
 *         u32 spent_initiative; u32 spent_surprise;
 *         u32 sub_ceiling; u32 carved; u8 breaker; u8 pad[3]
 *   -- types: n_types x { name[32] } ------------------------------
 *   -- regions: n_regions x ---------------------------------------
 *         name[IF5_NAME_MAX]; u8 ptype; u8 tier;
 *         u16 window; u16 parent
 *   -- the log: n_log x 48 bytes, field-canonical LE --------------
 *         vc.c[4]; value[2]; prov_a; prov_b; t; aux;
 *         u16 debit; u16 region; src; ptype; flags; commit
 *   -- the arena: n_arena bytes verbatim --------------------------
 *   -- receipts: n_receipts x -------------------------------------
 *         u8 origin; u8 hop; u16 pad; u32 reason_iid;
 *         u32 cost_initiative; u32 cost_surprise;
 *         u32 rollback_cut; u32 checkpoint
 *   -- cuts: n_cuts x ---------------------------------------------
 *         frontier.c[4]; u32 prefix; u32 prunes[8]; u32 n_prunes;
 *         name[IF5_NAME_MAX]
 *
 * The arena is opaque payload (its blob layouts are the owning
 * records' business — the batch seed blob is parsed with the same
 * native reads the core's own loader path uses); every structured
 * field above is explicit little-endian. Region creation seeds
 * are NOT carried: the replay derives them from the batch blobs
 * in the installed arena (the log is the source of truth; a
 * persisted copy would be a second truth).
 */

#define IF5_SNAP_MAGIC       "IF5SNAP1"
#define IF5_SNAP_VERSION     1u
#define IF5_SNAP_TYPE_NAME   32u
#define IF5_SNAP_SRC_ID      16u

/* ============================================================
 * Canonical little-endian field codec
 * ============================================================ */

typedef struct {
    uint8_t* p;
    uint32_t n, cap;
    bool     oom;
} buf;

static void bput(buf* b, const void* d, uint32_t n)
{
    if (b->oom) return;
    if (b->n + n > b->cap) {
        uint32_t nc = b->cap ? b->cap : 1024;
        uint8_t* q;
        while (nc < b->n + n) nc *= 2;
        q = (uint8_t*)realloc(b->p, nc);
        if (!q) { b->oom = true; return; }
        b->p = q;
        b->cap = nc;
    }
    memcpy(b->p + b->n, d, n);
    b->n += n;
}

static void b_u8(buf* b, uint8_t v)   { bput(b, &v, 1); }

static void b_u16(buf* b, uint16_t v)
{
    uint8_t d[2] = { (uint8_t)(v & 0xFFu), (uint8_t)(v >> 8) };
    bput(b, d, 2);
}

static void b_u32(buf* b, uint32_t v)
{
    uint8_t d[4] = { (uint8_t)(v & 0xFFu), (uint8_t)((v >> 8) & 0xFFu),
                     (uint8_t)((v >> 16) & 0xFFu), (uint8_t)((v >> 24) & 0xFFu) };
    bput(b, d, 4);
}

static void b_name(buf* b, const char* s, uint32_t cap)
{
    uint8_t zero[64];
    uint32_t nl = s ? (uint32_t)strlen(s) : 0;
    if (nl >= cap) nl = cap - 1;
    memset(zero, 0, sizeof zero);
    if (s) bput(b, s, nl);
    bput(b, zero, cap - nl);
}

/* The read side: a cursor over the installed image. */

typedef struct {
    const uint8_t* p;
    uint32_t n, pos;
    bool    bad;                    /* read past end / bad size */
} cur;

static uint32_t c_left(const cur* c) { return c->n - c->pos; }

static const uint8_t* c_take(cur* c, uint32_t n)
{
    const uint8_t* r;
    if (c->bad || c_left(c) < n) { c->bad = true; return NULL; }
    r = c->p + c->pos;
    c->pos += n;
    return r;
}

static uint8_t c_u8(cur* c)
{
    const uint8_t* d = c_take(c, 1);
    return d ? d[0] : 0;
}

static uint16_t c_u16(cur* c)
{
    const uint8_t* d = c_take(c, 2);
    if (!d) return 0;
    return (uint16_t)((uint16_t)d[0] | ((uint16_t)d[1] << 8));
}

static uint32_t c_u32(cur* c)
{
    const uint8_t* d = c_take(c, 4);
    if (!d) return 0;
    return (uint32_t)d[0] | ((uint32_t)d[1] << 8) |
           ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
}

/* NUL-padded name read: copies at most cap-1 chars + terminator. */
static void c_name(cur* c, char* dst, uint32_t cap)
{
    const uint8_t* d = c_take(c, cap);
    uint32_t i;
    if (!d) { dst[0] = 0; return; }
    for (i = 0; i < cap - 1; i++) dst[i] = (char)d[i];
    dst[cap - 1] = 0;
    /* the stored name must actually terminate inside the field */
    for (i = 0; i < cap - 1; i++) if (dst[i] == 0) return;
    dst[0] = 0;
    for (i = 0; i < cap; i++) if (d[i] != 0) { c->bad = true; return; }
}

static void diagf(char* diag, uint32_t cap, const char* fmt, ...)
{
    if (!diag || cap == 0) return;
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(diag, cap, fmt, ap);
        va_end(ap);
    }
}

/* ============================================================
 * Save
 * ============================================================ */

static void put_record(buf* b, const if5_intent* r)
{
    int i;
    for (i = 0; i < IF5_MAX_SOURCES; i++) b_u32(b, r->vc.c[i]);
    b_u32(b, r->value[0]);
    b_u32(b, r->value[1]);
    b_u32(b, r->prov_a);
    b_u32(b, r->prov_b);
    b_u32(b, r->t);
    b_u32(b, r->aux);
    b_u16(b, r->debit);
    b_u16(b, r->region);
    b_u8(b, r->src);
    b_u8(b, r->ptype);
    b_u8(b, r->flags);
    b_u8(b, r->commit);
}

static void put_view(buf* b, const if5_view* v)
{
    int i;
    b_u32(b, v->cut);
    b_u32(b, v->n_excl);
    for (i = 0; i < IF5_MAX_SELECTIONS; i++) b_u32(b, v->excl[i]);
    b_u32(b, v->n_sel);
    for (i = 0; i < IF5_MAX_SELECTIONS; i++) {
        b_u16(b, v->sel[i].rid);
        b_u16(b, 0);
        b_u32(b, v->sel[i].head_iid);
    }
}

static void put_stats(buf* b, const if5_stats* s)
{
    b_u32(b, s->submits);
    b_u32(b, s->causal_applies);
    b_u32(b, s->concurrent_arrivals);
    b_u32(b, s->lattice_checks);
    b_u32(b, s->tier2_merges);
    b_u32(b, s->tier1_forks);
    b_u32(b, s->tier3_arbitrations);
    b_u32(b, s->system_merge_debits);
    b_u32(b, s->breakers);
    b_u32(b, s->receipts);
    b_u32(b, s->excluded);
    b_u32(b, s->merge_intents);
    b_u32(b, s->cursor_moves);
    b_u32(b, s->denied_authority);
}

int if5_snapshot_save(if5_world* w, const char* path,
                      char* diag, uint32_t diag_cap)
{
    buf b;
    FILE* f;
    uint32_t i;
    int rc = IF5_SNAP_OK;

    if (!w || !path) return IF5_SNAP_ERR_IO;
    memset(&b, 0, sizeof b);

    bput(&b, IF5_SNAP_MAGIC, 8);
    b_u32(&b, IF5_SNAP_VERSION);
    b_u32(&b, w->tick);
    b_u32(&b, w->n_log);
    b_u32(&b, w->n_arena);
    b_u32(&b, w->n_receipts);
    b_u16(&b, w->n_regions);
    b_u8(&b, w->n_sources);
    b_u8(&b, w->n_cuts);
    b_u8(&b, w->sys_sid);
    b_u8(&b, w->n_types);
    b_u8(&b, 0); b_u8(&b, 0); b_u8(&b, 0);

    put_view(&b, &w->now);
    put_stats(&b, &w->stats);

    for (i = 0; i < w->n_sources; i++) {
        const if5_source* s = &w->sources[i];
        const if5_ledger* l = &w->ledgers[i];
        bput(&b, s->id, IF5_SNAP_SRC_ID);
        b_u8(&b, (uint8_t)s->kind);
        b_u8(&b, s->trust);
        b_u8(&b, 0); b_u8(&b, 0);
        b_u32(&b, s->initiative_budget);
        b_u32(&b, s->authority);
        /* the ledger's live state: spent is authoritative (the
         * standalone-debit residue is log-invisible — registered
         * in ADR-0028's Validation); carved/sub/breaker are
         * rebuild-exact and cross-checked at load. */
        b_u32(&b, l->spent_initiative);
        b_u32(&b, l->spent_surprise);
        b_u32(&b, l->sub_ceiling);
        b_u32(&b, l->carved);
        b_u8(&b, l->breaker_tripped ? 1 : 0);
        b_u8(&b, 0); b_u8(&b, 0); b_u8(&b, 0);
    }

    for (i = 0; i < w->n_types; i++)
        b_name(&b, w->types[i].name, IF5_SNAP_TYPE_NAME);

    for (i = 0; i < w->n_regions; i++) {
        const if5_region* r = &w->regions[i];
        b_name(&b, r->name, IF5_NAME_MAX);
        b_u8(&b, r->ptype);
        b_u8(&b, r->tier);
        b_u16(&b, r->window);
        b_u16(&b, r->parent);
    }

    for (i = 0; i < w->n_log; i++)
        put_record(&b, &w->log[i]);

    if (w->n_arena) bput(&b, w->arena, w->n_arena);

    for (i = 0; i < w->n_receipts; i++) {
        const if5_receipt* r = &w->receipts[i];
        b_u8(&b, r->origin);
        b_u8(&b, r->hop);
        b_u16(&b, 0);
        b_u32(&b, r->reason_iid);
        b_u32(&b, r->cost_initiative);
        b_u32(&b, r->cost_surprise);
        b_u32(&b, r->rollback_cut);
        b_u32(&b, r->checkpoint);
    }

    for (i = 0; i < w->n_cuts; i++) {
        const if5_cut* c = &w->cuts[i];
        int k;
        for (k = 0; k < IF5_MAX_SOURCES; k++) b_u32(&b, c->frontier.c[k]);
        b_u32(&b, c->prefix);
        for (k = 0; k < IF5_MAX_SELECTIONS; k++) b_u32(&b, c->prunes[k]);
        b_u32(&b, c->n_prunes);
        b_name(&b, w->cut_names[i], IF5_NAME_MAX);
    }

    if (b.oom) { free(b.p); return IF5_SNAP_ERR_IO; }

#ifdef _WIN32
    f = fopen(path, "wb");
    if (!f) {
        diagf(diag, diag_cap, "cannot open '%s' for writing", path);
        free(b.p);
        return IF5_SNAP_ERR_IO;
    }
#else
    /* explicit 0644, umask-independent (the shipping fb.c
     * precedent; CodeQL cpp/world-writable-file-creation) */
    {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            diagf(diag, diag_cap, "cannot open '%s' for writing", path);
            free(b.p);
            return IF5_SNAP_ERR_IO;
        }
        f = fdopen(fd, "wb");
        if (!f) {
            close(fd);
            diagf(diag, diag_cap, "cannot wrap '%s'", path);
            free(b.p);
            return IF5_SNAP_ERR_IO;
        }
    }
#endif
    if (fwrite(b.p, 1, b.n, f) != b.n) {
        diagf(diag, diag_cap, "short write on '%s'", path);
        rc = IF5_SNAP_ERR_IO;
    }
    if (fclose(f) != 0 && rc == IF5_SNAP_OK) {
        diagf(diag, diag_cap, "close failed on '%s'", path);
        rc = IF5_SNAP_ERR_IO;
    }
    free(b.p);
    return rc;
}

/* ============================================================
 * The rebuild pass (ADR-0028 Decision 4)
 * ============================================================ */

/* Is a record's payload family structural (the core's own
 * bookkeeping, never a fold participant)? Mirrors if5_submit's
 * list — the replay's alphabet. */
static bool is_structural(uint8_t ptype)
{
    return ptype == IF5_T_TIER || ptype == IF5_T_DELEG ||
           ptype == IF5_T_RECEIPT || ptype == IF5_T_CURSOR ||
           ptype == IF5_T_EXPORT || ptype == IF5_T_ARBIT ||
           ptype == IF5_T_MERGE || ptype == IF5_T_REGION ||
           ptype == IF5_T_REPARENT || ptype == IF5_T_PROCESS;
}

static uint64_t rec_value(const if5_intent* r)
{
    return ((uint64_t)r->value[1] << 32) | r->value[0];
}

/*
 * Derive the batch-creation seeds from the installed log: a
 * REGION record NOT immediately followed by a TIER record on the
 * same region is a batch load — its arena blob carries
 * {ptype, tier, seed} per region across rids [region .. aux].
 * Single-created regions seed their cache at 0 (the memset the
 * creator itself ran). The log is the source of truth; nothing
 * seed-shaped is persisted (a persisted copy would be a second
 * truth, and second truths drift).
 */
static void derive_region_seeds(if5_world* w)
{
    uint32_t i;
    uint32_t rid;
    for (i = 0; i < w->n_log; i++) {
        const if5_intent* rec = &w->log[i];
        uint32_t len = 0;
        const uint8_t* blob;
        if (rec->ptype != IF5_T_REGION) continue;
        if (i + 1 < w->n_log &&
            w->log[i + 1].ptype == IF5_T_TIER &&
            w->log[i + 1].region == rec->region)
            continue;                       /* single-create */
        /* batch: the span is [region .. aux] */
        blob = (const uint8_t*)if5__arena_get(
            w, ((uint64_t)rec->value[0] << 32) | rec->value[1], &len);
        if (!blob) continue;
        for (rid = rec->region; rid != 0 && rid <= rec->aux; rid++) {
            if5_region* r = if5__region(w, (uint16_t)rid);
            uint32_t k = rid - rec->region;
            uint64_t seed = 0;
            if (!r) continue;
            if ((k + 1) * 10u > len) break; /* malformed span */
            memcpy(&seed, blob + k * 10u + 2, 8);
            r->cache = seed;
        }
    }
}

/*
 * The pass itself. The world's `now` doubles as the replay's
 * live view (exactly the role it plays during incremental
 * construction), so the shared helpers — if5__vc_upto,
 * if5__is_excluded, if5__region_concurrent_upto — read the
 * historical view state for free. Returns 0 on success.
 */
int if5__rebuild_caches(if5_world* w)
{
    uint32_t i;
    if5_stats st;
    if5_vc vc_zero;
    uint32_t k;
    uint8_t* arbit_newcomer = NULL;   /* the record whose adjudication
                                       * appended an ARBIT witness — its
                                       * path counters land at the
                                       * witness, not at the record */

    memset(&st, 0, sizeof st);
    memset(&vc_zero, 0, sizeof vc_zero);

    if (w->n_log) {
        arbit_newcomer = (uint8_t*)calloc(w->n_log, 1);
        if (!arbit_newcomer) return IF5_SNAP_ERR_IO;
        for (i = 0; i < w->n_log; i++) {
            const if5_intent* rec = &w->log[i];
            uint32_t winner, loser, newcomer;
            if (rec->ptype != IF5_T_ARBIT) continue;
            winner = rec->value[0];
            loser = rec->aux;
            if (winner == 0 || loser == 0 ||
                winner > w->n_log || loser > w->n_log)
                continue;
            /* the newcomer is the later append — the record whose
             * adjudication emitted this witness. The OLD HEAD keeps
             * the counts its own submission earned (they happened
             * before the arbitration); only the newcomer's counting
             * moves to the witness. */
            newcomer = winner > loser ? winner : loser;
            arbit_newcomer[newcomer - 1] = 1;
        }
    }

    /* the replay starts from the empty view */
    w->now.cut = 0;
    w->now.n_excl = 0;
    w->now.n_sel = 0;
    w->vc_now = vc_zero;
    for (k = 0; k < IF5_MAX_SOURCES; k++) w->svc[k] = vc_zero;
    /* ledger spent/carved/sub rebuild from zero (budget ceilings
     * came in with the seeded sources); breaker flags reset */
    for (k = 0; k < IF5_MAX_SOURCES; k++) {
        w->ledgers[k].spent_initiative = 0;
        w->ledgers[k].spent_surprise = 0;
        w->ledgers[k].sub_ceiling = 0;
        w->ledgers[k].carved = 0;
        w->ledgers[k].breaker_tripped = false;
    }
    /* region caches start at their derived seeds; heads clear */
    for (k = 0; k < w->n_regions; k++) {
        w->regions[k].last_iid = 0;
        w->regions[k].head_a = 0;
        w->regions[k].head_b = 0;
    }
    derive_region_seeds(w);

    for (i = 1; i <= w->n_log; i++) {
        const if5_intent* rec = &w->log[i - 1];
        if5_region* r = if5__region(w, rec->region);

        st.submits++;
        w->vc_now = if5_vc_join(w->vc_now, rec->vc);

        switch (rec->ptype) {
        case IF5_T_REGION:
            /* single-create (REGION immediately followed by its
             * TIER record) touches last_iid; batch loads do not */
            if (i < w->n_log &&
                w->log[i].ptype == IF5_T_TIER &&
                w->log[i].region == rec->region &&
                r)
                r->last_iid = i;
            break;

        case IF5_T_TIER:
            /* the working-set tier is installed from the saved
             * live value (a config cache, not a fold cache) */
            break;

        case IF5_T_DELEG: {
            if5_sid delegate = (if5_sid)rec->value[1];
            uint32_t amount = rec->value[0];
            /* if5_delegate's order: the delegate learns the
             * delegator's knowledge, then the record's own vc. */
            if (delegate < w->n_sources)
                w->svc[delegate] = if5_vc_join(w->svc[delegate],
                                               w->svc[rec->src]);
            w->svc[rec->src] = rec->vc;
            if (delegate < w->n_sources) {
                w->ledgers[rec->src].carved += amount;
                w->ledgers[delegate].sub_ceiling += amount;
                w->sources[delegate].delegated_by = rec->src;
            }
            break;
        }

        case IF5_T_RECEIPT: {
            /* Classify by the reason word, exactly as the emit
             * sites wrote it: a PRE-DEBIT failure's receipt points
             * at its own iid (the submission it refuses lands
             * next); a TRAILING tier-2 system-debit failure points
             * at the unapplied newcomer immediately before it
             * (whose flags are still 0); anything else is a
             * standalone if5_debit overdraft. */
            uint32_t reason = rec->value[1];
            bool trailing = false;
            if (reason == i - 1 && i >= 2 &&
                w->log[i - 2].flags == 0 &&
                !is_structural(w->log[i - 2].ptype))
                trailing = true;
            st.breakers++;
            st.receipts++;
            /* the breaker trips on the overdraft this receipt
             * witnesses — the receipt's origin IS the ledger */
            w->ledgers[rec->src].breaker_tripped = true;
            w->svc[rec->src] = rec->vc;
            if (trailing) w->now.cut = i;
            break;
        }

        case IF5_T_CURSOR: {
            /* the undo sugar's three steps, replayed: rewind the
             * view, take the record's vc as the mover's knowledge,
             * adopt (with the revoking sources' rebase). */
            uint32_t pre_cut = w->now.cut;
            if5_view rewound = if5_view_rewind(w, &w->now, rec->value[0]);
            w->svc[rec->src] = rec->vc;
            w->now = rewound;
            if (w->now.cut > w->n_log) w->now.cut = w->n_log;
            for (k = 0; k < w->now.n_excl; k++) {
                if5_sid rs = w->log[w->now.excl[k] - 1].src;
                w->svc[rs] = if5__vc_upto(w, w->n_log);
            }
            st.cursor_moves++;
            if (rec->value[1] != pre_cut) {
                /* the witness disagrees with the replayed view
                 * history — out-of-alphabet cursor use */
                free(arbit_newcomer);
                return IF5_SNAP_ERR_CHECK;
            }
            break;
        }

        case IF5_T_MERGE:
            if (r) {
                r->head_a = i;
                r->head_b = 0;
                r->last_iid = i;
            }
            st.merge_intents++;
            w->svc[rec->src] = rec->vc;
            w->now.cut = i;
            break;

        case IF5_T_EXPORT:
            w->svc[rec->src] = rec->vc;
            w->now.cut = i;
            break;

        case IF5_T_ARBIT: {
            uint32_t winner = rec->value[0];
            uint32_t loser = rec->aux;
            st.tier3_arbitrations++;
            st.concurrent_arrivals++;      /* the newcomer's arrival */
            st.excluded++;                 /* the loser's installed flag */
            if (r && winner > loser) {
                /* the newcomer won: it carries the fold now */
                if (winner >= 1 && winner <= w->n_log) {
                    r->cache = rec_value(&w->log[winner - 1]);
                    r->last_iid = winner;
                }
            }
            w->now.cut = i;
            break;
        }

        case IF5_T_REPARENT:
        case IF5_T_PROCESS:
            /* structural submissions: knowledge join, applied
             * flag already installed, no adjudication. The
             * working-set parent is installed from the saved live
             * value (see TIER above for the same call). */
            w->svc[rec->src] = if5_vc_join(w->svc[rec->src], rec->vc);
            w->now.cut = i;
            break;

        default: {
            /* a value / world payload: the adjudication outcome
             * is IN the record; the replay reads it. */
            const if5_payload_type* ty =
                r ? if5__type(w, r->ptype) : NULL;
            w->svc[rec->src] = if5_vc_join(w->svc[rec->src], rec->vc);

            if (rec->flags & IF5_F_CONCUR_MERGE) {
                st.concurrent_arrivals++;
                st.lattice_checks++;
                st.system_merge_debits++;
                /* the merge flag PROVES the system's background
                 * debit succeeded (a failed debit leaves the
                 * newcomer unapplied) — the one log-invisible
                 * ledger effect, and it is not invisible at all:
                 * the flag is its witness. */
                if (w->sys_sid != IF5_SID_NULL &&
                    w->sys_sid < w->n_sources)
                    w->ledgers[w->sys_sid].spent_initiative += 1;
                st.tier2_merges++;
                if (r) {
                    r->cache = rec_value(rec);  /* the already-joined word */
                    r->last_iid = i;
                }
                w->now.cut = i;
            } else if (rec->flags & IF5_F_APPLIED) {
                if (arbit_newcomer && arbit_newcomer[i - 1]) {
                    /* the tier-3 newcomer: its cache update is the
                     * arbiter's winner bookkeeping (or nothing, for
                     * the loser — but the loser carries EXCLUDED,
                     * not APPLIED), and its path counters are taken
                     * at its witness below. */
                    if (r) {
                        r->cache = rec_value(rec);
                        r->last_iid = i;
                    }
                } else {
                    bool concurrent = r && if5__region_concurrent_upto(
                        w, rec->region, rec->vc, i - 1);
                    if (concurrent) {
                        /* tier-1 fork: heads and last_iid, and NO
                         * cache update — a forked region's working-
                         * set cache stays at the causal fold until
                         * the fork resolves (the fold is chain-
                         * selected; the cache is not; frozen slice
                         * semantics, replicated not repaired) */
                        st.concurrent_arrivals++;
                        st.tier1_forks++;
                        if (r) {
                            if (!r->head_b) r->head_a = r->last_iid;
                            r->head_b = i;
                            r->last_iid = i;
                        }
                    } else {
                        st.causal_applies++;
                        if (r) {
                            uint64_t out = 0;
                            if (ty && ty->sq) {
                                (void)ty->sq(r->cache, rec_value(rec),
                                             &out);
                                r->cache = out;
                            } else {
                                r->cache = rec_value(rec);
                                if (r->head_b) r->head_b = i;  /* chain B extends */
                            }
                            r->last_iid = i;
                        }
                    }
                }
                w->now.cut = i;
            } else if (rec->flags & IF5_F_EXCLUDED) {
                /* the tier-3 loser: its counters were taken at its
                 * ARBIT witness (which follows); nothing here. */
            } else {
                /* unapplied: pre-debit refusal (a receipt pointing
                 * at itself sits immediately before), tier-2
                 * system-debit failure (an unapplied record
                 * immediately before a trailing receipt was already
                 * counted there), or an authority denial. */
                if (i >= 2 &&
                    w->log[i - 2].ptype == IF5_T_RECEIPT &&
                    w->log[i - 2].value[1] == i - 1) {
                    w->now.cut = i;        /* submit's failure path
                                             * still advances the box */
                } else if (i < w->n_log &&
                           w->log[i].ptype == IF5_T_RECEIPT &&
                           w->log[i].value[1] == i) {
                    st.concurrent_arrivals++;
                    st.lattice_checks++;
                } else {
                    st.denied_authority++;
                }
            }
            break;
        }
        }
    }

    w->stats = st;
    w->tick = w->n_log + 1;
    free(arbit_newcomer);
    return IF5_SNAP_OK;
}

/* ============================================================
 * Load
 * ============================================================ */

static bool view_equal(const if5_view* a, const if5_view* b)
{
    int i;
    if (a->cut != b->cut || a->n_excl != b->n_excl ||
        a->n_sel != b->n_sel) return false;
    for (i = 0; i < IF5_MAX_SELECTIONS; i++)
        if (a->excl[i] != b->excl[i]) return false;
    for (i = 0; i < IF5_MAX_SELECTIONS; i++)
        if (a->sel[i].rid != b->sel[i].rid ||
            a->sel[i].head_iid != b->sel[i].head_iid) return false;
    return true;
}

static bool stats_equal(const if5_stats* a, const if5_stats* b)
{
    return a->submits == b->submits &&
           a->causal_applies == b->causal_applies &&
           a->concurrent_arrivals == b->concurrent_arrivals &&
           a->lattice_checks == b->lattice_checks &&
           a->tier2_merges == b->tier2_merges &&
           a->tier1_forks == b->tier1_forks &&
           a->tier3_arbitrations == b->tier3_arbitrations &&
           a->system_merge_debits == b->system_merge_debits &&
           a->breakers == b->breakers &&
           a->receipts == b->receipts &&
           a->excluded == b->excluded &&
           a->merge_intents == b->merge_intents &&
           a->cursor_moves == b->cursor_moves &&
           a->denied_authority == b->denied_authority;
}

static void get_record(cur* c, if5_intent* r)
{
    int i;
    memset(r, 0, sizeof *r);
    for (i = 0; i < IF5_MAX_SOURCES; i++) r->vc.c[i] = c_u32(c);
    r->value[0] = c_u32(c);
    r->value[1] = c_u32(c);
    r->prov_a   = c_u32(c);
    r->prov_b   = c_u32(c);
    r->t        = c_u32(c);
    r->aux      = c_u32(c);
    r->debit    = c_u16(c);
    r->region   = c_u16(c);
    r->src      = c_u8(c);
    r->ptype    = c_u8(c);
    r->flags    = c_u8(c);
    r->commit   = c_u8(c);
}

int if5_snapshot_load(if5_world* w, const char* path,
                      char* diag, uint32_t diag_cap)
{
    /* staged rows: everything is parsed from the image FIRST,
     * then the world is built — a refused load leaves the caller's
     * fresh world untouched except for the type table the caller
     * itself registered. */
    typedef struct {
        if5_source seed;
        uint32_t   spent_initiative, spent_surprise;
        uint32_t   sub_ceiling, carved;
        bool       breaker;
    } src_row;

    typedef struct {
        char     name[IF5_NAME_MAX];
        uint8_t  ptype, tier;
        uint16_t window, parent;
    } reg_row;

    FILE* f;
    uint8_t* img = NULL;
    long fsz;
    cur c;
    uint32_t version, tick, n_log, n_arena, n_receipts, n_regions;
    uint32_t n_sources, n_cuts, sys_sid, n_types, i, k;
    uint32_t magic_ok = 0;
    if5_view view;
    if5_stats pst;
    src_row  srows[IF5_MAX_SOURCES];
    char     tnames[IF5_MAX_PAYLOAD_TYPES][IF5_SNAP_TYPE_NAME];
    reg_row  rrows[IF5_MAX_REGIONS];
    if5_intent* logbuf = NULL;
    if5_receipt* rcbuf = NULL;
    if5_cut  crows[IF5_MAX_CUTS];
    char     cnames[IF5_MAX_CUTS][IF5_NAME_MAX];
    const uint8_t* arenabuf = NULL;
    int rc = IF5_SNAP_OK;

    if (!w || !path) return IF5_SNAP_ERR_IO;
    if (diag && diag_cap) diag[0] = 0;

    /* a snapshot loads into a FRESH world only: sources, regions,
     * history, addresses, receipts all empty (payload types are
     * the exception — the application re-registers them first,
     * and the table is verified slot by slot below). */
    if (w->n_sources || w->n_regions || w->n_log || w->n_cuts ||
        w->n_receipts) {
        diagf(diag, diag_cap, "world is not fresh (sources=%u regions=%u "
             "log=%u cuts=%u receipts=%u)", w->n_sources, w->n_regions,
             w->n_log, w->n_cuts, w->n_receipts);
        return IF5_SNAP_ERR_STATE;
    }

    f = fopen(path, "rb");
    if (!f) {
        diagf(diag, diag_cap, "cannot open '%s'", path);
        return IF5_SNAP_ERR_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (fsz = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        diagf(diag, diag_cap, "cannot size '%s'", path);
        return IF5_SNAP_ERR_IO;
    }
    /* A registered bound on the snapshot's size: the format's own
     * capacities (1024 regions, 4 sources, a 64-entry registry)
     * put a sane world in the kilobytes-to-megabytes range; 256
     * MiB is a ceiling no registered scenario approaches, and a
     * file beyond it is refused as malformed rather than trusted
     * with an unbounded allocation (CodeQL
     * cpp/unbounded-allocation — the bound is real, not cosmetic)
     */
    if (fsz <= 0 || fsz > 256L * 1024L * 1024L) {
        fclose(f);
        diagf(diag, diag_cap, "snapshot size %ld is outside the "
             "registered bound", fsz);
        return IF5_SNAP_ERR_BOUNDS;
    }
    img = (uint8_t*)malloc((size_t)fsz);
    if (!img) { fclose(f); return IF5_SNAP_ERR_IO; }
    if (fread(img, 1, (size_t)fsz, f) != (size_t)fsz) {
        free(img); fclose(f);
        diagf(diag, diag_cap, "short read on '%s'", path);
        return IF5_SNAP_ERR_IO;
    }
    fclose(f);

    c.p = img; c.n = (uint32_t)fsz; c.pos = 0; c.bad = false;

    /* --- header ------------------------------------------------ */
    {
        const uint8_t* m = c_take(&c, 8);
        if (m && memcmp(m, IF5_SNAP_MAGIC, 8) == 0) magic_ok = 1;
    }
    if (!magic_ok) {
        diagf(diag, diag_cap, "not a v5 snapshot (bad magic)");
        free(img);
        return IF5_SNAP_ERR_MAGIC;
    }
    version = c_u32(&c);
    if (version != IF5_SNAP_VERSION) {
        diagf(diag, diag_cap, "format version %u (reader knows %u)",
              version, IF5_SNAP_VERSION);
        free(img);
        return IF5_SNAP_ERR_VERSION;
    }

    tick       = c_u32(&c);
    n_log      = c_u32(&c);
    n_arena    = c_u32(&c);
    n_receipts = c_u32(&c);
    n_regions  = c_u16(&c);
    n_sources  = c_u8(&c);
    n_cuts     = c_u8(&c);
    sys_sid    = c_u8(&c);
    n_types    = c_u8(&c);
    (void)c_u8(&c); (void)c_u8(&c); (void)c_u8(&c);

    if (c.bad || n_sources > IF5_MAX_SOURCES ||
        n_regions > IF5_MAX_REGIONS || n_cuts > IF5_MAX_CUTS ||
        n_types > IF5_MAX_PAYLOAD_TYPES ||
        (sys_sid != IF5_SID_NULL && sys_sid >= n_sources)) {
        diagf(diag, diag_cap, "header bounds failed");
        free(img);
        return IF5_SNAP_ERR_BOUNDS;
    }

    /* --- the cursor --------------------------------------------- */
    view.cut    = c_u32(&c);
    view.n_excl = c_u32(&c);
    for (i = 0; i < IF5_MAX_SELECTIONS; i++) view.excl[i] = c_u32(&c);
    view.n_sel = c_u32(&c);
    for (i = 0; i < IF5_MAX_SELECTIONS; i++) {
        view.sel[i].rid = c_u16(&c);
        (void)c_u16(&c);
        view.sel[i].head_iid = c_u32(&c);
    }
    if (c.bad || view.n_excl > IF5_MAX_SELECTIONS ||
        view.n_sel > IF5_MAX_SELECTIONS || view.cut > n_log) {
        diagf(diag, diag_cap, "cursor bounds failed");
        free(img);
        return IF5_SNAP_ERR_BOUNDS;
    }
    for (i = 0; i < view.n_excl; i++)
        if (view.excl[i] == 0 || view.excl[i] > n_log) {
            diagf(diag, diag_cap, "cursor exclusion out of range");
            free(img);
            return IF5_SNAP_ERR_BOUNDS;
        }

    /* --- the stats cross-check ----------------------------------- */
    memset(&pst, 0, sizeof pst);
    pst.submits            = c_u32(&c);
    pst.causal_applies     = c_u32(&c);
    pst.concurrent_arrivals= c_u32(&c);
    pst.lattice_checks     = c_u32(&c);
    pst.tier2_merges       = c_u32(&c);
    pst.tier1_forks        = c_u32(&c);
    pst.tier3_arbitrations = c_u32(&c);
    pst.system_merge_debits= c_u32(&c);
    pst.breakers           = c_u32(&c);
    pst.receipts           = c_u32(&c);
    pst.excluded           = c_u32(&c);
    pst.merge_intents      = c_u32(&c);
    pst.cursor_moves       = c_u32(&c);
    pst.denied_authority   = c_u32(&c);
    if (c.bad) {
        diagf(diag, diag_cap, "file ends in the stats section");
        free(img);
        return IF5_SNAP_ERR_TRUNC;
    }

    /* --- sources -------------------------------------------------- */
    for (i = 0; i < n_sources; i++) {
        const uint8_t* id;
        memset(&srows[i], 0, sizeof srows[i]);
        id = c_take(&c, IF5_SNAP_SRC_ID);
        if (id) memcpy(srows[i].seed.id, id, IF5_SNAP_SRC_ID);
        srows[i].seed.id[IF5_SNAP_SRC_ID - 1] = 0;
        srows[i].seed.kind  = (if5_kind)c_u8(&c);
        srows[i].seed.trust = c_u8(&c);
        (void)c_u8(&c); (void)c_u8(&c);
        srows[i].seed.initiative_budget = c_u32(&c);
        srows[i].seed.authority         = c_u32(&c);
        srows[i].spent_initiative = c_u32(&c);
        srows[i].spent_surprise   = c_u32(&c);
        srows[i].sub_ceiling      = c_u32(&c);
        srows[i].carved           = c_u32(&c);
        srows[i].breaker          = c_u8(&c) != 0;
        (void)c_u8(&c); (void)c_u8(&c); (void)c_u8(&c);
    }
    if (c.bad) {
        diagf(diag, diag_cap, "file ends in the source section");
        free(img);
        return IF5_SNAP_ERR_TRUNC;
    }

    /* --- the type table (names only; the code re-registers) ------ */
    for (i = 0; i < n_types; i++) c_name(&c, tnames[i], IF5_SNAP_TYPE_NAME);
    if (c.bad) {
        diagf(diag, diag_cap, "file ends in the type section");
        free(img);
        return IF5_SNAP_ERR_TRUNC;
    }

    /* --- regions --------------------------------------------------- */
    for (i = 0; i < n_regions; i++) {
        c_name(&c, rrows[i].name, IF5_NAME_MAX);
        rrows[i].ptype  = c_u8(&c);
        rrows[i].tier   = c_u8(&c);
        rrows[i].window = c_u16(&c);
        rrows[i].parent = c_u16(&c);
        if (rrows[i].parent != 0 && rrows[i].parent >= i + 1) {
            diagf(diag, diag_cap, "region %u: parent %u violates the "
                 "creation invariant", i + 1, rrows[i].parent);
            free(img);
            return IF5_SNAP_ERR_BOUNDS;
        }
    }
    if (c.bad) {
        diagf(diag, diag_cap, "file ends in the region section");
        free(img);
        return IF5_SNAP_ERR_TRUNC;
    }

    /* --- the log --------------------------------------------------- */
    if (n_log) {
        logbuf = (if5_intent*)malloc((size_t)n_log * sizeof(if5_intent));
        if (!logbuf) { free(img); return IF5_SNAP_ERR_IO; }
        for (i = 0; i < n_log; i++) get_record(&c, &logbuf[i]);
        if (c.bad) {
            free(logbuf); free(img);
            diagf(diag, diag_cap, "file ends in the log section");
            return IF5_SNAP_ERR_TRUNC;
        }
    }

    /* --- the arena -------------------------------------------------- */
    if (n_arena) {
        arenabuf = c_take(&c, n_arena);
        if (!arenabuf) {
            free(logbuf); free(img);
            diagf(diag, diag_cap, "file ends in the arena section");
            return IF5_SNAP_ERR_TRUNC;
        }
    }

    /* --- receipts ---------------------------------------------------- */
    if (n_receipts) {
        rcbuf = (if5_receipt*)malloc((size_t)n_receipts * sizeof(if5_receipt));
        if (!rcbuf) { free(logbuf); free(img); return IF5_SNAP_ERR_IO; }
        for (i = 0; i < n_receipts; i++) {
            if5_receipt* r = &rcbuf[i];
            memset(r, 0, sizeof *r);
            r->origin          = c_u8(&c);
            r->hop             = c_u8(&c);
            (void)c_u16(&c);
            r->reason_iid      = c_u32(&c);
            r->cost_initiative = c_u32(&c);
            r->cost_surprise   = c_u32(&c);
            r->rollback_cut    = c_u32(&c);
            r->checkpoint      = c_u32(&c);
            if (r->origin >= n_sources ||
                r->reason_iid > n_log || r->rollback_cut > n_log) {
                free(rcbuf); free(logbuf); free(img);
                diagf(diag, diag_cap, "receipt %u out of range", i);
                return IF5_SNAP_ERR_BOUNDS;
            }
        }
        if (c.bad) {
            free(rcbuf); free(logbuf); free(img);
            diagf(diag, diag_cap, "file ends in the receipt section");
            return IF5_SNAP_ERR_TRUNC;
        }
    }

    /* --- the address book -------------------------------------------- */
    for (i = 0; i < n_cuts; i++) {
        for (k = 0; k < IF5_MAX_SOURCES; k++)
            crows[i].frontier.c[k] = c_u32(&c);
        crows[i].prefix = c_u32(&c);
        for (k = 0; k < IF5_MAX_SELECTIONS; k++)
            crows[i].prunes[k] = c_u32(&c);
        crows[i].n_prunes = c_u32(&c);
        c_name(&c, cnames[i], IF5_NAME_MAX);
        if (crows[i].prefix > n_log || crows[i].n_prunes > IF5_MAX_SELECTIONS) {
            free(logbuf); free(rcbuf); free(img);
            diagf(diag, diag_cap, "cut %u out of range", i + 1);
            return IF5_SNAP_ERR_BOUNDS;
        }
        for (k = 0; k < crows[i].n_prunes; k++)
            if (crows[i].prunes[k] == 0 || crows[i].prunes[k] > n_log) {
                free(logbuf); free(rcbuf); free(img);
                diagf(diag, diag_cap, "cut %u prune out of range", i + 1);
                return IF5_SNAP_ERR_BOUNDS;
            }
    }
    if (c.bad) {
        free(logbuf); free(rcbuf); free(img);
        diagf(diag, diag_cap, "file ends in the cut section");
        return IF5_SNAP_ERR_TRUNC;
    }
    if (c_left(&c) != 0) {
        free(logbuf); free(rcbuf); free(img);
        diagf(diag, diag_cap, "%u trailing bytes after the last section",
              c_left(&c));
        return IF5_SNAP_ERR_BOUNDS;
    }

    /* --- the type table verification (refuse on drift) -------------- */
    if (n_types != w->n_types) {
        diagf(diag, diag_cap, "type table: file has %u slots, world "
             "registered %u", n_types, w->n_types);
        free(logbuf); free(rcbuf); free(img);
        return IF5_SNAP_ERR_TYPE;
    }
    for (i = 0; i < n_types; i++) {
        const char* have = w->types[i].name;
        if (!have || strcmp(have, tnames[i]) != 0) {
            diagf(diag, diag_cap, "type slot %u: file says '%s', world "
                 "registered '%s'", i, tnames[i], have ? have : "(null)");
            free(logbuf); free(rcbuf); free(img);
            return IF5_SNAP_ERR_TYPE;
        }
    }

    /* --- seed the sources ------------------------------------------- */
    for (i = 0; i < n_sources; i++) {
        if5_source s = srows[i].seed;
        s.delegated_by = IF5_SID_NULL;   /* add_source forces it; the
                                          * chain rebuilds from DELEG
                                          * records in the replay */
        if (if5_add_source(w, &s) == IF5_SID_NULL) {
            diagf(diag, diag_cap, "cannot seed source %u", i);
            rc = IF5_SNAP_ERR_IO;
            goto fail;
        }
    }
    if (sys_sid != IF5_SID_NULL) if5_set_system_source(w, (if5_sid)sys_sid);

    /* --- seed the regions -------------------------------------------- */
    for (i = 0; i < n_regions; i++) {
        if5__region_seed(w, (uint16_t)(i + 1), rrows[i].name,
                        rrows[i].ptype, rrows[i].tier, rrows[i].parent,
                        rrows[i].window, 0);
    }
    w->n_regions = (uint16_t)n_regions;

    /* --- install the durable truth VERBATIM --------------------------- */
    if (n_log) {
        w->log = logbuf;
        w->n_log = n_log;
        w->cap_log = n_log;
    }
    if (n_arena) {
        w->arena = (uint8_t*)malloc(n_arena);
        if (!w->arena) { rc = IF5_SNAP_ERR_IO; goto fail; }
        memcpy(w->arena, arenabuf, n_arena);
        w->n_arena = n_arena;
        w->cap_arena = n_arena;
    }
    logbuf = NULL;                    /* ownership moved into the world */

    /* --- the rebuild pass --------------------------------------------- */
    rc = if5__rebuild_caches(w);
    if (rc != IF5_SNAP_OK) {
        diagf(diag, diag_cap, "rebuild refused (cursor history diverged)");
        goto fail;
    }

    /* --- the cross-checks (a divergence refuses the load) -------------- */
    if (!stats_equal(&w->stats, &pst)) {
        diagf(diag, diag_cap, "stats cross-check failed (file submits=%u "
             "causal=%u arrivals=%u forks=%u arbits=%u merges=%u; "
             "rebuild submits=%u causal=%u arrivals=%u forks=%u "
             "arbits=%u merges=%u)",
             pst.submits, pst.causal_applies, pst.concurrent_arrivals,
             pst.tier1_forks, pst.tier3_arbitrations, pst.tier2_merges,
             w->stats.submits, w->stats.causal_applies,
             w->stats.concurrent_arrivals, w->stats.tier1_forks,
             w->stats.tier3_arbitrations, w->stats.tier2_merges);
        rc = IF5_SNAP_ERR_CHECK;
        goto fail;
    }
    if (tick != w->tick) {
        diagf(diag, diag_cap, "tick cross-check failed (file %u, "
             "derived %u)", tick, w->tick);
        rc = IF5_SNAP_ERR_CHECK;
        goto fail;
    }
    if (!view_equal(&w->now, &view)) {
        diagf(diag, diag_cap, "cursor cross-check failed (file cut=%u "
             "excl=%u sel=%u; replay cut=%u excl=%u sel=%u)",
             view.cut, view.n_excl, view.n_sel,
             w->now.cut, w->now.n_excl, w->now.n_sel);
        rc = IF5_SNAP_ERR_CHECK;
        goto fail;
    }
    for (i = 0; i < n_sources; i++) {
        if5_ledger* l = &w->ledgers[i];
        if (l->carved != srows[i].carved ||
            l->sub_ceiling != srows[i].sub_ceiling ||
            l->breaker_tripped != srows[i].breaker) {
            diagf(diag, diag_cap, "source %u ledger cross-check failed "
                 "(carved %u/%u sub %u/%u breaker %d/%d)", i,
                 l->carved, srows[i].carved,
                 l->sub_ceiling, srows[i].sub_ceiling,
                 (int)l->breaker_tripped, (int)srows[i].breaker);
            rc = IF5_SNAP_ERR_CHECK;
            goto fail;
        }
        /* spent: the log-visible portion may omit the standalone
         * if5_debit residue (a successful standalone debit posts
         * no record — registered in ADR-0028's Validation); the
         * persisted totals are authoritative, but the visible
         * portion may never EXCEED them. */
        if (l->spent_initiative > srows[i].spent_initiative ||
            l->spent_surprise   > srows[i].spent_surprise) {
            diagf(diag, diag_cap, "source %u spent cross-check failed "
                 "(rebuild %u/%u exceeds persisted %u/%u)", i,
                 l->spent_initiative, l->spent_surprise,
                 srows[i].spent_initiative, srows[i].spent_surprise);
            rc = IF5_SNAP_ERR_CHECK;
            goto fail;
        }
        l->spent_initiative = srows[i].spent_initiative;
        l->spent_surprise   = srows[i].spent_surprise;
    }

    /* --- the receipts and the address book install -------------------- */
    if (n_receipts) {
        w->receipts = rcbuf;
        w->n_receipts = n_receipts;
        w->cap_receipts = n_receipts;
        rcbuf = NULL;
    }
    for (i = 0; i < n_cuts; i++) {
        /* the frontier is re-derived from the installed log and
         * cross-checked — the address book's cached knowledge must
         * be the log's knowledge, not a second opinion */
        if5_vc fr = if5__cut_frontier(w, crows[i].prefix,
                                      crows[i].prunes, crows[i].n_prunes);
        if (memcmp(&fr, &crows[i].frontier, sizeof(if5_vc)) != 0) {
            diagf(diag, diag_cap, "cut %u frontier cross-check failed",
                 i + 1);
            rc = IF5_SNAP_ERR_CHECK;
            goto fail;
        }
        w->cuts[i] = crows[i];
        memcpy(w->cut_names[i], cnames[i], IF5_NAME_MAX);
    }
    w->n_cuts = (uint8_t)n_cuts;
    w->now = view;                    /* equal to the replay's end */

    free(img);
    return IF5_SNAP_OK;

fail:
    free(logbuf);
    free(rcbuf);
    free(img);
    return rc;
}

/* The loader's region seeder: fills one table row. The public
 * creators LOG their records; the log is installed verbatim, so
 * seeding through the public path would duplicate history. This
 * path touches the working-set table only — the replay (and the
 * log itself, for the seed-parent fold) supplies everything
 * else. */
void if5__region_seed(if5_world* w, uint16_t rid, const char* name,
                      uint8_t ptype, uint8_t tier, uint16_t parent,
                      uint16_t window, uint64_t seed)
{
    if5_region* r;
    if (!w || rid == 0 || rid > IF5_MAX_REGIONS) return;
    r = &w->regions[rid - 1];
    memset(r, 0, sizeof *r);
    if (name) {
        size_t nl = strlen(name);
        if (nl >= IF5_NAME_MAX) nl = IF5_NAME_MAX - 1;
        memcpy(r->name, name, nl);
    }
    r->ptype  = ptype;
    r->tier   = tier;
    r->parent = parent;
    r->window = window;
    r->cache  = seed;
}
