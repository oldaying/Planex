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
    printf("Click or press keys. q/ESC to quit.\n\n");

    px_window_show(win);

    px_fb* fb = px_window_fb(win);

    /* Initial render */
    app_render_frame(desc, win, fb);
    px_window_present(win);

    bool running = true;
    double last_tick = px_now_ms();
    int last_w = desc->width, last_h = desc->height;

    while (running && !px_window_should_close(win)) {
        /* Drain all pending events (non-blocking) */
        bool event_changed = false;
        while (true) {
            px_event ev = px_window_poll_event(win);
            if (ev.kind == PX_EV_NONE) break;

            switch (ev.kind) {
                case PX_EV_MOUSE_DOWN:
                    if (desc->on_click) {
                        if (desc->on_click(ev.x, ev.y, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_MOUSE_MOVE:
                    if (desc->on_mouse_move) {
                        if (desc->on_mouse_move(ev.x, ev.y, desc->user)) {
                            event_changed = true;
                        }
                    }
                    break;
                case PX_EV_MOUSE_UP:
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
