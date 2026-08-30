/*
 * app.c — event loop + perception/render dispatch (post-ADR-0005 Phase 2)
 *
 * Encapsulates the boilerplate:
 *   - Create window
 *   - Show + initial render
 *   - Event loop: dispatch mouse/key/close to user callbacks
 *   - 60fps render tick (when animations are active or on_tick returns true)
 *   - Re-render after state changes
 *
 * Phase 2: if desc->perception is set, use it instead of desc->render.
 * The perception function returns a fresh px_fb* each frame which
 * the loop blits to the window. This is the ADR-0005 Phase 2
 * "perception-driven rendering" path.
 *
 * v0.7 (Line 1): when desc->intent_graph is set, pointer-downs are
 * COMPILED before dispatch — px_afford_compile resolves the topmost
 * region against the graph's AFFORDS edges and the afforded closure
 * triggers with a px_pointer_intent payload. Raw-coordinate dispatch
 * (on_click) remains as the fallback for unresolved clicks.
 *
 * v0.8 (Line 1): the SAME graph serves the keyboard channel. When
 * intent_graph is set, Tab/Shift-Tab walk the derived focus ring
 * (px_afford_focus_next/prev; on_focus reports each move) and
 * Enter/Space compile the focused region's afforded closure with a
 * px_key_intent payload (px_afford_compile_focus) — same dispatch
 * path as a pointer-down, same last-declared-first resolution. Keys
 * that compile to nothing fall back to on_key unchanged.
 *
 * v0.8 (Line 2): the process form — drag-begin afford. A down on a
 * region that affords a PROCESS (an AFFORDS edge targeting a
 * px_interaction) compiles to that process instead of a closure:
 * reset + begin + the press becomes the first trajectory sample.
 * While the compiled process is active, moves SAMPLE it (the inert
 * hot path — preview is derived per frame from the trajectory, the
 * palette_afford pattern) and the release COMMITS it; on_mouse_move
 * / on_mouse_up do not fire (the process owns the gesture). An
 * app-side cancel (its hook, its on_key) is honored: a move/up that
 * finds the process terminal clears it and falls through to normal
 * routing. A new press while a process is active cancels it
 * ("superseded by a new press") and routes normally. Regions
 * affording only closures keep the v0.7 immediate-trigger semantics,
 * byte-for-byte — the process form never runs for them.
 *
 * Backward compat: if only desc->render is set (old API), use it.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include <stdio.h>
#include <string.h>

/* Target 60fps = ~16.67ms per frame */
#define FRAME_INTERVAL_MS 16

/* px_sleep_ms is defined in planex.h (static inline) */

/* Adapter so we can hand a window-event callback down to the backend,
 * but still call the user's px_app_desc.on_resize with just (w,h,user). */
static void app_resize_adapter(px_window* w, int width, int height, void* user) {
    (void)w;
    const px_app_desc* desc = (const px_app_desc*)user;
    if (desc && desc->on_resize) {
        desc->on_resize(width, height, desc->user);
    }
}

/* Render one frame: use perception if set, else legacy render callback.
 * Copies perception fb pixels to window fb, then frees perception fb.
 *
 * v0.6 fast path: when both framebuffers have the same width, the copy
 * is a per-row memcpy instead of a per-pixel function call (the v0.5
 * path made feedback latency scale with W*H function calls — the
 * "feedback has no time budget" finding). The per-pixel fallback is
 * kept for the mismatched-width case (clipped copy). */
static void app_render_frame(const px_app_desc* desc, px_window* win, px_fb* fb) {
    (void)win;  /* window handle not used — fb is already the window's fb */
    if (desc->perception) {
        px_fb* pfb = desc->perception(desc->user);
        if (pfb) {
            int W = px_fb_width(pfb);
            int H = px_fb_height(pfb);
            int winW = px_fb_width(fb);
            int winH = px_fb_height(fb);
            int copyW = (W < winW) ? W : winW;
            int copyH = (H < winH) ? H : winH;
            const uint32_t* src = px_fb_pixels(pfb);
            if (W == winW) {
                /* Fast path: contiguous rows — memcpy per row. */
                uint32_t* dst = px_fb_pixels_mutable(fb);
                size_t row_bytes = (size_t)copyW * sizeof(uint32_t);
                for (int y = 0; y < copyH; y++) {
                    memcpy(dst + (size_t)y * W, src + (size_t)y * W, row_bytes);
                }
            } else {
                /* Clipped path: widths differ — per-pixel copy. */
                for (int y = 0; y < copyH; y++) {
                    for (int x = 0; x < copyW; x++) {
                        px_fb_set_pixel(fb, x, y, src[y * W + x]);
                    }
                }
            }
            px_fb_free(pfb);
        }
    } else if (desc->render) {
        desc->render(fb, desc->user);
    }
}

int px_app_run(const px_app_desc* desc) {
    if (!desc) return 1;
    if (!desc->perception && !desc->render) return 1;

    px_window* win = px_window_new(desc->width, desc->height, desc->title);
    if (!win) {
        fprintf(stderr, "Planex: cannot create window (set DISPLAY)\n");
        return 1;
    }

    /* Hook resize callback */
    if (desc->on_resize) {
        px_window_on_resize(win, app_resize_adapter, (void*)desc);
    }

    printf("Planex app: %s\n", desc->title ? desc->title : "(untitled)");
    printf("Backend: %s  %dx%d (resizable)  scale=%.2f\n",
           px_window_backend_name(), desc->width, desc->height,
           px_window_scale(win));
    printf("Click or press keys. Tab moves focus. q/ESC to quit.\n\n");

    px_window_show(win);

    px_fb* fb = px_window_fb(win);

    /* Initial render */
    app_render_frame(desc, win, fb);
    px_window_present(win);

    bool running = true;
    double last_tick = px_now_ms();
    int last_w = desc->width, last_h = desc->height;

    /* v0.8 (Line 1): the keyboard channel's focus state. NULL =
     * focus nowhere (the Tab-from-nowhere start); px_afford_focus_next
     * normalizes it onto the ring. Loop-local by design: focus is
     * interaction state scoped to this run, not registry state. */
    px_region* focus = NULL;

    /* v0.8 (Line 2): the active compiled process, if a drag is
     * underway. Loop-local for the same reason as focus. NULL when
     * no gesture owns the pointer stream. */
    px_interaction* active = NULL;

    while (running && !px_window_should_close(win)) {
        /* Drain all pending events (non-blocking) */
        bool event_changed = false;
        while (true) {
            px_event ev = px_window_poll_event(win);
            if (ev.kind == PX_EV_NONE) break;

            switch (ev.kind) {
                case PX_EV_MOUSE_DOWN:
                    /* v0.8 (Line 2): the process form FIRST. A region
                     * affording a process owns the down: the press is
                     * genuinely ambiguous (tap vs drag), and only the
                     * trajectory resolves it — the tap is a
                     * small-displacement COMMIT, the process's own
                     * bridges reach the discrete act. Dual-form
                     * regions therefore begin the process here; the
                     * closure form below never runs for them. */
                    if (desc->intent_graph) {
                        px_drag_intent di;
                        px_interaction* proc = px_afford_compile_process(
                            desc->intent_graph, (double)ev.x, (double)ev.y,
                            ev.button, &di);
                        if (proc) {
                            /* A press during an active process ends it:
                             * one pointer, one gesture at a time. The
                             * cancel fires the app's bridges first so
                             * it can settle state before the new
                             * gesture begins. */
                            if (active) {
                                px_interaction_cancel(
                                    active, "superseded by a new press");
                                active = NULL;
                            }
                            /* Rearm (terminal outcomes are final — the
                             * stable edge target must survive its
                             * second drag), then begin + the press as
                             * the first trajectory sample. The compile
                             * product seeds the sample: the position
                             * and button are payload context, never
                             * routing keys. */
                            px_interaction_reset(proc);
                            px_interaction_begin(proc);
                            px_int_sample press = {
                                px_now_ms(), di.x, di.y, 0.0,
                                di.button, ev.modifiers
                            };
                            px_interaction_sample(proc, &press);
                            active = proc;
                            event_changed = true;
                            break;
                        }
                    }
                    /* v0.7 (Line 1): intent compilation — the closure
                     * form. When the app opts in via intent_graph,
                     * the click resolves against the region registry
                     * + AFFORDS edges, and the afforded closure
                     * triggers with a semantic payload. Unresolved
                     * clicks (empty space, regions affording nothing)
                     * fall through to the raw-coordinate callback —
                     * the fallback, not the path. */
                    if (desc->intent_graph) {
                        px_pointer_intent pi;
                        px_closure* afforded = px_afford_compile(
                            desc->intent_graph, (double)ev.x, (double)ev.y,
                            ev.button, &pi);
                        if (afforded) {
                            px_closure_trigger(afforded, &pi, sizeof(pi));
                            event_changed = true;
                            break;
                        }
                    }
                    if (desc->on_click) {
                        if (desc->on_click(ev.x, ev.y, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_MOUSE_MOVE:
                    /* v0.8 (Line 2): an active compiled process owns
                     * the move stream — samples are inert (no observer
                     * fan-out); the app derives preview per frame from
                     * the trajectory, not per event from a callback. A
                     * process the app itself cancelled is terminal:
                     * drop it and fall through to normal routing (the
                     * gesture ended without an up). */
                    if (active) {
                        px_int_phase ph = px_interaction_phase(active);
                        if (ph == PX_INT_COMMITTED ||
                            ph == PX_INT_CANCELLED) {
                            active = NULL;
                        } else {
                            px_int_sample move = {
                                px_now_ms(), (double)ev.x, (double)ev.y,
                                0.0, 0, ev.modifiers
                            };
                            px_interaction_sample(active, &move);
                            event_changed = true;
                            break;
                        }
                    }
                    if (desc->on_mouse_move) {
                        if (desc->on_mouse_move(ev.x, ev.y, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_MOUSE_UP:
                    /* v0.8 (Line 2): the release resolves the active
                     * process — the release position is the last
                     * sample, COMMIT fires the app's bridges (the
                     * hook decides tap-vs-drag by measure). If the
                     * app already cancelled it, just drop it. Either
                     * way the pointer stream is released. */
                    if (active) {
                        px_int_phase ph = px_interaction_phase(active);
                        if (ph != PX_INT_COMMITTED &&
                            ph != PX_INT_CANCELLED) {
                            px_int_sample release = {
                                px_now_ms(), (double)ev.x, (double)ev.y,
                                0.0, ev.button, ev.modifiers
                            };
                            px_interaction_sample(active, &release);
                            px_interaction_commit(active);
                        }
                        active = NULL;
                        event_changed = true;
                        break;
                    }
                    if (desc->on_mouse_up) {
                        if (desc->on_mouse_up(ev.x, ev.y, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_WHEEL:
                    /* v0.6: scroll wheel — a continuous channel, distinct
                     * from discrete clicks. Feed interactions or estimate
                     * scroll state from the dy ticks. */
                    if (desc->on_wheel) {
                        if (desc->on_wheel(ev.x, ev.y, ev.wheel_dy,
                                           desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_KEY_DOWN:
                    if (ev.key_char == 'q' || ev.key_char == 'Q' || ev.key_char == 27) {
                        running = false;
                        continue;
                    }
                    /* v0.8 (Line 1): the keyboard channel — compile
                     * BEFORE dispatch, same order as pointer-downs.
                     * Tab/Shift-Tab move the derived focus ring;
                     * Enter/Space compile the focused region's
                     * afforded closure with a px_key_intent payload.
                     * Keys that compile to nothing (including a
                     * focus ring with nothing on it) fall through
                     * to on_key — the fallback, not the path. */
                    if (desc->intent_graph) {
                        if (ev.key_char == '\t') {
                            px_region* next = (ev.modifiers & PX_MOD_SHIFT)
                                ? px_afford_focus_prev(desc->intent_graph, focus)
                                : px_afford_focus_next(desc->intent_graph, focus);
                            if (next && next != focus) {
                                focus = next;
                                if (desc->on_focus) {
                                    desc->on_focus(px_region_label(focus),
                                                   desc->user);
                                }
                                event_changed = true;
                                break;
                            }
                            if (next && next == focus && desc->on_focus) {
                                /* Ring of one: focus unchanged, but the
                                 * app still hears where it is. */
                                desc->on_focus(px_region_label(focus),
                                               desc->user);
                                break;
                            }
                            /* Empty ring: fall through to on_key. */
                        } else if (ev.key_char == '\r' || ev.key_char == '\n' ||
                                   ev.key_char == ' ') {
                            px_key_intent ki;
                            px_closure* afforded = px_afford_compile_focus(
                                desc->intent_graph, focus, ev.key_char, &ki);
                            if (afforded) {
                                px_closure_trigger(afforded, &ki, sizeof(ki));
                                event_changed = true;
                                break;
                            }
                            /* No focus / focus affords nothing: fall
                             * through to on_key. */
                        }
                    }
                    if (desc->on_key) {
                        if (desc->on_key(ev.key_char, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_IME_COMMIT:
                    /* Stage 9: IME committed UTF-8 text */
                    if (desc->on_ime_commit) {
                        if (desc->on_ime_commit(ev.ime_text, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_IME_COMPOSE:
                    /* Preedit intermediate — Stage 9 doesn't render preedit yet */
                    break;
                case PX_EV_CLOSE:
                    running = false;
                    break;
                default:
                    break;
            }
        }

        /* Check for resize (poll_event handles ConfigureNotify internally;
         * we detect it by comparing window size) */
        int cur_w = px_window_width(win);
        int cur_h = px_window_height(win);
        bool resized = (cur_w != last_w || cur_h != last_h);
        if (resized) {
            last_w = cur_w;
            last_h = cur_h;
            /* px_window_poll_event already called desc->on_resize via
             * the backend's resize callback chain. We just need to
             * re-render at the new size. */
            event_changed = true;
        }

        /* Time-based tick */
        double now = px_now_ms();
        double dt = now - last_tick;
        bool tick_changed = false;
        if (dt >= FRAME_INTERVAL_MS) {
            last_tick = now;
            if (desc->on_tick) {
                tick_changed = desc->on_tick(dt, desc->user);
            }
        }

        /* Check if any registered estimate is animating */
        bool animating = false;
        if (desc->animated_estimates && desc->n_animated > 0) {
            for (int i = 0; i < desc->n_animated; i++) {
                if (px_estimate_is_animating(desc->animated_estimates[i])) {
                    animating = true;
                    break;
                }
            }
        }

        /* Re-render if anything changed OR animations are active */
        if (event_changed || tick_changed || animating) {
            /* fb may have been resized inside poll_event — re-read it */
            fb = px_window_fb(win);
            app_render_frame(desc, win, fb);
            px_window_present(win);
        } else {
            /* Sleep to avoid busy-loop */
            px_sleep_ms(5);
        }
    }

    px_window_free(win);
    return 0;
}
