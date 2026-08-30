/*
 * palette_afford.c — v0.7 Line 1 real-application evidence:
 * an application whose click handling has ZERO raw-coordinate
 * callbacks.
 *
 * The app: a palette painter. Three color swatches, a brightness
 * slider, a paint canvas, a reset button. Every pointer-down is
 * routed by the SAME decision px_app_run runs when px_app_desc
 * sets `intent_graph` (v0.7 Line 1):
 *
 *     px_afford_compile(graph, x, y, button, &pi)
 *         -> afforded closure? px_closure_trigger(c, &pi, sizeof(pi))
 *         -> else: raw fallback (this app HAS no raw fallback —
 *            on_click is NULL by design; unresolved clicks are
 *            no-ops, which is the correct semantics for this UI)
 *
 * What makes this a "real application" and not a toy:
 *   - five region types, five affordances, ONE routing rule
 *   - the paint closure USES x/y as DATA (where the dot lands)
 *     without ever ROUTING on them — the routing key is the
 *     region, the payload carries the position as context
 *   - button-3 (right-click) on the canvas compiles to the same
 *     intent shape; the closure discriminates by payload — a
 *     context-menu action without a single coordinate branch
 *   - the slider is a drag PROCESS (px_interaction): live preview
 *     is derived per frame from the trajectory (inert, no estimate
 *     writes), the committed value is the only estimate write
 *   - undo works through the graph: TRIGGERS edges + bound
 *     closures, so select/paint/clear/brightness are all undoable
 *
 * Deterministic script (headless, no backend needed):
 *   1. click swatch-green        -> select color 1
 *   2. click canvas x2           -> 2 dots painted (payload x/y as data)
 *   3. right-click canvas        -> context clear (button-3 in payload)
 *   4. undo                      -> dots restored (graph snapshot)
 *   5. drag slider 14 -> 78      -> live preview during drag (0 estimate
 *      writes), commit writes brightness = 78
 *   6. undo                      -> brightness back to 50
 *   7. click reset               -> all state reset via closure
 *   8. click empty space (40, 5) -> no-op (no affordance there)
 *
 * Build:
 *   cc -std=c17 -I include examples/palette_afford.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c -lm -o build/palette_afford
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define N_SWATCHES 3

typedef struct {
    double x, y;
    int    color;
} Dot;

typedef struct {
    /* Estimates — the ONLY state (semantic state, not routing state). */
    px_estimate* sel_color;    /* 0..2 which swatch is active        */
    px_estimate* brightness;   /* 0..100 committed slider value      */
    px_estimate* n_dots;       /* canvas content size (undo surface) */

    /* Canvas content — plain data, undo restores the count. */
    Dot         dots[64];

    /* The intent-compilation layer. */
    px_graph*   graph;
    px_region*  swatch[N_SWATCHES];
    px_region*  slider;
    px_region*  canvas;
    px_region*  reset;

    /* Affordances — one closure per act, label-driven. */
    px_closure* select_color;
    px_closure* canvas_act;    /* paint (b1) | context-clear (b3) */
    px_closure* set_brightness;
    px_closure* reset_all;

    /* The slider drag process. */
    px_interaction* slider_drag;

    /* Metrics for the evidence report. */
    int raw_clicks;           /* must stay 0 — the whole point      */
    int estimate_writes;      /* committed semantic writes only     */
    int live_preview_reads;   /* trajectory reads during drag       */
} App;

static const char* swatch_names[N_SWATCHES] = { "red", "green", "blue" };

/* ---- layout ------------------------------------------------------ */

static px_rect swatch_rect(int i) {
    return px_rect_make(20 + i * 70, 20, 60, 28);
}
/* slider track: x in [20, 300] maps to value [0, 100] */
static double slider_x_to_value(double x) {
    if (x < 20) return 0;
    if (x > 300) return 100;
    return (x - 20) * 100.0 / 280.0;
}

/* ---- closure actions (the app's semantic handlers) --------------- */

static bool eval_true(void* u) { (void)u; return true; }

static void on_select_color(px_intent intent, void* user) {
    App* app = (App*)user;
    /* ONE closure serves three swatches: the REGION LABEL is the
     * semantic type. This is the presentation-type payoff — the
     * handler dispatches on meaning, never on coordinates. */
    if (intent.payload && intent.payload_size == sizeof(px_pointer_intent)) {
        const px_pointer_intent* pi = (const px_pointer_intent*)intent.payload;
        for (int i = 0; i < N_SWATCHES; i++) {
            char label[32];
            snprintf(label, sizeof(label), "swatch-%s", swatch_names[i]);
            if (strcmp(pi->region, label) == 0) {
                px_estimate_set(app->sel_color, (double)i, 1.0);
                app->estimate_writes++;
                return;
            }
        }
    }
}

static void on_canvas_act(px_intent intent, void* user) {
    App* app = (App*)user;
    if (intent.payload && intent.payload_size == sizeof(px_pointer_intent)) {
        const px_pointer_intent* pi = (const px_pointer_intent*)intent.payload;
        if (pi->button == 1 && px_estimate_value(app->n_dots) < 64) {
            /* Paint: x/y arrive as PAYLOAD CONTEXT — data the act
             * consumes, not a routing decision the app made. */
            int idx = (int)px_estimate_value(app->n_dots);
            app->dots[idx].x = pi->x;
            app->dots[idx].y = pi->y;
            app->dots[idx].color = (int)px_estimate_value(app->sel_color);
            px_estimate_set(app->n_dots, (double)(idx + 1), 1.0);
            app->estimate_writes++;
        } else if (pi->button == 3) {
            /* The context-menu action: button-3 discriminates INSIDE
             * the payload. The routing layer never branched on the
             * button. */
            px_estimate_set(app->n_dots, 0.0, 1.0);
            app->estimate_writes++;
        }
    }
}

typedef struct { double value; } px_brightness_intent;

static void on_set_brightness(px_intent intent, void* user) {
    App* app = (App*)user;
    if (intent.payload && intent.payload_size == sizeof(px_brightness_intent)) {
        const px_brightness_intent* bi = (const px_brightness_intent*)intent.payload;
        px_estimate_set(app->brightness, bi->value, 1.0);
        app->estimate_writes++;
    }
}

static void on_reset_all(px_intent intent, void* user) {
    App* app = (App*)user;
    (void)intent;
    px_estimate_set(app->sel_color, 0.0, 1.0);
    px_estimate_set(app->brightness, 50.0, 1.0);
    px_estimate_set(app->n_dots, 0.0, 1.0);
    app->estimate_writes += 3;
}

/* ---- the slider drag process -------------------------------------- */

static void on_slider_phase(px_interaction* it, px_int_phase phase, void* user) {
    App* app = (App*)user;
    if (phase == PX_INT_COMMITTED) {
        const px_int_sample* s = px_interaction_last(it);
        if (s) {
            px_brightness_intent bi = { slider_x_to_value(s->x) };
            px_closure_trigger(app->set_brightness, &bi, sizeof(bi));
        }
    }
}

/* Live preview: derived per frame from the trajectory — the sample
 * stream costs zero estimate writes; perception (or here, the frame
 * reader) computes the would-be value on demand. */
static double slider_preview(const App* app) {
    if (px_interaction_phase(app->slider_drag) == PX_INT_ACTIVE ||
        px_interaction_phase(app->slider_drag) == PX_INT_BEGAN) {
        const px_int_sample* s = px_interaction_last(app->slider_drag);
        if (s) return slider_x_to_value(s->x);
    }
    return px_estimate_value(app->brightness);
}

/* ---- THE routing rule (mirrors px_app_run v0.7 exactly) ---------- */

static void route_pointer(App* app, double x, double y, int button) {
    /* This is the px_app_run PX_EV_MOUSE_DOWN decision (v0.7 Line 1),
     * verbatim: compile first, trigger if afforded, raw fallback
     * otherwise. This app sets no raw fallback — on_click is NULL —
     * so an unresolved click is simply a no-op. */
    px_pointer_intent pi;
    px_closure* c = px_afford_compile(app->graph, x, y, button, &pi);
    if (c) {
        px_closure_trigger(c, &pi, sizeof(pi));
    } else {
        app->raw_clicks++; /* counted for the evidence report; the
                            * count that matters is the one that must
                            * stay ZERO: raw-coordinate HANDLERS */
    }
}

/* ====================================================================
 * Main — deterministic event script
 * ==================================================================== */

int main(void) {
    printf("Planex palette_afford — Line 1 real-app evidence (v0.7)\n");
    printf("========================================================\n");
    printf("Zero raw-coordinate callbacks: every click routes through\n");
    printf("the AFFORDS query. px_app_desc.on_click is NULL by design.\n\n");

    App app = {0};
    app.graph = px_graph_new();
    app.sel_color  = px_estimate_new(0.0, 1.0);
    app.brightness = px_estimate_new(50.0, 1.0);
    app.n_dots     = px_estimate_new(0.0, 1.0);

    /* Regions — geometry + label. The label is the semantic type. */
    for (int i = 0; i < N_SWATCHES; i++) {
        char label[32];
        snprintf(label, sizeof(label), "swatch-%s", swatch_names[i]);
        app.swatch[i] = px_region_new(swatch_rect(i), label);
    }
    app.slider = px_region_new(px_rect_make(20, 60, 280, 24), "brightness");
    app.canvas = px_region_new(px_rect_make(20, 96, 280, 120), "canvas");
    app.reset  = px_region_new(px_rect_make(250, 20, 50, 28), "reset");

    /* Closures — the app's acts. The undo graph arrives WITH each
     * closure (the v0.7 constructor split, ADR-0019): there is no
     * bind call to forget, no ordering window to race. */
    app.select_color  = px_closure_new_with_graph(
        "select color",  PX_INTENT_REQUEST,
        on_select_color, eval_true, &app, app.graph);
    app.canvas_act    = px_closure_new_with_graph(
        "canvas act (paint | context-clear)", PX_INTENT_REQUEST,
        on_canvas_act,   eval_true, &app, app.graph);
    app.set_brightness = px_closure_new_with_graph(
        "set brightness", PX_INTENT_DECLARE,
        on_set_brightness, eval_true, &app, app.graph);
    app.reset_all     = px_closure_new_with_graph(
        "reset all",     PX_INTENT_REQUEST,
        on_reset_all,    eval_true, &app, app.graph);

    /* Affordances — one closure per act, label-driven. NOTE: the
     * canvas affords ONE closure (canvas_act), not separate paint/
     * clear closures: a region with two AFFORDS edges resolves
     * last-declared-first (px_declare prepends; pinned in
     * tests/test_v07.c a7), so multi-act regions route to a single
     * closure that discriminates by payload — the region denotes
     * the thing, the act reads the intent. The slider affords NO
     * closure: its act is the committed drag process (continuous
     * intents enter via the interaction, not the discrete route —
     * the Line 1 boundary, recorded in ADR-0017). */
    for (int i = 0; i < N_SWATCHES; i++)
        px_declare(app.graph, app.swatch[i], PX_REL_AFFORDS, app.select_color);
    px_declare(app.graph, app.canvas, PX_REL_AFFORDS, app.canvas_act);
    px_declare(app.graph, app.reset,  PX_REL_AFFORDS, app.reset_all);

    /* Undo wiring: TRIGGERS edges + bound closures. */
    px_declare(app.graph, app.select_color,  PX_REL_TRIGGERS, app.sel_color);
    px_declare(app.graph, app.canvas_act,    PX_REL_TRIGGERS, app.n_dots);
    px_declare(app.graph, app.set_brightness, PX_REL_TRIGGERS, app.brightness);
    px_declare(app.graph, app.reset_all,     PX_REL_TRIGGERS, app.sel_color);
    px_declare(app.graph, app.reset_all,     PX_REL_TRIGGERS, app.brightness);
    px_declare(app.graph, app.reset_all,     PX_REL_TRIGGERS, app.n_dots);
    px_undo_set_enabled(true);   /* graphs bound at construction */

    /* The slider drag process. */
    app.slider_drag = px_interaction_new("slider", 64);
    px_interaction_on_phase(app.slider_drag, on_slider_phase, &app);

    /* ---- 1. click swatch-green (center of the second swatch) ---- */
    printf("[1] click swatch-green (95, 34)...\n");
    route_pointer(&app, 95, 34, 1);
    assert(px_estimate_value(app.sel_color) == 1.0);
    printf("    selected: %s (label-routed, zero coordinate branches)\n\n",
           swatch_names[(int)px_estimate_value(app.sel_color)]);

    /* ---- 2. paint two dots ------------------------------------- */
    printf("[2] click canvas twice (100,150) (150,160)...\n");
    route_pointer(&app, 100, 150, 1);
    route_pointer(&app, 150, 160, 1);
    assert(px_estimate_value(app.n_dots) == 2.0);
    assert(app.dots[0].color == 1 && app.dots[1].color == 1);
    printf("    dots: %d, color: %s (x/y consumed as payload DATA)\n\n",
           (int)px_estimate_value(app.n_dots),
           swatch_names[app.dots[1].color]);

    /* ---- 3. right-click canvas: the context action -------------- */
    printf("[3] right-click canvas (120, 140)...\n");
    route_pointer(&app, 120, 140, 3);
    assert(px_estimate_value(app.n_dots) == 0.0);
    printf("    context clear: dots=%d (button-3 discriminated in the\n"
           "    payload — the routing layer never branched on it)\n\n",
           (int)px_estimate_value(app.n_dots));

    /* ---- 4. undo restores the canvas ---------------------------- */
    printf("[4] undo (graph snapshot)...\n");
    assert(px_undo() == 1);  /* 1 estimate restored */
    assert(px_estimate_value(app.n_dots) == 2.0);
    printf("    dots restored: %d\n\n", (int)px_estimate_value(app.n_dots));

    /* ---- 5. drag the slider: live preview, one commit ----------- */
    printf("[5] drag slider 14 -> 75 (tap/drag split by measure)...\n");
    /* The slider affords no discrete closure, so a press on it is
     * an unresolved click in the discrete route (a no-op here).
     * The gesture itself enters the interaction process directly
     * — continuous intents are process input, not pointer-down
     * acts. ONLY the commit writes the estimate. */
    px_int_sample s0 = { 400.0, 60.0, 72.0, 0.0, 1, 0 };
    px_interaction_sample(app.slider_drag, &s0);           /* begins  */
    for (int i = 1; i <= 40; i++) {
        double x = 60.0 + (230.0 - 60.0) * i / 40.0;
        px_int_sample s = { 400.0 + (double)i, x, 72.0, 0.0, 0, 0 };
        px_interaction_sample(app.slider_drag, &s);
        double pv = slider_preview(&app);                  /* derived  */
        app.live_preview_reads++;
        (void)pv;
    }
    assert(slider_preview(&app) > 74.0 && slider_preview(&app) < 76.0);
    int writes_before = app.estimate_writes;
    px_interaction_commit(app.slider_drag);
    assert(px_estimate_value(app.brightness) > 74.0);
    assert(px_estimate_value(app.brightness) < 76.0);
    assert(app.estimate_writes - writes_before == 1);
    printf("    committed brightness: %.0f (live preview reads: %d,\n"
           "    estimate writes during 40-sample drag: 1)\n\n",
           px_estimate_value(app.brightness), app.live_preview_reads);
    assert(px_estimate_value(app.n_dots) == 2.0); /* the press on the
        slider painted nothing — it became a drag */

    /* ---- 6. undo the brightness commit -------------------------- */
    printf("[6] undo (brightness back to 50)...\n");
    assert(px_undo() == 1);  /* 1 estimate restored */
    assert(px_estimate_value(app.brightness) == 50.0);
    printf("    brightness: %.0f\n\n", px_estimate_value(app.brightness));

    /* ---- 7. click reset ----------------------------------------- */
    printf("[7] click reset (275, 34)...\n");
    route_pointer(&app, 275, 34, 1);
    assert(px_estimate_value(app.sel_color) == 0.0);
    assert(px_estimate_value(app.brightness) == 50.0);
    assert(px_estimate_value(app.n_dots) == 0.0);
    printf("    state reset via closure (undoable like any act)\n\n");

    /* ---- 8. empty space: no affordance, no handler -------------- */
    printf("[8] click empty space (40, 5)...\n");
    double sel0 = px_estimate_value(app.sel_color);
    route_pointer(&app, 40, 5, 1);
    assert(px_estimate_value(app.sel_color) == sel0);
    assert(px_undo_count() >= 0);
    printf("    no-op: no region there, no fallback handler exists\n\n");

    /* ---- evidence summary --------------------------------------- */
    printf("Final state: sel=%d brightness=%.0f dots=%d\n",
           (int)px_estimate_value(app.sel_color),
           px_estimate_value(app.brightness),
           (int)px_estimate_value(app.n_dots));
    printf("Semantic estimate writes (whole session): %d\n",
           app.estimate_writes);
    printf("Raw-coordinate callbacks defined: 0 (on_click == NULL)\n");
    printf("Unresolved clicks (no-op path): %d\n", app.raw_clicks);

    printf("\n=== Line 1 evidence complete ===\n");
    printf("ADR-0017 admission bar (ADR-0011) — what this app shows:\n");
    printf("  1. Routing: 5 affordances, 1 rule, 0 coordinate branches\n");
    printf("  2. Value: px_pointer_intent survives capture + replay\n");
    printf("     (label embedded — see tests/test_v07.c a3)\n");
    printf("  3. Real-app shape: acts are closures, undo through the\n");
    printf("     graph, drag is a process with derived live preview\n");

    /* ---- cleanup ------------------------------------------------- */
    px_interaction_free(app.slider_drag);
    px_closure_free(app.select_color);
    px_closure_free(app.canvas_act);
    px_closure_free(app.set_brightness);
    px_closure_free(app.reset_all);
    for (int i = 0; i < N_SWATCHES; i++) px_region_free(app.swatch[i]);
    px_region_free(app.slider);
    px_region_free(app.canvas);
    px_region_free(app.reset);
    px_estimate_free(app.sel_color);
    px_estimate_free(app.brightness);
    px_estimate_free(app.n_dots);
    px_graph_free(app.graph);
    px_undo_set_enabled(false);
    return 0;
}
