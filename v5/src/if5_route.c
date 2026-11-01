/*
 * if5_route.c — intent compilation as the type registry's routing
 * (ADR-0027 Decision 2f, the mechanism layer's sixth mapping).
 *
 * The shipping compile step (src/hit.c's px_afford_compile line)
 * resolves a physical hit into a full semantic payload and
 * triggers: resolution by position, payload assembly, closure
 * fire. This module re-derives the compilation as ROUTING
 * THROUGH THE OPEN REGISTRY: a compile function (text -> value
 * word + submit options) attached to a registered payload type;
 * if5_compile routes, parses, and submits through the ordinary
 * channel.
 *
 * The spatial resolution half of the shipping step is NOT CARRIED
 * and never will be: hit-testing on visual geometry cannot exist
 * in a core with no visual fields (S15's contract; the rebound
 * gate's vocabulary ban). In v5 the caller NAMES the region —
 * resolution is the client's job, routing is the core's.
 *
 * The sugar property is contract (M6): the compiled record is
 * byte-equal to the direct submission of the same value with the
 * same options. Compilation adds routing; it adds nothing else —
 * no hidden defaults, no timestamping, no normalization the
 * direct path would not also do.
 *
 * Built-in routes (NUM decimal / SET hex / STR 8-byte line)
 * register through the same public channel applications use —
 * B2's equality-of-standing rule, the third application (payload
 * types, projections, and now routes). Lineage: R2; no slice
 * ancestor.
 */
#include "if5_internal.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

bool if5_route_register(if5_world* w, uint8_t ptype, if5_compile_fn fn)
{
    if (!w || ptype >= IF5_MAX_PAYLOAD_TYPES) return false;
    /* fn == NULL unregisters (an extension type's route removed);
     * the built-ins are not protected — equality of standing means
     * the application outranks the defaults, on the record. */
    w->routes[ptype] = fn;
    return true;
}

uint32_t if5_compile(if5_world* w, if5_sid src, uint16_t rid,
                     uint8_t ptype, const char* text,
                     const if5_submit_opts* opts)
{
    if5_submit_opts o;
    uint64_t value;

    if (!w || !text) return 0;
    if (ptype >= IF5_MAX_PAYLOAD_TYPES || !w->routes[ptype]) return 0;

    /* The route owns the whole parse: value AND any option
     * defaults it wants to contribute. A base option set (from the
     * caller) is the starting state — the route may override
     * (e.g., a SET route that wants to debit surprise). The NULL
     * default is OMNISCIENT (observed_cut = the full log), the
     * same convention if5_user_act carries: compilation is sugar
     * over the direct path, and M6's byte-equality demand pins the
     * two together — a blind compile is the CALLER's explicit
     * choice, passed in opts, never a hidden default that would
     * silently diverge the routed record from the direct one. */
    if (opts) o = *opts;
    else { memset(&o, 0, sizeof o); o.observed_cut = w->n_log; }
    if (!w->routes[ptype](text, &value, &o)) return 0;

    /* The ordinary channel, nothing else (M6's byte-equality is
     * about THIS call being indistinguishable from the direct
     * one). */
    return if5_submit(w, src, rid, ptype, value, &o);
}

/* --- the built-in routes, through the public channel -------------- */

static bool if5__route_num(const char* text, uint64_t* value,
                           if5_submit_opts* opts)
{
    char* end = NULL;
    unsigned long long v;

    (void)opts;
    if (!text || !*text) return false;
    errno = 0;
    v = strtoull(text, &end, 10);
    if (errno == ERANGE) return false;
    if (!end || *end != '\0') return false;
    if (isspace((unsigned char)*text)) return false;
    *value = (uint64_t)v;
    return true;
}

static bool if5__route_set(const char* text, uint64_t* value,
                           if5_submit_opts* opts)
{
    const char* p;

    (void)opts;
    if (!text || strncmp(text, "0x", 2) != 0) return false;
    if (!text[2]) return false;
    *value = 0;
    for (p = text + 2; *p; p++) {
        uint64_t d;
        if (*p >= '0' && *p <= '9')      d = (uint64_t)(*p - '0');
        else if (*p >= 'a' && *p <= 'f') d = (uint64_t)(*p - 'a' + 10);
        else if (*p >= 'A' && *p <= 'F') d = (uint64_t)(*p - 'A' + 10);
        else return false;
        if (*value > (UINT64_MAX >> 4)) return false;
        *value = (*value << 4) | d;
    }
    return true;
}

static bool if5__route_str(const char* text, uint64_t* value,
                           if5_submit_opts* opts)
{
    uint64_t v = 0;
    size_t len, i;

    (void)opts;
    if (!text) return false;
    len = strlen(text);
    if (len > 8) return false;            /* the STR family is the
                                           * 8-byte line — longer
                                           * text is the caller's
                                           * error, not a silent
                                           * truncation */
    for (i = 0; i < len; i++)
        v = (v << 8) | (uint64_t)(unsigned char)text[i];
    *value = v;
    return true;
}

void if5__register_builtin_routes(if5_world* w)
{
    /* The three world-payload families that carry parseable text
     * semantics at library grade. Registration order is not
     * observable (routes are keyed by ptype, not by index), unlike
     * the projection registry — but the SET of routes is. */
    (void)if5_route_register(w, IF5_T_NUM, if5__route_num);
    (void)if5_route_register(w, IF5_T_SET, if5__route_set);
    (void)if5_route_register(w, IF5_T_STR, if5__route_str);
}
