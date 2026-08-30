/*
 * app.h — Planex application helper (post-ADR-0005 Phase 2)
 *
 * Goal: factor out the common window event loop + perception
 * dispatch so widget demos only specify:
 *   - perception (drives rendering — replaces old render callback)
 *   - mouse-click callback (decides what to do given x, y)
 *   - key callback (decides what to do given a key char)
 *
 * Per ADR-0005 Phase 2: the `render` callback was replaced by
 * `perception`. The app loop calls px_perception_invoke_all()
 * each frame, and the perception function returns a px_fb*
 * which the loop blits to the window.
 *
 * NOT a new abstraction — just a convenience helper.
 */
#ifndef PLANEX_APP_H
#define PLANEX_APP_H

#include "planex/planex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int         width;
    int         height;
    const char* title;

    /* Phase 2: Perception function that returns a px_fb* to blit.
     * Called every frame. The returned fb is copied to the window.
     * Caller owns the returned fb (loop frees it after blit).
     *
     * This REPLACES the old `render` callback. Per ADR-0005 Phase 2,
     * rendering is now perception-driven, not callback-driven.
     *
     * If both `perception` and `render` are set, `perception` wins
     * (render is ignored). This preserves backward compatibility
     * with old demos that haven't migrated yet. */
    px_fb* (*perception)(void* user);

    /* DEPRECATED (Phase 2): use `perception` instead.
     * Called every frame to draw to fb. Kept for backward compat
     * with demos that haven't migrated to the perception API. */
    void  (*render)(px_fb* fb, void* user);

    /* Called on left-click. Return true if state changed (will re-render). */
    bool  (*on_click)(int x, int y, void* user);

    /* v0.3: Called on mouse move. Return true if state changed.
     * Fires on every mouse move event (60+ Hz when mouse is moving).
     * Use for hover, drag tracking, etc. */
    bool  (*on_mouse_move)(int x, int y, void* user);

    /* v0.3: Called on mouse button release. Return true if state changed.
     * Use for drag-end, drop, etc. */
    bool  (*on_mouse_up)(int x, int y, void* user);

    /* v0.6: Called on scroll wheel / trackpad scroll.
     * dy is in ticks: positive = down/away, negative = up/toward.
     * (x, y) is the cursor position at scroll time.
     * Return true if state changed (forces re-render). */
    bool  (*on_wheel)(int x, int y, int dy, void* user);

    /* Called on key press. Return true if state changed. */
    bool  (*on_key)(char key, void* user);

    /* Optional: called when IME commits UTF-8 text (Stage 9).
     * utf8_text is null-terminated UTF-8 (may be multi-byte for CJK).
     * Return true if state changed (forces re-render). */
    bool  (*on_ime_commit)(const char* utf8_text, void* user);

    /* Optional: called every frame (after render) to update app state.
     * Use this to advance animations, perform per-frame logic, etc.
     * dt_ms = milliseconds since last tick (useful for physics).
     * Return true if state changed (forces re-render next frame).
     * May be NULL — app loop will still render if any estimate is animating. */
    bool  (*on_tick)(double dt_ms, void* user);

    /* Optional: list of estimates that should force re-render when
     * animating. The app loop checks these and re-renders if any
     * is animating. May be NULL. */
    px_estimate** animated_estimates;
    int           n_animated;

    /* Optional: called when window is resized.
     * width/height are in LOGICAL pixels (CSS-like units).
     * Use px_window_scale() to get DPI scale if needed.
     * After this returns, render() will be called once. */
    void  (*on_resize)(int width, int height, void* user);

    /* v0.7 (Line 1): opt-in intent compilation for pointer-downs.
     * When set, each PX_EV_MOUSE_DOWN is compiled BEFORE dispatch:
     * the topmost region at (x, y) is resolved against this graph's
     * AFFORDS edges, and the afforded closure is triggered with a
     * px_pointer_intent payload (see planex.h) — the app's closure
     * receives WHERE-in-semantics (region label), not WHERE-in-
     * pixels as a routing key. Clicks that resolve to no afforded
     * closure fall back to on_click unchanged. NULL (the default)
     * keeps raw-coordinate dispatch exactly as before — the path is
     * opt-in per ADR-0016's evidence-gathering posture. */
    px_graph*   intent_graph;

    /* v0.8 (Line 1): optional focus-change notification for the
     * keyboard channel. When intent_graph is set, Tab/Shift-Tab
     * move the focus through the DERIVED focus ring (regions that
     * afford closures, in creation order — see px_afford_focus_*
     * in planex.h); each move calls this with the newly focused
     * region's LABEL (a value: the region may be freed later).
     * The framework never draws a focus indicator — the app
     * renders one from this callback, or ignores focus entirely. */
    void  (*on_focus)(const char* region_label, void* user);

    void*       user;
} px_app_desc;

/* Run an app descriptor. Returns 0 on clean exit. */
int px_app_run(const px_app_desc* desc);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_APP_H */
