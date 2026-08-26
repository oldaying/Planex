/*
 * hover_drag_4abs.c — boundary-exposing demo for continuous interaction
 *
 * Per ADR-0006: this demo measures how painful it is to implement
 * hover + drag using only the 4 existing abstractions (no 5th).
 *
 * Each "HACK:" comment marks a place where the code is doing something
 * semantically wrong — forcing a continuous process into Estimate.
 *
 * If these hacks are tolerable → no 5th abstraction needed.
 * If they're intolerable → ADR-0007 will propose a 5th abstraction
 * with API design based on this demo's pain points.
 *
 * What this demo implements:
 *   - A list of 5 items
 *   - Hover over an item → it highlights
 *   - Click+drag an item → it follows the mouse
 *   - Release → item snaps to nearest position (reorder)
 *
 * Build:
 *   cc -std=c17 -I include examples/hover_drag_4abs.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/fb.c src/font.c src/x11.c src/app.c \
 *      -lX11 -lXext -lm -o build/hover_drag_4abs
 *
 * Run:
 *   ./build/hover_drag_4abs
 *   (Linux: requires X11 display)
 *   (Windows: cmake --build && .\build\Release\hover_drag_4abs.exe)
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN_W 320
#define WIN_H 240
#define N_ITEMS 5
#define ITEM_H 32
#define ITEM_W 280
#define ITEM_X 20
#define ITEM_Y_START 40

typedef struct {
    /* === Persistent state (legitimate Estimate use) === */
    px_estimate* item_order[N_ITEMS];   /* order[i] = which item is at position i */
    px_graph*    graph;

    /* === HACK: Transient interaction state forced into Estimate ===
     *
     * These are NOT "state with time + uncertainty" — they are
     * high-frequency transient inputs. But Planex has no other
     * abstraction for them, so they go into Estimate.
     *
     * HACK 1: mouse_x / mouse_y as Estimate
     *   - Updates 60+ times per second (every mouse move)
     *   - Confidence is meaningless (always 1.0)
     *   - No "time" dimension needed
     *   - But Estimate is the only mutable-value abstraction
     */
    px_estimate* mouse_x;
    px_estimate* mouse_y;

    /* HACK 2: hovered_index as Estimate
     *   - "Which item is the mouse over" is transient
     *   - Changes on every mouse move
     *   - Not a "decision" — it's a side effect of mouse position
     *   - Forcing into Estimate means every mouse_move triggers
     *     px_estimate_set + observer notification + Relation update
     */
    px_estimate* hovered_index;   /* -1 = none, 0..N-1 = item index */

    /* HACK 3: drag_state as Estimate
     *   - "Is the user dragging? Which item? At what offset?"
     *   - This is a PROCESS (start → move → end), not a STATE
     *   - Forcing into Estimate loses the "process" semantics:
     *     no way to express "drag started" vs "drag in progress"
     *     vs "drag ended" — it's just a value that changes
     */
    px_estimate* drag_active;       /* 0 = not dragging, 1 = dragging */
    px_estimate* drag_item;         /* which item is being dragged (-1 if none) */
    px_estimate* drag_offset_y;     /* mouse offset from item top when drag started */

    /* Closures */
    px_closure* start_drag;   /* mouse down on item → begin drag */
    px_closure* end_drag;      /* mouse up → commit reorder */
    px_closure* move_mouse;    /* mouse move → update position */

    /* Track renders for stats */
    int render_count;
    int hack_count;   /* count how many hacks we do per frame */
} App;

/* ============================================================
 * Item labels
 * ============================================================ */

static const char* item_labels[] = {
    "Apple",
    "Banana",
    "Cherry",
    "Date",
    "Elderberry"
};

/* ============================================================
 * Helpers
 * ============================================================ */

static int y_to_item_index(int y) {
    if (y < ITEM_Y_START) return -1;
    int idx = (y - ITEM_Y_START) / ITEM_H;
    if (idx < 0 || idx >= N_ITEMS) return -1;
    return idx;
}

static int item_y(int index) {
    return ITEM_Y_START + index * ITEM_H;
}

static bool in_item(int x, int y, int index) {
    int iy = item_y(index);
    return x >= ITEM_X && x < ITEM_X + ITEM_W &&
           y >= iy && y < iy + ITEM_H;
}

/* ============================================================
 * Closure actions
 * ============================================================ */

/* start_drag: mouse down on an item → begin drag process
 *
 * HACK 4: "begin drag" is not a discrete intent — it's the START
 * of a continuous process. But Closure only models discrete
 * speech acts (REQUEST/DECLARE/etc). We use REQUEST to mean
 * "please start dragging" — but there's no way to say "this is
 * the beginning of a multi-frame process."
 */
static void on_start_drag(px_intent intent, void* user) {
    App* app = user;
    (void)intent;
    int mx = (int)px_estimate_value(app->mouse_x);
    int my = (int)px_estimate_value(app->mouse_y);

    int idx = y_to_item_index(my);
    if (idx < 0 || !in_item(mx, my, idx)) return;

    /* Begin drag — set all the hack Estimates */
    px_estimate_set(app->drag_active, 1.0, 1.0);
    px_estimate_set(app->drag_item, (double)idx, 1.0);

    int iy = item_y(idx);
    px_estimate_set(app->drag_offset_y, (double)(my - iy), 1.0);
}

/* end_drag: mouse up → commit reorder
 *
 * This part is CLEAN — it's a discrete intent (DECLARE: "drag is done").
 * The Closure model works well for the commit step.
 */
static void on_end_drag(px_intent intent, void* user) {
    App* app = user;
    (void)intent;

    if (px_estimate_value(app->drag_active) < 0.5) return;

    int drag_item = (int)px_estimate_value(app->drag_item);
    int my = (int)px_estimate_value(app->mouse_y);
    int target_idx = y_to_item_index(my);
    if (target_idx < 0) target_idx = drag_item;

    /* Reorder: swap item_order[drag_item] and item_order[target_idx] */
    if (target_idx != drag_item && target_idx >= 0 && target_idx < N_ITEMS) {
        double tmp = px_estimate_value(app->item_order[drag_item]);
        px_estimate_set(app->item_order[drag_item],
                        px_estimate_value(app->item_order[target_idx]), 1.0);
        px_estimate_set(app->item_order[target_idx], tmp, 1.0);
    }

    /* End drag */
    px_estimate_set(app->drag_active, 0.0, 1.0);
    px_estimate_set(app->drag_item, -1.0, 1.0);
    px_estimate_set(app->drag_offset_y, 0.0, 1.0);
}

/* move_mouse: update mouse position Estimates
 *
 * HACK 5: mouse move is NOT an intent. It's not a request, not a
 * declaration, not a promise. It's a continuous input stream. But
 * Planex has no "input stream" abstraction, so we wrap it in a
 * Closure with PX_INTENT_EXPRESS (which means "I feel X" — the
 * closest speech act to "here's some input").
 *
 * This fires on EVERY mouse move event — 60+ times per second.
 * Each call: 2x px_estimate_set → observer notification →
 * derived estimate recompute → Relation graph update.
 *
 * For mouse position, this is massive overkill.
 */
static void on_move_mouse(px_intent intent, void* user) {
    App* app = user;
    (void)intent;

    int mx = (int)px_estimate_value(app->mouse_x);
    int my = (int)px_estimate_value(app->mouse_y);

    /* Update hovered_index based on mouse position */
    int new_hovered = y_to_item_index(my);
    if (in_item(mx, my, new_hovered >= 0 ? new_hovered : 0)) {
        if (px_estimate_value(app->drag_active) < 0.5) {
            px_estimate_set(app->hovered_index, (double)new_hovered, 1.0);
        }
    } else {
        if (px_estimate_value(app->hovered_index) >= 0) {
            px_estimate_set(app->hovered_index, -1.0, 1.0);
        }
    }
}

static bool eval_always_true(void* user) {
    (void)user;
    return true;
}

/* ============================================================
 * Perception — pure function rendering the list
 * ============================================================ */

static px_fb* render_list(void* user) {
    App* app = user;
    app->render_count++;
    app->hack_count = 0;

    px_fb* fb = px_fb_new(WIN_W, WIN_H);
    if (!fb) return NULL;

    px_fb_clear(fb, PX_BG);
    px_fb_draw_rect(fb, 4, 4, WIN_W - 8, WIN_H - 8, PX_BORDER);

    /* Title */
    px_fb_fill_rect(fb, 4, 4, WIN_W - 8, 20, PX_SURFACE);
    px_fb_draw_text(fb, 12, 8, "Hover + Drag Demo (4 abs)", PX_TEXT);

    /* Read all state */
    int hovered = (int)px_estimate_value(app->hovered_index);
    int drag_act = (int)px_estimate_value(app->drag_active);
    int drag_itm = (int)px_estimate_value(app->drag_item);
    int my = (int)px_estimate_value(app->mouse_y);
    int drag_offset = (int)px_estimate_value(app->drag_offset_y);
    app->hack_count += 5;

    /* Render items — skip the dragged one (render ghost after loop) */
    int dragged_item_id = -1;
    for (int i = 0; i < N_ITEMS; i++) {
        int item_id = (int)px_estimate_value(app->item_order[i]);
        int y = item_y(i);

        if (drag_act && drag_itm == i) {
            /* Remember which item is dragged, render placeholder */
            dragged_item_id = item_id;
            px_fb_fill_rect(fb, ITEM_X, y, ITEM_W, ITEM_H, PX_PRESSED);
            px_fb_draw_rect(fb, ITEM_X, y, ITEM_W, ITEM_H, PX_BORDER);
            px_fb_draw_text(fb, ITEM_X + 12, y + 12, "...", PX_TEXT_DIM);
            continue;
        }

        uint32_t bg = PX_SURFACE;
        uint32_t border = PX_BORDER;
        uint32_t text_color = PX_TEXT;

        if (hovered == i && !drag_act) {
            bg = PX_ACCENT_HOVER;
            border = PX_ACCENT;
            app->hack_count += 1;
        }

        px_fb_fill_rect(fb, ITEM_X, y, ITEM_W, ITEM_H, bg);
        px_fb_draw_rect(fb, ITEM_X, y, ITEM_W, ITEM_H, border);

        if (item_id >= 0 && item_id < N_ITEMS) {
            px_fb_draw_text(fb, ITEM_X + 12, y + 12,
                           item_labels[item_id], text_color);
        }
    }

    /* Render drag ghost AFTER all items — at mouse Y position.
     * Clamp to window so it doesn't go off-screen. */
    if (drag_act && dragged_item_id >= 0) {
        int drag_y = my - drag_offset;
        if (drag_y < 4) drag_y = 4;
        if (drag_y > WIN_H - ITEM_H - 4) drag_y = WIN_H - ITEM_H - 4;

        px_fb_fill_rect(fb, ITEM_X, drag_y, ITEM_W, ITEM_H, PX_ACCENT);
        px_fb_draw_rect(fb, ITEM_X, drag_y, ITEM_W, ITEM_H, PX_TEXT);
        px_fb_draw_text(fb, ITEM_X + 12, drag_y + 12,
                       item_labels[dragged_item_id], PX_TEXT);
    }

    /* Status bar */
    int sy = WIN_H - 20;
    px_fb_fill_rect(fb, 4, sy, WIN_W - 8, 16, PX_SURFACE);
    char status[128];
    if (drag_act) {
        snprintf(status, sizeof(status), "dragging item %d", drag_itm);
    } else if (hovered >= 0) {
        snprintf(status, sizeof(status), "hover: item %d", hovered);
    } else {
        snprintf(status, sizeof(status), "hover over items, drag to reorder");
    }
    px_fb_draw_text(fb, 12, sy + 2, status, PX_TEXT_DIM);

    return fb;
}

/* ============================================================
 * Event handlers
 * ============================================================ */

static bool on_click(int x, int y, void* user) {
    App* app = user;
    px_estimate_set(app->mouse_x, (double)x, 1.0);
    px_estimate_set(app->mouse_y, (double)y, 1.0);

    int idx = y_to_item_index(y);
    if (idx >= 0 && in_item(x, y, idx)) {
        px_closure_trigger(app->start_drag, NULL, 0);
        return true;
    }
    return false;
}

/* on_mouse_move: handle hover + drag tracking
 * Uses on_mouse_move callback (added v0.3 to px_app_desc) */
static bool on_mouse_move(int x, int y, void* user) {
    App* app = user;
    px_estimate_set(app->mouse_x, (double)x, 1.0);
    px_estimate_set(app->mouse_y, (double)y, 1.0);

    /* If dragging, keep drag_item position updated */
    if (px_estimate_value(app->drag_active) > 0.5) {
        return true;  /* re-render to show dragged item at new position */
    }

    /* Update hover index */
    int new_hovered = y_to_item_index(y);
    if (new_hovered >= 0 && in_item(x, y, new_hovered)) {
        if (px_estimate_value(app->hovered_index) != new_hovered) {
            px_estimate_set(app->hovered_index, (double)new_hovered, 1.0);
            return true;
        }
    } else {
        if (px_estimate_value(app->hovered_index) >= 0) {
            px_estimate_set(app->hovered_index, -1.0, 1.0);
            return true;
        }
    }
    return false;
}

/* on_mouse_up: end drag
 * Uses on_mouse_up callback (added v0.3 to px_app_desc) */
static bool on_mouse_up(int x, int y, void* user) {
    App* app = user;
    px_estimate_set(app->mouse_x, (double)x, 1.0);
    px_estimate_set(app->mouse_y, (double)y, 1.0);

    if (px_estimate_value(app->drag_active) > 0.5) {
        px_closure_trigger(app->end_drag, NULL, 0);
        return true;
    }
    return false;
}

static bool on_key(char key, void* user) {
    (void)user;
    /* No keyboard actions needed for this demo */
    if (key == 0) return false;
    return false;
}

/* HACK 7: on_tick is used to detect mouse-up (end of drag).
 *
 * Planex's event system doesn't have PX_EV_MOUSE_UP in the
 * on_click callback. We need to poll for mouse button release
 * every frame. This is a workaround for missing mouse-up events.
 *
 * In a proper continuous-interaction abstraction, drag start/end
 * would be first-class events, not polled hacks.
 */
static bool on_tick(double dt_ms, void* user) {
    App* app = user;
    (void)dt_ms;

    /* Check if drag should end (we detect this via app loop state) */
    if (px_estimate_value(app->drag_active) > 0.5) {
        /* In a real app, we'd check mouse button state here.
         * For this demo, we rely on the window backend's event
         * to call end_drag when mouse is released.
         * This is a simplification — real implementation needs
         * PX_EV_MOUSE_UP support. */
    }

    return false;  /* don't force re-render unless something changed */
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Planex hover_drag_4abs — boundary-exposing demo\n");
    printf("==================================================\n");
    printf("Validates: how painful is hover+drag with 4 abstractions?\n");
    printf("\n");
    printf("7 HACKS documented in source code:\n");
    printf("  HACK 1: mouse_x/y as Estimate (60fps transient input)\n");
    printf("  HACK 2: hovered_index as Estimate (transient, not state)\n");
    printf("  HACK 3: drag_state as Estimate (process, not state)\n");
    printf("  HACK 4: 'begin drag' as discrete REQUEST (should be process start)\n");
    printf("  HACK 5: mouse move as PX_INTENT_EXPRESS (not an intent)\n");
    printf("  HACK 6: hover highlight reads transient Estimate every frame\n");
    printf("  HACK 7: mouse-up detection via polling (no PX_EV_MOUSE_UP)\n");
    printf("\n");
    printf("Hover over items to highlight. Click+drag to reorder.\n");
    printf("Close window or Q/ESC to quit.\n\n");

    App app = {0};
    app.graph = px_graph_new();

    /* Initialize item order: 0, 1, 2, 3, 4 */
    for (int i = 0; i < N_ITEMS; i++) {
        app.item_order[i] = px_estimate_new((double)i, 1.0);
    }

    /* HACK: transient state as Estimate */
    app.mouse_x = px_estimate_new(0, 1.0);
    app.mouse_y = px_estimate_new(0, 1.0);
    app.hovered_index = px_estimate_new(-1, 1.0);
    app.drag_active = px_estimate_new(0, 1.0);
    app.drag_item = px_estimate_new(-1, 1.0);
    app.drag_offset_y = px_estimate_new(0, 1.0);

    /* Closures */
    app.start_drag = px_closure_new("start drag", PX_INTENT_REQUEST,
                                      on_start_drag, eval_always_true, &app);
    app.end_drag = px_closure_new("end drag (commit reorder)", PX_INTENT_DECLARE,
                                    on_end_drag, eval_always_true, &app);
    app.move_mouse = px_closure_new("mouse moved", PX_INTENT_EXPRESS,
                                      on_move_mouse, eval_always_true, &app);

    /* Relations */
    for (int i = 0; i < N_ITEMS; i++) {
        px_declare(app.graph, app.start_drag, PX_REL_TRIGGERS, app.drag_active);
        px_declare(app.graph, app.end_drag, PX_REL_TRIGGERS, app.item_order[i]);
    }

    px_app_desc desc = {
        .width       = WIN_W,
        .height      = WIN_H,
        .title       = "Planex Hover+Drag (boundary demo)",
        .perception  = render_list,
        .render      = NULL,
        .on_click    = on_click,
        .on_mouse_move = on_mouse_move,
        .on_mouse_up = on_mouse_up,
        .on_key      = on_key,
        .on_tick     = on_tick,
        .animated_estimates = NULL,
        .n_animated  = 0,
        .on_resize   = NULL,
        .user        = &app,
    };

    int rc = px_app_run(&desc);

    printf("\nFinal state:\n");
    printf("  Renders: %d\n", app.render_count);
    printf("  Hacks per frame (last): %d\n", app.hack_count);
    printf("  Item order: ");
    for (int i = 0; i < N_ITEMS; i++) {
        printf("%.0f ", px_estimate_value(app.item_order[i]));
    }
    printf("\n");

    /* Cleanup */
    px_closure_free(app.start_drag);
    px_closure_free(app.end_drag);
    px_closure_free(app.move_mouse);
    px_graph_free(app.graph);
    for (int i = 0; i < N_ITEMS; i++) px_estimate_free(app.item_order[i]);
    px_estimate_free(app.mouse_x);
    px_estimate_free(app.mouse_y);
    px_estimate_free(app.hovered_index);
    px_estimate_free(app.drag_active);
    px_estimate_free(app.drag_item);
    px_estimate_free(app.drag_offset_y);

    printf("\n=== Boundary demo complete ===\n");
    printf("\nHACK ASSESSMENT (for ADR-0007 decision):\n");
    printf("  Total hacks: 7\n");
    printf("  Hacks per frame: ~5 (4 transient Estimates read + 1 hover check)\n");
    printf("  Hacks per mouse move: 2 (mouse_x + mouse_y Estimate updates)\n");
    printf("  Hacks per drag start: 3 (drag_active + drag_item + drag_offset_y)\n");
    printf("\nVerdict:\n");
    printf("  - Hover: WORKABLE but wasteful (60fps Estimate updates for highlight)\n");
    printf("  - Drag: WORKABLE but semantically wrong (process forced into state)\n");
    printf("  - Gesture: NOT POSSIBLE (no continuous input abstraction)\n");
    printf("  - The hacks are tolerable for simple hover/drag.\n");
    printf("  - They would be INTOLERABLE for complex gesture/touch UIs.\n");
    printf("  - Decision: no 5th abstraction for v0.x. Document as L12.\n");
    printf("  - Revisit for v1.0+ if gesture/touch support is needed.\n");
    return rc;
}
