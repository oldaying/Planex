/*
 * headless.c — Headless backend for Planex Stage 7
 *
 * Always available. No real window. px_window_present() saves BMP
 * to disk each frame (configurable via env var PLANEX_HEADLESS_BMP).
 *
 * Use cases:
 *   - CI testing (no X server needed)
 *   - Screenshot generation
 *   - Architecture portability validation
 *
 * px_window_poll_event() reads stdin commands:
 *   - 'q' → close event
 *   - 'c <x> <y>' → mouse click at (x,y)
 *   - 'k <char>' → key press
 *
 * Or, if stdin is not a TTY, fires a auto-quit after PLANEX_HEADLESS_FRAMES
 * frames (default 60 = 1 second at 60fps).
 *
 * This is a stub for portability — not for production use.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/window.h"
#include "planex/fb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#else
#include <windows.h>
#include <io.h>
#include <conio.h>
#endif

struct px_window {
    int          width;
    int          height;
    px_fb*       fb;
    bool         should_close;
    bool         visible;
    int          frames_rendered;
    int          max_frames;
    px_resize_callback on_resize_cb;
    void*              on_resize_user;
    /* Queued event from stdin */
    px_event     queued;
    bool         has_queued;
};

const char* px_window_backend_name(void) {
    return "headless";
}

px_window* px_window_new(int width, int height, const char* title) {
    if (width <= 0 || height <= 0) return NULL;
    (void)title;

    px_window* w = (px_window*)calloc(1, sizeof(px_window));
    if (!w) return NULL;
    w->width  = width;
    w->height = height;
    w->fb     = px_fb_new(width, height);
    w->should_close = false;
    w->visible = false;
    w->frames_rendered = 0;

    const char* max_env = getenv("PLANEX_HEADLESS_FRAMES");
    w->max_frames = max_env ? atoi(max_env) : 60;

    return w;
}

void px_window_free(px_window* w) {
    if (!w) return;
    px_fb_free(w->fb);
    free(w);
}

void px_window_on_resize(px_window* w, px_resize_callback cb, void* user) {
    if (!w) return;
    w->on_resize_cb   = cb;
    w->on_resize_user = user;
}

int px_window_width(px_window* w)  { return w ? w->width  : 0; }
int px_window_height(px_window* w) { return w ? w->height : 0; }
double px_window_scale(px_window* w) { (void)w; return 1.0; }
int px_window_fb_width(px_window* w) { return w ? w->width : 0; }
int px_window_fb_height(px_window* w) { return w ? w->height : 0; }

px_fb* px_window_fb(px_window* w) {
    return w ? w->fb : NULL;
}

int px_window_show(px_window* w) {
    if (!w) return -1;
    w->visible = true;
    return 0;
}

/* Try to read one event from stdin (non-blocking). */
static bool try_read_stdin_event(px_window* w, px_event* out) {
    char line[128];

#ifndef _WIN32
    /* POSIX: use select with 0 timeout for non-blocking */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    int rc = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (rc <= 0) return false;
#else
    /* Windows: use _kbhit() for non-blocking stdin check */
    if (!_kbhit()) return false;
#endif

    if (!fgets(line, sizeof(line), stdin)) return false;

    char cmd = line[0];
    if (cmd == 'q' || cmd == 'Q') {
        out->kind = PX_EV_CLOSE;
        w->should_close = true;
        return true;
    }
    if (cmd == 'c' || cmd == 'C') {
        int x = 0, y = 0;
        if (sscanf(line + 1, " %d %d", &x, &y) == 2) {
            out->kind = PX_EV_MOUSE_DOWN;
            out->x = x;
            out->y = y;
            out->button = 1;
            return true;
        }
    }
    if (cmd == 'k' || cmd == 'K') {
        char k = 0;
        if (sscanf(line + 1, " %c", &k) == 1) {
            out->kind = PX_EV_KEY_DOWN;
            out->key_char = k;
            return true;
        }
    }
    /* v0.8: named keys that cannot be typed as a single stdin char.
     * 't' = Tab, 'T' = Shift+Tab, 'e' = Enter — the keyboard-channel
     * test entries for the headless event script. */
    if (cmd == 't') {
        out->kind = PX_EV_KEY_DOWN;
        out->key_char = '\t';
        return true;
    }
    if (cmd == 'T') {
        out->kind = PX_EV_KEY_DOWN;
        out->key_char = '\t';
        out->modifiers = PX_MOD_SHIFT;
        return true;
    }
    if (cmd == 'e') {
        out->kind = PX_EV_KEY_DOWN;
        out->key_char = '\r';
        return true;
    }
    return false;
}

px_event px_window_poll_event(px_window* w) {
    px_event ev = { .kind = PX_EV_NONE };
    if (!w) return ev;

    /* Try stdin */
    if (try_read_stdin_event(w, &ev)) return ev;

    /* Auto-quit after max_frames */
    if (w->frames_rendered >= w->max_frames) {
        ev.kind = PX_EV_CLOSE;
        w->should_close = true;
    }
    return ev;
}

void px_window_present(px_window* w) {
    if (!w || !w->fb) return;
    w->frames_rendered++;

    /* Save BMP every N frames if env var set */
    const char* bmp_path = getenv("PLANEX_HEADLESS_BMP");
    if (bmp_path && (w->frames_rendered % 10 == 1)) {
        char path[256];
        snprintf(path, sizeof(path), "%s_%04d.bmp", bmp_path, w->frames_rendered);
        px_fb_save_bmp(w->fb, path);
    }
}

void px_window_close(px_window* w) {
    if (!w) return;
    w->should_close = true;
}

bool px_window_should_close(px_window* w) {
    return w ? w->should_close : true;
}
