/* v4/src/interpretant.c — essence #3: Peirce interpretant (NEW)
 *
 * The interpretant is the meaning generated in the actor when they
 * encounter the representamen. Peirce's triad is:
 *   representamen -> object -> interpretant
 *
 * The system can express its *intended* interpretant (what it WANTED
 * the actor to take the representamen to mean). The actor's *actual*
 * interpretant is either observed (the actor did X with the
 * representamen) or predicted by an interpret_fn.
 *
 * Mismatch between intended and actual -> Breakdown candidate.
 *
 * v4: this is a NEW abstraction. v3 Path B bolted it onto Perception
 * as a sub-API. v4 makes it first-class.
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_interpretant {
    px_perception*  representamen_source;  /* weak ref, not owned */
    px_actor*        actor;                  /* weak ref, not owned */
    char*            intended;              /* system-side semantics */
    px_interpret_fn  fn;                     /* prediction fn */
    void*            fn_user;
};

px_interpretant* px_interpretant_new(px_perception* representamen_source,
                                       px_actor* actor) {
    px_interpretant* it = (px_interpretant*)calloc(1, sizeof(px_interpretant));
    if (!it) return NULL;
    it->representamen_source = representamen_source;
    it->actor = actor;
    it->intended = NULL;
    it->fn = NULL;
    it->fn_user = NULL;
    return it;
}

void px_interpretant_free(px_interpretant* it) {
    if (!it) return;
    free(it->intended);
    free(it);
}

void px_interpretant_set_intended(px_interpretant* it, const char* semantics) {
    if (!it) return;
    free(it->intended);
    if (semantics) {
        it->intended = (char*)malloc(strlen(semantics) + 1);
        if (it->intended) strcpy(it->intended, semantics);
    } else {
        it->intended = NULL;
    }
}

const char* px_interpretant_intended(const px_interpretant* it) {
    return it ? it->intended : NULL;
}

void px_interpretant_set_interpret_fn(px_interpretant* it,
                                        px_interpret_fn fn, void* user) {
    if (!it) return;
    it->fn = fn;
    it->fn_user = user;
}

void* px_interpretant_predict(px_interpretant* it, void* representamen) {
    if (!it || !it->fn) return NULL;
    return it->fn(representamen, it->actor, it->fn_user);
}

bool px_interpretant_matches_intended(px_interpretant* it, void* actual) {
    if (!it || !it->intended || !actual) return false;
    /* Simple string equality: if actual is a string, compare with intended.
     * This is a deliberate v4 simplification. More sophisticated matching
     * (e.g. interpretant-as-struct, semantic similarity) would be a v0.6
     * concern. */
    return strcmp(it->intended, (const char*)actual) == 0;
}
