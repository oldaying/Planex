/*
 * window.h — Window backend abstraction for Planex Stage 7
 *
 * Goal: separate "where pixels go" from "what pixels to draw".
 * The same Relation + Estimate + Closure + px_fb pipeline works
 * on X11, Win32, Cocoa, headless, etc.
 *
 * Design: px_window is a backend-specific wrapper around px_fb.
 * It exposes:
 *   - show()          create native window
 *   - poll_event()    non-blocking event poll (mouse/keyboard/close)
 *   - present()       copy fb pixels to window
 *   - close()         destroy window
 *
 * Stage 7: backends selected at compile time via PLANEX_BACKEND_* macro
 *   - X11 (default on Linux/BSD)
 *   - WIN32 (default on Windows; uses GDI for now, Direct2D later)
 *   - COCOA (default on macOS; uses Core Graphics + NSWindow)
 *   - HEADLESS (always available; just BMP output, no real window)
 *
 * If no backend macro is set, Makefile auto-detects by platform.
 */
#ifndef PLANEX_WINDOW_H
#define PLANEX_WINDOW_H

/* Forward declare px_fb to avoid circular include (planex.h includes
 * both fb.h and window.h). */
typedef struct px_fb px_fb;
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Window
 * ============================================================ */

typedef struct px_window px_window;

/* Event types returned by px_window_poll_event() */
typedef enum {
    PX_EV_NONE = 0,
    PX_EV_MOUSE_MOVE,
    PX_EV_MOUSE_DOWN,
    PX_EV_MOUSE_UP,
    PX_EV_KEY_DOWN,
    PX_EV_KEY_UP,
    PX_EV_CLOSE,
    PX_EV_IME_COMPOSE,   /* IME preedit intermediate text (Stage 9) */
    PX_EV_IME_COMMIT,   /* IME final committed text (Stage 9) */
    PX_EV_WHEEL,        /* Scroll wheel / trackpad scroll (v0.6) */
} px_event_kind;

typedef struct {
    px_event_kind kind;
    int           x;        /* mouse x for mouse events */
    int           y;        /* mouse y for mouse events */
    int           button;   /* 1=left, 2=middle, 3=right for mouse */
    int           key;      /* X11 keycode for key events */
    char          key_char; /* ASCII char if printable, else 0 */

    /* v0.8: modifier-key bitmask for key events (PX_MOD_*).
     * Backends that do not report modifiers leave it 0 — the
     * unmodified semantics. */
    int           modifiers;

    /* v0.6: wheel delta in ticks, used when kind == PX_EV_WHEEL.
     * Positive = scroll down/away from user; negative = up/toward.
     * x/y carry the cursor position at scroll time. */
    int           wheel_dy;

    /* IME composition (Stage 9) — used when kind == PX_EV_IME_COMMIT
     * or PX_EV_IME_COMPOSE. UTF-8 encoded, may be multibyte. */
    char          ime_text[64];  /* null-terminated UTF-8 */
} px_event;

/* v0.8: modifier flags for px_event.modifiers. A backend that
 * cannot report a modifier simply never sets its bit. */
#define PX_MOD_SHIFT 0x1
#define PX_MOD_CTRL  0x2
#define PX_MOD_ALT   0x4

/* New event kinds for IME (Stage 9) */
/* PX_EV_IME_COMPOSE:  IME is composing (intermediate state, preedit text)
 * PX_EV_IME_COMMIT:   IME committed final text (user pressed space/enter) */

/* Create a window with given dimensions and title.
 * Backend is selected at compile time (X11 in Stage 2). */
px_window* px_window_new(int width, int height, const char* title);
void       px_window_free(px_window* w);

/* Get the framebuffer associated with this window.
 * Render closures draw to this fb, then px_window_present() shows it.
 * NOTE: width/height may change at any time (ConfigureNotify), so
 *       always re-read px_fb_width/height in render callbacks. */
px_fb*     px_window_fb(px_window* w);

/* Current window size in logical pixels (CSS-like units).
 * On a 2x HiDPI display, fb physical size = logical * scale. */
int        px_window_width(px_window* w);
int        px_window_height(px_window* w);

/* DPI scale factor (Stage 15).
 * 1.0 = standard DPI (96 on Windows, 72 on macOS)
 * 2.0 = Retina / HiDPI
 * 1.5 = 144 DPI Windows display
 * Call this after px_window_show() to get the correct value. */
double     px_window_scale(px_window* w);

/* Physical (fb) dimensions = logical * scale.
 * fb is always allocated at physical resolution. */
int        px_window_fb_width(px_window* w);
int        px_window_fb_height(px_window* w);

/* Set a resize callback. Called when the window is resized by the
 * user or programmatically. The callback receives the new width/height.
 * Inside the callback, px_window_fb(w) already reflects the new size.
 * May be NULL — backend will just resize the fb without notifying. */
typedef void (*px_resize_callback)(px_window* w, int width, int height, void* user);
void       px_window_on_resize(px_window* w, px_resize_callback cb, void* user);

/* Show the window. Returns 0 on success. */
int        px_window_show(px_window* w);

/* Poll for next event (non-blocking). Returns PX_EV_NONE if no event. */
px_event   px_window_poll_event(px_window* w);

/* Copy framebuffer pixels to the window surface. Call after render. */
void       px_window_present(px_window* w);

/* Request close. The window will be destroyed on next free(). */
void       px_window_close(px_window* w);

/* Check if the user requested close (clicked X, alt-f4, etc.). */
bool       px_window_should_close(px_window* w);

/* ============================================================
 * Backend info
 * ============================================================ */

const char* px_window_backend_name(void);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_WINDOW_H */
