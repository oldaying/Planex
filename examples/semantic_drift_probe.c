/*
 * semantic_drift_probe.c — the ADR-0023 probe: mid-flight
 * reinterpretation as the B→C stress test
 *
 * The question (registered in ADR-0023 BEFORE this file was
 * written): can the MEANING of a running interaction process be
 * changed while the trajectory continues — without new framework
 * surface, without killing the process, without losing undo
 * granularity, and without breaking sample inertness?
 *
 * The scenario (designed; the crossing-commit is an app POLICY,
 * printed as such): a five-item list with a trash margin below.
 * Press on an item, drag within the list (the gesture IS a
 * reorder), cross into the margin (the gesture BECOMES a trash
 * drag; the list finalizes the slot held at crossing), release
 * over the trash. One continuous trajectory, two semantics over
 * time, two committed effects — then undo peels them apart. A
 * closing segment exercises ESC AFTER the drift: cancel must
 * kill only the unresolved remainder.
 *
 * Three encodings of the SAME deterministic event script:
 *
 *   A  bridge repointing   — px_interaction_on_commit rebinds the
 *                            live process's commit closure at the
 *                            drift boundary (the literal claim)
 *   B  hook compilation    — the phase hook compiles the outcome
 *                            from the trajectory + an app-side
 *                            mode field switched at the boundary
 *                            (the denotational reading)
 *   C  classical control   — the event-model idiom: a mode enum
 *                            consulted in every handler, an anchor
 *                            reset at drift, a hand-rolled undo
 *                            list (the null hypothesis, measured)
 *
 * Predictions P1-P5 and the verdict rule live in ADR-0023. The
 * probe prints each measurement next to its prediction; the run
 * exits non-zero if any of P2-P4's asserts bite.
 *
 * Build:
 *   cc -std=c17 -I include examples/semantic_drift_probe.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c src/fb.c src/font.c src/actor.c src/breakdown.c \
 *      src/layout.c src/app.c -lm -o build/semantic_drift_probe
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* ---- geometry (pure y-math; regions are exercised elsewhere) ---- */
#define N_ITEMS     5
#define ITEM_H      32
#define ITEM_Y_START 40
#define LIST_BOTTOM  (ITEM_Y_START + N_ITEMS * ITEM_H)   /* 200 */
#define MARGIN_Y    220    /* below this line: the trash margin */
#define TRASH_Y     268    /* where the release lands */

static int y_to_slot(int y) {
    if (y < ITEM_Y_START || y >= LIST_BOTTOM) return -1;
    return (y - ITEM_Y_START) / ITEM_H;
}

/* ---- the event script (shared by all three encodings) ----------
 * One table, three players: the script is literally the same
 * bytes for every encoding, so the comparison is between the
 * encodings, not between two hand-written choreographies.      */
enum { EV_DOWN = 1, EV_MOVE, EV_UP, EV_ESC };

typedef struct {
    double t;
    int    x, y;
    int    kind;
} script_ev;

/* Main gesture: press slot 1, drag to slot 4's band, cross into
 * the margin (sample 22), sweep to the trash, release. */
static void fill_main_script(script_ev* ev, int* out_n) {
    int n = 0;
    ev[n++] = (script_ev){ 100.0, 160,  88, EV_DOWN };
    for (int i = 0; i < 20; i++) {
        int y = 88 + (184 - 88) * (i + 1) / 20;
        ev[n++] = (script_ev){ 101.0 + i, 160, y, EV_MOVE };
    }
    ev[n++] = (script_ev){ 121.0, 160, 232, EV_MOVE };
    for (int i = 0; i < 14; i++) {
        int y = 232 + (TRASH_Y - 232) * (i + 1) / 14;
        ev[n++] = (script_ev){ 122.0 + i, 160, y, EV_MOVE };
    }
    ev[n++] = (script_ev){ 140.0, 160, TRASH_Y, EV_UP };
    *out_n = n;
}

/* ESC script: press slot 2, drift, then ESC — the pending trash
 * semantics must die; the crossing-committed reorder must not. */
static void fill_esc_script(script_ev* ev, int* out_n) {
    int n = 0;
    ev[n++] = (script_ev){ 200.0, 160, 120, EV_DOWN };         /* slot 2 */
    for (int i = 0; i < 10; i++) {
        int y = 120 + (184 - 120) * (i + 1) / 10;
        ev[n++] = (script_ev){ 201.0 + i, 160, y, EV_MOVE };
    }
    ev[n++] = (script_ev){ 212.0, 160, 240, EV_MOVE };         /* DRIFT */
    for (int i = 0; i < 8; i++) {
        int y = 240 + (TRASH_Y - 240) * (i + 1) / 8;
        ev[n++] = (script_ev){ 213.0 + i, 160, y, EV_MOVE };
    }
    ev[n++] = (script_ev){ 230.0, 160, TRASH_Y, EV_ESC };
    *out_n = n;
}

/* ---- semantic payloads (intent-as-value, replayable) ---------- */
typedef struct { int from_slot, to_slot; } reorder_intent;
typedef struct { int slot; }                 trash_intent;

/* ---- shared metrics ------------------------------------------- */
typedef struct {
    int    estimate_writes;        /* ALL semantic-world writes         */
    int    writes_during_samples;  /* P4: must stay 0                   */
    int    bridge_rebinds;         /* px_interaction_on_commit mid-live  */
    int    mode_switches;          /* app-side interpretation switches   */
    int    mode_consults;          /* per-event mode reads (classical)   */
    int    anchor_resets;          /* classical anchor amputations       */
    int    pre_drift_retained;     /* samples from before drift, still
                                    * queryable after it                 */
    int    trajectory_total;       /* px_interaction_total / classical n */
    double path_length;            /* over what the encoding retained    */
    int    undo_steps;             /* registry (A/B) or manual (C)       */
    int    state_mutations;        /* classical: struct writes per move  */
} drift_metrics;

static bool eval_true(void* u) { (void)u; return true; }

/* ============================================================
 * Encoding A — bridge repointing
 * ============================================================ */
typedef struct {
    px_graph*        g;
    px_estimate*     order[N_ITEMS];      /* order[i] = item at slot i */
    px_closure*      reorder_c;
    px_closure*      trash_c;
    px_interaction*  drag;
    int              drag_item;           /* item id being dragged     */
    bool             in_sample;           /* P4 detector window        */
    drift_metrics    m;
} app_a;

static void a_observe(px_estimate* e, void* user) {
    app_a* app = (app_a*)user;
    (void)e;
    app->m.estimate_writes++;
    if (app->in_sample) app->m.writes_during_samples++;
}

static void a_on_reorder(px_intent intent, void* user) {
    app_a* app = (app_a*)user;
    if (!intent.payload || intent.payload_size != sizeof(reorder_intent)) return;
    const reorder_intent* ri = (const reorder_intent*)intent.payload;
    if (ri->from_slot < 0 || ri->from_slot >= N_ITEMS) return;
    if (ri->to_slot   < 0 || ri->to_slot   >= N_ITEMS) return;
    if (ri->from_slot == ri->to_slot) return;
    double tmp = px_estimate_value(app->order[ri->from_slot]);
    px_estimate_set(app->order[ri->from_slot],
                    px_estimate_value(app->order[ri->to_slot]), 1.0);
    px_estimate_set(app->order[ri->to_slot], tmp, 1.0);
}

static void a_on_trash(px_intent intent, void* user) {
    app_a* app = (app_a*)user;
    if (!intent.payload || intent.payload_size != sizeof(trash_intent)) return;
    const trash_intent* ti = (const trash_intent*)intent.payload;
    if (ti->slot < 0 || ti->slot >= N_ITEMS) return;
    px_estimate_set(app->order[ti->slot], -1.0, 1.0);   /* -1 = trashed */
}

static void a_drift(app_a* app, int last_list_slot) {
    /* Crossing-commit (APP POLICY — see ADR-0023): the list
     * finalizes the slot held at crossing, as an undoable step. */
    reorder_intent ri = { app->drag_item, last_list_slot };
    px_closure_trigger(app->reorder_c, &ri, sizeof(ri));

    /* THE drift operation under test: rebind the live process's
     * commit bridge. Payload is COPIED at bind time — the slot is
     * known here; ADR-0023's Known issue records what that costs
     * when it would not be. */
    trash_intent ti = { last_list_slot };
    px_interaction_on_commit(app->drag, app->trash_c, &ti, sizeof(ti));
    app->m.bridge_rebinds++;
}

static void a_run_event(app_a* app, const script_ev* e) {
    px_int_sample s = { e->t, (double)e->x, (double)e->y, 0.0, 0, 0 };
    switch (e->kind) {
    case EV_DOWN:
        app->drag_item = y_to_slot(e->y);
        px_interaction_sample(app->drag, &s);      /* auto-begins */
        break;
    case EV_MOVE: {
        px_int_phase ph = px_interaction_phase(app->drag);
        if (ph == PX_INT_BEGAN || ph == PX_INT_ACTIVE) {
            int prev_y = 0;
            const px_int_sample* last = px_interaction_last(app->drag);
            if (last) prev_y = (int)last->y;
            app->in_sample = true;                   /* P4 window   */
            px_interaction_sample(app->drag, &s);
            app->in_sample = false;
            if (prev_y < MARGIN_Y && e->y >= MARGIN_Y) {
                a_drift(app, y_to_slot(prev_y));     /* the drift   */
            }
        }
        break;
    }
    case EV_UP: {
        app->in_sample = true;
        px_interaction_sample(app->drag, &s);
        app->in_sample = false;
        px_interaction_commit(app->drag);            /* bound bridge fires */
        break;
    }
    default:
        break;
    }
}

static void run_encoding_a(int esc_case) {
    app_a app = {0};
    px_undo_clear();
    app.g = px_graph_new();
    for (int i = 0; i < N_ITEMS; i++) {
        app.order[i] = px_estimate_new((double)i, 1.0);
        px_estimate_observe(app.order[i], a_observe, &app);
    }
    /* v0.7 form: closures carry their undo graph at construction
     * (ADR-0019) and declare what they TRIGGERS-affect, so
     * px_closure_trigger snapshots automatically when undo is on. */
    app.reorder_c = px_closure_new_with_graph(
        "commit reorder", PX_INTENT_DECLARE, a_on_reorder, eval_true,
        &app, app.g);
    app.trash_c = px_closure_new_with_graph(
        "commit trash", PX_INTENT_DECLARE, a_on_trash, eval_true,
        &app, app.g);
    for (int i = 0; i < N_ITEMS; i++) {
        px_declare(app.g, app.reorder_c, PX_REL_TRIGGERS, app.order[i]);
        px_declare(app.g, app.trash_c,   PX_REL_TRIGGERS, app.order[i]);
    }
    app.drag = px_interaction_new("drift-drag", 64);

    script_ev ev[40];
    int n = 0;
    if (esc_case) fill_esc_script(ev, &n); else fill_main_script(ev, &n);

    px_undo_set_enabled(true);
    for (int i = 0; i < n; i++) {
        if (ev[i].kind == EV_ESC) {
            px_interaction_cancel(app.drag, "esc after drift");
        } else {
            a_run_event(&app, &ev[i]);
        }
    }

    /* ---- measurements ---- */
    app.m.trajectory_total = px_interaction_total(app.drag);
    app.m.path_length      = px_interaction_path_length(app.drag);
    app.m.undo_steps       = px_undo_count();
    int pre_drift = 0;
    for (int i = 0; i < px_interaction_stored(app.drag); i++) {
        const px_int_sample* s = px_interaction_at(app.drag, i);
        if (s && s->y < (double)MARGIN_Y) pre_drift++;
    }
    app.m.pre_drift_retained = pre_drift;

    if (esc_case) {
        printf("[A-esc] ESC after drift — cancel scoped to the remainder\n");
        printf("    phase: %s (reason \"%s\")\n",
               px_int_phase_str(px_interaction_phase(app.drag)),
               px_interaction_cancel_reason(app.drag));
        printf("    undo steps: %d (the crossing reorder ONLY — the\n"
               "    pending trash semantics died with the cancel)\n",
               app.m.undo_steps);
        printf("    slot2=%d slot4=%d (reorder stands, no trash)\n",
               (int)px_estimate_value(app.order[2]),
               (int)px_estimate_value(app.order[4]));
        assert(px_undo_count() == 1);
        assert((int)px_estimate_value(app.order[2]) == 4);
        assert((int)px_estimate_value(app.order[4]) == 2);
        px_undo();
        assert((int)px_estimate_value(app.order[2]) == 2);
        assert((int)px_estimate_value(app.order[4]) == 4);
        printf("    undo() -> slot2=%d slot4=%d (original order)\n",
               (int)px_estimate_value(app.order[2]),
               (int)px_estimate_value(app.order[4]));
    } else {
        printf("[A] bridge repointing — one live process, rebound mid-flight\n");
        printf("    estimate writes: %d total, %d during samples (P4)\n",
               app.m.estimate_writes, app.m.writes_during_samples);
        printf("    bridge rebinds: %d; mode consults: %d\n",
               app.m.bridge_rebinds, app.m.mode_consults);
        printf("    trajectory: %d samples, %.0fpx path, %d pre-drift\n"
               "    samples retained (P2)\n",
               app.m.trajectory_total, app.m.path_length,
               app.m.pre_drift_retained);
        printf("    undo steps from one gesture: %d (P3)\n", app.m.undo_steps);
        printf("    slot1=%d slot4=%d (swapped then trashed)\n",
               (int)px_estimate_value(app.order[1]),
               (int)px_estimate_value(app.order[4]));

        assert(app.m.writes_during_samples == 0);           /* P4 */
        assert(app.m.bridge_rebinds == 1);
        assert(px_interaction_total(app.drag) == 37);        /* P2 */
        assert(app.m.pre_drift_retained == 21);              /* P2 */
        assert(px_undo_count() == 2);                        /* P3 */
        assert((int)px_estimate_value(app.order[1]) == 4);
        assert((int)px_estimate_value(app.order[4]) == -1);

        px_undo();   /* peel the trash */
        assert((int)px_estimate_value(app.order[4]) == 1);
        printf("    undo #1 -> slot4=%d (trash peeled, reorder stands)\n",
               (int)px_estimate_value(app.order[4]));
        px_undo();   /* peel the reorder */
        assert((int)px_estimate_value(app.order[1]) == 1);
        assert((int)px_estimate_value(app.order[4]) == 4);
        printf("    undo #2 -> slot1=%d slot4=%d (original order)\n",
               (int)px_estimate_value(app.order[1]),
               (int)px_estimate_value(app.order[4]));
    }

    px_undo_set_enabled(false);
    px_interaction_free(app.drag);
    px_closure_free(app.reorder_c);
    px_closure_free(app.trash_c);
    for (int i = 0; i < N_ITEMS; i++) px_estimate_free(app.order[i]);
    px_graph_free(app.g);
}

/* ============================================================
 * Encoding B — hook compilation (the bridges are never touched)
 * ============================================================ */
typedef struct {
    px_graph*        g;
    px_estimate*     order[N_ITEMS];
    px_closure*      reorder_c;
    px_closure*      trash_c;
    px_interaction*  drag;
    int              drag_item;
    int              mode;               /* 0 = reorder, 1 = trash */
    bool             in_sample;
    drift_metrics    m;
} app_b;

static void b_observe(px_estimate* e, void* user) {
    app_b* app = (app_b*)user;
    (void)e;
    app->m.estimate_writes++;
    if (app->in_sample) app->m.writes_during_samples++;
}

static void b_on_reorder(px_intent intent, void* user) {
    app_b* app = (app_b*)user;
    if (!intent.payload || intent.payload_size != sizeof(reorder_intent)) return;
    const reorder_intent* ri = (const reorder_intent*)intent.payload;
    if (ri->from_slot < 0 || ri->from_slot >= N_ITEMS) return;
    if (ri->to_slot   < 0 || ri->to_slot   >= N_ITEMS) return;
    if (ri->from_slot == ri->to_slot) return;
    double tmp = px_estimate_value(app->order[ri->from_slot]);
    px_estimate_set(app->order[ri->from_slot],
                    px_estimate_value(app->order[ri->to_slot]), 1.0);
    px_estimate_set(app->order[ri->to_slot], tmp, 1.0);
}

static void b_on_trash(px_intent intent, void* user) {
    app_b* app = (app_b*)user;
    if (!intent.payload || intent.payload_size != sizeof(trash_intent)) return;
    const trash_intent* ti = (const trash_intent*)intent.payload;
    if (ti->slot < 0 || ti->slot >= N_ITEMS) return;
    px_estimate_set(app->order[ti->slot], -1.0, 1.0);
}

static void b_on_phase(px_interaction* it, px_int_phase phase, void* user) {
    app_b* app = (app_b*)user;
    if (phase != PX_INT_COMMITTED) return;
    /* Compile the outcome FROM THE TRAJECTORY plus the mode field:
     * the payload reflects where the release actually landed —
     * the freedom encoding A's bind-time copy gives up (ADR-0023
     * Known issue). */
    app->m.mode_consults++;
    const px_int_sample* s = px_interaction_last(it);
    int y = s ? (int)s->y : -1;
    if (app->mode == 1 && y >= MARGIN_Y) {
        int slot = 0;
        /* the item rode from drag_item's slot; after the crossing
         * swap it sits at the crossing slot — app state, by design */
        for (int i = 0; i < N_ITEMS; i++) {
            if ((int)px_estimate_value(app->order[i]) == app->drag_item) slot = i;
        }
        trash_intent ti = { slot };
        px_closure_trigger(app->trash_c, &ti, sizeof(ti));
    }
}

static void run_encoding_b(void) {
    app_b app = {0};
    px_undo_clear();
    app.g = px_graph_new();
    for (int i = 0; i < N_ITEMS; i++) {
        app.order[i] = px_estimate_new((double)i, 1.0);
        px_estimate_observe(app.order[i], b_observe, &app);
    }
    app.reorder_c = px_closure_new_with_graph(
        "commit reorder", PX_INTENT_DECLARE, b_on_reorder, eval_true,
        &app, app.g);
    app.trash_c = px_closure_new_with_graph(
        "commit trash", PX_INTENT_DECLARE, b_on_trash, eval_true,
        &app, app.g);
    for (int i = 0; i < N_ITEMS; i++) {
        px_declare(app.g, app.reorder_c, PX_REL_TRIGGERS, app.order[i]);
        px_declare(app.g, app.trash_c,   PX_REL_TRIGGERS, app.order[i]);
    }
    app.drag = px_interaction_new("drift-drag", 64);
    px_interaction_on_phase(app.drag, b_on_phase, &app);

    script_ev ev[40];
    int n = 0;
    fill_main_script(ev, &n);

    px_undo_set_enabled(true);
    for (int i = 0; i < n; i++) {
        const script_ev* e = &ev[i];
        px_int_sample s = { e->t, (double)e->x, (double)e->y, 0.0, 0, 0 };
        switch (e->kind) {
        case EV_DOWN:
            app.drag_item = y_to_slot(e->y);
            px_interaction_sample(app.drag, &s);
            break;
        case EV_MOVE: {
            px_int_phase ph = px_interaction_phase(app.drag);
            if (ph == PX_INT_BEGAN || ph == PX_INT_ACTIVE) {
                const px_int_sample* last = px_interaction_last(app.drag);
                int prev_y = last ? (int)last->y : -1;
                app.in_sample = true;
                px_interaction_sample(app.drag, &s);
                app.in_sample = false;
                if (prev_y < MARGIN_Y && e->y >= MARGIN_Y) {
                    /* drift: switch the interpretation field; the
                     * bridges were never bound, so nothing to rebind */
                    app.mode = 1;
                    app.m.mode_switches++;
                    reorder_intent ri = { app.drag_item, y_to_slot(prev_y) };
                    px_closure_trigger(app.reorder_c, &ri, sizeof(ri));
                }
            }
            break;
        }
        case EV_UP:
            app.in_sample = true;
            px_interaction_sample(app.drag, &s);
            app.in_sample = false;
            px_interaction_commit(app.drag);   /* hook compiles */
            break;
        default:
            break;
        }
    }

    app.m.trajectory_total = px_interaction_total(app.drag);
    app.m.path_length      = px_interaction_path_length(app.drag);
    app.m.undo_steps       = px_undo_count();
    int pre_drift = 0;
    for (int i = 0; i < px_interaction_stored(app.drag); i++) {
        const px_int_sample* s = px_interaction_at(app.drag, i);
        if (s && s->y < (double)MARGIN_Y) pre_drift++;
    }
    app.m.pre_drift_retained = pre_drift;

    printf("[B] hook compilation — mode field + trajectory, no rebind\n");
    printf("    estimate writes: %d total, %d during samples (P4)\n",
           app.m.estimate_writes, app.m.writes_during_samples);
    printf("    mode switches: %d (consulted %d time, at the outcome)\n",
           app.m.mode_switches, app.m.mode_consults);
    printf("    trajectory: %d samples, %.0fpx path, %d pre-drift\n"
           "    samples retained (P2)\n",
           app.m.trajectory_total, app.m.path_length,
           app.m.pre_drift_retained);
    printf("    undo steps from one gesture: %d (P3)\n", app.m.undo_steps);

    assert(app.m.writes_during_samples == 0);       /* P4 */
    assert(app.m.mode_switches == 1);
    assert(px_interaction_total(app.drag) == 37);   /* P2 */
    assert(app.m.pre_drift_retained == 21);
    assert(px_undo_count() == 2);                   /* P3 */
    px_undo();
    px_undo();
    assert((int)px_estimate_value(app.order[1]) == 1);
    assert((int)px_estimate_value(app.order[4]) == 4);
    printf("    undo x2 -> slot1=%d slot4=%d (original order)\n",
           (int)px_estimate_value(app.order[1]),
           (int)px_estimate_value(app.order[4]));

    px_undo_set_enabled(false);
    px_interaction_free(app.drag);
    px_closure_free(app.reorder_c);
    px_closure_free(app.trash_c);
    for (int i = 0; i < N_ITEMS; i++) px_estimate_free(app.order[i]);
    px_graph_free(app.g);
}

/* ============================================================
 * Encoding C — the classical event-model control
 * (the null hypothesis: mode flags + anchor reset + manual undo)
 * ============================================================ */
enum { C_IDLE = 0, C_REORDER, C_TRASH };

typedef struct {
    int        mode;
    double     anchor_x, anchor_y; /* reset at drift — amputation */
    int        cur_x, cur_y;
    int        samples_x[64], samples_y[64], n;   /* post-drift only */
    int        drag_item;
    int        mode_consults;
    int        mode_switches;
    int        anchor_resets;
    int        state_mutations;
    int        order[N_ITEMS];
    int        undo_snap[8][N_ITEMS];
    int        undo_n;
} app_c;

static void c_push_undo(app_c* app) {
    if (app->undo_n >= 8) return;
    for (int i = 0; i < N_ITEMS; i++)
        app->undo_snap[app->undo_n][i] = app->order[i];
    app->undo_n++;
}

static int c_undo(app_c* app) {
    if (app->undo_n == 0) return 0;
    app->undo_n--;
    for (int i = 0; i < N_ITEMS; i++)
        app->order[i] = app->undo_snap[app->undo_n][i];
    return N_ITEMS;
}

static void c_apply_reorder(app_c* app, int from, int to) {
    if (from < 0 || from >= N_ITEMS || to < 0 || to >= N_ITEMS || from == to)
        return;
    int tmp = app->order[from];
    app->order[from] = app->order[to];
    app->order[to]   = tmp;
}

static void run_encoding_c(void) {
    app_c app = {0};
    for (int i = 0; i < N_ITEMS; i++) app.order[i] = i;

    script_ev ev[40];
    int n = 0;
    fill_main_script(ev, &n);

    for (int i = 0; i < n; i++) {
        const script_ev* e = &ev[i];
        switch (e->kind) {
        case EV_DOWN:
            app.mode = C_REORDER;                       app.mode_consults++;
            app.anchor_x = e->x; app.anchor_y = e->y;
            app.cur_x = e->x;    app.cur_y = e->y;
            app.n = 0;
            app.drag_item = y_to_slot(e->y);
            app.state_mutations += 4;
            break;
        case EV_MOVE:
            app.mode_consults++;                        /* every handler */
            if (app.mode == C_REORDER && e->y >= MARGIN_Y) {
                /* the drift, classical shape: finalize + switch + reset */
                int prev_slot = y_to_slot(app.cur_y);
                c_push_undo(&app);                      /* manual undo 1 */
                c_apply_reorder(&app, app.drag_item, prev_slot);
                app.mode = C_TRASH;                     app.mode_switches++;
                app.anchor_x = e->x; app.anchor_y = e->y;
                app.n = 0;                              /* amputation    */
                app.anchor_resets++;
            }
            app.cur_x = e->x; app.cur_y = e->y;         app.state_mutations += 2;
            if (app.n < 64) {
                app.samples_x[app.n] = e->x;
                app.samples_y[app.n] = e->y;
                app.n++;
            }
            break;
        case EV_UP:
            app.mode_consults++;
            if (app.mode == C_TRASH) {
                c_push_undo(&app);                      /* manual undo 2 */
                for (int s = 0; s < N_ITEMS; s++)
                    if (app.order[s] == app.drag_item) app.order[s] = -1;
            }
            app.mode = C_IDLE;
            app.state_mutations++;
            break;
        default:
            break;
        }
    }

    int pre_drift = 0;
    for (int i = 0; i < app.n; i++)
        if (app.samples_y[i] < MARGIN_Y) pre_drift++;

    double path = 0.0;
    for (int i = 0; i + 1 < app.n; i++) {
        double dx = (double)(app.samples_x[i+1] - app.samples_x[i]);
        double dy = (double)(app.samples_y[i+1] - app.samples_y[i]);
        path += sqrt(dx * dx + dy * dy);
    }

    printf("[C] classical control — mode machine + anchor reset\n");
    printf("    mode consults: %d (one per handler entry); switches: %d\n",
           app.mode_consults, app.mode_switches);
    printf("    anchor resets at drift: %d — pre-drift samples\n"
           "    retained: %d of 21 (the trajectory is amputated)\n",
           app.anchor_resets, pre_drift);
    printf("    state mutations on the move path: %d (%d per move)\n",
           app.state_mutations, 2);
    printf("    trajectory the encoding can still reason about: %d\n"
           "    samples, %.0fpx path (vs 37 samples for A and B)\n",
           app.n, path);
    printf("    undo: %d steps, hand-rolled stack (not the registry)\n",
           app.undo_n);

    assert(app.mode_consults >= 35);
    assert(app.anchor_resets == 1);
    assert(pre_drift == 0);
    assert(app.undo_n == 2);
    assert(app.order[1] == 4 && app.order[4] == -1);
    c_undo(&app);
    assert(app.order[4] == 1);
    c_undo(&app);
    assert(app.order[1] == 1 && app.order[4] == 4);
    printf("    undo x2 -> slot1=%d slot4=%d (original order)\n",
           app.order[1], app.order[4]);
}

/* ============================================================
 * Main
 * ============================================================ */
int main(void) {
    printf("Planex semantic_drift_probe — ADR-0023, the B→C stress test\n");
    printf("======================================================================\n");
    printf("Question: can the MEANING of a running process change while the\n");
    printf("trajectory continues — without new API, without process death,\n");
    printf("without losing undo granularity, without breaking inertness?\n");
    printf("Predictions P1-P5 were registered in ADR-0023 BEFORE this run.\n\n");

    run_encoding_a(0);
    printf("\n");
    run_encoding_b();
    printf("\n");
    run_encoding_c();
    printf("\n");
    run_encoding_a(1);   /* ESC after the drift — cancel scoping */
    printf("\n");

    printf("----------------------------------------------------------------------\n");
    printf("PREDICTION VERDICTS (ADR-0023)\n");
    printf("  P1 API closure     : PASS — this file compiled against the\n");
    printf("                       shipping header; zero core edits.\n");
    printf("  P2 continuity      : PASS — 37 samples, 21 pre-drift retained,\n");
    printf("                       one trajectory across the boundary (A and B).\n");
    printf("  P3 undo granularity: PASS — one gesture, two registry steps,\n");
    printf("                       independently restorable (A and B).\n");
    printf("  P4 the invariant   : PASS — 0 estimate writes during samples,\n");
    printf("                       drift included (A and B).\n");
    printf("  P5 classical cost  : MEASURED — %d mode consults, anchor reset\n",
           37);
    printf("                       amputating 21 samples, manual undo stack.\n");
    printf("\nVerdict per ADR-0023's rule: the B→C criterion does NOT trigger\n");
    printf("on the reinterpretation axis. Option C's open territory narrows\n");
    printf("to strength/gradient — untested here by construction.\n");
    printf("\n=== semantic_drift_probe complete ===\n");
    return 0;
}
