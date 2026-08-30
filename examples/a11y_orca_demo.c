/*
 * a11y_orca_demo.c — v0.8 Cross-cutting A: the orca end-to-end
 * evidence app
 *
 * The v0.7 AT-SPI2 bridge shipped behind PX_A11Y_ATSPI with a CI
 * compile-probe; what it lacked was the observed pass: a Planex app
 * navigable by a real screen reader. This app is that pass. It is
 * deliberately the SMALLEST app that exercises the whole chain —
 *
 *   X key event -> px_app_run focus ring (ADR-0020: the ring is
 *   DERIVED from the AFFORDS graph) -> on_focus -> the a11y query
 *   side (role/name/value/state) -> px_a11y_bridge_atspi_flush ->
 *   the AtkObject mirror -> atk-bridge -> D-Bus -> orca -> speech
 *
 * — because the point is not widget coverage (palette_afford and
 * designer_tools carry that); the point is that focus order, being
 * graph data since Line 1, projects onto the accessibility bus
 * without a second source of truth. The keyboard channel and the
 * screen reader read the same AFFORDS edges.
 *
 * What the app is: three color swatches (red/green/blue) and a
 * reset button. All four regions afford closures, so all four sit
 * on the derived focus ring: Tab walks red -> green -> blue ->
 * reset -> red...; Enter/Space activates the focused swatch via a
 * px_key_intent; pointer clicks route through the same graph via
 * px_pointer_intent (dual channel, one graph). Selection state
 * lives in ONE estimate (sel_color; -1 = none) — the a11y mirror
 * is derived from it on every flush, never hand-tracked.
 *
 * Build WITHOUT the bridge (any backend, the default):
 *   make examples            # windowed set, stub attach, runs fine
 *
 * Build WITH the bridge (Linux, needs atk + atk-bridge dev):
 *   scripts/verify_orca_e2e.sh --build-only
 * (the harness owns the full compile line: every src translation
 * unit + atk/atk-bridge via pkg-config; see that script)
 *
 * Run under orca (the observed pass):
 *   scripts/verify_orca_e2e.sh
 * or by hand: Xvfb :99 + a D-Bus session + orca --debug, then
 * Tab/Enter through the ring and read orca's speech log.
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include "planex/a11y.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN_W 340
#define WIN_H 130

#define N_SWATCHES 3
static const char* swatch_names[N_SWATCHES] = { "red", "green", "blue" };
static const uint32_t swatch_colors[N_SWATCHES] = {
    0xFF5252u, 0xFF4FC3A5u, 0xFF5294F2u
};

typedef struct {
    /* The ONLY semantic state (A1: the estimate is the state). */
    px_estimate* sel_color;        /* -1 none, 0..2 = swatch index */

    /* The intent-compilation layer. */
    px_graph*   graph;
    px_region*  swatch[N_SWATCHES];
    px_region*  reset;
    px_closure* select_color;
    px_closure* reset_all;

    /* The a11y channel: query side + the (optional) AT-SPI2 bridge.
     * The query side works everywhere; the bridge is NULL unless
     * the build carries -DPX_A11Y_ATSPI (honest stub otherwise). */
    px_a11y*        a11y;
    px_a11y_bridge* bridge;

    /* Trace for the verification harness (stdout ground truth). */
    int  focus_moves;
    int  activations;
    char focus_label[32];          /* label of the current element  */
} App;

/* ---- layout ------------------------------------------------------ */

static px_rect swatch_rect(int i) {
    return px_rect_make(20 + i * 80, 20, 70, 30);
}

/* ---- the a11y mirror: derive the CURRENT element from the graph
 * state — never a parallel bookkeeping. The label is the semantic
 * type (same string the keyboard intent carries), the role/value/
 * state follow from what the region IS + what the estimates say.  */

static void a11y_mirror(App* app, const char* label) {
    if (!app->a11y) return;
    if (label) {
        strncpy(app->focus_label, label, sizeof(app->focus_label) - 1);
        app->focus_label[sizeof(app->focus_label) - 1] = 0;
    }
    label = app->focus_label;

    unsigned state = PX_A11Y_STATE_ENABLED | PX_A11Y_STATE_FOCUSED;
    char value[32] = "";

    int sel = (int)px_estimate_value(app->sel_color);
    for (int i = 0; i < N_SWATCHES; i++) {
        char l[32];
        snprintf(l, sizeof(l), "swatch-%s", swatch_names[i]);
        if (strcmp(label, l) == 0) {
            px_a11y_set_role(app->a11y, PX_A11Y_ROLE_BUTTON);
            px_a11y_set_name(app->a11y, l);
            snprintf(value, sizeof(value), "%s",
                     i == sel ? "selected" : "unselected");
            if (i == sel) state |= PX_A11Y_STATE_SELECTED;
            px_a11y_set_value(app->a11y, value);
            px_a11y_set_state(app->a11y, state);
            return;
        }
    }
    if (strcmp(label, "reset") == 0) {
        px_a11y_set_role(app->a11y, PX_A11Y_ROLE_BUTTON);
        px_a11y_set_name(app->a11y, "reset");
        px_a11y_set_value(app->a11y, value);
        px_a11y_set_state(app->a11y, state);
        return;
    }
    /* Before the first Tab the current element is the window. */
    px_a11y_set_role(app->a11y, PX_A11Y_ROLE_WINDOW);
    px_a11y_set_name(app->a11y, "Planex orca demo");
    px_a11y_set_value(app->a11y, "");
    px_a11y_set_state(app->a11y, PX_A11Y_STATE_ENABLED);
}

/* ---- closure actions (the app's semantic handlers) --------------- */

static bool eval_true(void* u) { (void)u; return true; }

static void announce_and_flush(App* app, const char* msg) {
    px_a11y_announce(app->a11y, msg);
    /* Re-mirror the current element first: an activation can change
     * the SELECTED state of the focused region itself (e.g. reset
     * clearing the selection while focus sits on reset). */
    a11y_mirror(app, NULL);
    px_a11y_bridge_atspi_flush(app->bridge);
}

static void on_select_color(px_intent intent, void* user) {
    App* app = (App*)user;
    /* Both channels, one closure: the region label arrives in the
     * payload — pointer or key, the handler never asks which. */
    const char* region = NULL;
    if (intent.payload && intent.payload_size == sizeof(px_pointer_intent)) {
        region = ((const px_pointer_intent*)intent.payload)->region;
    } else if (intent.payload && intent.payload_size == sizeof(px_key_intent)) {
        region = ((const px_key_intent*)intent.payload)->region;
    }
    if (!region) return;

    for (int i = 0; i < N_SWATCHES; i++) {
        char l[32];
        snprintf(l, sizeof(l), "swatch-%s", swatch_names[i]);
        if (strcmp(region, l) == 0) {
            px_estimate_set(app->sel_color, (double)i, 1.0);
            app->activations++;
            char msg[64];
            snprintf(msg, sizeof(msg), "Selected %s", swatch_names[i]);
            printf("[act] %s\n", msg);
            announce_and_flush(app, msg);
            return;
        }
    }
}

static void on_reset_all(px_intent intent, void* user) {
    (void)intent;
    App* app = (App*)user;
    px_estimate_set(app->sel_color, -1.0, 1.0);
    app->activations++;
    printf("[act] Reset: nothing selected\n");
    announce_and_flush(app, "Reset: nothing selected");
}

/* ---- app-framework callbacks ------------------------------------- */

static void on_focus(const char* region_label, void* user) {
    App* app = (App*)user;
    app->focus_moves++;
    printf("[focus] %s\n", region_label);
    a11y_mirror(app, region_label);
    px_a11y_bridge_atspi_flush(app->bridge);
}

/* Per-frame flush: the mirror sync is change-guarded, and — the
 * part that matters — the flush pumps the bridge's D-Bus traffic
 * (incoming AT-client reads). Without a regular flush the app
 * answers no one and clients mark it hung (see the bridge source,
 * round three of the orca run). ~60 Hz here; the pump only drains
 * pending work. */
static bool on_tick(double dt_ms, void* user) {
    (void)dt_ms;
    App* app = (App*)user;
    px_a11y_bridge_atspi_flush(app->bridge);
    return false;
}

static px_fb* render(void* user) {
    App* app = (App*)user;
    px_fb* fb = px_fb_new(WIN_W, WIN_H);
    if (!fb) return NULL;

    px_fb_clear(fb, 0xFF20242Bu);
    px_fb_draw_rect(fb, 0, 0, WIN_W, WIN_H, 0xFF3A3F4Bu);

    int sel = (int)px_estimate_value(app->sel_color);
    int focused = -1;             /* which swatch holds focus  */
    for (int i = 0; i < N_SWATCHES; i++) {
        char l[32];
        snprintf(l, sizeof(l), "swatch-%s", swatch_names[i]);
        if (strcmp(app->focus_label, l) == 0) focused = i;
    }
    for (int i = 0; i < N_SWATCHES; i++) {
        px_rect r = swatch_rect(i);
        px_fb_fill_rect(fb, (int)r.x, (int)r.y, (int)r.w, (int)r.h,
                        swatch_colors[i]);
        if (i == sel) {
            /* The selection marker — pixels derived from the
             * estimate, exactly like the a11y mirror above. */
            px_fb_draw_rect(fb, (int)r.x + 6, (int)r.y + 6,
                            (int)r.w - 12, (int)r.h - 12, 0xFFFFFFFFu);
        }
        if (i == focused) {
            /* The focus indicator the framework never draws — the
             * app renders it FROM the focus data (app.h contract). */
            px_fb_draw_rect(fb, (int)r.x - 2, (int)r.y - 2,
                            (int)r.w + 4, (int)r.h + 4, 0xFFFFFF00u);
        }
    }

    px_fb_fill_rect(fb, 262, 20, 58, 30, 0xFF3A3F4Bu);
    px_fb_draw_rect(fb, 262, 20, 58, 30, 0xFF8A8F9Bu);
    px_fb_draw_text(fb, 272, 26, "reset", 0xFFD5DAE3u);
    if (strcmp(app->focus_label, "reset") == 0) {
        px_fb_draw_rect(fb, 260, 18, 62, 34, 0xFFFFFF00u);
    }

    char status[96];
    snprintf(status, sizeof(status), "Selected: %s    Tab: focus  Enter: activate",
             sel >= 0 && sel < N_SWATCHES ? swatch_names[sel] : "none");
    px_fb_draw_text(fb, 20, 70, status, 0xFF8A8F9Bu);
    px_fb_draw_text(fb, 20, 88, "q: quit", 0xFF60656Fu);

    return fb;
}

/* ====================================================================
 * Main
 * ==================================================================== */

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);   /* the harness reads live */
    printf("Planex orca demo — Cross-cutting A evidence app (v0.8)\n");
    printf("=======================================================\n");
    printf("Focus ring derived from AFFORDS edges; the a11y mirror\n");
    printf("projects the same graph onto AT-SPI2.\n\n");

    App app = {0};
    app.sel_color = px_estimate_new(-1.0, 1.0);
    app.graph     = px_graph_new();

    for (int i = 0; i < N_SWATCHES; i++) {
        char label[32];
        snprintf(label, sizeof(label), "swatch-%s", swatch_names[i]);
        app.swatch[i] = px_region_new(swatch_rect(i), label);
    }
    app.reset = px_region_new(px_rect_make(262, 20, 58, 30), "reset");

    app.select_color = px_closure_new_with_graph(
        "select color", PX_INTENT_REQUEST,
        on_select_color, eval_true, &app, app.graph);
    app.reset_all = px_closure_new_with_graph(
        "reset all", PX_INTENT_REQUEST,
        on_reset_all, eval_true, &app, app.graph);

    for (int i = 0; i < N_SWATCHES; i++)
        px_declare(app.graph, app.swatch[i], PX_REL_AFFORDS,
                   app.select_color);
    px_declare(app.graph, app.reset, PX_REL_AFFORDS, app.reset_all);

    /* The a11y channel: query side always, the bridge when compiled.
     * The window does not exist yet (px_app_run creates it); the
     * query side tolerates that — it never dereferences the window,
     * and the bridge mirrors only names/roles/states. */
    app.a11y = px_a11y_new(NULL);
    app.bridge = px_a11y_bridge_atspi_attach(app.a11y, "planex-orca-demo");
    printf("[a11y] bridge: %s\n", app.bridge
           ? "attached (AT-SPI2 mirror live)"
           : "stub — rebuild with -DPX_A11Y_ATSPI for orca");

    /* Prime the mirror with the window element before the ring
     * walk starts, so orca meets a sane tree from the first frame. */
    a11y_mirror(&app, NULL);
    px_a11y_bridge_atspi_flush(app.bridge);

    px_app_desc desc = {
        .width        = WIN_W,
        .height       = WIN_H,
        .title        = "Planex orca demo",
        .perception   = render,
        .intent_graph = app.graph,
        .on_focus     = on_focus,
        .on_tick      = on_tick,
        .user         = &app,
    };
    int rc = px_app_run(&desc);

    /* Read the estimates BEFORE freeing (the summary reports the
     * final semantic state — the same state the mirror projected). */
    int final_sel = (int)px_estimate_value(app.sel_color);
    px_a11y_bridge_atspi_detach(app.bridge);
    px_a11y_free(app.a11y);
    px_closure_free(app.reset_all);
    px_closure_free(app.select_color);
    px_region_free(app.reset);
    for (int i = 0; i < N_SWATCHES; i++) px_region_free(app.swatch[i]);
    px_estimate_free(app.sel_color);
    px_graph_free(app.graph);

    printf("\n[summary] focus moves=%d activations=%d selected=%d\n",
           app.focus_moves, app.activations, final_sel);
    (void)rc;
    return 0;
}
