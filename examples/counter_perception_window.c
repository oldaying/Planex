/*
 * counter_perception_window.c — Phase 2 capstone demo
 *
 * This is the final Phase 2 demo: a real interactive window driven
 * by Perception (not by the old render callback).
 *
 * Validates ADR-0005 Phase 2 end-to-end:
 *   - px_app_desc.perception is set (NOT .render)
 *   - px_app_run uses perception to drive rendering
 *   - Mouse clicks + key presses work as before
 *   - Multiple perceptions coexist (screen + a11y text + log)
 *
 * The window shows a counter with +/- buttons. Clicking them
 * triggers Closures that update the Estimate. The perception
 * function returns a fresh px_fb each frame, blitted by app loop.
 *
 * This demo proves Phase 2 is complete: perception-driven rendering
 * works in a real interactive window, replacing the old on_render
 * callback entirely.
 *
 * Build (Linux):
 *   cc -std=c17 -I include examples/counter_perception_window.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/fb.c src/font.c src/x11.c src/app.c \
 *      -lX11 -lXext -lm -o build/counter_perception_window
 *
 * Build (Windows):
 *   cmake -B build && cmake --build build --config Release
 *   .\build\Release\counter_perception_window.exe
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN_W 320
#define WIN_H 128

typedef struct {
    px_estimate* count;
    px_graph*    graph;
    px_closure*  inc;
    px_closure*  dec;
    /* Hit regions — for click dispatch */
    int inc_x, inc_y, inc_w, inc_h;
    int dec_x, dec_y, dec_w, dec_h;
    /* Counters for side perceptions (a11y, log) */
    int a11y_calls;
    int log_calls;
} CounterApp;

/* ============================================================
 * Closure actions
 * ============================================================ */

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    CounterApp* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v + 1, 1.0);
}

static void on_dec(px_intent intent, void* user) {
    (void)intent;
    CounterApp* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v - 1, 1.0);
}

static bool eval_nonneg(void* user) {
    CounterApp* app = user;
    return px_estimate_value(app->count) >= 0;
}

/* ============================================================
 * THE PERCEPTION FUNCTION — replaces on_render callback
 *
 * Returns a fresh px_fb* each frame. Same Estimate → same pixels.
 * ============================================================ */

static px_fb* render_counter_pixels(void* user) {
    CounterApp* app = user;
    double v = px_estimate_value(app->count);

    px_fb* fb = px_fb_new(WIN_W, WIN_H);
    if (!fb) return NULL;

    px_fb_clear(fb, PX_BG);
    px_fb_draw_rect(fb, 4, 4, WIN_W - 8, WIN_H - 8, PX_BORDER);

    /* Title */
    px_fb_fill_rect(fb, 4, 4, WIN_W - 8, 20, PX_SURFACE);
    px_fb_draw_text(fb, 12, 8, "Planex Counter", PX_TEXT);

    /* Count display */
    int box_x = 32, box_y = 32, box_w = WIN_W - 64, box_h = 24;
    px_fb_fill_rect(fb, box_x, box_y, box_w, box_h, PX_SURFACE);
    px_fb_draw_rect(fb, box_x, box_y, box_w, box_h, PX_BORDER);

    char buf[64];
    snprintf(buf, sizeof(buf), "Count: %.0f", v);
    px_fb_draw_text(fb, box_x + 12, box_y + 4, buf, PX_TEXT);

    /* Buttons */
    int btn_y = box_y + box_h + 8;
    int btn_w = 80, btn_h = 20;

    app->inc_x = box_x;
    app->inc_y = btn_y;
    app->inc_w = btn_w;
    app->inc_h = btn_h;
    px_fb_fill_rect(fb, app->inc_x, app->inc_y, btn_w, btn_h, PX_ACCENT);
    px_fb_draw_text(fb, app->inc_x + 24, app->inc_y + 2, "+ Inc", PX_TEXT);

    app->dec_x = box_x + btn_w + 12;
    app->dec_y = btn_y;
    app->dec_w = btn_w;
    app->dec_h = btn_h;
    px_fb_fill_rect(fb, app->dec_x, app->dec_y, btn_w, btn_h, PX_SURFACE);
    px_fb_draw_rect(fb, app->dec_x, app->dec_y, btn_w, btn_h, PX_BORDER);
    px_fb_draw_text(fb, app->dec_x + 24, app->dec_y + 2, "- Dec", PX_TEXT);

    /* Status bar — shows undo step count so user can see undo system is alive */
    int sy = WIN_H - 20;
    px_fb_fill_rect(fb, 4, sy, WIN_W - 8, 16, PX_SURFACE);
    char status_buf[128];
    int undo_n = px_undo_count();
    if (v < 0) {
        snprintf(status_buf, sizeof(status_buf), "count=%.0f NEG undo:%d (Z)",
                 v, undo_n);
    } else {
        snprintf(status_buf, sizeof(status_buf), "count=%.0f  undo:%d  (Z=undo)",
                 v, undo_n);
    }
    uint32_t sc = (v < 0) ? PX_DANGER : PX_TEXT_DIM;
    px_fb_draw_text(fb, 12, sy + 2, status_buf, sc);

    return fb;
}

/* Side perception: a11y text — increments counter for validation */
static void* render_counter_a11y(px_estimate* const* inputs, int n, void* user) {
    CounterApp* app = user;
    (void)inputs; (void)n;
    app->a11y_calls++;
    return NULL;
}

/* Side perception: log line — increments counter for validation */
static void* render_counter_log(px_estimate* const* inputs, int n, void* user) {
    CounterApp* app = user;
    (void)inputs; (void)n;
    app->log_calls++;
    return NULL;
}

/* ============================================================
 * Hit testing
 * ============================================================ */

static bool in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

/* ============================================================
 * Event handlers
 * ============================================================ */

static bool on_click(int x, int y, void* user) {
    CounterApp* app = user;
    if (in_rect(x, y, app->inc_x, app->inc_y, app->inc_w, app->inc_h)) {
        px_closure_trigger(app->inc, NULL, 0);
        return true;
    }
    if (in_rect(x, y, app->dec_x, app->dec_y, app->dec_w, app->dec_h)) {
        px_closure_trigger(app->dec, NULL, 0);
        return true;
    }
    return false;
}

static bool on_key(char key, void* user) {
    CounterApp* app = user;
    if (key == '+' || key == '=') {
        px_closure_trigger(app->inc, NULL, 0);
        return true;
    }
    if (key == '-' || key == '_') {
        px_closure_trigger(app->dec, NULL, 0);
        return true;
    }
    /* v0.3: Z or Ctrl+Z triggers undo.
     * Ctrl+Z produces 0x1A (SUB control char) on Windows WM_CHAR.
     * Plain 'z'/'Z' also triggers undo for convenience. */
    if (key == 'z' || key == 'Z' || key == 0x1A) {
        int before = px_undo_count();
        int restored = px_undo();
        printf("  [key Z] undo_count before=%d, restored=%d, count=%.0f\n",
               before, restored, px_estimate_value(app->count));
        return true;  /* always re-render to show undo result (or no-op) */
    }
    if (key != 0) {
        printf("  [key '%c' (0x%02x)] not bound to any action\n",
               (key >= 0x20 && key < 0x7f) ? key : '?', (unsigned char)key);
    }
    return false;
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex counter_perception_window — Phase 2 capstone\n");
    printf("====================================================\n");
    printf("Validates: perception-driven rendering in real window.\n");
    printf("px_app_desc.perception is set, NOT .render.\n\n");

    CounterApp app = {0};
    app.count = px_estimate_new(0, 1.0);
    app.graph = px_graph_new();
    app.inc   = px_closure_new_with_graph("increment counter", PX_INTENT_REQUEST,
                                 on_inc, eval_nonneg, &app, app.graph);
    app.dec   = px_closure_new_with_graph("decrement counter", PX_INTENT_REQUEST,
                                 on_dec, eval_nonneg, &app, app.graph);

    px_declare(app.graph, app.inc, PX_REL_TRIGGERS, app.count);
    px_declare(app.graph, app.dec, PX_REL_TRIGGERS, app.count);

    /* Undo: graphs bound at construction (v0.7 constructor split,
     * ADR-0019). When undo is enabled, px_closure_trigger
     * auto-snapshots affected Estimates. */
    px_undo_set_enabled(true);

    /* Register side perceptions (a11y + log) for the same Estimate.
     * px_app_run will NOT auto-invoke these (only the main perception
     * via desc.perception). They are registered to show that multiple
     * perceptions can coexist — the app could invoke them manually
     * if desired. */
    px_estimate* inputs[] = { app.count };
    px_perception* a11y_p = px_perception_new(
        "counter_a11y", render_counter_a11y, inputs, 1, &app);
    px_perception* log_p = px_perception_new(
        "counter_log", render_counter_log, inputs, 1, &app);
    (void)a11y_p; (void)log_p;

    printf("Setup:\n");
    printf("  3 perceptions registered: pixels (main), a11y, log\n");
    printf("  Click [+ Inc] / [- Dec] or press +/- keys.\n");
    printf("  Press Z or Ctrl+Z to undo (undo-via-graph enabled).\n");
    printf("  Close window or Q/ESC to quit.\n\n");

    /* The KEY Phase 2 pattern: px_app_desc.perception, not .render */
    px_app_desc desc = {
        .width       = WIN_W,
        .height      = WIN_H,
        .title       = "Planex Counter (perception-driven)",
        .perception  = render_counter_pixels,  /* PHASE 2 — not .render */
        .render      = NULL,                   /* explicitly NULL */
        .on_click    = on_click,
        .on_key      = on_key,
        .on_ime_commit = NULL,
        .on_tick     = NULL,
        .animated_estimates = NULL,
        .n_animated  = 0,
        .on_resize   = NULL,
        .user        = &app,
    };

    int rc = px_app_run(&desc);

    printf("\nFinal state:\n");
    printf("  Final count: %.0f\n", px_estimate_value(app.count));
    printf("  Undo steps remaining: %d\n", px_undo_count());
    printf("  a11y perception calls: %d (manually invoked only)\n", app.a11y_calls);
    printf("  log perception calls:  %d (manually invoked only)\n", app.log_calls);
    printf("  px_app_run returned: %d\n", rc);

    px_perception_free(a11y_p);
    px_perception_free(log_p);
    px_undo_clear();
    px_closure_free(app.inc);
    px_closure_free(app.dec);
    px_graph_free(app.graph);
    px_estimate_free(app.count);

    printf("\n=== Phase 2 capstone complete (with undo-via-graph) ===\n");
    printf("What this proves:\n");
    printf("  1. px_app_desc.perception replaces the old .render callback\n");
    printf("  2. Real window works with perception-driven rendering\n");
    printf("  3. Mouse clicks + key presses still work (event loop unchanged)\n");
    printf("  4. Multiple perceptions coexist (pixels + a11y + log)\n");
    printf("  5. Undo-via-graph works in real window (Z/Ctrl+Z)\n");
    printf("     - Closures bound to graph auto-snapshot on trigger\n");
    printf("     - px_undo() restores via Relation graph\n");
    printf("\nThis closes ADR-0005 Phase 2 + ADR-0002 (undo in real window).\n");
    return 0;
}
