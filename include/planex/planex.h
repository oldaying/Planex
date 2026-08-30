/*
 * planex.h - Planex aggregate header
 *
 * Planex = Plane + X
 *
 * An experiment: what if a UI library's core abstractions were
 * Relation + Estimate + Closure, not Component + State + Event?
 *
 * Three core abstractions:
 *   - Relation: basic existence (UI is a network, not a tree)
 *   - Estimate: state with time + uncertainty
 *   - Closure:  7-stage interaction unit (Goal→Intent→Action→
 *               Execution→Perception→Interpretation→Evaluation)
 *
 * These come from:
 *   - Cognitive science (Norman 7-stage model, Friston predictive coding)
 *   - Mathematics (relation networks, constraint systems)
 *   - Alternative UI history (Sketchpad 1963, Garnet interactor,
 *     Genera presentation types)
 *
 * Status: Stage 0 — validating abstractions via stdout.
 * Pixel rendering comes in Stage 1.
 *
 * Quick start:
 *
 *   #include "planex/planex.h"
 *
 *   px_graph*     g     = px_graph_new();
 *   px_estimate*  count = px_estimate_new(0, 1.0);
 *
 *   px_closure* inc = px_closure_new(
 *       "increment counter", PX_INTENT_REQUEST,
 *       on_inc, eval_nonneg, &app);
 *
 *   px_declare(g, inc,    PX_REL_TRIGGERS,   count);
 *   px_declare(g, &app,   PX_REL_CONTAINS,   count);
 *
 *   px_closure_trigger(inc, NULL, 0);
 *
 *   px_closure_free(inc);
 *   px_graph_free(g);
 *   px_estimate_free(count);
 *
 * Compatibility: C17, zero external dependencies.
 */
#ifndef PLANEX_H
#define PLANEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================
 * Basic geometry type
 * ============================================================ */

typedef struct {
    float x, y, w, h;
} px_rect;

static inline px_rect px_rect_make(float x, float y, float w, float h) {
    px_rect r = { x, y, w, h };
    return r;
}

/* ============================================================
 * Cross-platform sleep (used by examples + tests)
 * ============================================================ */

#ifdef _WIN32
#include <windows.h>
static inline void px_sleep_ms(int ms) { Sleep(ms); }
#else
#include <time.h>
static inline void px_sleep_ms(int ms) {
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
#endif

/* ============================================================
 * Version
 * ============================================================ */

#define PLANEX_VERSION_MAJOR 0
#define PLANEX_VERSION_MINOR 7
#define PLANEX_VERSION_PATCH 0
#define PLANEX_VERSION "0.7.0"

/* ============================================================
 * Relation — basic existence
 *
 * UI is a network of relations, not a tree of components.
 * Relations are first-class: queryable, constrainable,
 * subscribable. "Components" are stable configurations of
 * relations, not primitive atoms.
 * ============================================================ */

typedef struct px_relation px_relation;
typedef struct px_graph    px_graph;

typedef enum {
    PX_REL_BESIDE,        /* spatial:      a beside b             */
    PX_REL_DEPENDS_ON,    /* dependency:   a depends on b         */
    PX_REL_TRIGGERS,      /* causal:       a triggers b           */
    PX_REL_VARIES_WITH,   /* temporal:     a varies with b        */
    PX_REL_AFFORDS,       /* affordance:   a affords action b     */
    PX_REL_CONTAINS,      /* containment:  a contains b           */
    /* v3 prototype (essence derivation v3, see docs/concepts/   */
    /* essence-derivation-v3.md). These make the 3-place nature */
    /* of relational ontology first-class: every relation can    */
    /* be scoped to an actor (NULL = universal).                */
    PX_REL_WITHDRAWS_FOR, /* Zuhandenheit: a withdrawn from actor b  */
    PX_REL_PRESENTS_FOR,  /* breakdown:   a present to actor b      */
    PX_REL_INTERPRETS_AS, /* semiotics:   a is read by actor b as c */
    PX_REL_COUNT          /* sentinel                                */
} px_rel_kind;

px_graph*    px_graph_new(void);
void         px_graph_free(px_graph* g);

/* Declare a relation. Returns NULL on failure. */
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b);

/* Retire the first relation matching (a, kind, b). Returns true if
 * an edge was removed — the edge-lifecycle counterpart of
 * px_has_relation.
 *
 * EDGE LIFECYCLE CONTRACT: edges name their endpoints by pointer,
 * and nothing cascades. Freeing an endpoint (region, closure,
 * interaction, estimate) without retiring its edges leaves them
 * DANGLING — px_query/px_afford_at will hand callers a dead
 * pointer. The declaring code owns the edges: retire them before
 * freeing the endpoint. (The CI-found dangling-AFFORDS regression
 * in examples/hover_drag_interaction.c is the pinning example;
 * tests/test_v07.c section F locks the discipline.) */
bool         px_undeclare(px_graph* g, void* a, px_rel_kind kind, void* b);

/* Query: does this relation exist? */
bool         px_has_relation(px_graph* g, void* a, px_rel_kind kind, void* b);

/* Query: all nodes related to `node` via `kind`. */
typedef struct {
    void** items;
    int    count;
} px_node_list;

px_node_list px_query(px_graph* g, void* node, px_rel_kind kind);
void         px_node_list_free(px_node_list* list);

/* Get the count of relations of a specific kind in the graph. */
int          px_graph_count(const px_graph* g);

/* ============================================================
 * Estimate — state with time + uncertainty
 *
 * State is not a static value. It is an estimate:
 *   - Carries confidence (Bayesian flavor)
 *   - Can be a trajectory (Time → Value) for animations
 *   - Changes notify observers
 *
 * Inspired by:
 *   - Conal Elliott FRP (Behavior = Time → a)
 *   - Friston predictive coding (state as posterior)
 *   - Spreadsheet cells (auto-dependency tracking)
 * ============================================================ */

typedef struct px_estimate px_estimate;
typedef void (*px_estimate_observer)(px_estimate* e, void* user);

px_estimate* px_estimate_new(double value, double confidence);
void         px_estimate_free(px_estimate* e);

/* v0.5: queries are now pure (const-correct). Previously these
 * auto-sampled animations on every read, which was an L2 semantic
 * leak (apparent query with side effect). The new contract is:
 *
 *   1. px_estimate_set / px_estimate_animate mutate state.
 *   2. px_estimate_advance(e, t_ms) explicitly advances animation
 *      state to time t_ms, finalizing if elapsed >= duration and
 *      caching the interpolated value mid-animation.
 *   3. px_estimate_value / px_estimate_now / px_estimate_is_animating
 *      are pure queries — they read the cached state without
 *      mutating anything.
 *
 * Migration: callers who relied on the old auto-sampling behavior
 * should call px_estimate_advance(e, px_now_ms()) before reading.
 *
 * See docs/concepts/leak-budgets.md §2 for the leak that this retires. */
double       px_estimate_value(const px_estimate* e);
double       px_estimate_confidence(const px_estimate* e);

/* v0.5: explicitly advance an estimate's animation state to time
 * t_ms (typically px_now_ms()). If the animation has finished
 * (elapsed >= duration), finalizes: clears animating, sets value
 * to target, fires observers. If mid-animation, caches the
 * interpolated value at t_ms (does NOT fire observers — continuous
 * change is not a discrete event). If not animating, no-op.
 *
 * Safe to call repeatedly; cheap when not animating. The "advance
 * then read" pattern replaces the old auto-sampling queries. */
void         px_estimate_advance(px_estimate* e, double t_ms);

/* Set value (cancels any ongoing animation). Notifies observers. */
void         px_estimate_set(px_estimate* e, double value, double confidence);

/* Begin a trajectory from current value to `target` over `duration_ms`.
 * Use px_estimate_sample() to read intermediate values, or call
 * px_estimate_advance(e, px_now_ms()) to bring cached value up to
 * current time before reading px_estimate_value(). This is the
 * kernel of all animation in Planex. */
void         px_estimate_animate(px_estimate* e, double target, double duration_ms);

/* Sample value at time t_ms (relative to animation start).
 * Uses ease-out curve. Returns current cached value if not animating.
 * Pure function of (state, t) — no side effects.
 *
 * To get the current animated value at the current wall-clock time,
 * either:
 *   px_estimate_advance(e, px_now_ms());  // then read px_estimate_value
 * or directly:
 *   px_estimate_sample(e, px_now_ms() - start_time);  // if start_time known
 * The first form is preferred (start_time is internal). */
double       px_estimate_sample(px_estimate* e, double t_ms);

/* Get current cached value (alias of px_estimate_value). Kept for
 * back-compat with code that reads the cached value. To get the
 * animated value at the current time, call px_estimate_advance
 * first. */
double       px_estimate_now(const px_estimate* e);

/* Get the current monotonic time in milliseconds.
 * Used as the time source for animations. */
double       px_now_ms(void);

/* Is this estimate currently animating? Pure query — does not
 * finalize. To finalize an in-progress animation, call
 * px_estimate_advance(e, px_now_ms()). */
bool         px_estimate_is_animating(const px_estimate* e);

/* Subscribe to value changes. Returns immediately; observer
 * is called on every px_estimate_set(). */
void         px_estimate_observe(px_estimate* e, px_estimate_observer fn, void* user);

/* v0.6: predictive-coding loop — confidence gains a framework-side
 * consumer (Friston: "state as posterior estimate").
 *
 *   px_estimate_predict(e, expected, tolerance);  // register prediction
 *   px_estimate_set(e, observed, conf);           // resolves it
 *   px_estimate_surprise(e);                       // |observed - expected|
 *
 * On the resolving set: confidence = conf * exp(-|observed - expected|
 * / tolerance). Observations that violate predictions reduce
 * confidence even when the caller asserts 1.0; accurate predictions
 * keep it. One-shot per predict(); tolerance <= 0 is treated as 1.0.
 * Retires the "confidence is decorative" finding (no framework logic
 * consumed it before v0.6). */
void         px_estimate_predict(px_estimate* e, double expected,
                                 double tolerance);

/* v0.6: absolute prediction error of the last prediction-resolved
 * px_estimate_set (0.0 when no prediction was pending). Pure query. */
double       px_estimate_surprise(const px_estimate* e);

/* ============================================================
 * v0.7 (Line 3) — Estimate schema: the describable value contract
 *
 * Rich-typed hosts give every value a type for free; a C framework
 * can pay for the same semantics explicitly. An opt-in schema beside
 * the value declares WHAT the double means (kind), what it is CALLED
 * (name), and HOW it denotates (print) / compares (equal). Not a
 * type system — a describable contract. Zero cost when unset (the
 * zero-dependency rule is non-negotiable): the schema pointer is
 * borrowed, app-owned, typically a static const.
 *
 * What the contract buys:
 *   - tests assert "this estimate is INT and equals 3" (kind + value),
 *     not byte equality through a pointer
 *   - the a11y query side names values through the schema
 *     (px_a11y_set_value_estimate) instead of hand-formatted strings
 *   - the semantic-primitive axiom gains a mechanism carrier beside
 *     its documentation (leak-budgets.md: the void* L1 entry's
 *     retirement path — the contract half closes; the pointer half
 *     stays as documented host cost)
 * ============================================================ */

typedef enum {
    PX_VAL_NONE = 0,   /* no schema declared (the default) */
    PX_VAL_INT,        /* value() is an integer quantity (count, index) */
    PX_VAL_DOUBLE,     /* value() is a real measurement */
    PX_VAL_PERCENT,    /* value() is a 0..100 percentage */
    PX_VAL_BOOL,       /* value() is 0/1 */
    PX_VAL_TEXT,       /* value() is app-managed; print() denotates */
} px_value_kind;

typedef struct {
    px_value_kind kind;
    const char*   name;   /* semantic name ("count", "brightness"); NULL ok */

    /* Optional: denotate the value to text. NULL = kind default
     * (INT "%ld", DOUBLE "%.2f", PERCENT "%.0f%%", BOOL "on"/"off",
     * TEXT "<text>"). */
    void (*print)(double value, char* out, size_t out_size);

    /* Optional: kind-aware equality. NULL = kind default (INT/BOOL/
     * PERCENT exact; DOUBLE within 1e-9). */
    bool (*equal)(double a, double b);
} px_estimate_schema;

/* Declare (or replace, or clear with NULL) this estimate's schema.
 * The schema is BORROWED: the app owns its storage and lifetime —
 * a static const is the intended form. Zero cost when never called. */
void                      px_estimate_set_schema(px_estimate* e,
                                                 const px_estimate_schema* s);

/* The declared schema, or NULL when none (pure query). */
const px_estimate_schema* px_estimate_schema_of(const px_estimate* e);

/* Denotate the estimate's value through its schema (or the kind
 * default; "<untyped value>" when no schema). Writes at most
 * out_size bytes including the terminator. The seam the a11y query
 * side and kind-aware tests read. */
void                      px_estimate_describe(const px_estimate* e,
                                               char* out, size_t out_size);

/* Kind-aware equality through the schema (or the kind default);
 * false when either estimate has no schema or the kinds differ. */
bool                      px_estimate_value_equal(const px_estimate* a,
                                                  const px_estimate* b);

/* Stable name for a kind ("INT", "DOUBLE", ...); "NONE" for 0. */
const char*               px_value_kind_name(px_value_kind k);

/* ============================================================
 * Derived Estimate — automatic dependency tracking
 *
 * A derived estimate's value is computed from other estimates
 * via a pure function. When any source estimate changes, the
 * derived value is recomputed automatically — no manual
 * `recompute_*()` calls needed.
 *
 * Spreadsheet model: derived cell = formula + source cells.
 *
 * Usage:
 *   double sum(const px_estimate** srcs, int n, void* user) {
 *       double s = 0;
 *       for (int i = 0; i < n; i++) s += px_estimate_value(srcs[i]);
 *       return s;
 *   }
 *   px_estimate* srcs[] = {a, b, c};
 *   px_estimate* total = px_derived_new(sum, NULL, srcs, 3);
 *
 * v0.5: cycle detection is now implemented via a per-estimate
 * "recomputing" flag in px_derived_recompute. Cycles no longer
 * cause stack overflow; values along a cycle may be stale.
 * ============================================================ */

typedef double (*px_derive_fn)(px_estimate* const* sources, int n, void* user);

/* Create a derived estimate. The initial value is computed
 * immediately by calling fn with current source values.
 * Sources array is copied internally (caller may free it).
 * Returns NULL on failure. */
px_estimate* px_derived_new(px_derive_fn fn, void* user,
                             px_estimate** sources, int n_sources);

/* Stage 19: Dynamic sources — add/remove at runtime.
 * Lets derived estimates track dynamic lists (e.g. todo items
 * that are added/removed at runtime). */
px_estimate* px_derived_new_dynamic(px_derive_fn fn, void* user);
int          px_derived_add_source(px_estimate* derived, px_estimate* source);
int          px_derived_remove_source(px_estimate* derived, px_estimate* source);
int          px_derived_source_count(const px_estimate* derived);

/* Manually recompute a derived estimate. Usually unnecessary —
 * derived values auto-update when sources change. Use this only
 * when sources are mutated in ways that bypass px_estimate_set()
 * (e.g. animation sampling, direct memory writes).
 *
 * v0.5: cycle detection is now implemented. If the dependency
 * graph contains a cycle (A derived from B, B derived from A),
 * recompute will not stack-overflow; the cycle is broken by a
 * per-estimate "recomputing" flag. Values along the cycle may
 * be stale (the cycle has no well-defined resolution), but the
 * program does not crash. See docs/concepts/leak-budgets.md §2
 * for the L2 leak this retires. */
void         px_derived_recompute(px_estimate* derived);

/* ============================================================
 * Closure — 5-stage execution unit (user → machine direction)
 *
 * Per ADR-0005: Closure was 7 stages (Norman's complete model).
 * Now Closure covers only the execution side (Norman stages 1-4 + 7):
 *   1. Goal          (what the user wants)
 *   2. Intent        (translated to specific intent — typed value)
 *   3. Action        (the actual operation)
 *   4. Execution     (runtime carries it out)
 *   5. Evaluation    (machine-side: did we achieve the goal?)
 *
 * The evaluation side (Norman stages 5-6: Perception, Interpretation)
 * moved to the new Perception abstraction (see below).
 *
 * Mainstream UI libraries model only stages 2-4 (intent→action→exec)
 * as onClick callbacks. Planex models all 5 execution-side stages,
 * plus the evaluation side via Perception.
 *
 * Intent is a *value*, not a callback — this enables undo/redo,
 * replay, AI-agent driving, and time travel.
 * ============================================================ */

typedef enum {
    PX_INTENT_ASSERT,    /* "state is X"          — declare state    */
    PX_INTENT_REQUEST,   /* "please do X"          — ask for action   */
    PX_INTENT_PROMISE,   /* "I will do X"          — async, has time  */
    PX_INTENT_DECLARE,   /* "X is now true"        — change world     */
    PX_INTENT_EXPRESS,   /* "I feel X"             — pure expression  */
    PX_INTENT_COUNT      /* sentinel                                    */
} px_intent_kind;

typedef struct {
    px_intent_kind kind;
    void*          payload;       /* owned by closure until next trigger */
    size_t         payload_size;
} px_intent;

typedef struct px_closure px_closure;
typedef void  (*px_action_fn) (px_intent intent, void* user);
typedef bool  (*px_eval_fn)   (void* user);

/* Create a closure. `goal` is a human-readable description.
 * `action` and `evaluation` may be NULL.
 *
 * Per ADR-0005: the `perception` parameter was REMOVED. Perception
 * is now a separate first-class abstraction — see px_perception_new()
 * below. Existing code that passed a perception callback should move
 * it to a separate px_perception_new() call. */
px_closure* px_closure_new(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_eval_fn       evaluation,
    void*            user);

void         px_closure_free(px_closure* c);

/* v0.7 Line 5 (ADR-0019): create a closure WITH its undo graph —
 * the constructor split that retires the last aggregate L2 leak.
 * The graph arrives with the closure, before any trigger can race
 * it; the ordering mistake (trigger before bind) is unwritable.
 * Passing NULL graph is the explicit no-undo form (same as px_closure_
 * new; the v0.6 one-time warning still guards enabled-undo misuse). */
px_closure*  px_closure_new_with_graph(
                 const char*      goal,
                 px_intent_kind   intent_kind,
                 px_action_fn     action,
                 px_eval_fn       evaluation,
                 void*            user,
                 px_graph*        graph);

/* v0.3: bind a graph to this closure for undo-via-graph.
 * After binding, if px_undo_is_enabled(), px_closure_trigger
 * will automatically snapshot affected Estimates before action.
 * See undo.c and ADR-0002.
 *
 * DEPRECATED since v0.7 (ADR-0019): the two-call form has an
 * ordering dependency the type system cannot enforce — the last
 * aggregate L2 leak. Prefer px_closure_new_with_graph; this stays
 * callable through the deprecation window (deprecation-registry.md). */
void         px_closure_bind_graph(px_closure* c, px_graph* g);

/* Trigger the closure with a payload. The payload is copied
 * internally and retrievable via px_closure_last_intent()
 * until the next trigger — this enables replay/serialization. */
void         px_closure_trigger(px_closure* c, void* payload, size_t size);

/* Replay a previously captured intent through this closure.
 *
 * ADR claim: "intent is a value, enables undo/redo/replay/AI agent driving".
 * This API closes that loop: capture an intent via
 * px_closure_last_intent(), then replay it later (or on another
 * closure with the same action signature).
 *
 * Replay runs the full trigger pipeline (action + perception +
 * evaluation + undo recording if bound). Combined with px_undo(),
 * this gives the classic undo→replay = redo pattern:
 *
 *   1. px_closure_trigger(c, &payload, sizeof(payload));  // apply
 *   2. px_intent captured = px_closure_last_intent(c);     // capture
 *   3. px_undo();                                          // revert
 *   4. px_closure_replay(c, captured);                      // redo
 *
 * Lifetime: replay copies the payload internally, so it is safe
 * to call even when `intent.payload` points into the closure's
 * own last_payload (the common case after px_closure_last_intent).
 * The caller does not need to copy first.
 *
 * For cross-closure replay (e.g., AI agent sends intent from
 * closure A to closure B), the caller is responsible for
 * ensuring B's action can interpret A's payload format. */
void         px_closure_replay(px_closure* c, px_intent intent);

/* Get the last triggered intent (for replay, logging, undo/redo). */
px_intent    px_closure_last_intent(const px_closure* c);

/* Has the goal been evaluated as achieved? */
bool         px_closure_evaluated(const px_closure* c);

/* ============================================================
 * Closure Feedback (Stage 17 — completing the interaction loop)
 *
 * Norman's 7 stages include Perception → Interpretation → Evaluation.
 * Planex's Closure had Action→State→Render but was missing the machine→user
 * feedback channel. These functions complete the loop.
 *
 * Feedback flow:
 *   1. User triggers Closure
 *   2. Action runs, changes Estimate
 *   3. px_closure_feedback() sets what the machine tells the user
 *   4. App reads feedback → a11y announces it, visual shows it
 *
 * Promise/Declare flow (machine-initiated):
 *   1. Machine calls px_closure_promise() — "I will do X"
 *   2. Async work happens
 *   3. Machine calls px_closure_declare() — "X is done"
 *   4. App reads status → updates UI accordingly
 * ============================================================ */

typedef enum {
    PX_CLOSURE_IDLE = 0,      /* not triggered yet */
    PX_CLOSURE_RUNNING,       /* action in progress (promise sent) */
    PX_CLOSURE_DONE,          /* completed successfully (declare sent) */
    PX_CLOSURE_FAILED,       /* failed */
} px_closure_status;

/* Set feedback text — what the machine tells the user after action.
 * Read by px_closure_feedback(). Automatically sent to a11y if available. */
void         px_closure_set_feedback(px_closure* c, const char* text);
const char*  px_closure_feedback(const px_closure* c);

/* Machine-initiated: promise to do something (async action started).
 * Sets status to RUNNING. Sets feedback to message. */
void         px_closure_promise(px_closure* c, const char* message);

/* Machine-initiated: declare that something is done.
 * Sets status to DONE. Sets feedback to message. */
void         px_closure_declare(px_closure* c, const char* message);

/* Machine-initiated: declare failure.
 * Sets status to FAILED. Sets feedback to message. */
void         px_closure_fail(px_closure* c, const char* message);

/* Get current status of the closure. */
px_closure_status px_closure_get_status(const px_closure* c);

/* Human-readable status name. */
const char*  px_closure_status_str(px_closure_status s);

/* Human-readable intent kind name (for debugging). */
const char*  px_intent_kind_str(px_intent_kind k);

/* Human-readable relation kind name (for debugging). */
const char*  px_rel_kind_str(px_rel_kind k);

/* ============================================================
 * Perception — 4th abstraction (machine → user direction)
 *
 * Per ADR-0005: Perception is a first-class abstraction covering
 * Norman's stages 5 (Perception) and 6 (Interpretation, user-side).
 * It is the denotation of state — how Estimates become perceivable.
 *
 * A perception is a pure function: it takes a set of Estimates as
 * input and returns a denotation (pixel buffer, a11y tree, log
 * string, etc.). Same inputs → same output, no side effects.
 *
 * Multiple perceptions can coexist for the same Estimates — one
 * for screen pixels, one for a11y, one for headless test snapshots.
 * This is the (c)-route validated by counter_denotative.c,
 * calculator_denotative.c, and counter_interactive.c.
 *
 * Phase 1 (this version): API exists, internally a registry stub.
 * Phase 2 (v0.3): runtime will auto-invoke perceptions when their
 * source Estimates change.
 * ============================================================ */

typedef struct px_perception px_perception;

/* A perception function: takes a set of Estimates as input,
 * returns a denotation (caller-owned).
 * Pure function: same inputs → same output, no side effects. */
typedef void* (*px_perceive_fn)(px_estimate* const* inputs, int n_inputs, void* user);

/* Create a perception. `name` is for debugging / a11y.
 * `inputs` is the array of Estimates this perception depends on.
 * The array is copied internally (caller may free it).
 * Returns NULL on failure. */
px_perception* px_perception_new(
    const char*     name,
    px_perceive_fn   fn,
    px_estimate**    inputs,
    int              n_inputs,
    void*            user);

void           px_perception_free(px_perception* p);

const char*    px_perception_name(const px_perception* p);

/* v0.6: register the destructor used to free cached representamens.
 * Perception fns are heterogeneous (px_fb*, char*, ...), so the
 * framework cannot guess how to free a denotation; the caller who
 * knows the type registers its destructor here. From then on, cache
 * overwrites, turn-boundary clears, and px_perception_free all free
 * the old value through it. NULL restores the v0.5 behavior (leak on
 * overwrite). Retires the documented v0.5 representamen leak for
 * perceptions that declare their denotation type.
 *
 * Ownership contract: once cached, the perception owns the
 * representamen; px_perception_invoke_single returns a borrowed
 * pointer valid until the next turn boundary. */
void           px_perception_set_free_fn(px_perception* p,
                                         void (*free_fn)(void*));

/* Phase 2 (v0.3): query API — which perceptions depend on a given Estimate?
 * Used by the app loop to know which perceptions to re-evaluate.
 * Returns heap-allocated array (caller frees) of matching perceptions.
 * *out_count is set to the number of matches (0 if none). */
px_perception** px_perceptions_for_estimate(px_estimate* est, int* out_count);

/* Total registered perceptions (for debugging). */
int            px_perception_count(void);

/* v0.5 Phase 2: perceptions are now auto-invoked when their source
 * estimates change (px_estimate_set triggers invoke_for_estimate
 * internally). The three invoke_* operations below are kept as
 * DIAGNOSTIC seams — manual invocation is for testing/debugging
 * and for the px_loop's explicit view-refresh path. In normal
 * application code, you no longer need to call these.
 *
 * See docs/concepts/leak-budgets.md §4 for the L2 leak this retires
 * (the three ops previously "existed only because Phase 2 was not
 * wired up"; that is no longer their raison d'être). */

/* Invoke all registered perceptions. Calls each perception's fn
 * with its inputs and user. Returns the number of perceptions invoked.
 *
 * Diagnostic — auto-invocation handles the normal case (estimate
 * changes). Use this for: tests, benchmarks, explicit view refresh
 * outside a px_loop. */
int            px_perception_invoke_all(void);

/* Invoke a single perception's fn and return the produced
 * representamen. Used by px_loop_step to obtain the representamen
 * before calling px_perception_interpret.
 *
 * Returns NULL if p is NULL or p->fn is NULL. */
void*          px_perception_invoke_single(px_perception* p);

/* Invoke only perceptions that depend on the given Estimate.
 * More efficient than invoke_all when only one Estimate changed.
 * Returns the number of perceptions invoked.
 *
 * Diagnostic — auto-invoked by px_estimate_set when an estimate
 * changes. Use this for tests or for re-perceiving after a state
 * change that bypassed estimate_set (e.g. direct memory write). */
int            px_perception_invoke_for_estimate(px_estimate* est);

/* ============================================================
 * Undo via Relation graph (v0.3)
 *
 * Per ADR-0002: Relation's necessity is proven by undo-via-graph.
 * When a Closure triggers, we snapshot only the Estimates reachable
 * from that Closure via PX_REL_TRIGGERS edges. This is impossible
 * in Solid.js because Solid tracks dependencies per-effect, not as
 * a globally queryable graph.
 *
 * px_undo_record(closure): snapshots affected Estimates before action
 * px_undo(): restores the last snapshot
 * px_undo_count(): how many undo steps are available
 * px_undo_clear(): clears the undo history
 * ============================================================ */

/* Snapshot Estimates affected by a Closure (via TRIGGERS relation)
 * and push onto undo stack. Called by px_closure_trigger when
 * undo recording is enabled.
 * Returns the number of Estimates snapshotted, or -1 on error. */
int            px_undo_record(px_graph* g, px_closure* c);

/* Restore the last undo snapshot. Returns the number of Estimates
 * restored, or 0 if no undo available. */
int            px_undo(void);

/* Number of undo steps available. */
int            px_undo_count(void);

/* Clear all undo history. */
void           px_undo_clear(void);

/* Enable/disable undo recording globally (default: disabled).
 * When disabled, px_closure_trigger does NOT call px_undo_record.
 * This avoids unnecessary snapshotting when undo is not needed. */
void           px_undo_set_enabled(bool enabled);
bool           px_undo_is_enabled(void);

/* ============================================================
 * Feedback — Closed-loop coupling (v0.4, ADR-0008)
 *
 * Feedback is the 5th essence category (per essence-derivation-v2.md).
 * It is the closed loop of:
 *
 *   intent → action → state change → perception → next intent
 *
 * Before v0.4, this loop was implicit — application code manually
 * called px_perception_invoke_for_estimate() after px_closure_trigger().
 * The loop existed structurally but was not first-class.
 *
 * v0.4 promoted the loop to first-class via `px_loop`.
 * v0.5 closed the implicit-seam gap: px_estimate_set now auto-invokes
 * dependent perceptions (Phase 2). The loop's role narrows to
 * audit + interpretant capture + replay (still essential).
 *
 * `px_loop` API:
 *
 *   1. px_loop_new(closure, perception) — bind intent side to view side
 *   2. px_loop_step(loop) — run one full iteration:
 *        a. if closure has pending intent: trigger it
 *        b. invoke perception (sees post-action state)
 *        c. record this iteration in the audit log
 *   3. px_loop_audit() — return the iteration history
 *   4. px_loop_pause() / px_loop_resume() — interrupt the loop
 *   5. px_loop_replay(audit, n) — replay the last n iterations
 *
 * Why this is essence, not feature:
 * - Heidegger: breakdown is the moment feedback fails — essence
 *   revealed by its absence
 * - Suchman: feedback is what makes action situated
 * - Math (CSP/statechart): loop/transition is primitive
 * - Norman: "to be in charge, the user must be informed" (feedback
 *   closes the gulf of evaluation)
 *
 * Without Feedback as first-class, Planex cannot:
 * - audit which perception fired after which trigger
 * - interrupt the loop (batch updates, modal blocking)
 * - replay trigger→perception sequences (testing, debugging)
 * - detect breakdown (perception failed to fire, loop stalled)
 * ============================================================ */

typedef struct px_loop px_loop;

/* Create a loop binding a Closure (intent side) to a Perception
 * (view side). The loop does NOT take ownership — caller still owns
 * both. Returns NULL on failure. */
px_loop*        px_loop_new(px_closure* c, px_perception* p);

void            px_loop_free(px_loop* loop);

/* Run one iteration of the loop. If `trigger_payload` is non-NULL,
 * triggers the closure first; then invokes the perception.
 * Returns 1 if perception ran, 0 if paused or skipped. */
int             px_loop_step(px_loop* loop,
                              void* trigger_payload,
                              size_t trigger_size);

/* Run one iteration WITHOUT triggering — useful for view-only refresh
 * after external state changes (e.g., animation tick). */
int             px_loop_step_view_only(px_loop* loop);

/* Pause/resume the loop. While paused, step() is a no-op. */
void            px_loop_pause(px_loop* loop);
void            px_loop_resume(px_loop* loop);
bool            px_loop_is_paused(const px_loop* loop);

/* Audit log: each iteration records (closure triggered? perception
 * invoked? timestamp). Returns the count, or fills `out` if non-NULL. */
typedef struct {
    bool   closure_triggered;
    bool   perception_invoked;
    /* v3 prototype: semantic dimensions of the loop's return edge. */
    bool   interpretant_constructed;  /* did interpret_fn run? */
    int    perlocution_kind;           /* PX_PERLOC_* or 0       */
    int    breakdown_transition;      /* 0=none, +1=entered,    */
                                       /* -1=recovered           */
    double timestamp_ms;
    /* v0.6: feedback-budget dimensions (the "instantly visible" half
     * of the feedback axiom, made measurable). iteration_ms is the
     * wall-clock duration of the step; budget_ms is the loop's
     * declared deadline (0 = none); budget_exceeded is
     * budget_ms > 0 && iteration_ms > budget_ms. */
    double iteration_ms;
    double budget_ms;
    bool   budget_exceeded;
    /* v0.7 Line 2 (budget as contract): propagation accounting — what
     * did PROPAGATION cost this step, as opposed to what the frame
     * cost? propagation_edges is the number of Relation-graph edges
     * examined during the step (undo snapshots, affordance queries,
     * dependency lookups — see px_relation_edges_walked());
     * propagation_depth is the deepest derived-recompute chain
     * observed since the previous audit entry was written (see
     * px_derive_depth_peak()). Both are zero for steps whose
     * propagation touches no graph and no deriveds — which is itself
     * an honest answer. */
    unsigned long long propagation_edges;
    int                propagation_depth;
} px_loop_audit_entry;

int             px_loop_audit_count(const px_loop* loop);
int             px_loop_audit_get(const px_loop* loop,
                                  px_loop_audit_entry* out,
                                  int max_entries);

/* v0.7 Line 2 (budget as contract): every loop has a budget by
 * DEFAULT — PX_LOOP_DEFAULT_BUDGET_MS (16ms, one 60fps frame), the
 * feedback axiom's deadline. A loop that ships without a deadline is
 * the Garnet/Amulet failure mode (propagation cost never measured is
 * never budgeted), so the default is finite; passing 0 remains the
 * EXPLICIT opt-out ("no deadline, on purpose"), not the silent
 * default. Negative values are clamped to 0.
 *
 * Every audit entry then carries iteration_ms / budget_ms /
 * budget_exceeded plus the v0.7 propagation dimensions — "was the
 * feedback timely, and what did propagation cost?" are queryable
 * facts, not hopes. An overrun is LOUD: warn-once on stderr in every
 * build, abort via assert when the library is compiled with
 * -DPX_DEBUG_BUDGET (strict mode). px_loop_budget_overruns() counts. */
#define PX_LOOP_DEFAULT_BUDGET_MS 16.0

void            px_loop_set_budget(px_loop* loop, double budget_ms);
double          px_loop_budget(const px_loop* loop);

/* v0.7: how many audit entries exceeded the budget so far. Pure
 * query; the warn-once stderr notice fires on the first only. */
int             px_loop_budget_overruns(const px_loop* loop);

/* v0.7 Line 2: propagation accounting, process-global counters.
 * px_relation_edges_walked() is the monotonic count of relation
 * edges examined by graph traversals since process start (the audit
 * entry records per-step deltas). px_derive_depth_peak() returns the
 * deepest derived-recompute chain observed since its previous call
 * (read-and-reset; px_loop_step calls it once per iteration). */
unsigned long long px_relation_edges_walked(void);
int                px_derive_depth_peak(void);   /* pure read */
void               px_derive_depth_reset(void);  /* explicit reset */

/* Replay the last `n` audit entries: re-trigger each closure that
 * was triggered, re-invoke each perception that was invoked. Useful
 * for testing/debugging. Returns the count actually replayed. */
int             px_loop_replay(px_loop* loop, int n);

/* Clear audit log. */
void            px_loop_audit_clear(px_loop* loop);

/* ============================================================
 * Layout — Relation-driven (Stage 18: kernel fix)
 *
 * Before: developer hand-writes coordinates in render().
 * After: Relation graph drives layout — BESIDE = horizontal,
 *        CONTAINS = parent/child, stacked = vertical.
 *
 * Helpers for common layouts (no full constraint solver yet):
 * ============================================================ */

/* Place a rect to the right of prev_rect. */
px_rect px_layout_beside(px_rect prev, int width, int gap);

/* Place a rect below prev_rect. */
px_rect px_layout_below(px_rect prev, int height, int gap);

/* Center a rect in a container. */
px_rect px_layout_center(px_rect container, int w, int h);

/* ============================================================
 * Render — Stage 1 helper (not a new abstraction!)
 *
 * Render is conceptually a *Closure* that consumes Relation graph +
 * Estimates and produces pixels. It does NOT add a 4th abstraction;
 * it's a view onto the 3 existing abstractions.
 *
 * See planex/fb.h for framebuffer API (fill_rect / draw_text / save_bmp).
 * ============================================================ */

#include "planex/fb.h"

/* ============================================================
 * Window — Stage 2 (X11 backend; Stage 3+ adds Win32/Cocoa)
 *
 * A window owns a framebuffer; backends copy fb to native surface.
 * Same Relation + Estimate + Closure pipeline works on any backend.
 *
 * See planex/window.h for window + event API.
 * ============================================================ */

#include "planex/window.h"

/* Accessibility (Stage 16) — screen reader support.
 * See planex/a11y.h for role/state/announce API. */
#include "planex/a11y.h"

/* ============================================================
 * v3 PROTOTYPE — essence derivation v3 (Path B)
 *
 * Per docs/concepts/essence-derivation-v3.md, v2 sampled only 6
 * traditions and missed 3 (semiotics, cybernetics, perlocutionary
 * pragmatics), which surfaces 4 essence categories Planex v0.4
 * does not first-class cover:
 *   - Interpretant (Peirce triad's third term)
 *   - Perlocution (Searle speech-act level 3)
 *   - Breakdown (Heidegger Zuhandenheit / Winograd-Flores / Dourish)
 *   - 3-place Relational-ontology (Situatedness; actor parameter)
 *
 * Path B (recommended in v3 doc) keeps v0.4's 5 abstractions,
 * augments Closure/Perception/Relation internally, and adds a
 * 6th abstraction px_breakdown. This header section is the
 * prototype API — implementing only this much validates that
 * the v3 essence categories are expressible in Planex's C17,
 * zero-dependency style without breaking v0.4 ABI for callers
 * that stick to the old API surface.
 *
 * Status: prototype, not yet canonical. See ADR-0009 (Proposed)
 * and essence-derivation-v3.md Part V for the design rationale.
 * ============================================================ */

/* ---------- Actor (first-class struct, NOT an abstraction) --------
 * The Actor is the human (or AI agent) whose situational relation
 * to the system gives the boundary meaning. Per Suchman, Heidegger,
 * Maturana: UI cannot be defined without the actor. But the actor
 * is a parameter to Relation/Breakdown/Perlocution/Interpretant,
 * not itself an essence abstraction.
 */
typedef struct px_actor px_actor;

px_actor*    px_actor_new(const char* id, void* user_data);
void         px_actor_free(px_actor* a);
const char*  px_actor_id(const px_actor* a);
void*        px_actor_user_data(const px_actor* a);

/* ---------- Relation: 3-place variant ----------------------------
 * px_declare_for accepts an actor parameter; the old px_declare
 * is preserved as a wrapper that passes actor=NULL (universal).
 * NULL actor means the relation holds for all actors; non-NULL
 * means it holds only for that actor (situated).
 */
px_relation* px_declare_for(px_graph* g, void* a, px_rel_kind kind,
                            void* b, px_actor* actor);

/* Query: relations of `kind` that hold for `actor` (NULL=universal).
 * Relations declared with actor=NULL match every actor query;
 * relations declared with a specific actor only match that actor. */
px_node_list px_query_for(px_graph* g, void* node, px_rel_kind kind,
                          px_actor* actor);

/* ---------- Closure: perlocution sub-API -------------------------
 * Perlocution is the *effect* of the system's utterance on the
 * actor's mental state — distinct from closure_status (operational)
 * and from intent_kind (illocutionary force of the actor's input).
 * E.g. "Saved successfully" (INFORM) vs "Saved. 3 fields were
 * auto-corrected." (INFORM + surprise) vs "Validation failed"
 * (ALERT) are three different perlocutionary outcomes even when
 * status=DONE in all cases.
 */
typedef enum {
    PX_PERLOC_UNSPECIFIED = 0,
    PX_PERLOC_INFORM,      /* "now you know X"             */
    PX_PERLOC_PERSUADE,    /* "now you should believe X"  */
    PX_PERLOC_REASSURE,    /* "now you need not worry"    */
    PX_PERLOC_ALERT,       /* "now you should attend"      */
    PX_PERLOC_FRUSTRATE,   /* "now you may give up"        */
    PX_PERLOC_SURPRISE,    /* "now you should re-evaluate" */
    PX_PERLOC_COUNT
} px_perlocution_kind;

void                  px_closure_set_perlocution(px_closure* c,
                                                  px_perlocution_kind kind,
                                                  const char* outcome_text);
px_perlocution_kind   px_closure_perlocution_kind(const px_closure* c);
const char*           px_closure_perlocution_text(const px_closure* c);
const char*           px_perlocution_kind_str(px_perlocution_kind k);

/* ---------- Perception: interpretant sub-API ---------------------
 * The system's intended interpretant is what the system *wanted*
 * the actor to take the representamen to mean. Optionally, an
 * interpret_fn predicts the actor's actual interpretant given
 * the representamen + actor (Layer 5 hook; NULL = no prediction).
 *
 * The loop's audit records interpretant_constructed=true iff an
 * interpret_fn was registered and successfully returned a non-NULL
 * interpretant for this iteration.
 */
typedef void* (*px_interpret_fn)(void* representamen,
                                  px_actor* actor,
                                  void* user);

void          px_perception_set_intended_interpretant(px_perception* p,
                                                       const char* semantics);
void          px_perception_set_interpret_fn(px_perception* p,
                                              px_interpret_fn fn,
                                              void* user);
const char*   px_perception_intended_interpretant(const px_perception* p);

/* Invoke the registered interpret_fn for this perception + actor.
 * Called by px_loop_step after the perceive fn produced a
 * representamen. Returns the interpretant, or NULL if no
 * interpret_fn is registered or it returned NULL. */
void*         px_perception_interpret(px_perception* p,
                                       void* representamen,
                                       px_actor* actor);

/* ---------- px_loop: extended audit + breakdown integration -------
 * Mark this iteration as having triggered a breakdown transition
 * (+1 entered, -1 recovered). The next px_loop_step will record
 * the transition in its audit entry's breakdown_transition field.
 */
void          px_loop_mark_breakdown(px_loop* loop, int transition,
                                      const char* reason);

/* ---------- Breakdown (6th abstraction, v3 prototype) ------------
 * Per Heidegger Zuhandenheit/Vorhandenheit, Winograd/Flores
 * breakdown-recovery, Dourish embodiment, Suchman situatedness:
 * a UI that cannot break down is not a UI. Breakdown is the moment
 * the boundary becomes visible to the actor.
 *
 * This abstraction records *semantic* breakdown — the actor's
 * interpretant no longer matches the system's representamen —
 * distinguished from operational loop stall (which px_loop audit
 * captures via perception_invoked=false).
 *
 * A Breakdown is *per actor* (A's breakdown is not B's) and has
 * a *recovery path*. Recovery can be actor-driven (the user figures
 * it out) or system-driven (undo, explanation, adaptation).
 */
typedef struct px_breakdown px_breakdown;

typedef enum {
    PX_BD_NONE = 0,
    PX_BD_INTERPRETANT_MISMATCH,  /* actor misread representamen */
    PX_BD_AFFORDANCE_LOST,          /* tool stopped withdrawing    */
    PX_BD_LOOP_STALL,               /* semantic loop broke          */
    PX_BD_SITUATION_SHIFT,           /* situation changed, old
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
const char*   px_breakdown_kind_str(px_breakdown_kind k);
bool          px_breakdown_is_recovered(const px_breakdown* b);

/* Bridge to Relation: a breakdown declares PX_REL_PRESENTS_FOR
 * for the related node + actor, marking the node as present-to-hand
 * (it has broken down for this actor). */
void          px_breakdown_to_relation(px_breakdown* b, px_graph* g,
                                         void* node);

/* ============================================================
 * v0.6 PROTOTYPE — Interaction: a continuous interaction process
 * (6th abstraction candidate)
 *
 * Per ADR-0016 (proposed) and continuous-intent-speculation.md
 * Option B. ADR-0006 deferred the continuous-interaction
 * abstraction to v1.0+ pending evidence; hover_drag_4abs.c supplied
 * that evidence ("INTOLERABLE for complex gesture/touch UIs").
 * This prototype gathers the next round of evidence WITHOUT
 * touching the 5 canonical abstractions — same protocol as the
 * v3 prototype section above.
 *
 * What it models: a process with identity, trajectory, and outcome.
 *
 *     begin(ev) → sample(ev)* → commit(ev) | cancel(reason)
 *
 * vs the existing abstractions (orthogonality):
 *   - Estimate  = persistent state ("state with time + uncertainty").
 *                 An interaction is NOT state: it has bounded
 *                 lifetime and an outcome. Samples do NOT notify
 *                 observers or auto-invoke perceptions — the 60Hz
 *                 hot path is inert. This is what kills the
 *                 hover_drag_4abs.c HACKs 1-3 (mouse position as
 *                 Estimate = observer fan-out per mouse move).
 *   - Closure   = discrete speech act (ASSERT/REQUEST/...). An
 *                 interaction is not an act but a PROCESS that
 *                 RESOLVES to an act at commit — bridged via
 *                 px_interaction_on_commit / the phase hook. This
 *                 kills HACKs 4-5 ("begin drag" as REQUEST, mouse
 *                 move as EXPRESS).
 *   - Perception = pure function. An interaction is mutable — the
 *                 only Planex object allowed to change per-sample
 *                 without notification. Perceptions may READ the
 *                 current sample (via user data) as an input.
 *   - Relation  = graph. Interactions participate in relations
 *                 like any node (e.g. region AFFORDS interaction,
 *                 interaction TRIGGERS closure).
 *
 * Denotational semantics: an interaction denotes a trajectory
 * [t0..tn] → Sample plus an outcome ∈ {committed, cancelled}.
 * Queries are pure functions of that trajectory.
 *
 * Traditions sampled (see ADR-0016 for the full derivation):
 *   - Garnet Interactors (Myers 1990) — interaction technique as object
 *   - CSP process algebra (Hoare) — prefix + choice: P = begin→P', commit⊕cancel
 *   - Statecharts (Harel) — the missing "do action" of ongoing activity
 *   - Direct manipulation (Shneiderman 1983) — "continuous representation,
 *     incremental actions" — the process IS the unit
 *   - FRP (Elliott) — Behavior covers continuous values, not
 *     bounded processes with outcomes (the gap this fills)
 * ============================================================ */

typedef struct px_interaction px_interaction;

/* Lifecycle phase of a process. IDLE→BEGAN→ACTIVE→(COMMITTED|CANCELLED)
 * is the only legal progression; terminal ops are idempotent no-ops
 * after the first call. */
typedef enum {
    PX_INT_IDLE = 0,
    PX_INT_BEGAN,       /* begin() called, no samples yet            */
    PX_INT_ACTIVE,      /* at least one sample appended              */
    PX_INT_COMMITTED,   /* ended successfully — outcome = committed  */
    PX_INT_CANCELLED    /* aborted — outcome = cancelled             */
} px_int_phase;

const char* px_int_phase_str(px_int_phase p);

/* A single sample. Deliberately a value type with no pointers —
 * samples are data, not objects. Channels:
 *   x, y   — primary 2D channel (pointer position, touch point)
 *   value  — generic scalar channel (pressure, angle, wheel ticks)
 *   button — button/key id carried through the process (0 = none)
 *   flags  — app-defined bit flags (e.g. modifier keys)          */
typedef struct {
    double   t_ms;      /* timestamp: px_now_ms() or app clock */
    double   x, y;
    double   value;
    int      button;
    unsigned flags;
} px_int_sample;

/* Phase-transition callback. Called at begin/commit/cancel ONLY —
 * never per sample. This is the process→intent compilation point:
 * the hook inspects the trajectory (px_interaction_*) and triggers
 * the Closure the process resolves to. */
typedef void (*px_int_hook)(px_interaction* it, px_int_phase phase, void* user);

/* Create an interaction that can retain `capacity` samples (ring
 * buffer; oldest are overwritten when full, total count is kept).
 * capacity <= 0 uses 32. Returns NULL on failure. */
px_interaction* px_interaction_new(const char* name, int capacity);
void            px_interaction_free(px_interaction* it);
const char*     px_interaction_name(const px_interaction* it);

/* Feed the process. sample() auto-begins an IDLE interaction.
 * Terminal ops after the first are no-ops. commit()/cancel() fire
 * the phase hook and the bound bridge (below). cancel() records a
 * reason string (queryable; NULL = "(none)"). */
void            px_interaction_begin(px_interaction* it);
void            px_interaction_sample(px_interaction* it, const px_int_sample* s);
void            px_interaction_commit(px_interaction* it);
void            px_interaction_cancel(px_interaction* it, const char* reason);

/* Pure queries. */
px_int_phase        px_interaction_phase(const px_interaction* it);
const char*         px_interaction_cancel_reason(const px_interaction* it);
const px_int_sample* px_interaction_last(const px_interaction* it);   /* NULL if none */
int                 px_interaction_stored(const px_interaction* it);  /* ≤ capacity */
int                 px_interaction_total(const px_interaction* it);   /* since begin */
const px_int_sample* px_interaction_at(const px_interaction* it, int i); /* 0 = oldest stored */

/* Derived measures over the retained trajectory (pure, O(stored)).
 * duration = last.t - first-stored.t; displacement = |last - first|;
 * path_length = Σ |s[i+1] - s[i]|; velocity = |last - prev| / Δt
 * (0 when fewer than two samples or Δt <= 0). */
double          px_interaction_duration_ms(const px_interaction* it);
double          px_interaction_displacement(const px_interaction* it);
double          px_interaction_path_length(const px_interaction* it);
double          px_interaction_velocity(const px_interaction* it);

/* Bridge 1 — phase hook: the process→intent compilation point. */
void            px_interaction_on_phase(px_interaction* it, px_int_hook fn, void* user);

/* Bridge 2 — commit/cancel resolve to Closure triggers (discrete
 * intents). The payload is copied at BIND time (intent-as-value:
 * the payload becomes part of last_intent and is replayable).
 * Pass NULL/0 for closures whose action reads the interaction via
 * its user pointer instead. */
void            px_interaction_on_commit(px_interaction* it, px_closure* c,
                                         void* payload, size_t payload_size);
void            px_interaction_on_cancel(px_interaction* it, px_closure* c);

/* Bridge 3 — publish the phase to an Estimate at TRANSITIONS only
 * (never per sample). The estimate is set to (double)phase:
 * IDLE=0, BEGAN=1, ACTIVE=2, COMMITTED=3, CANCELLED=4. This is the
 * ONLY sanctioned path from interaction to Estimate — the seam that
 * keeps "process" from being conflated with "state". The estimate's
 * lifetime is the caller's responsibility. */
void            px_interaction_publish_phase(px_interaction* it, px_estimate* est);

/* ============================================================
 * v0.6 PROTOTYPE — Region + affordance query (intent compilation)
 *
 * A px_region is pure data (geometry + label) — NOT an abstraction.
 * It exists to be the `a` node of an AFFORDS relation:
 *
 *     px_declare(g, region, PX_REL_AFFORDS, closure);
 *
 * px_afford_at(g, x, y) then answers "which affordance contains
 * (x, y)?" — the missing compile step from PHYSICAL event to
 * SEMANTIC intent (hit-testing as a graph query, per the audit
 * finding that on_click(ev.x, ev.y) raw-coordinate dispatch
 * outsources Norman's execution-gulf translation to every app).
 *
 * Region registry: regions are process-global (like perceptions);
 * px_region_at / px_afford_at scan most-recently-declared-first, so
 * later declarations are "on top" (z-order by declaration order).
 * ============================================================ */

typedef struct px_region px_region;

px_region*  px_region_new(px_rect r, const char* label);
void        px_region_free(px_region* r);
px_rect     px_region_rect(const px_region* r);
const char* px_region_label(const px_region* r);
void        px_region_set_rect(px_region* r, px_rect rect); /* re-layout in place */

/* Which region contains (x, y)? Topmost (latest-declared) wins.
 * NULL if none. Pure query over the registry. */
px_region*  px_region_at(double x, double y);

/* Intent compilation: the closure afforded at (x, y). Finds the
 * topmost containing region, then queries `g` for (region
 * AFFORDS closure) edges; returns the first closure found, NULL
 * if none. The caller triggers it with a semantic payload. */
px_closure* px_afford_at(px_graph* g, double x, double y);

/* ============================================================
 * v0.7 (Line 1) — compiled pointer intents (app-level routing)
 *
 * px_afford_compile is the routing half of intent compilation:
 * the window-free step px_app_run runs when a px_app_desc sets
 * `intent_graph`. A pointer-down at (x, y) resolves against the
 * region registry + the graph's AFFORDS edges; if it lands on an
 * afforded closure, that closure triggers with a px_pointer_intent
 * payload — the app never sees a raw coordinate.
 *
 * The payload is a VALUE by construction: the region label is
 * embedded (not pointed to), so the intent survives capture,
 * serialization, and px_closure_replay. x/y/button ride along as
 * context for the closure's action — they are payload, not the
 * routing key (the routing key was the region).
 * ============================================================ */

/* Matches the label capacity of px_region (see hit.c). */
#define PX_REGION_LABEL_MAX 64

typedef struct {
    double x;     /* pointer x at the event — context, not routing key */
    double y;     /* pointer y at the event — context, not routing key */
    int    button;/* 1=left 2=middle 3=right — meaning is the closure's */
    char   region[PX_REGION_LABEL_MAX]; /* topmost region's label, embedded */
} px_pointer_intent;

/* Compile a pointer-down into (closure, payload). Returns the
 * afforded closure and fills *out; returns NULL (and zeroes *out)
 * when the position hits no region or the region affords nothing
 * — the caller then falls back to raw-coordinate dispatch.
 * Window-free and backend-free by design: the compile step is a
 * pure function of (registry, graph, x, y), so it is testable
 * without a display (tests/test_v07.c section A). */
px_closure* px_afford_compile(px_graph* g, double x, double y,
                              int button, px_pointer_intent* out);

/* ============================================================
 * v0.8 (Line 1) — compiled keyboard intents (the second channel)
 *
 * v0.7 proved the type drives routing for the POINTER channel.
 * The channel axiom (A6) demands the same graph serve every
 * channel; until v0.8 the keyboard had no compile step — a
 * keyboard user could not be served by intent compilation at
 * all (limitations L15a).
 *
 * The focus ring is DERIVED, not declared: a region joins the
 * ring by affording at least one closure (the AFFORDS edges the
 * app already declared). Ring order is region CREATION order —
 * the app-declared order, read forward; the z-order scan is the
 * same registry read backward. No layout-derived ordering: layout
 * is Perception's business, not Relation's.
 *
 * The same resolution rule as pointer compiles applies to a
 * region with several AFFORDS edges: last-declared-first (pinned
 * in tests/test_v07.c a7) — one ring, one rule.
 * ============================================================ */

/* The keyboard intent value — same value contract as
 * px_pointer_intent: the region label is EMBEDDED so the intent
 * survives capture/replay after the region is freed; the key is
 * context for the closure's act, not a routing key (the routing
 * key was the focused region). */
typedef struct {
    char region[PX_REGION_LABEL_MAX]; /* focused region's label, embedded */
    int  key;    /* activating key ('\r', '\n', ' ') — context, not key */
} px_key_intent;

/* The focus ring, in creation order.
 *
 *   px_afford_focus_first(g)  — the first focusable region, NULL
 *                               if the ring is empty
 *   px_afford_focus_next(g, r)  — r's successor; wraps (last -> first)
 *   px_afford_focus_prev(g, r)  — r's predecessor; wraps (first -> last)
 *
 * `from` == NULL starts at the ring's head (the Tab-from-nowhere
 * semantics: the first Tab focuses the first focusable region).
 * A `from` that is freed, or affords nothing (not on the ring),
 * is treated as nowhere: the head is returned. All three are
 * pure queries over (registry, graph) — window-free, testable
 * headless (tests/test_v08.c section A). */
px_region* px_afford_focus_first(px_graph* g);
px_region* px_afford_focus_next(px_graph* g, px_region* from);
px_region* px_afford_focus_prev(px_graph* g, px_region* from);

/* Compile a keyboard activation on the focused region: same
 * resolution as a pointer-down compile, but the routing key is
 * the focus, not a position. Returns the afforded closure and
 * fills *out; returns NULL (and zeroes *out) when `focused` is
 * NULL, freed, or affords nothing — the caller then falls back
 * to the raw-key callback. Window-free by design. */
px_closure* px_afford_compile_focus(px_graph* g, px_region* focused,
                                    int key, px_key_intent* out);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_H */
