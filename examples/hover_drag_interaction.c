/*
 * hover_drag_interaction.c — boundary-CLOSING demo for continuous interaction
 *
 * This is the counterpart of hover_drag_4abs.c (the boundary-EXPOSING
 * demo per ADR-0006). That demo measured the pain of hover+drag with
 * only 4 abstractions and concluded:
 *
 *   "Hover: WORKABLE but wasteful ... Drag: WORKABLE but semantically
 *    wrong ... Gesture: NOT POSSIBLE ... They would be INTOLERABLE
 *    for complex gesture/touch UIs."
 *
 * ADR-0006's decision protocol says that evidence feeds the next ADR
 * (this one: ADR-0016; promoted by ADR-0018). This demo implements the SAME list
 * reorder scenario with the v0.6 prototype→v0.7 canonical — px_interaction
 * + px_region/px_afford_at (intent compilation) — and measures the
 * difference:
 *
 *   hover_drag_4abs.c (Estimate hack)   hover_drag_interaction.c (v0.6→v0.7)
 *   ---------------------------------   ------------------------------------
 *   6 transient Estimates (HACK 1-3)    1 interaction + 5 regions
 *   2 estimate_set per mouse move       0 estimate writes per sample
 *   observer fan-out per move           hot path inert (O(1) append)
 *   mouse move = PX_INTENT_EXPRESS      mouse move = sample (not an act)
 *   drag state = 3 Estimates            drag state = trajectory
 *   hover = Estimate churn              hover = pure region query
 *   gesture: NOT POSSIBLE               swipe/tap derivable by measure
 *
 * The script (headless, deterministic):
 *   1. hover across items            → highlight computed per frame
 *   2. press on item 1, drag to 4    → process with 60 samples
 *   3. release                       → commit → reorder closure
 *   4. press on item 0, drag, ESC    → cancel → NO reorder
 *   5. quick tap on item 2           → tap (tiny displacement), no drag
 *
 * Build:
 *   cc -std=c17 -I include examples/hover_drag_interaction.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c src/fb.c src/font.c -lm -o build/hover_drag_interaction
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define N_ITEMS 5
#define ITEM_H  32
#define ITEM_W  280
#define ITEM_X  20
#define ITEM_Y_START 40

typedef struct {
    /* Persistent state — the ONLY Estimates (legitimate use). */
    px_estimate* item_order[N_ITEMS];   /* order[i] = item at slot i */
    px_graph*    graph;

    /* Continuous interaction process (the 7th canonical abstraction). */
    px_interaction* drag;

    /* Ambient pointer position — a plain struct field, NOT an Estimate.
     * This is the architectural point: the old demo paid 2 estimate_set
     * + observer fan-out + perception auto-invoke per mouse move; here
     * the ambient stream costs one struct write and hover is derived
     * at render time by region query. */
    px_int_sample pointer;
    bool          has_pointer;

    /* Regions — the intent-compilation layer. */
    px_region*  item_region[N_ITEMS];

    /* Discrete intents the process resolves to. */
    px_closure* commit_reorder;
    px_closure* cancel_drag;

    /* Metrics: what the OLD demo paid per event, vs this one. */
    int   mouse_moves;
    int   estimate_writes;      /* via observer on item_order[0]      */
    int   observer_fires;
    int   hook_fires;
    int   drag_item;            /* slot captured at begin             */
} App;

static const char* item_labels[] = {
    "Apple", "Banana", "Cherry", "Date", "Elderberry"
};

/* ---- helpers --------------------------------------------------- */

static int y_to_slot(int y) {
    if (y < ITEM_Y_START) return -1;
    int idx = (y - ITEM_Y_START) / ITEM_H;
    if (idx < 0 || idx >= N_ITEMS) return -1;
    return idx;
}

static void on_item0_change(px_estimate* e, void* user) {
    App* app = (App*)user;
    (void)e;
    app->observer_fires++;
}

/* ---- closures (discrete intents) -------------------------------- */

typedef struct {
    int from_slot;
    int to_slot;
} px_reorder_intent;

static void on_commit_reorder(px_intent intent, void* user) {
    App* app = (App*)user;
    if (intent.payload && intent.payload_size == sizeof(px_reorder_intent)) {
        px_reorder_intent* ri = (px_reorder_intent*)intent.payload;
        if (ri->from_slot >= 0 && ri->from_slot < N_ITEMS &&
            ri->to_slot   >= 0 && ri->to_slot   < N_ITEMS &&
            ri->from_slot != ri->to_slot) {
            double tmp = px_estimate_value(app->item_order[ri->from_slot]);
            app->estimate_writes += 2;
            px_estimate_set(app->item_order[ri->from_slot],
                            px_estimate_value(app->item_order[ri->to_slot]), 1.0);
            px_estimate_set(app->item_order[ri->to_slot], tmp, 1.0);
        }
    }
}

static void on_cancel_drag(px_intent intent, void* user) {
    App* app = (App*)user;
    (void)intent;
    /* Cancellation is first-class: nothing is reordered, but the
     * semantic world is INFORMED (feedback channel could announce it). */
    (void)app;
}

static bool eval_true(void* u) { (void)u; return true; }

/* ---- process→intent compilation (the phase hook) ---------------- */

static void on_drag_phase(px_interaction* it, px_int_phase phase, void* user) {
    App* app = (App*)user;
    app->hook_fires++;

    if (phase == PX_INT_BEGAN) {
        /* Capture which slot the drag started on (intent compilation
         * FROM the trajectory's first sample). */
        const px_int_sample* s = px_interaction_last(it);
        app->drag_item = s ? y_to_slot((int)s->y) : -1;
    } else if (phase == PX_INT_COMMITTED) {
        /* Compile the outcome into a semantic payload: reorder. The
         * payload is a VALUE — captured in last_intent, replayable. */
        const px_int_sample* s = px_interaction_last(it);
        int to_slot = s ? y_to_slot((int)s->y) : -1;
        if (app->drag_item >= 0 && to_slot >= 0 && to_slot != app->drag_item) {
            px_reorder_intent ri = { app->drag_item, to_slot };
            px_closure_trigger(app->commit_reorder, &ri, sizeof(ri));
        }
        app->drag_item = -1;
    } else if (phase == PX_INT_CANCELLED) {
        app->drag_item = -1;
    }
    /* PX_INT_ACTIVE never reaches the hook — per-sample life is inert. */
}

/* ---- the event side (what px_app_run would dispatch) ------------- */

static void feed_mouse_move(App* app, double t, int x, int y) {
    app->mouse_moves++;
    px_int_sample s = { t, (double)x, (double)y, 0.0, 0, 0 };
    app->pointer = s;               /* ambient stream: one struct write */
    app->has_pointer = true;
    /* While a drag process is running, each move is part of its
     * trajectory — the O(1) inert append. */
    px_int_phase ph = px_interaction_phase(app->drag);
    if (ph == PX_INT_BEGAN || ph == PX_INT_ACTIVE) {
        px_interaction_sample(app->drag, &s);
    }
}

static void feed_mouse_down(App* app, double t, int x, int y) {
    px_int_sample s = { t, (double)x, (double)y, 0.0, 1, 0 };
    app->pointer = s;
    app->has_pointer = true;
    px_interaction_sample(app->drag, &s);   /* auto-begins the process */
}

static void feed_mouse_up(App* app, double t, int x, int y) {
    px_int_sample s = { t, (double)x, (double)y, 0.0, 0, 0 };
    app->pointer = s;
    app->has_pointer = true;
    px_interaction_sample(app->drag, &s);
    px_interaction_commit(app->drag);
}

/* ---- "perception" — pure function reading state + trajectory ----- */

static const char* render_status(App* app) {
    /* Hover is COMPUTED here (pure region query over the ambient
     * pointer) — not stored. The old demo paid an estimate_set per
     * hover change; here it costs a scan at render time and nothing
     * between frames. */
    static char status[128];
    if (px_interaction_phase(app->drag) == PX_INT_ACTIVE ||
        px_interaction_phase(app->drag) == PX_INT_BEGAN) {
        const px_int_sample* s = px_interaction_last(app->drag);
        int slot = s ? y_to_slot((int)s->y) : -1;
        snprintf(status, sizeof(status), "dragging slot %d (samples=%d)",
                 slot, px_interaction_total(app->drag));
        return status;
    }
    if (app->has_pointer) {
        px_region* r = px_region_at(app->pointer.x, app->pointer.y);
        if (r) {
            snprintf(status, sizeof(status), "hover: %s", px_region_label(r));
            return status;
        }
    }
    return "idle";
}

/* ============================================================
 * Main — deterministic event script
 * ============================================================ */

int main(void) {
    printf("Planex hover_drag_interaction — boundary-CLOSING demo (v0.6 prototype→v0.7 canonical, ADR-0018)\n");
    printf("======================================================================\n");
    printf("Validates: hover+drag+cancel+tap with px_interaction + affordances.\n");
    printf("Counterpart of hover_drag_4abs.c (ADR-0006 evidence → ADR-0016).\n\n");

    App app = {0};
    app.graph = px_graph_new();

    for (int i = 0; i < N_ITEMS; i++) {
        app.item_order[i] = px_estimate_new((double)i, 1.0);
    }
    px_estimate_observe(app.item_order[0], on_item0_change, &app);

    /* Regions: the intent-compilation layer. Declaring the AFFORDS
     * relation makes hit-testing a graph query. */
    for (int i = 0; i < N_ITEMS; i++) {
        char label[32];
        snprintf(label, sizeof(label), "item %s", item_labels[i]);
        app.item_region[i] = px_region_new(
            px_rect_make(ITEM_X, ITEM_Y_START + i * ITEM_H, ITEM_W, ITEM_H),
            label);
    }

    /* Closures: the discrete intents processes resolve to. */
    app.commit_reorder = px_closure_new("commit reorder", PX_INTENT_DECLARE,
                                         on_commit_reorder, eval_true, &app);
    app.cancel_drag   = px_closure_new("cancel drag", PX_INTENT_EXPRESS,
                                        on_cancel_drag, eval_true, &app);

    /* The interaction process — one object, four phases, inert samples. */
    app.drag = px_interaction_new("list-drag", 64);
    px_interaction_on_phase(app.drag, on_drag_phase, &app);
    px_interaction_on_cancel(app.drag, app.cancel_drag);

    /* Affordance declarations: slots afford the drag process. (The
     * relation is ordinary graph data — query + constrain like any.) */
    for (int i = 0; i < N_ITEMS; i++) {
        px_declare(app.graph, app.item_region[i], PX_REL_AFFORDS, app.drag);
    }

    /* ---- 1. hover sweep: 40 moves across the list ---------------- */
    printf("[1] hover sweep (40 mouse moves)...\n");
    for (int i = 0; i < 40; i++) {
        int y = ITEM_Y_START + (i * 4) % (N_ITEMS * ITEM_H);
        feed_mouse_move(&app, (double)i, 160, y);
    }
    printf("    status after sweep: %s\n", render_status(&app));
    printf("    estimate writes during sweep: %d\n", app.estimate_writes);
    printf("    observer fires during sweep:  %d (old demo: ~80)\n\n",
           app.observer_fires);

    /* ---- 2. drag item 1 → slot 4, release ------------------------ */
    printf("[2] drag slot 1 to slot 4 (60 samples)...\n");
    int from_y = ITEM_Y_START + 1 * ITEM_H + 16;
    int to_y   = ITEM_Y_START + 4 * ITEM_H + 16;
    feed_mouse_down(&app, 100.0, 160, from_y);
    assert(px_interaction_phase(app.drag) == PX_INT_ACTIVE);
    for (int i = 0; i < 60; i++) {
        int y = from_y + (to_y - from_y) * (i + 1) / 60;
        feed_mouse_move(&app, 100.0 + (double)i, 160, y);
    }
    feed_mouse_up(&app, 180.0, 160, to_y);
    assert(px_interaction_phase(app.drag) == PX_INT_COMMITTED);
    assert(px_estimate_value(app.item_order[1]) == 4.0);  /* Banana → slot 4 */
    assert(px_estimate_value(app.item_order[4]) == 1.0);  /* Elderberry → slot 1 */
    printf("    committed: slot1=%d slot4=%d (reordered)\n",
           (int)px_estimate_value(app.item_order[1]),
           (int)px_estimate_value(app.item_order[4]));
    printf("    hook fired %d times for 62 events (old demo: 0 hooks,\n"
           "    3 Estimates × 62 writes instead)\n\n", app.hook_fires);

    /* Re-arm: a committed process is terminal; the app loop would
     * create (or reset) the next one. We exercise a fresh one here.
     * The AFFORDS edges name the process by POINTER — the edges are
     * retired BEFORE the process is freed and re-declared for its
     * successor. Skipping the retire step leaves dangling edges:
     * px_afford_at would hand callers a dead pointer whenever the
     * allocator does not reuse the freed address (the CI-found
     * regression — it passed on Debian glibc by layout luck and
     * aborted on the Ubuntu runner). */
    for (int i = 0; i < N_ITEMS; i++) {
        px_undeclare(app.graph, app.item_region[i], PX_REL_AFFORDS, app.drag);
    }
    px_interaction_free(app.drag);
    app.drag = px_interaction_new("list-drag-2", 64);
    px_interaction_on_phase(app.drag, on_drag_phase, &app);
    px_interaction_on_cancel(app.drag, app.cancel_drag);
    for (int i = 0; i < N_ITEMS; i++) {
        px_declare(app.graph, app.item_region[i], PX_REL_AFFORDS, app.drag);
    }

    /* ---- 3. drag + ESC → cancel: nothing reorders ----------------- */
    printf("[3] drag slot 0 toward slot 3, then ESC (cancel)...\n");
    int drag_from = ITEM_Y_START + 0 * ITEM_H + 16;
    int drag_mid  = ITEM_Y_START + 2 * ITEM_H + 16;
    feed_mouse_down(&app, 200.0, 160, drag_from);
    for (int i = 0; i < 30; i++) {
        int y = drag_from + (drag_mid - drag_from) * (i + 1) / 30;
        feed_mouse_move(&app, 200.0 + (double)i, 160, y);
    }
    px_interaction_cancel(app.drag, "esc pressed");
    assert(px_interaction_phase(app.drag) == PX_INT_CANCELLED);
    assert(strcmp(px_interaction_cancel_reason(app.drag), "esc pressed") == 0);
    /* Slot 0 still holds item 0 — cancellation committed nothing. */
    assert(px_estimate_value(app.item_order[0]) == 0.0);
    printf("    cancelled (reason=\"%s\"), slot0=%d (unchanged)\n\n",
           px_interaction_cancel_reason(app.drag),
           (int)px_estimate_value(app.item_order[0]));

    /* ---- 4. quick tap on slot 2: tap ≠ drag ---------------------- */
    printf("[4] quick tap on slot 2 (2 samples, 6px)...\n");
    px_interaction* tap = px_interaction_new("tap", 8);
    px_interaction_on_phase(tap, on_drag_phase, &app);
    px_int_sample t1 = { 300.0, 160, ITEM_Y_START + 2 * ITEM_H + 16, 0, 1, 0 };
    px_int_sample t2 = { 330.0, 166, ITEM_Y_START + 2 * ITEM_H + 16, 0, 0, 0 };
    px_interaction_sample(tap, &t1);
    px_interaction_sample(tap, &t2);
    px_interaction_commit(tap);
    double disp = px_interaction_displacement(tap);
    printf("    displacement: %.0fpx → classified as TAP (<4px would be\n"
           "    noise); velocity %.2f px/ms\n",
           disp, px_interaction_velocity(tap));
    assert(disp > 0.0 && disp < 10.0);
    px_interaction_free(tap);

    /* ---- gesture derivation (the old demo's "NOT POSSIBLE") ------- */
    printf("\n[5] gesture derivation from trajectory measures...\n");
    px_interaction* swipe = px_interaction_new("swipe", 32);
    for (int i = 0; i < 10; i++) {
        px_int_sample s = { (double)i * 8, (double)i * 60, 100.0, 0, 0, 0 };
        px_interaction_sample(swipe, &s);
    }
    px_interaction_commit(swipe);
    printf("    swipe: displacement=%.0f path=%.0f velocity=%.1f px/ms\n",
           px_interaction_displacement(swipe),
           px_interaction_path_length(swipe),
           px_interaction_velocity(swipe));
    printf("    → derivable (old demo verdict: \"Gesture: NOT POSSIBLE\")\n");
    px_interaction_free(swipe);

    /* ---- summary -------------------------------------------------- */
    printf("\nFinal state:\n");
    printf("  mouse moves fed:    %d\n", app.mouse_moves);
    printf("  estimate writes:    %d (exactly 2 — one reorder commit)\n",
           app.estimate_writes);
    printf("  observer fires:     %d (order[0] never changed: 0; +2 init)\n",
           app.observer_fires);
    printf("  item order:         ");
    for (int i = 0; i < N_ITEMS; i++) {
        printf("%s ", item_labels[(int)px_estimate_value(app.item_order[i])]);
    }
    printf("\n");

    /* Affordance check — intent compilation end to end. With the
     * retire-then-re-declare discipline above this asserts the LIVE
     * process pointer on every allocator layout (no address-reuse
     * luck involved); it tightened from the old two-way assert,
     * whose second arm (commit_reorder) was never an AFFORDS
     * target and only ever matched by accident.
     *
     * v0.8 (Line 2): the check reads through the PROCESS form —
     * px_afford_at is kind-filtered now (a process target is not a
     * closure; the pre-v0.8 blind cast is what this line used to
     * lean on). Same discipline, honest vehicle. */
    px_drag_intent di;
    px_interaction* afforded = px_afford_compile_process(
        app.graph, 160, ITEM_Y_START + 3 * ITEM_H + 5, 1, &di);
    printf("  afford_at(slot 3):  %s\n",
           afforded ? "found (drag process)" : "NULL");
    assert((void*)afforded == (void*)app.drag);
    assert(px_afford_at(app.graph, 160,
                        ITEM_Y_START + 3 * ITEM_H + 5) == NULL);

    /* ---- cleanup -------------------------------------------------- */
    px_interaction_free(app.drag);
    px_closure_free(app.commit_reorder);
    px_closure_free(app.cancel_drag);
    for (int i = 0; i < N_ITEMS; i++) {
        px_region_free(app.item_region[i]);
        px_estimate_free(app.item_order[i]);
    }
    px_graph_free(app.graph);

    printf("\n=== Boundary-closing demo complete ===\n");
    printf("\nHACK RETIREMENT (vs hover_drag_4abs.c):\n");
    printf("  HACK 1 (mouse as Estimate, 60fps churn):   RETIRED — samples are inert\n");
    printf("  HACK 2 (hover as Estimate):                RETIRED — pure region query\n");
    printf("  HACK 3 (drag state as 3 Estimates):        RETIRED — one process object\n");
    printf("  HACK 4 (begin-drag as REQUEST):            RETIRED — begin is a phase\n");
    printf("  HACK 5 (mouse move as EXPRESS intent):     RETIRED — sample ≠ act\n");
    printf("  HACK 6 (hover read per frame):             KEPT (by design — perception\n");
    printf("                                             reads the stream as input)\n");
    printf("  HACK 7 (poll for mouse-up):                N/A headless; real backends\n");
    printf("                                             deliver PX_EV_MOUSE_UP/WHEEL\n");
    printf("\nVerdict for ADR-0016:\n");
    printf("  - The 5 of 7 hacks that forced PROCESS into STATE are gone.\n");
    printf("  - The estimate-write count for the whole session is 2 (one\n");
    printf("    reorder), versus ~O(events) in the old demo.\n");
    printf("  - Cancel is a first-class outcome; gesture is derivable.\n");
    printf("  - Status: promoted — the 7th canonical abstraction since\n");
    printf("    v0.7 (ADR-0018). Samples stay inert; THE INVARIANT is now\n");
    printf("    normative — test_v06_interaction.c section D enforces it.\n");
    return 0;
}
