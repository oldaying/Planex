/* v4/src/closure.c — essence #4: Illocution
 *
 * An illocutionary act is what the actor IS DOING in issuing an
 * utterance: asserting, requesting, promising, declaring, expressing
 * (Searle 1975). Closure is the reified form of that act as a value.
 *
 * Intent is a *value*, not a callback — this enables undo/redo,
 * replay, agent-driving, time travel.
 *
 * v4 BREAK: Closure no longer has set_feedback / promise / declare /
 * fail / set_perlocution / get_status. All perlocutionary outcomes
 * moved to the Perlocution abstraction (essence #5).
 *
 * v4 BREAK: px_intent_kind is now const char* (open symbol system),
 * not an enum. Built-ins provided as extern const (defined in intent.c).
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_closure {
    char*           goal;
    px_intent_kind  kind;
    px_action_fn    action;
    px_eval_fn      eval;
    void*           user;

    /* last triggered intent (for replay / undo) */
    px_intent       last_intent;
    void*           last_payload_storage;   /* heap copy of last payload */
    size_t          last_payload_capacity;
    bool            evaluated;
};

px_closure* px_closure_new(const char*      goal,
                             px_intent_kind   kind,
                             px_action_fn     action,
                             px_eval_fn       eval,
                             void*            user) {
    px_closure* c = (px_closure*)calloc(1, sizeof(px_closure));
    if (!c) return NULL;
    if (goal) {
        c->goal = (char*)malloc(strlen(goal) + 1);
        if (!c->goal) { free(c); return NULL; }
        strcpy(c->goal, goal);
    }
    c->kind = kind;
    c->action = action;
    c->eval = eval;
    c->user = user;
    c->last_intent.kind = kind;
    c->last_intent.payload = NULL;
    c->last_intent.payload_size = 0;
    c->last_payload_storage = NULL;
    c->last_payload_capacity = 0;
    c->evaluated = false;
    return c;
}

void px_closure_free(px_closure* c) {
    if (!c) return;
    free(c->goal);
    free(c->last_payload_storage);
    free(c);
}

/* Copy payload into closure-owned storage. Returns pointer into
 * closure-owned memory; valid until next trigger / replay / free. */
static void* store_payload(px_closure* c, void* payload, size_t size) {
    if (!c) return NULL;
    if (size == 0 || !payload) {
        c->last_intent.payload = NULL;
        c->last_intent.payload_size = 0;
        return NULL;
    }
    if (size > c->last_payload_capacity) {
        void* newbuf = realloc(c->last_payload_storage, size);
        if (!newbuf) return NULL;
        c->last_payload_storage = newbuf;
        c->last_payload_capacity = size;
    }
    memcpy(c->last_payload_storage, payload, size);
    c->last_intent.payload = c->last_payload_storage;
    c->last_intent.payload_size = size;
    return c->last_payload_storage;
}

void px_closure_trigger(px_closure* c, void* payload, size_t size) {
    if (!c) return;
    c->last_intent.kind = c->kind;
    store_payload(c, payload, size);
    if (c->action) {
        c->action(c->last_intent, c->user);
    }
    if (c->eval) {
        c->evaluated = c->eval(c->user);
    } else {
        c->evaluated = true;
    }
}

void px_closure_replay(px_closure* c, px_intent intent) {
    if (!c) return;
    c->last_intent.kind = intent.kind;
    store_payload(c, intent.payload, intent.payload_size);
    if (c->action) {
        c->action(c->last_intent, c->user);
    }
    if (c->eval) {
        c->evaluated = c->eval(c->user);
    } else {
        c->evaluated = true;
    }
}

px_intent px_closure_last_intent(const px_closure* c) {
    if (!c) {
        px_intent empty = { NULL, NULL, 0 };
        return empty;
    }
    return c->last_intent;
}

const char* px_closure_goal(const px_closure* c) {
    return c ? c->goal : NULL;
}

px_intent_kind px_closure_intent_kind(const px_closure* c) {
    return c ? c->kind : NULL;
}

bool px_closure_evaluated(const px_closure* c) {
    return c ? c->evaluated : false;
}
