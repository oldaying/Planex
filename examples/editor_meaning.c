/*
 * editor_meaning.c — Meaning-formation school prototype
 *
 * Prototype for the phenomenological school documented in
 * docs/concepts/alternative-perspectives.md.
 *
 * What this prototype demonstrates:
 *
 *   The four phenomenological abstractions (Context, Visibility, Trace,
 *   Affordance) can be implemented on top of Planex's existing
 *   Relation + Estimate + Closure, without requiring new core types.
 *
 *   It implements a minimal "code editor session" — the kind of UI
 *   the cognitive/information-transfer school cannot express well,
 *   but the meaning-formation school is designed for.
 *
 * What this prototype proves (or disproves):
 *
 *   1. Context, Trace, Affordance can be built as user-side concepts
 *      on top of existing Planex abstractions.
 *   2. The "undo as natural consequence" claim holds: Trace makes
 *      undo trivial without a dedicated undo system.
 *   3. The "Affordance as dynamic" claim holds: available actions
 *      change as the user navigates the Trace.
 *   4. The "Visibility with history" claim holds: showing the
 *      trajectory, not just current value, is expressible.
 *
 * What this prototype deliberately does NOT do:
 *
 *   - Does NOT add new types to include/planex/*.h
 *   - Does NOT change Planex's core abstractions
 *   - Does NOT claim this is the "right" way — only that it's possible
 *
 * The phenomenological school is recorded in alternative-perspectives.md
 * as a school Planex does NOT adopt. This prototype is a research
 * demonstration, not a commitment to adopt.
 *
 * Build (manual):
 *   cc -std=c17 -I include examples/editor_meaning.c \
 *      src/relation.c src/estimate.c src/closure.c src/fb.c src/font.c \
 *      -lm -o build/editor_meaning
 *
 * Run: ./build/editor_meaning
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TRACE 32
#define MAX_AFFORDANCES 8

/* =================================================================
 * PHENOMENOLOGICAL ABSTRACTION 1: Context
 *
 * The user's current activity frame. Same state, different context
 * → different meaning.
 *
 * Implemented as a tagged wrapper around Planex's px_estimate.
 * The Estimate still holds the value; Context holds the meaning.
 * ================================================================= */

typedef struct {
    const char* activity;       /* what the user is doing, e.g. "debugging init bug" */
    const char* focus_label;    /* what the focused value means in this context */
    time_t      started_at;
} px_context;

static void px_context_init(px_context* ctx, const char* activity) {
    ctx->activity = activity;
    ctx->focus_label = "value";
    ctx->started_at = time(NULL);
}

static void px_context_switch(px_context* ctx, const char* new_activity,
                              const char* new_focus_label) {
    ctx->activity = new_activity;
    ctx->focus_label = new_focus_label;
    ctx->started_at = time(NULL);
    printf("  [Context] switched to: %s (focus: %s)\n",
           new_activity, new_focus_label);
}

/* =================================================================
 * PHENOMENOLOGICAL ABSTRACTION 2: Trace
 *
 * The user's cognitive history — not just state changes, but how
 * the user got here. Each entry records what the user did and what
 * it meant in context.
 *
 * Implemented as a ring buffer of records. Each record references
 * a Closure (which already has Intent-as-value) plus a description
 * of what the user was trying to do.
 *
 * Key property: Trace makes undo trivial. To "go back", just
 * move the trace pointer back and re-derive state from there.
 * No separate undo system needed.
 * ================================================================= */

typedef struct {
    char        description[128];   /* human-readable: "incremented counter" */
    char        intent_kind[32];    /* "REQUEST", "PROMISE", etc. */
    double      state_before;      /* Estimate value before this step */
    double      state_after;       /* after */
    time_t      timestamp;
    bool        is_branch_point;    /* user returned here to fork */
} px_trace_entry;

typedef struct {
    px_trace_entry entries[MAX_TRACE];
    int            count;
    int            current;         /* index of "now" in the trace */
} px_trace;

static void px_trace_init(px_trace* t) {
    t->count = 0;
    t->current = -1;
}

static void px_trace_record(px_trace* t, const char* description,
                              const char* intent_kind,
                              double state_before, double state_after) {
    if (t->count >= MAX_TRACE) {
        /* ring buffer: shift everything left */
        for (int i = 1; i < MAX_TRACE; i++)
            t->entries[i-1] = t->entries[i];
        t->count = MAX_TRACE - 1;
    }
    px_trace_entry* e = &t->entries[t->count];
    snprintf(e->description, sizeof(e->description), "%s", description);
    snprintf(e->intent_kind, sizeof(e->intent_kind), "%s", intent_kind);
    e->state_before = state_before;
    e->state_after = state_after;
    e->timestamp = time(NULL);
    e->is_branch_point = false;
    t->count++;
    t->current = t->count - 1;
}

static void px_trace_jump_to(px_trace* t, int index, px_estimate* est) {
    if (index < 0 || index >= t->count) return;
    t->current = index;
    t->entries[index].is_branch_point = true;
    /* restore state */
    px_estimate_set(est, t->entries[index].state_after, 1.0);
    printf("  [Trace] jumped to step %d: %s (state → %.0f)\n",
           index, t->entries[index].description, t->entries[index].state_after);
}

static void px_trace_print(px_trace* t) {
    printf("  [Trace] cognitive history (%d steps, now at %d):\n",
           t->count, t->current);
    for (int i = 0; i < t->count; i++) {
        const char* marker = (i == t->current) ? "▶" : " ";
        const char* branch = t->entries[i].is_branch_point ? " ⟲" : "";
        printf("    %s %2d. [%s] %s (state: %.0f → %.0f)%s\n",
               marker, i, t->entries[i].intent_kind,
               t->entries[i].description,
               t->entries[i].state_before, t->entries[i].state_after,
               branch);
    }
}

/* =================================================================
 * PHENOMENOLOGICAL ABSTRACTION 3: Affordance
 *
 * What the user can do *now*. Dynamic — recomputed as state and
 * context change. Each affordance is a Closure plus a description
 * of when it's available.
 *
 * Key property: replaces "menus" and "buttons". The UI shows what's
 * possible, not what's been pre-defined.
 * ================================================================= */

typedef struct {
    char        label[64];
    px_closure* closure;
    bool        (*available_fn)(double state, void* user);
    void*       user;
    bool        is_available;
} px_affordance;

typedef struct {
    px_affordance items[MAX_AFFORDANCES];
    int           count;
} px_affordance_set;

static void px_affordance_set_init(px_affordance_set* as) {
    as->count = 0;
}

static void px_affordance_add(px_affordance_set* as, const char* label,
                                px_closure* closure,
                                bool (*available_fn)(double, void*),
                                void* user) {
    if (as->count >= MAX_AFFORDANCES) return;
    px_affordance* a = &as->items[as->count++];
    snprintf(a->label, sizeof(a->label), "%s", label);
    a->closure = closure;
    a->available_fn = available_fn;
    a->user = user;
    a->is_available = false;
}

static void px_affordance_set_recompute(px_affordance_set* as, double state) {
    printf("  [Affordance] recomputing for state=%.0f:\n", state);
    for (int i = 0; i < as->count; i++) {
        px_affordance* a = &as->items[i];
        if (a->available_fn) {
            a->is_available = a->available_fn(state, a->user);
        } else {
            a->is_available = true;
        }
        printf("    %s %s\n",
               a->is_available ? "[✓]" : "[ ]",
               a->label);
    }
}

/* =================================================================
 * PHENOMENOLOGICAL ABSTRACTION 4: Visibility (simplified)
 *
 * How state is made perceivable. In the meaning-formation school,
 * Visibility includes history/trend, not just current value.
 *
 * This prototype uses a simpler Visibility: just shows the trajectory
 * from Trace. A fuller implementation would render the trajectory
 * graphically.
 * ================================================================= */

static void px_visibility_render(px_trace* t, px_context* ctx, double current) {
    printf("  [Visibility] context: %s | focus: %s | current: %.0f\n",
           ctx->activity, ctx->focus_label, current);
    if (t->count > 0) {
        printf("    trajectory: ");
        for (int i = 0; i < t->count; i++) {
            printf("%.0f", t->entries[i].state_after);
            if (i < t->count - 1) printf(" → ");
        }
        printf("\n");
    }
}

/* =================================================================
 * The "App" — a minimal code editor session simulation
 *
 * We use counter as the state (representing "code line being edited"),
 * but the structure is the same as a real editor session.
 * ================================================================= */

typedef struct {
    px_estimate*     line;        /* the "current line number" being edited */
    px_graph*        graph;
    /* Planex core abstractions */
    px_closure*      inc;         /* advance to next line */
    px_closure*      dec;         /* go back a line */
    px_closure*      fix_bug;     /* mark bug as fixed */
    /* Phenomenological abstractions */
    px_context       context;
    px_trace         trace;
    px_affordance_set affordances;
} EditorApp;

/* --- Action callbacks (Planex Closure callbacks) --- */

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    EditorApp* app = user;
    double before = px_estimate_value(app->line);
    px_estimate_set(app->line, before + 1, 1.0);
    double after = px_estimate_value(app->line);
    px_trace_record(&app->trace, "advance to next line",
                    "REQUEST", before, after);
}

static void on_dec(px_intent intent, void* user) {
    (void)intent;
    EditorApp* app = user;
    double before = px_estimate_value(app->line);
    px_estimate_set(app->line, before - 1, 1.0);
    double after = px_estimate_value(app->line);
    px_trace_record(&app->trace, "go back a line",
                    "REQUEST", before, after);
}

static void on_fix_bug(px_intent intent, void* user) {
    (void)intent;
    EditorApp* app = user;
    double v = px_estimate_value(app->line);
    px_trace_record(&app->trace, "marked bug as fixed at this line",
                    "DECLARE", v, v);
    /* context switches because the activity changed */
    px_context_switch(&app->context,
                      "verifying fix", "line under verification");
}

/* --- Affordance availability predicates --- */

static bool avail_always(double state, void* user) {
    (void)state; (void)user;
    return true;
}

static bool avail_only_if_positive(double state, void* user) {
    (void)user;
    return state > 0;
}

static bool avail_only_if_unfixed(double state, void* user) {
    (void)state;
    EditorApp* app = user;
    /* available only if context is still "debugging" */
    return strcmp(app->context.activity, "debugging init bug") == 0;
}

/* =================================================================
 * Main — runs a simulated editor session
 * ================================================================= */

int main(void) {
    printf("Planex editor_meaning — phenomenological school prototype\n");
    printf("==========================================================\n");
    printf("Demonstrates: Context / Trace / Affordance / Visibility\n");
    printf("Built on top of Planex's existing Relation+Estimate+Closure.\n");
    printf("Source: alternative-perspectives.md (school 3, not adopted)\n\n");

    EditorApp app = {0};
    app.line  = px_estimate_new(0, 1.0);
    app.graph = px_graph_new();

    app.inc     = px_closure_new("advance line", PX_INTENT_REQUEST,
                                   on_inc, NULL, &app);
    app.dec     = px_closure_new("go back line", PX_INTENT_REQUEST,
                                   on_dec, NULL, &app);
    app.fix_bug = px_closure_new("mark bug fixed", PX_INTENT_DECLARE,
                                   on_fix_bug, NULL, &app);

    px_declare(app.graph, app.inc,     PX_REL_TRIGGERS, app.line);
    px_declare(app.graph, app.dec,     PX_REL_TRIGGERS, app.line);
    px_declare(app.graph, app.fix_bug, PX_REL_TRIGGERS, app.line);
    px_declare(app.graph, &app,        PX_REL_AFFORDS,  app.inc);
    px_declare(app.graph, &app,        PX_REL_AFFORDS,  app.dec);
    px_declare(app.graph, &app,        PX_REL_AFFORDS,  app.fix_bug);

    /* Initialize phenomenological abstractions */
    px_context_init(&app.context, "debugging init bug");
    app.context.focus_label = "line under inspection";
    px_trace_init(&app.trace);
    px_affordance_set_init(&app.affordances);

    /* Register affordances — these are dynamic, not pre-defined buttons */
    px_affordance_add(&app.affordances, "advance to next line",
                      app.inc, avail_always, NULL);
    px_affordance_add(&app.affordances, "go back a line",
                      app.dec, avail_only_if_positive, NULL);
    px_affordance_add(&app.affordances, "mark bug as fixed at this line",
                      app.fix_bug, avail_only_if_unfixed, &app);

    /* --- Simulate the session --- */

    printf("\n--- Session start ---\n\n");

    /* Initial state */
    printf("[Step 0] initial state\n");
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    px_affordance_set_recompute(&app.affordances, px_estimate_value(app.line));
    printf("\n");

    /* User advances a few lines */
    printf("[Step 1] user action: advance\n");
    px_closure_trigger(app.inc, NULL, 0);
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    px_affordance_set_recompute(&app.affordances, px_estimate_value(app.line));
    printf("\n");

    printf("[Step 2] user action: advance\n");
    px_closure_trigger(app.inc, NULL, 0);
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    printf("\n");

    printf("[Step 3] user action: advance\n");
    px_closure_trigger(app.inc, NULL, 0);
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    printf("\n");

    /* User realizes they need to inspect an earlier line — jump back */
    printf("[Step 4] user wants to inspect an earlier line\n");
    printf("  (in cognitive school: would need a separate 'undo' button)\n");
    printf("  (in phenomenological school: just navigate the Trace)\n");
    px_trace_jump_to(&app.trace, 1, app.line);
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    px_affordance_set_recompute(&app.affordances, px_estimate_value(app.line));
    printf("\n");

    /* Print full trace */
    printf("[Step 5] full cognitive history:\n");
    px_trace_print(&app.trace);
    printf("\n");

    /* User finds the bug and marks it */
    printf("[Step 6] user marks bug as fixed\n");
    px_closure_trigger(app.fix_bug, NULL, 0);
    px_visibility_render(&app.trace, &app.context, px_estimate_value(app.line));
    px_affordance_set_recompute(&app.affordances, px_estimate_value(app.line));
    printf("\n");

    /* Note: affordances changed because context changed */
    printf("[Observation] after fix_bug:\n");
    printf("  - Context switched from 'debugging' to 'verifying'\n");
    printf("  - 'mark bug as fixed' is no longer available (avail_only_if_unfixed)\n");
    printf("  - This is dynamic Affordance, not a disabled button\n\n");

    /* --- Cleanup --- */
    px_closure_free(app.inc);
    px_closure_free(app.dec);
    px_closure_free(app.fix_bug);
    px_graph_free(app.graph);
    px_estimate_free(app.line);

    printf("=== Prototype complete ===\n\n");
    printf("What we observed:\n");
    printf("  1. Context switched when user's activity changed (debugging → verifying)\n");
    printf("  2. Trace recorded every step with intent kind + state before/after\n");
    printf("  3. Affordance recomputed dynamically as state/context changed\n");
    printf("  4. 'Undo' happened by Trace navigation — no separate undo system\n");
    printf("  5. Visibility showed trajectory, not just current value\n");
    printf("\nWhat this means:\n");
    printf("  The phenomenological school is implementable on top of Planex's\n");
    printf("  existing abstractions, without modifying the core. But the resulting\n");
    printf("  UI feels different — it's about *meaning formation*, not *state transfer*.\n");
    printf("\nPlanex does NOT adopt this school (see alternative-perspectives.md).\n");
    printf("This prototype is research demonstration, not a commitment.\n");
    return 0;
}
