/* v4/src/actor.c — px_actor first-class struct (NOT an abstraction)
 *
 * Per Suchman, Heidegger, Maturana: UI cannot be defined without
 * the actor. But the actor is a parameter to four abstractions
 * (Relation, Interpretant, Perlocution, Breakdown), not itself
 * an essence abstraction.
 *
 * The actor is identified by a string id (stable across the session)
 * and carries opaque user_data (the application knows what it is —
 * user profile, permissions, role, etc.).
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_actor {
    char*  id;
    void*  user_data;
};

px_actor* px_actor_new(const char* id, void* user_data) {
    if (!id) return NULL;
    px_actor* a = (px_actor*)malloc(sizeof(px_actor));
    if (!a) return NULL;
    a->id = (char*)malloc(strlen(id) + 1);
    if (!a->id) { free(a); return NULL; }
    strcpy(a->id, id);
    a->user_data = user_data;
    return a;
}

void px_actor_free(px_actor* a) {
    if (!a) return;
    free(a->id);
    free(a);
}

const char* px_actor_id(const px_actor* a) {
    return a ? a->id : NULL;
}

void* px_actor_user_data(const px_actor* a) {
    return a ? a->user_data : NULL;
}
