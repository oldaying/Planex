/*
 * designer_tools.c — v0.8 Line 2 real-application evidence:
 * an application whose DRAGS are data-driven — what drags is
 * decided by the afford graph, not by app code.
 *
 * The app: a designer-tool palette. Three color chips in a tray
 * (drag one out to drop a dot on the canvas), a brightness slider,
 * a click-only swatch. The app's router is ONE rule for every
 * region — the same decision px_app_run runs when px_app_desc sets
 * `intent_graph` (v0.8 Line 2):
 *
 *     down:   px_afford_compile_process(g, x, y, btn, &di)
 *             -> afforded process? reset + begin + press sample
 *     else:   px_afford_compile(g, x, y, btn, &pi)
 *             -> afforded closure? trigger it (v0.7, unchanged)
 *     else:   no-op (no raw fallback in this app)
 *     move:   active process? sample it (preview is DERIVED per
 *             frame from the trajectory — zero estimate writes)
 *     up:     active process? release sample + COMMIT
 *
 * What makes this a "real application" and not a toy:
 *   - FIVE regions, two drag processes, one closure, ONE routing
 *     rule — the router branches on NOTHING but the compile results
 *   - which regions drag is GRAPH DATA: chip-red, chip-green and
 *     chip-blue afford the shared chip_drag process; the slider
 *     affords its own slider_drag; the swatch affords only the
 *     select closure. The app never hand-wires a begin.
 *   - chip-blue is the DUAL-FORM region: it affords the process AND
 *     the select closure. The press is ambiguous (tap vs drag);
 *     the commit hook resolves by MEASURE — a tap (displacement
 *     < 8px) re-compiles the press position through the CLOSURE
 *     form (the graph, not an if-chain), a drag drops a dot.
 *   - the slider's drag works TWICE on the same process object —
 *     the AFFORDS edge points at a stable target, and
 *     px_interaction_reset is the rearm between gestures.
 *   - live preview is derived per frame from the trajectory
 *     (the palette_afford pattern): 40-sample drag, 1 estimate write
 *
 * Deterministic script (headless, no backend needed):
 *   1. drag chip-red   -> canvas   : red dot at the drop point
 *   2. drag chip-green -> canvas   : green dot at the drop point
 *   3. TAP chip-blue   (dual form) : select fires (tap = small-
 *      displacement commit, re-compiled through the closure form)
 *   4. drag chip-blue  (dual form) : blue dot — the SAME region
 *      drags, resolved by measure, not by mode
 *   5. click swatch    (closure-only, the v0.7 control): select
 *      fires on the DOWN, no process ever begins
 *   6. drag slider 14 -> 78       : 40 samples, live preview
 *      derived, ONE estimate write at commit
 *   7. drag slider 78 -> 20       : the SECOND drag on the same
 *      process (the reset/rearm pin)
 *   8. press empty space          : no-op
 *   9. drag to empty space        : release outside the canvas —
 *      no dot (drop semantics live in the commit hook)
 *  10. mid-drag cancel            : no dot, reason recorded
 *
 * Build:
 *   cc -std=c17 -I include examples/designer_tools.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c -lm -o build/designer_tools
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TAP_THRESHOLD_PX 8.0

typedef struct {
    double x, y;
    int    color;   /* 0=red 1=green 2=blue */
} Dot;

typedef struct {
    /* Estimates — the ONLY semantic state. */
    px_estimate* sel_color;
    px_estimate* brightness;
    px_estimate* n_dots;

    /* Canvas content. */
    Dot         dots[64];

    /* The intent-compilation layer. */
    px_graph*   graph;
    px_region*  chip[3];
    px_region*  slider;
    px_region*  canvas;
    px_region*  swatch;

    /* The affordances: ONE closure, TWO processes. */
    px_closure*     select;
    px_interaction* chip_drag;     /* shared by the three chips  */
    px_interaction* slider_drag;   /* the slider's own process   */

    /* The app-side pointer stream state (loop-local in px_app_run;
     * kept here because the script IS the event loop). */
    px_interaction* active;

    /* Metrics for the evidence report. */
    int raw_begins;      /* hand-wired begins: must stay 0     */
    int routed_begins;   /* begins compiled through the graph  */
    int raw_clicks;      /* unresolved downs (no-op path)      */
    int estimate_writes;
    int live_preview_reads;
} App;

static const char* chip_names[3] = { "red", "green", "blue" };

/* ---- layout ------------------------------------------------------ */

static px_rect chip_rect(int i) {
    return px_rect_make(10 + i * 50, 10, 40, 24);
}
/* slider track: x in [10, 310] maps to value [0, 100] */
static double slider_x_to_value(double x) {
    if (x < 10) return 0;
    if (x > 310) return 100;
    return (x - 10) * 100.0 / 300.0;
}

/* ---- the select closure (serves taps + the click-only swatch) ---- */

static bool eval_true(void* u) { (void)u; return true; }

static void on_select(px_intent intent, void* user) {
    App* app = (App*)user;
    /* The same payload-shape sorting as palette_afford: the region
     * label is the semantic type; the channel/form is a projection. */
    const char* region = NULL;
    if (intent.payload && intent.payload_size == sizeof(px_pointer_intent)) {
        region = ((const px_pointer_intent*)intent.payload)->region;
    } else if (intent.payload && intent.payload_size == sizeof(px_drag_intent)) {
        region = ((const px_drag_intent*)intent.payload)->region;
    }
    if (!region) return;
    for (int i = 0; i < 3; i++) {
        char label[32];
        snprintf(label, sizeof(label), "chip-%s", chip_names[i]);
        if (strcmp(region, label) == 0) {
            px_estimate_set(app->sel_color, (double)i, 1.0);
            app->estimate_writes++;
            return;
        }
    }
    /* The click-only swatch denotes "select red" — the same
     * label-driven dispatch, one more label shape. */
    if (strcmp(region, "swatch") == 0) {
        px_estimate_set(app->sel_color, 0.0, 1.0);
        app->estimate_writes++;
    }
}

/* ---- the chip-drag process: arbitration by MEASURE --------------- */

static void on_chip_commit(px_interaction* it, px_int_phase phase, void* user) {
    App* app = (App*)user;
    if (phase != PX_INT_COMMITTED) return;

    const px_int_sample* first = px_interaction_at(it, 0);
    const px_int_sample* last  = px_interaction_last(it);
    if (!first || !last) return;

    /* Which chip was pressed? Re-derived from the trajectory's own
     * first sample (the hover_drag_interaction idiom) — the press
     * position, not a mode flag the app set. */
    px_region* pressed = px_region_at(first->x, first->y);
    if (!pressed) return;

    /* TAP or DRAG? The measure decides — not a mode, not a timer. */
    double dx = last->x - first->x;
    double dy = last->y - first->y;
    if (dx * dx + dy * dy < TAP_THRESHOLD_PX * TAP_THRESHOLD_PX) {
        /* The tap path: re-compile the press position through the
         * CLOSURE form. The dual-form region's discrete act is
         * reached THROUGH the process's commit — the graph decides,
         * the app just asks again. */
        px_pointer_intent pi;
        px_closure* c = px_afford_compile(app->graph, first->x, first->y,
                                          first->button, &pi);
        if (c) px_closure_trigger(c, &pi, sizeof(pi));
        return;
    }

    /* The drag path: a dot lands at the release position, colored
     * by the pressed chip. Drop semantics: the release must be on
     * the canvas — a query, not a branch on coordinates. */
    px_region* drop = px_region_at(last->x, last->y);
    if (drop != app->canvas) return;
    if (px_estimate_value(app->n_dots) >= 64) return;

    int color = 0;
    for (int i = 0; i < 3; i++) {
        if (pressed == app->chip[i]) { color = i; break; }
    }
    int idx = (int)px_estimate_value(app->n_dots);
    app->dots[idx].x = last->x;
    app->dots[idx].y = last->y;
    app->dots[idx].color = color;
    px_estimate_set(app->n_dots, (double)(idx + 1), 1.0);
    app->estimate_writes++;
}

/* ---- the slider-drag process: derived preview, one write --------- */

static void on_slider_commit(px_interaction* it, px_int_phase phase, void* user) {
    App* app = (App*)user;
    if (phase != PX_INT_COMMITTED) return;
    const px_int_sample* s = px_interaction_last(it);
    if (!s) return;
    px_estimate_set(app->brightness, slider_x_to_value(s->x), 1.0);
    app->estimate_writes++;
}

/* Live preview: derived per frame from the trajectory — the sample
 * stream costs zero estimate writes (the palette_afford pattern). */
static double slider_preview(const App* app) {
    if (px_interaction_phase(app->slider_drag) == PX_INT_ACTIVE ||
        px_interaction_phase(app->slider_drag) == PX_INT_BEGAN) {
        const px_int_sample* s = px_interaction_last(app->slider_drag);
        if (s) return slider_x_to_value(s->x);
    }
    return px_estimate_value(app->brightness);
}

/* ---- THE routing rule (mirrors px_app_run v0.8 exactly) --------- */

static void route_down(App* app, double x, double y, int button) {
    /* This is the px_app_run PX_EV_MOUSE_DOWN decision, verbatim:
     * the process form FIRST, then the closure form, then the raw
     * fallback (which this app leaves NULL by design). The rule
     * branches on NOTHING but the compile results — which regions
     * drag is graph data, decided by the AFFORDS edges below. */
    px_drag_intent di;
    px_interaction* proc = px_afford_compile_process(app->graph, x, y,
                                                     button, &di);
    if (proc) {
        if (app->active) {
            px_interaction_cancel(app->active, "superseded by a new press");
        }
        px_interaction_reset(proc);           /* the rearm — a stable
                                               * edge target       */
        px_interaction_begin(proc);
        px_int_sample press = { 400.0, di.x, di.y, 0.0, di.button, 0 };
        px_interaction_sample(proc, &press);
        app->active = proc;
        app->routed_begins++;
        return;
    }

    px_pointer_intent pi;
    px_closure* c = px_afford_compile(app->graph, x, y, button, &pi);
    if (c) {
        px_closure_trigger(c, &pi, sizeof(pi));
        return;
    }
    app->raw_clicks++;   /* the no-op path (counted, not handled) */
}

static void route_move(App* app, double x, double y, double t_ms) {
    if (app->active) {
        px_int_phase ph = px_interaction_phase(app->active);
        if (ph == PX_INT_COMMITTED || ph == PX_INT_CANCELLED) {
            app->active = NULL;             /* app-side cancel    */
        } else {
            px_int_sample move = { t_ms, x, y, 0.0, 0, 0 };
            px_interaction_sample(app->active, &move);
            return;
        }
    }
    /* No raw move handler in this app: unresolved moves are the
     * process's business or nobody's. */
}

static void route_up(App* app, double x, double y, double t_ms) {
    if (app->active) {
        px_int_phase ph = px_interaction_phase(app->active);
        if (ph != PX_INT_COMMITTED && ph != PX_INT_CANCELLED) {
            px_int_sample release = { t_ms, x, y, 0.0, 0, 0 };
            px_interaction_sample(app->active, &release);
            px_interaction_commit(app->active);
        }
        app->active = NULL;
        return;
    }
}

/* A scripted drag: down at (x0,y0), n interpolated moves, up at
 * (x1,y1). Deterministic timestamps (the script IS the event loop). */
static void script_drag(App* app, double x0, double y0,
                        double x1, double y1, int n, int button) {
    route_down(app, x0, y0, button);
    for (int i = 1; i <= n; i++) {
        double t = 400.0 + (double)i;
        double x = x0 + (x1 - x0) * i / (double)n;
        double y = y0 + (y1 - y0) * i / (double)n;
        route_move(app, x, y, t);
        (void)slider_preview(app);   /* the per-frame derived read */
        app->live_preview_reads++;
    }
    route_up(app, x1, y1, 400.0 + (double)n + 1);
}

/* ====================================================================
 * Main — deterministic event script
 * ==================================================================== */

int main(void) {
    printf("Planex designer_tools — Line 2 real-app evidence (v0.8)\n");
    printf("========================================================\n");
    printf("Drags are data-driven: what drags is decided by the\n");
    printf("AFFORDS graph. The router is ONE rule, zero region\n");
    printf("branches, zero hand-wired begins.\n\n");

    App app = {0};
    app.graph = px_graph_new();
    app.sel_color  = px_estimate_new(0.0, 1.0);
    app.brightness = px_estimate_new(50.0, 1.0);
    app.n_dots     = px_estimate_new(0.0, 1.0);

    /* Regions — geometry + label. The label is the semantic type. */
    for (int i = 0; i < 3; i++) {
        char label[32];
        snprintf(label, sizeof(label), "chip-%s", chip_names[i]);
        app.chip[i] = px_region_new(chip_rect(i), label);
    }
    app.slider = px_region_new(px_rect_make(10, 44, 300, 24), "brightness");
    app.canvas = px_region_new(px_rect_make(10, 78, 300, 110), "canvas");
    app.swatch = px_region_new(px_rect_make(270, 10, 40, 24), "swatch");

    /* Closures + processes — the affordances. */
    app.select = px_closure_new("select", PX_INTENT_REQUEST,
                                on_select, eval_true, &app);
    app.chip_drag   = px_interaction_new("chip drag", 64);
    app.slider_drag = px_interaction_new("slider drag", 64);
    px_interaction_on_phase(app.chip_drag,   on_chip_commit,   &app);
    px_interaction_on_phase(app.slider_drag, on_slider_commit, &app);

    /* THE graph data — what drags, what clicks, what does both:
     *   every chip      AFFORDS the shared chip_drag process
     *   chip-blue       also affords the select closure (DUAL form)
     *   the slider      affords its own slider_drag process
     *   the swatch      affords ONLY the select closure (v0.7 shape)
     * Nothing else in the app decides draggability. */
    for (int i = 0; i < 3; i++)
        px_declare(app.graph, app.chip[i], PX_REL_AFFORDS, app.chip_drag);
    px_declare(app.graph, app.chip[2], PX_REL_AFFORDS, app.select);
    px_declare(app.graph, app.slider,  PX_REL_AFFORDS, app.slider_drag);
    px_declare(app.graph, app.swatch,  PX_REL_AFFORDS, app.select);

    /* Sanity: drag-ability is a query, and it says what the edges say. */
    assert(px_region_affords_process(app.graph, app.chip[0]));
    assert(px_region_affords_process(app.graph, app.chip[1]));
    assert(px_region_affords_process(app.graph, app.chip[2]));
    assert(px_region_affords_process(app.graph, app.slider));
    assert(!px_region_affords_process(app.graph, app.canvas));
    assert(!px_region_affords_process(app.graph, app.swatch));

    /* ---- 1. drag chip-red onto the canvas ------------------------ */
    printf("[1] drag chip-red (30,22) -> canvas (150,120)...\n");
    script_drag(&app, 30, 22, 150, 120, 20, 1);
    assert(px_estimate_value(app.n_dots) == 1.0);
    assert(app.dots[0].x == 150 && app.dots[0].y == 120);
    assert(app.dots[0].color == 0);
    printf("    red dot at (%.0f,%.0f) — routed by the graph, dropped\n"
           "    by the commit hook's own region query\n\n",
           app.dots[0].x, app.dots[0].y);

    /* ---- 2. drag chip-green -------------------------------------- */
    printf("[2] drag chip-green (80,22) -> canvas (200,140)...\n");
    script_drag(&app, 80, 22, 200, 140, 20, 1);
    assert(px_estimate_value(app.n_dots) == 2.0);
    assert(app.dots[1].color == 1);
    printf("    green dot at (%.0f,%.0f) — the SAME process, the SAME\n"
           "    rule; the chip is read from the trajectory\n\n",
           app.dots[1].x, app.dots[1].y);

    /* ---- 3. TAP chip-blue: the dual form, tap path --------------- */
    printf("[3] TAP chip-blue (135,22) — no movement...\n");
    {
        int writes0 = app.estimate_writes;
        script_drag(&app, 135, 22, 135, 22, 1, 1);   /* displacement 0 */
        assert(px_estimate_value(app.sel_color) == 2.0);
        assert(px_estimate_value(app.n_dots) == 2.0); /* no dot: tap */
        assert(app.estimate_writes - writes0 == 1);
        printf("    tap -> select fired blue (small-displacement commit\n");
        printf("    re-compiled through the CLOSURE form — the graph\n");
        printf("    decided, the measure arbitrated)\n\n");
    }

    /* ---- 4. drag chip-blue: the dual form, drag path -------------- */
    printf("[4] drag chip-blue (135,22) -> canvas (100,160)...\n");
    script_drag(&app, 135, 22, 100, 160, 20, 1);
    assert(px_estimate_value(app.n_dots) == 3.0);
    assert(app.dots[2].color == 2);
    assert(px_estimate_value(app.sel_color) == 2.0); /* unchanged: the
        drag path never touched the closure */
    printf("    blue dot at (%.0f,%.0f) — the SAME region, the SAME\n"
           "    process, a different MEASURE: one region, two acts,\n"
           "    zero mode flags\n\n", app.dots[2].x, app.dots[2].y);

    /* ---- 5. click the swatch: the v0.7 control ------------------- */
    printf("[5] click swatch (290,22) — closure-only region...\n");
    {
        int begins0 = app.routed_begins;
        route_down(&app, 290, 22, 1);   /* a press, not a drag */
        assert(px_estimate_value(app.sel_color) == 0.0);  /* "red" via
            the label path — the swatch is chip-shaped to on_select */
        assert(app.routed_begins == begins0);  /* no process began */
        printf("    select fired on the DOWN (v0.7 semantics, kept\n");
        printf("    byte-for-byte for closure-only regions)\n\n");
    }

    /* ---- 6. drag the slider: derived preview, one write ----------- */
    printf("[6] drag slider 14 -> 78...\n");
    {
        int writes0 = app.estimate_writes;
        int reads0  = app.live_preview_reads;
        script_drag(&app, 14, 56, 244, 56, 40, 1);
        assert(px_estimate_value(app.brightness) > 77.0);
        assert(px_estimate_value(app.brightness) < 79.0);
        assert(app.estimate_writes - writes0 == 1);
        printf("    committed brightness: %.0f (preview reads: %d,\n"
               "    estimate writes during the 42-sample drag: 1)\n\n",
               px_estimate_value(app.brightness),
               app.live_preview_reads - reads0);
    }

    /* ---- 7. drag the slider AGAIN: the reset pin ----------------- */
    printf("[7] drag slider 250 -> 70 (the SECOND drag)...\n");
    {
        int writes0 = app.estimate_writes;
        script_drag(&app, 250, 56, 220, 56, 10, 1);
        assert(px_estimate_value(app.brightness) > 69.0);
        assert(px_estimate_value(app.brightness) < 71.0);
        assert(app.estimate_writes - writes0 == 1);
        printf("    committed brightness: %.0f — the SAME process\n"
               "    object served its second gesture (the AFFORDS\n"
               "    edge points at a stable target; reset rearmed it)\n\n",
               px_estimate_value(app.brightness));
    }

    /* ---- 8. press empty space: no affordance ---------------------- */
    printf("[8] press empty space (5,200)...\n");
    {
        double dots0 = px_estimate_value(app.n_dots);
        route_down(&app, 5, 200, 1);
        route_up(&app, 5, 200, 401.0);
        assert(px_estimate_value(app.n_dots) == dots0);
        assert(app.active == NULL);
        printf("    no-op: no region there, no handler exists\n\n");
    }

    /* ---- 9. drag to empty space: no dot --------------------------- */
    printf("[9] drag chip-red -> empty space (5,200)...\n");
    {
        double dots0 = px_estimate_value(app.n_dots);
        script_drag(&app, 30, 22, 5, 200, 20, 1);
        assert(px_estimate_value(app.n_dots) == dots0);
        printf("    gesture committed, no dot — the drop query in the\n");
        printf("    commit hook found no canvas under the release\n\n");
    }

    /* ---- 10. mid-drag cancel: no dot ------------------------------ */
    printf("[10] mid-drag cancel (app-side, e.g. an ESC handler)...\n");
    {
        double dots0 = px_estimate_value(app.n_dots);
        int writes0  = app.estimate_writes;
        route_down(&app, 30, 22, 1);          /* chip-red        */
        route_move(&app, 60, 40, 410.0);      /* mid-flight      */
        px_interaction_cancel(app.chip_drag, "app escape");
        assert(px_interaction_phase(app.chip_drag) == PX_INT_CANCELLED);
        route_move(&app, 100, 80, 411.0);     /* post-cancel: the
                                                 router releases   */
        route_up(&app, 150, 120, 412.0);
        assert(app.active == NULL);
        assert(px_estimate_value(app.n_dots) == dots0);
        assert(app.estimate_writes == writes0);
        printf("    cancelled mid-gesture: no dot, no write, the\n");
        printf("    stream released on the next event (reason: \"%s\")\n\n",
               px_interaction_cancel_reason(app.chip_drag));
    }

    /* ---- evidence summary ---------------------------------------- */
    printf("Final state: sel=%s brightness=%.0f dots=%d\n",
           chip_names[(int)px_estimate_value(app.sel_color)],
           px_estimate_value(app.brightness),
           (int)px_estimate_value(app.n_dots));
    printf("Semantic estimate writes (whole session): %d\n",
           app.estimate_writes);
    printf("Hand-wired begins (px_interaction_* outside the router): 0\n");
    printf("Begins compiled through the graph: %d\n", app.routed_begins);
    printf("Raw-coordinate handlers defined: 0 (on_click == NULL)\n");
    printf("Unresolved presses (no-op path): %d\n", app.raw_clicks);
    assert(app.raw_begins == 0);

    printf("\n=== Line 2 evidence complete ===\n");
    printf("ADR-0017/0018 joint obligation (L15b) — what this app shows:\n");
    printf("  1. Drags route through the afford graph: 5 regions, 2\n");
    printf("     processes, 1 closure, ONE routing rule, 0 region\n");
    printf("     branches, 0 hand-wired begins\n");
    printf("  2. The begin seam is gone: a down on a process-affording\n");
    printf("     region compiles to the process (px_drag_intent embeds\n");
    printf("     the label — the value contract, tests/test_v08.c e4)\n");
    printf("  3. One graph, two forms: the closure form still serves\n");
    printf("     the swatch on the down; the dual chip resolves tap vs\n");
    printf("     drag by MEASURE at commit (see tests/test_v08.c f2)\n");
    printf("  4. Process reuse: the slider's second drag runs on the\n");
    printf("     same object (px_interaction_reset — the stable target)\n");

    /* ---- cleanup ------------------------------------------------- */
    px_interaction_free(app.chip_drag);
    px_interaction_free(app.slider_drag);
    px_closure_free(app.select);
    for (int i = 0; i < 3; i++) px_region_free(app.chip[i]);
    px_region_free(app.slider);
    px_region_free(app.canvas);
    px_region_free(app.swatch);
    px_estimate_free(app.sel_color);
    px_estimate_free(app.brightness);
    px_estimate_free(app.n_dots);
    px_graph_free(app.graph);
    return 0;
}
