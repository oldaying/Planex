/*
 * planex.h - Planex v4 clean-room header
 *
 * v4 = essence derivation v4 (see docs/concepts/essence-derivation-v4-clean.md).
 *
 * PRINCIPLE: UI essence as first principle. The 8 abstractions below are
 * derived directly from the 9 essence categories identified in v3, with
 * NO backward-compatibility constraint. Each essence category gets its
 * own abstraction, sized to fit the essence.
 *
 * The 8 abstractions (one per essence category implemented):
 *
 *   Estimate        - Object / state              (essence #1)
 *   Perception      - Representamen (sign vehicle)  (essence #2)
 *   Interpretant    - Peirce interpretant            (essence #3) NEW
 *   Closure         - Illocutionary force             (essence #4)
 *   Perlocution     - Searle perlocutionary effect    (essence #5) NEW
 *   Relation        - 3-place relational ontology     (essence #6)
 *   px_loop         - Loop topology                   (essence #7)
 *   Breakdown       - Zuhandenheit / Vorhandenheit    (essence #8)
 *
 * Deferred (no implementation):
 *   Adaptation      - Hoffman / Friston               (essence #9)
 *   Medium-ness     - Kay / Engelbart / Victor         (essence #10)
 *
 * Plus a first-class struct (NOT an abstraction):
 *   px_actor        - the human/agent whose situational relation gives
 *                     the boundary meaning; passed into Relation,
 *                     Interpretant, Perlocution, Breakdown.
 *
 * Compatibility: C17, zero external dependencies.
 *
 * ABI breaks from v0.4 / v3 Path B (INTENTIONAL):
 *   - px_intent_kind : enum -> const char*
 *   - px_declare     : now requires px_actor* (no 2-place macro)
 *   - px_loop_new    : now takes 4 bindings (Closure, Perception,
 *                      Interpretant, Perlocution), not 2
 *   - px_closure_*   : no perlocution/feedback/promise/declare/fail
 *                       (moved to Perlocution abstraction)
 *   - px_perception_*: no intended_interpretant/interpret_fn
 *                       (moved to Interpretant abstraction)
 */
#ifndef PLANEX_V4_H
#define PLANEX_V4_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * Version (v4 verification artifact — does NOT bump shipping Planex)
 * ============================================================ */

#define PLANEX_V4_VERSION_MAJOR 0
#define PLANEX_V4_VERSION_MINOR 1
#define PLANEX_V4_VERSION_PATCH 0
#define PLANEX_V4_VERSION "0.1.0-v4"

/* ============================================================
 * Actor — first-class struct (NOT an abstraction)
 *
 * The human (or AI agent) whose situational relation to the system
 * gives the boundary meaning. Per Suchman, Heidegger, Maturana: UI
 * cannot be defined without the actor. But the actor is not itself
 * an essence category — it is a parameter to Relation, Interpretant,
 * Perlocution, and Breakdown.
 *
 * Promoting it to an abstraction would over-claim without 3-tradition
 * convergence on "actor as essence".
 * ============================================================ */

typedef struct px_actor px_actor;

px_actor*    px_actor_new(const char* id, void* user_data);
void         px_actor_free(px_actor* a);
const char*  px_actor_id(const px_actor* a);
void*        px_actor_user_data(const px_actor* a);

/* ============================================================
 * Estimate — essence #1: Object / state
 *
 * State is not a static value. It is an estimate:
 *   - Carries confidence (Bayesian / Friston flavor)
 *   - Can be a trajectory (Time -> Value) for animations
 *   - Changes notify observers
 *   - Can be derived from other estimates (spreadsheet semantics)
 *
 * Inspired by Conal Elliott FRP (Behavior = Time -> a), Friston
 * predictive coding (state as posterior), spreadsheet cells
 * (auto-dependency tracking).
 *
 * UNCHANGED FROM v0.4 — this abstraction was already essence-correct.
 * ============================================================ */

typedef struct px_estimate px_estimate;
typedef void (*px_estimate_observer)(px_estimate* e, void* user);

px_estimate* px_estimate_new(double value, double confidence);
void         px_estimate_free(px_estimate* e);

/* Read the current value, auto-sampling animation if in progress. */
double       px_estimate_now(px_estimate* e);

/* Read the current value WITHOUT auto-sampling. Returns the last
 * set/sampled value. Use px_estimate_now() unless you need to
 * peek at the un-animated value. */
double       px_estimate_value(px_estimate* e);

double       px_estimate_confidence(const px_estimate* e);

/* Set value (cancels any ongoing animation). Notifies observers. */
void         px_estimate_set(px_estimate* e, double value, double confidence);

/* Begin a trajectory from current value to `target` over `duration_ms`.
 * Use px_estimate_now() to read intermediate values. */
void         px_estimate_animate(px_estimate* e, double target, double duration_ms);

/* Sample value at time t_ms (relative to animation start). */
double       px_estimate_sample(px_estimate* e, double t_ms);

bool         px_estimate_is_animating(px_estimate* e);

void         px_estimate_observe(px_estimate* e, px_estimate_observer fn, void* user);

/* Get the current monotonic time in milliseconds. */
double       px_now_ms(void);

/* Derived estimate: value = fn(sources). Auto-recomputes when any
 * source changes. Caller may free the sources array. */
typedef double (*px_derive_fn)(px_estimate* const* sources, int n, void* user);

px_estimate* px_derived_new(px_derive_fn fn, void* user,
                             px_estimate** sources, int n_sources);
void         px_derived_recompute(px_estimate* derived);

/* ============================================================
 * Perception — essence #2: Representamen (sign vehicle)
 *
 * A perception is a pure function: takes a set of Estimates as
 * input and returns a denotation (pixel buffer, a11y tree, log
 * string, etc.). Same inputs -> same output, no side effects.
 *
 * Multiple perceptions can coexist for the same Estimates — one
 * for screen pixels, one for a11y, one for headless test snapshots.
 *
 * v4 BREAK: Perception no longer has set_intended_interpretant or
 * set_interpret_fn. Those moved to the Interpretant abstraction
 * (essence #3). Perception now ONLY produces the representamen.
 * ============================================================ */

typedef struct px_perception px_perception;

typedef void* (*px_perceive_fn)(px_estimate* const* inputs, int n_inputs, void* user);

px_perception* px_perception_new(const char* name,
                                   px_perceive_fn fn,
                                   px_estimate** inputs,
                                   int n_inputs,
                                   void* user);

void           px_perception_free(px_perception* p);
const char*    px_perception_name(const px_perception* p);

/* Invoke this perception's fn with current input values.
 * Returns the produced representamen (caller owns the memory
 * the function returned; if it returned a static buffer, caller
 * must copy before next invoke). NULL if p is NULL or p->fn is NULL. */
void*          px_perception_invoke(px_perception* p);

/* Get the inputs this perception depends on (for inspection). */
px_estimate** px_perception_inputs(const px_perception* p, int* out_count);

/* Total registered perceptions (debug). */
int            px_perception_count(void);

/* ============================================================
 * Interpretant — essence #3: Peirce interpretant  (NEW)
 *
 * The interpretant is the meaning generated in the actor when they
 * encounter the representamen. Peirce's triad is:
 *   representamen -> object -> interpretant
 *
 * The system can express its *intended* interpretant (what it
 * WANTED the actor to take the representamen to mean). The actor's
 * *actual* interpretant is either observed (the actor did X with
 * the representamen) or predicted by an interpret_fn.
 *
 * Mismatch between intended and actual -> Breakdown candidate.
 *
 * v4 BREAK: this abstraction did not exist in v0.4. v3 Path B
 * bolted it onto Perception as a sub-API. v4 makes it first-class.
 * ============================================================ */

typedef struct px_interpretant px_interpretant;

/* A function that predicts the actor's actual interpretant given
 * the representamen and the actor. Returns the predicted interpretant
 * (caller-owned), or NULL if no prediction can be made. */
typedef void* (*px_interpret_fn)(void* representamen,
                                  px_actor* actor,
                                  void* user);

/* Create an interpretant bound to a perception (representamen source)
 * and an actor. Does not take ownership of either. */
px_interpretant* px_interpretant_new(px_perception* representamen_source,
                                       px_actor* actor);
void             px_interpretant_free(px_interpretant* it);

/* System-side: what the system WANTED the actor to take the
 * representamen to mean. Free-text semantics. */
void             px_interpretant_set_intended(px_interpretant* it,
                                                const char* semantics);
const char*      px_interpretant_intended(const px_interpretant* it);

/* Optional: register a function that predicts the actor's actual
 * interpretant from representamen + actor. NULL fn = no prediction. */
void             px_interpretant_set_interpret_fn(px_interpretant* it,
                                                    px_interpret_fn fn,
                                                    void* user);

/* Run the registered interpret_fn on the given representamen.
 * Returns the predicted interpretant, or NULL if no fn or fn returned NULL. */
void*            px_interpretant_predict(px_interpretant* it, void* representamen);

/* Check whether the actual (observed or predicted) interpretant
 * matches the intended semantics. Uses simple string equality
 * between intended and (char*)actual if actual is non-NULL.
 * False if actual is NULL or no intended was set.
 *
 * This is the predicate that triggers Breakdown when false. */
bool             px_interpretant_matches_intended(px_interpretant* it,
                                                    void* actual);

/* ============================================================
 * Closure — essence #4: Illocution
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
 * moved to the Perlocution abstraction (essence #5). Operational
 * status is now derived from the perlocution, not stored on Closure.
 *
 * v4 BREAK: px_intent_kind is now const char* (open symbol system),
 * not an enum. Domains needing custom illocutionary forces pass
 * any string. Built-ins provided as extern const.
 * ============================================================ */

typedef const char* px_intent_kind;

extern const px_intent_kind PX_INTENT_ASSERT;
extern const px_intent_kind PX_INTENT_REQUEST;
extern const px_intent_kind PX_INTENT_PROMISE;
extern const px_intent_kind PX_INTENT_DECLARE;
extern const px_intent_kind PX_INTENT_EXPRESS;

/* String comparison (since strcmp, not ==, is needed). */
bool  px_intent_kind_eq(px_intent_kind a, px_intent_kind b);
const char* px_intent_kind_str(px_intent_kind k);

typedef struct {
    px_intent_kind kind;
    void*          payload;       /* owned by closure until next trigger */
    size_t         payload_size;
} px_intent;

typedef struct px_closure px_closure;
typedef void  (*px_action_fn)(px_intent intent, void* user);
typedef bool  (*px_eval_fn)(void* user);

px_closure* px_closure_new(const char*      goal,
                             px_intent_kind   kind,
                             px_action_fn     action,
                             px_eval_fn       eval,
                             void*            user);

void         px_closure_free(px_closure* c);

void         px_closure_trigger(px_closure* c, void* payload, size_t size);

/* Replay a previously captured intent. The payload is copied
 * internally; safe even if intent.payload points into closure's
 * own last_payload. */
void         px_closure_replay(px_closure* c, px_intent intent);

px_intent    px_closure_last_intent(const px_closure* c);
const char*  px_closure_goal(const px_closure* c);
px_intent_kind px_closure_intent_kind(const px_closure* c);
bool         px_closure_evaluated(const px_closure* c);

/* ============================================================
 * Perlocution — essence #5: Searle perlocutionary effect (NEW)
 *
 * Perlocution is the *effect* of the system's utterance on the
 * actor's mental state — distinct from illocution (Closure, what
 * the system is doing) and from operational status (which is
 * DERIVED from perlocution, not stored separately).
 *
 *   "Saved successfully"              -> PX_PERLOC_INFORM
 *   "Saved. 3 fields were auto-corrected." -> PX_PERLOC_INFORM (+ surprise)
 *   "Validation failed: email required"   -> PX_PERLOC_ALERT
 *   "Working on it..."                -> PX_PERLOC_REASSURE (status=RUNNING)
 *   "Connection lost. Try again?"     -> PX_PERLOC_FRUSTRATE
 *
 * v4 BREAK: this abstraction did not exist in v0.4. v3 Path B bolted
 * it onto Closure as a sub-API. v4 makes it first-class.
 *
 * v4 BREAK: Closure's px_closure_status / px_closure_promise /
 * px_closure_declare / px_closure_fail are GONE. Operational status
 * is now derived from the perlocution (px_perlocution_status).
 * ============================================================ */

typedef enum {
    PX_PERLOC_UNSPECIFIED = 0,
    PX_PERLOC_INFORM,      /* "now you know X"             */
    PX_PERLOC_PERSUADE,    /* "now you should believe X"   */
    PX_PERLOC_REASSURE,    /* "now you need not worry"     */
    PX_PERLOC_ALERT,       /* "now you should attend"      */
    PX_PERLOC_FRUSTRATE,   /* "now you may give up"        */
    PX_PERLOC_SURPRISE,    /* "now you should re-evaluate" */
    PX_PERLOC_COUNT
} px_perlocution_kind;

typedef struct px_perlocution px_perlocution;

/* Create a perlocutionary outcome bound to a closure + actor.
 * Closure provides the illocutionary context; actor is whose
 * mental state is being acted upon. Does not take ownership. */
px_perlocution* px_perlocution_new(px_closure* c, px_actor* actor);
void            px_perlocution_free(px_perlocution* p);

/* Set the perlocutionary force + outcome text. */
void            px_perlocution_set(px_perlocution* p,
                                     px_perlocution_kind kind,
                                     const char* outcome_text);

px_perlocution_kind px_perlocution_kind_get(const px_perlocution* p);
const char*         px_perlocution_text(const px_perlocution* p);
const char*         px_perlocution_kind_str(px_perlocution_kind k);

/* Operational status is DERIVED from perlocution, not stored.
 * IDLE = no perlocution set; RUNNING = REASSURE without terminal;
 * DONE = INFORM/PERSUADE/SURPRISE; FAILED = ALERT/FRUSTRATE. */
typedef enum {
    PX_STATUS_IDLE = 0,
    PX_STATUS_RUNNING,
    PX_STATUS_DONE,
    PX_STATUS_FAILED
} px_operational_status;

px_operational_status px_perlocution_status(const px_perlocution* p);
const char*           px_status_str(px_operational_status s);

/* ============================================================
 * Relation — essence #6: Relational ontology (3-place)
 *
 * UI is a network of relations, not a tree of components. Relations
 * are first-class: queryable, constrainable, subscribable.
 * "Components" are stable configurations of relations, not
 * primitive atoms.
 *
 * Per Heidegger / Simmel: relation is primitive; things are stable
 * configurations of relations. Per Suchman / situatedness: relations
 * are SITUATED — they hold for an actor, not universally.
 *
 * v4 BREAK: the CANONICAL constructor is 3-place (with actor).
 * There is no 2-place wrapper macro. If you want the universal
 * relation, pass actor=NULL.
 * ============================================================ */

typedef struct px_relation px_relation;
typedef struct px_graph    px_graph;

typedef enum {
    PX_REL_BESIDE,        /* spatial:     a beside b             */
    PX_REL_DEPENDS_ON,    /* dependency:  a depends on b         */
    PX_REL_TRIGGERS,      /* causal:      a triggers b           */
    PX_REL_VARIES_WITH,   /* temporal:    a varies with b        */
    PX_REL_AFFORDS,       /* affordance:  a affords action b     */
    PX_REL_CONTAINS,      /* containment: a contains b           */
    /* v4 first-class — Zuhandenheit / breakdown / semiotics */
    PX_REL_WITHDRAWS_FOR, /* Zuhandenheit: a withdrawn from actor b */
    PX_REL_PRESENTS_FOR,  /* breakdown:   a present to actor b     */
    PX_REL_INTERPRETS_AS, /* semiotics:   a read by actor b as c   */
    PX_REL_COUNT
} px_rel_kind;

px_graph*    px_graph_new(void);
void         px_graph_free(px_graph* g);

/* CANONICAL 3-place constructor. actor=NULL means universal. */
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind,
                          void* b, px_actor* actor);

bool         px_has_relation(px_graph* g, void* a, px_rel_kind kind,
                              void* b, px_actor* actor);

typedef struct {
    void** items;
    int    count;
} px_node_list;

/* Query: all nodes related to `node` via `kind` that hold for `actor`.
 * Relations declared with actor=NULL match every actor query;
 * relations declared with a specific actor only match that actor. */
px_node_list px_query(px_graph* g, void* node, px_rel_kind kind,
                        px_actor* actor);
void         px_node_list_free(px_node_list* list);

int          px_graph_count(const px_graph* g);

const char*  px_rel_kind_str(px_rel_kind k);

/* ============================================================
 * px_loop — essence #7: Loop topology
 *
 * The closed loop of:
 *   intent -> action -> state change -> perception -> interpretant
 *           -> perlocution -> next intent
 *
 * v4 BREAK: px_loop_new now takes FOUR bindings (Closure, Perception,
 * Interpretant, Perlocution). v0.4 / v3 took only two. The loop now
 * binds all four essence dimensions of the return edge:
 *   - representamen produced?       (perception_invoked)
 *   - interpretant constructed?     (interpretant_constructed)
 *   - perlocution emitted?          (perlocution_kind)
 *   - breakdown transitioned?        (breakdown_transition)
 * ============================================================ */

typedef struct px_loop px_loop;

px_loop* px_loop_new(px_closure* c,
                       px_perception* p,
                       px_interpretant* it,
                       px_perlocution* per);

void     px_loop_free(px_loop* loop);

/* Run one iteration. If trigger_payload is non-NULL, triggers the
 * closure first; then invokes the perception; then runs the
 * interpretant's interpret_fn if registered; reads the perlocution
 * kind; checks for breakdown transitions. Returns 1 if perception
 * ran, 0 if paused or skipped. */
int      px_loop_step(px_loop* loop, void* trigger_payload, size_t size);

/* Run one iteration WITHOUT triggering — view-only refresh
 * after external state changes (e.g. animation tick). */
int      px_loop_step_view_only(px_loop* loop);

void     px_loop_pause(px_loop* loop);
void     px_loop_resume(px_loop* loop);
bool     px_loop_is_paused(const px_loop* loop);

typedef struct {
    bool   closure_triggered;
    bool   perception_invoked;
    bool   interpretant_constructed;
    int    perlocution_kind;        /* PX_PERLOC_* or 0 */
    int    breakdown_transition;   /* 0=none, +1=entered, -1=recovered */
    double timestamp_ms;
} px_loop_audit_entry;

int      px_loop_audit_count(const px_loop* loop);
int      px_loop_audit_get(const px_loop* loop,
                            px_loop_audit_entry* out, int max_entries);
int      px_loop_replay(px_loop* loop, int n);
void     px_loop_audit_clear(px_loop* loop);

/* Mark this iteration as having triggered a breakdown transition.
 * Called by the Breakdown abstraction when a breakdown is recorded
 * or recovered. The next px_loop_step will record it. */
void     px_loop_mark_breakdown(px_loop* loop, int transition,
                                  const char* reason);

/* ============================================================
 * Breakdown — essence #8: Zuhandenheit / Vorhandenheit
 *
 * Per Heidegger (Zuhandenheit/Vorhandenheit), Winograd/Flores
 * (breakdown-recovery), Dourish (embodiment), Suchman (situatedness):
 * a UI that cannot break down is not a UI. Breakdown is the moment
 * the boundary becomes visible to the actor.
 *
 * Records *semantic* breakdown — the actor's interpretant no longer
 * matches the system's representamen — distinguished from operational
 * loop stall (which px_loop audit captures via perception_invoked=false).
 *
 * Per actor: A's breakdown is not B's. Has a recovery path: actor-driven
 * ("figured it out") or system-driven (undo / explanation / adaptation).
 *
 * In v3 this was a prototype 6th abstraction; in v4 it is canonical.
 * ============================================================ */

typedef struct px_breakdown px_breakdown;

typedef enum {
    PX_BD_NONE = 0,
    PX_BD_INTERPRETANT_MISMATCH,   /* actor misread representamen */
    PX_BD_AFFORDANCE_LOST,          /* tool stopped withdrawing    */
    PX_BD_LOOP_STALL,               /* semantic loop broke         */
    PX_BD_SITUATION_SHIFT,           /* situation changed; old
                                         relations no longer hold   */
    PX_BD_COUNT
} px_breakdown_kind;

px_breakdown* px_breakdown_record(px_actor* actor,
                                    px_breakdown_kind kind,
                                    const char* reason,
                                    void* related);
void          px_breakdown_recover(px_breakdown* b, const char* how);
int           px_breakdown_count(px_actor* actor);
px_breakdown* px_breakdown_get(px_actor* actor, int idx);
const char*   px_breakdown_reason(const px_breakdown* b);
px_breakdown_kind px_breakdown_kind_get(const px_breakdown* b);
const char*   px_breakdown_kind_str(px_breakdown_kind k);
bool          px_breakdown_is_recovered(const px_breakdown* b);

/* Bridge to Relation: declares PX_REL_PRESENTS_FOR(node, actor) —
 * the node is now present-to-hand for this actor (it has broken down). */
void          px_breakdown_to_relation(px_breakdown* b, px_graph* g,
                                         void* node);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_V4_H */
