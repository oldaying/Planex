/*
 * actor.c — Actor (v3 prototype, essence derivation v3 / Path B)
 *
 * The Actor is the human (or AI agent) whose situational relation to
 * the system gives the boundary meaning. Per Suchman (1987, situated
 * action), Heidegger (1927, Zuhandenheit), Maturana/Varela (1980,
 * autopoiesis): UI cannot be defined without the actor.
 *
 * The actor is *not itself* an abstraction in Planex's essence
 * taxonomy — it is a *parameter* that Relation, Breakdown,
 * Perlocution, and Interpretant APIs take. Treating "actor" as a
 * struct rather than a 6th essence abstraction is a deliberate scope
 * decision (Path B over Path C, see essence-derivation-v3.md § IV):
 *
 *   - Path B (chosen): actor is a struct, parameterized into other
 *     abstractions' APIs.
 *   - Path C (rejected): actor would be a 7th abstraction, violating
 *     non-goals.md NG-3 (no kitchen-sink API).
 *
 * This module is intentionally small — the actor's *meaning* is
 * established by its use in Relation / Breakdown / Perlocution /
 * Interpretant, not by data it stores. The struct carries only an
 * identifier string and opaque user_data (the caller's per-actor
 * context: session, history, preferences, etc.).
 *
 * Thread safety: NOT thread-safe. Like the rest of Planex.
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

struct px_actor {
    char*  id;          /* caller-provided identifier string       */
    void*  user_data;   /* caller-owned context (session, history) */
};

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_actor* px_actor_new(const char* id, void* user_data) {
    if (!id) return NULL;

    px_actor* a = (px_actor*)calloc(1, sizeof(px_actor));
    if (!a) return NULL;

    a->id = px_strdup(id);
    if (!a->id) {
        free(a);
        return NULL;
    }
    a->user_data = user_data;
    return a;
}

void px_actor_free(px_actor* a) {
    if (!a) return;
    free(a->id);
    /* NOTE: does NOT free a->user_data — caller owns it. */
    free(a);
}

const char* px_actor_id(const px_actor* a) {
    return a ? a->id : NULL;
}

void* px_actor_user_data(const px_actor* a) {
    return a ? a->user_data : NULL;
}
