/*
 * closure.c — Closure (5-stage execution unit, user → machine direction)
 *
 * Per ADR-0005: Closure was 7 stages (Norman's complete model).
 * Now Closure covers only execution side (Norman stages 1-4 + 7):
 *   1. Goal          (human-readable description)
 *   2. Intent        (typed value, not a callback!)
 *   3. Action        (function that mutates state)
 *   4. Execution     (runtime invokes action)
 *   5. Evaluation    (function that checks if goal is achieved)
 *
 * Norman stages 5 (Perception) and 6 (Interpretation) moved to
 * the new Perception abstraction (see src/perception.c).
 *
 * Mainstream UI libraries model only stages 2-4 as onClick
 * callbacks. Planex models all 5 execution-side stages, plus
 * the evaluation side via Perception.
 *
 * Intent is a *value*, not a function. This enables:
 *   - Undo/redo (replay Intent sequence)
 *   - Time travel (sample state at any t)
 *   - Audit log (every interaction is recorded)
 *
 * Inspired by:
 *   - Reenskaug 1979 MVC (Controller = intent translator)
 *   - Winograd/Flores (Conversation for Action)
 *   - Hancock interaction trees (ITree)
 *   - re-frame interceptor chains
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

struct px_closure {
    char*            goal;
    px_intent_kind   intent_kind;
    px_action_fn     action;
    px_eval_fn       evaluation;
    void*            user;

    /* Last triggered intent (with owned copy of payload). */
    px_intent        last_intent;
    void*            last_payload;
    bool             last_evaluated;

    /* Stage 17: Feedback + machine-initiated status */
    char              feedback[256];
    px_closure_status status;

    /* v0.3: optional graph binding for undo-via-graph.
     * If set AND undo is enabled, px_closure_trigger calls
     * px_undo_record(graph, this) before running the action. */
    px_graph*         undo_graph;

    /* v0.6: one-shot warning flag for the bind-ordering L2 leak
     * (leak-budgets.md Closure §L2-1). If undo recording is globally
     * enabled but this closure has no bound graph, the FIRST trigger
     * warns on stderr — previously undo silently did nothing, which
     * violated Norman's "system status visibility" for the developer.
     * The warning fires once per closure, not per trigger. */
    bool              warned_no_graph;

    /* v3 prototype: perlocution sub-API.
     * Perlocution is the *effect* of the system's utterance on the
     * actor's mental state — distinct from `status` (operational)
     * and from `intent_kind` (illocutionary force of the actor's
     * input). Default perlocution_kind=PX_PERLOC_UNSPECIFIED. */
    px_perlocution_kind perlocution_kind;
    char                perlocution_text[256];
};

static const char* const k_intent_names[] = {
    "ASSERT",       /* PX_INTENT_ASSERT    */
    "REQUEST",      /* PX_INTENT_REQUEST   */
    "PROMISE",      /* PX_INTENT_PROMISE   */
    "DECLARE",      /* PX_INTENT_DECLARE   */
    "EXPRESS",      /* PX_INTENT_EXPRESS   */
    "COUNT",        /* PX_INTENT_COUNT     */
};

/* v0.8 (Line 2): process-global registry of live closures — the
 * regions/perceptions/interactions precedent. Consumer: px_is_closure,
 * the kind predicate the afford compile uses to filter AFFORDS
 * targets (pointer identity, no type punning on void* nodes).
 * Before this, px_afford_at cast the FIRST non-NULL target to
 * px_closure* — a latent type confusion the process form would
 * have tripped over (region AFFORDS interaction). */
typedef struct px_clo_node {
    px_closure*         c;
    struct px_clo_node* next;
} px_clo_node;

static px_clo_node* g_closures = NULL;

bool px_is_closure(const void* node) {
    /* Pointer identity against the live registry — the node is never
     * dereferenced, so an estimate, an interaction, a region, a freed
     * closure, or NULL all answer false without any type punning. */
    if (!node) return false;
    for (px_clo_node* n = g_closures; n; n = n->next) {
        if ((const void*)n->c == node) return true;
    }
    return false;
}

px_closure* px_closure_new(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_eval_fn       evaluation,
    void*            user) {

    if (!goal) return NULL;
    if (intent_kind < 0 || intent_kind >= PX_INTENT_COUNT) return NULL;

    px_closure* c = (px_closure*)calloc(1, sizeof(px_closure));
    if (!c) return NULL;

    c->goal = px_strdup(goal);
    if (!c->goal) {
        free(c);
        return NULL;
    }
    c->intent_kind = intent_kind;
    c->action      = action;
    c->evaluation  = evaluation;
    c->user        = user;
    c->last_payload = NULL;
    c->last_evaluated = false;
    c->undo_graph  = NULL;  /* not bound by default */

    /* v0.8 (Line 2): register (see the registry block above). */
    px_clo_node* node = (px_clo_node*)calloc(1, sizeof(px_clo_node));
    if (!node) {
        free(c->goal);
        free(c);
        return NULL;
    }
    node->c = c;
    node->next = g_closures;
    g_closures = node;
    return c;
}

/* v0.7 Line 5 (ADR-0019): the constructor split that retires the last
 * aggregate L2 leak. The graph arrives WITH the closure — before any
 * trigger can race it — so the bind_graph ordering dependency (call
 * bind BEFORE trigger or undo silently does nothing) becomes
 * unwritable by construction. */
px_closure* px_closure_new_with_graph(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_eval_fn       evaluation,
    void*            user,
    px_graph*        graph) {

    px_closure* c = px_closure_new(goal, intent_kind, action,
                                   evaluation, user);
    if (!c) return NULL;
    c->undo_graph = graph;   /* bound at birth; no two-call window */
    return c;
}

/* v0.3: bind a graph to this closure for undo-via-graph.
 * After binding, if px_undo_is_enabled(), px_closure_trigger
 * will automatically snapshot affected Estimates before action.
 *
 * DEPRECATED since v0.7 (ADR-0019): the two-call form is the last
 * aggregate L2 leak — an ordering dependency the type system cannot
 * enforce. Use px_closure_new_with_graph(...) instead; this form
 * remains callable through the deprecation window (registry:
 * deprecation-registry.md). */
void px_closure_bind_graph(px_closure* c, px_graph* g) {
    if (c) c->undo_graph = g;
}

void px_closure_free(px_closure* c) {
    if (!c) return;
    px_clo_node** pp = &g_closures;
    while (*pp) {
        if ((*pp)->c == c) {
            px_clo_node* dead = *pp;
            *pp = dead->next;
            free(dead);
            break;
        }
        pp = &((*pp)->next);
    }
    free(c->goal);
    free(c->last_payload);
    free(c);
}

void px_closure_trigger(px_closure* c, void* payload, size_t size) {
    if (!c) return;

    /* Free previous payload, save new one (copied). */
    free(c->last_payload);
    c->last_payload = NULL;

    c->last_intent.kind         = c->intent_kind;
    c->last_intent.payload_size = size;

    if (size > 0 && payload) {
        c->last_payload = malloc(size);
        if (c->last_payload) {
            memcpy(c->last_payload, payload, size);
            c->last_intent.payload = c->last_payload;
        } else {
            c->last_intent.payload = NULL;
            c->last_intent.payload_size = 0;
        }
    } else {
        c->last_intent.payload = NULL;
        c->last_intent.payload_size = 0;
    }

    /* v0.3: if undo is enabled AND closure has bound graph,
     * snapshot affected Estimates before action runs.
     *
     * v0.6: if undo is enabled but NO graph is bound, warn once.
     * Before this, forgetting px_closure_bind_graph made undo
     * silently ineffective — the type system cannot enforce call
     * ordering, so the runtime must make the omission visible. */
    if (px_undo_is_enabled() && c->undo_graph) {
        px_undo_record(c->undo_graph, c);
    } else if (px_undo_is_enabled() && !c->undo_graph && !c->warned_no_graph) {
        fprintf(stderr,
                "[planex] warning: undo is enabled but closure \"%s\" has no "
                "bound graph — undo will not record this closure. "
                "Call px_closure_bind_graph(c, g) before triggering.\n",
                c->goal ? c->goal : "(unnamed)");
        c->warned_no_graph = true;
    }

    /* Stage 3-4: Action + Execution */
    if (c->action) {
        c->action(c->last_intent, c->user);
    }

    /* Per ADR-0005: Stage 5 (Perception) moved to the Perception abstraction.
     * See src/perception.c and px_perception_new().
     * Closure no longer invokes perception here. */

    /* Stage 7: Evaluation — kernel fix: failure auto-produces feedback */
    if (c->evaluation) {
        c->last_evaluated = c->evaluation(c->user);
        if (!c->last_evaluated) {
            /* Goal not achieved — auto-set FAILED status + feedback */
            c->status = PX_CLOSURE_FAILED;
            snprintf(c->feedback, sizeof(c->feedback),
                     "evaluation failed: goal \"%s\" not achieved",
                     c->goal ? c->goal : "(unnamed)");
        } else {
            /* Goal achieved — auto-set DONE if not already set by action */
            if (c->status == PX_CLOSURE_IDLE || c->status == PX_CLOSURE_RUNNING) {
                c->status = PX_CLOSURE_DONE;
                if (c->feedback[0] == 0) {
                    snprintf(c->feedback, sizeof(c->feedback),
                             "goal \"%s\" achieved",
                             c->goal ? c->goal : "(unnamed)");
                }
            }
        }
    } else {
        c->last_evaluated = false;
    }
}

px_intent px_closure_last_intent(const px_closure* c) {
    if (!c) {
        px_intent empty = { 0, NULL, 0 };
        return empty;
    }
    return c->last_intent;
}

void px_closure_replay(px_closure* c, px_intent intent) {
    if (!c) return;
    /* Copy the payload BEFORE calling trigger.
     *
     * Why: if `intent` came from px_closure_last_intent() on THIS closure,
     * then `intent.payload` points into `c->last_payload`. The first thing
     * px_closure_trigger does is `free(c->last_payload)` — so if we pass
     * `intent.payload` directly, it becomes a dangling pointer mid-trigger
     * and the subsequent memcpy reads freed memory.
     *
     * By copying first, replay is safe regardless of whether intent.payload
     * points into the closure's internal storage or some external buffer.
     *
     * The copy is freed after trigger returns (trigger makes its own
     * internal copy via malloc+memcpy). */
    void*  payload_copy = NULL;
    size_t payload_size  = intent.payload_size;

    if (intent.payload_size > 0 && intent.payload) {
        payload_copy = malloc(intent.payload_size);
        if (!payload_copy) return;
        memcpy(payload_copy, intent.payload, intent.payload_size);
    }

    px_closure_trigger(c, payload_copy, payload_size);
    free(payload_copy);
}

bool px_closure_evaluated(const px_closure* c) {
    return c ? c->last_evaluated : false;
}

const char* px_intent_kind_str(px_intent_kind k) {
    if (k < 0 || k >= PX_INTENT_COUNT) return "?";
    return k_intent_names[k];
}

/* ============================================================
 * Stage 17: Feedback + machine-initiated status
 *
 * Completes Norman's 7-stage loop:
 *   Intent → Action → State → Perception → Interpretation → Evaluation
 *                                              ↑
 *                                         feedback lives here
 * ============================================================ */

static const char* const k_status_names[] = {
    "IDLE",    /* PX_CLOSURE_IDLE */
    "RUNNING", /* PX_CLOSURE_RUNNING */
    "DONE",    /* PX_CLOSURE_DONE */
    "FAILED",  /* PX_CLOSURE_FAILED */
};

const char* px_closure_status_str(px_closure_status s) {
    int n = (int)(sizeof(k_status_names) / sizeof(k_status_names[0]));
    if ((int)s < 0 || (int)s >= n) return "?";
    return k_status_names[s];
}

void px_closure_set_feedback(px_closure* c, const char* text) {
    if (!c || !text) return;
    strncpy(c->feedback, text, sizeof(c->feedback) - 1);
    c->feedback[sizeof(c->feedback) - 1] = 0;
}

const char* px_closure_feedback(const px_closure* c) {
    return c ? c->feedback : "";
}

void px_closure_promise(px_closure* c, const char* message) {
    if (!c) return;
    c->status = PX_CLOSURE_RUNNING;
    if (message) {
        strncpy(c->feedback, message, sizeof(c->feedback) - 1);
        c->feedback[sizeof(c->feedback) - 1] = 0;
    }
}

void px_closure_declare(px_closure* c, const char* message) {
    if (!c) return;
    c->status = PX_CLOSURE_DONE;
    if (message) {
        strncpy(c->feedback, message, sizeof(c->feedback) - 1);
        c->feedback[sizeof(c->feedback) - 1] = 0;
    }
}

void px_closure_fail(px_closure* c, const char* message) {
    if (!c) return;
    c->status = PX_CLOSURE_FAILED;
    if (message) {
        strncpy(c->feedback, message, sizeof(c->feedback) - 1);
        c->feedback[sizeof(c->feedback) - 1] = 0;
    }
}

px_closure_status px_closure_get_status(const px_closure* c) {
    return c ? c->status : PX_CLOSURE_IDLE;
}

/* ============================================================
 * v3 prototype — perlocution sub-API
 *
 * Perlocution is the *effect* of the system's utterance on the
 * actor's mental state. Distinct from `status` (operational:
 * IDLE/RUNNING/DONE/FAILED) and from `intent_kind` (illocutionary
 * force of the actor's input: ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS).
 *
 * Example: two closures both complete successfully (status=DONE),
 * but one sets perlocution=INFORM with text "Saved." while the
 * other sets perlocution=ALERT with text "Saved. 3 fields were
 * auto-corrected." The actor's next intent will differ — the loop
 * audit can record that semantic difference.
 *
 * Inspired by Searle 1969 (Speech Acts, level 3: perlocutionary).
 * ============================================================ */

static const char* const k_perloc_names[] = {
    "UNSPECIFIED",  /* PX_PERLOC_UNSPECIFIED */
    "INFORM",       /* PX_PERLOC_INFORM      */
    "PERSUADE",     /* PX_PERLOC_PERSUADE    */
    "REASSURE",     /* PX_PERLOC_REASSURE    */
    "ALERT",        /* PX_PERLOC_ALERT       */
    "FRUSTRATE",    /* PX_PERLOC_FRUSTRATE   */
    "SURPRISE",     /* PX_PERLOC_SURPRISE    */
};

const char* px_perlocution_kind_str(px_perlocution_kind k) {
    int n = (int)(sizeof(k_perloc_names) / sizeof(k_perloc_names[0]));
    if ((int)k < 0 || (int)k >= n) return "?";
    return k_perloc_names[k];
}

void px_closure_set_perlocution(px_closure* c,
                                  px_perlocution_kind kind,
                                  const char* outcome_text) {
    if (!c) return;
    c->perlocution_kind = kind;
    if (outcome_text) {
        strncpy(c->perlocution_text, outcome_text,
                sizeof(c->perlocution_text) - 1);
        c->perlocution_text[sizeof(c->perlocution_text) - 1] = 0;
    } else {
        c->perlocution_text[0] = 0;
    }
}

px_perlocution_kind px_closure_perlocution_kind(const px_closure* c) {
    return c ? c->perlocution_kind : PX_PERLOC_UNSPECIFIED;
}

const char* px_closure_perlocution_text(const px_closure* c) {
    return c ? c->perlocution_text : "";
}
