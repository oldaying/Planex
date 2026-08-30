/*
 * x11.c — X11 window backend for Planex Stage 5
 *
 * Stage 5: XShm (MIT-SHM) shared memory optimization.
 * - If XShmQueryExtension returns true: use XShmCreateImage +
 *   XShmPutImage (zero-copy: X server reads from shared memory)
 * - Else: fall back to XPutImage (Stage 2 behavior)
 *
 * The framebuffer (px_fb) still stores RGBA. We convert RGBA → BGRX
 * into the shared memory segment before XShmPutImage. This is the
 * same conversion as Stage 2, but the resulting pixels are read by
 * the X server directly from shared memory (no IPC copy).
 *
 * Net speedup: 1 less memcpy per frame for the XPutImage path.
 * For a 1024x768 window at 60fps, this saves ~150MB/s of memory
 * bandwidth — significant for large windows.
 *
 * Stage 2 limitations still apply (no resize, no IME, no DPI, etc.).
 */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlocale.h>
#include "planex/window.h"
#include "planex/fb.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>

struct px_window {
    Display*     display;
    Window       window;
    GC           gc;
    int          screen;
    int          width;           /* logical width (CSS pixels) */
    int          height;          /* logical height (CSS pixels) */
    double       scale;          /* DPI scale: 1.0 = normal, 2.0 = Retina */
    px_fb*       fb;              /* allocated at physical size (logical * scale) */
    bool         should_close;
    bool         visible;

    /* XImage (wraps either shared memory or local buffer) */
    XImage*      image;
    char*        image_data;

    /* XShm state */
    bool         shm_available;
    XShmSegmentInfo shm_info;

    /* Resize callback */
    px_resize_callback on_resize_cb;
    void*              on_resize_user;

    /* XIM / XIC for IME input (Stage 9) */
    XIM          xim;
    XIC          xic;
    bool         ime_active;
};

/* ============================================================
 * Backend info
 * ============================================================ */

const char* px_window_backend_name(void) {
    return "X11";
}

/* ============================================================
 * DPI scale detection (Stage 15)
 *
 * X11 doesn't have a universal DPI API. We use the screen's
 * millimeter dimensions + pixel count to compute DPI:
 *   dpi = pixels / (mm / 25.4)
 *   scale = dpi / 96.0
 *
 * If XRandR is available we could use it for per-monitor DPI,
 * but for Stage 15 the screen-level DPI is sufficient.
 * ============================================================ */

static double detect_dpi_scale(Display* display, int screen) {
    int width_px = DisplayWidth(display, screen);
    int width_mm = DisplayWidthMM(display, screen);
    if (width_mm <= 0) return 1.0;
    double dpi = (double)width_px / ((double)width_mm / 25.4);
    double scale = dpi / 96.0;
    /* Clamp to reasonable range */
    if (scale < 0.5) scale = 0.5;
    if (scale > 4.0) scale = 4.0;
    /* Round to nearest 0.25 to avoid fractional pixel issues */
    scale = ((int)(scale * 4.0 + 0.5)) / 4.0;
    if (scale < 1.0) scale = 1.0;  /* don't upscale below 1x */
    return scale;
}

/* Compute physical size from logical + scale */
static int phys_w(px_window* w) { return (int)(w->width * w->scale); }
static int phys_h(px_window* w) { return (int)(w->height * w->scale); }

/* ============================================================
 * Window lifecycle
 * ============================================================ */

px_window* px_window_new(int width, int height, const char* title) {
    if (width <= 0 || height <= 0) return NULL;

    px_window* w = (px_window*)calloc(1, sizeof(px_window));
    if (!w) return NULL;

    w->display = XOpenDisplay(NULL);
    if (!w->display) {
        fprintf(stderr, "Planex X11: cannot open display (set $DISPLAY)\n");
        free(w);
        return NULL;
    }

    w->screen = DefaultScreen(w->display);
    w->width  = width;             /* logical size */
    w->height = height;
    w->should_close = false;
    w->visible = false;

    /* Stage 15: detect DPI scale */
    w->scale = detect_dpi_scale(w->display, w->screen);
    fprintf(stderr, "Planex X11: DPI scale = %.2f\n", w->scale);

    /* Check XShm availability */
    int major, minor, ignore;
    Bool pixmaps_ok;
    w->shm_available = false;
    if (XShmQueryExtension(w->display) &&
        XShmQueryVersion(w->display, &major, &minor, &pixmaps_ok)) {
        /* Test if we can actually create a shared memory segment */
        /* (some X servers report extension but reject shm) */
        w->shm_available = true;
        fprintf(stderr, "Planex X11: XShm available (v%d.%d, pixmaps=%d)\n",
                major, minor, pixmaps_ok);
    } else {
        fprintf(stderr, "Planex X11: XShm not available, using XPutImage\n");
    }
    (void)ignore;

    Window root = RootWindow(w->display, w->screen);
    unsigned long black = BlackPixel(w->display, w->screen);

    w->window = XCreateSimpleWindow(
        w->display, root,
        0, 0, width, height,
        0, black, black);

    if (title) XStoreName(w->display, w->window, title);

    XSelectInput(w->display, w->window,
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        StructureNotifyMask);  /* ConfigureNotify comes via this */

    w->gc = XCreateGC(w->display, w->window, 0, NULL);

    /* Stage 15: allocate fb at physical resolution (logical * scale) */
    int pw = phys_w(w);
    int ph = phys_h(w);
    w->fb = px_fb_new(pw, ph);
    if (!w->fb) {
        fprintf(stderr, "Planex X11: cannot create framebuffer\n");
        XFreeGC(w->display, w->gc);
        XDestroyWindow(w->display, w->window);
        XCloseDisplay(w->display);
        free(w);
        return NULL;
    }

    /* Allocate BGRX image buffer at physical resolution (Stage 15) */
    int img_w = phys_w(w);
    int img_h = phys_h(w);
    size_t buf_size = (size_t)img_w * img_h * 4;
    Visual* vis = DefaultVisual(w->display, w->screen);
    int depth = DefaultDepth(w->display, w->screen);

    if (w->shm_available) {
        /* Try XShmCreateImage + shared memory segment */
        w->image = XShmCreateImage(
            w->display,
            vis,
            depth,
            ZPixmap,
            NULL,
            &w->shm_info,
            img_w, img_h);
        if (w->image) {
            /* Allocate shared memory segment of the right size */
            w->shm_info.shmid = shmget(IPC_PRIVATE, (size_t)w->image->bytes_per_line * w->image->height,
                                        IPC_CREAT | 0777);
            if (w->shm_info.shmid >= 0) {
                w->shm_info.shmaddr = (char*)shmat(w->shm_info.shmid, NULL, 0);
                if (w->shm_info.shmaddr != (char*)-1) {
                    w->shm_info.readOnly = False;
                    w->image->data = w->shm_info.shmaddr;
                    w->image_data = w->shm_info.shmaddr;  /* alias for present() */

                    if (XShmAttach(w->display, &w->shm_info) == 0) {
                        /* Attach failed — fall back to non-shared path */
                        shmdt(w->shm_info.shmaddr);
                        shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                        XDestroyImage(w->image);
                        w->image = NULL;
                        w->shm_available = false;
                    } else {
                        /* Fully synced so X server has attached before we use */
                        XSync(w->display, False);
                        /* Mark segment for deletion now — it stays alive until
                         * we detach, but is auto-cleaned after. */
                        shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                    }
                } else {
                    /* shmat failed — fall back */
                    shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                    XDestroyImage(w->image);
                    w->image = NULL;
                    w->shm_available = false;
                }
            } else {
                /* shmget failed — fall back */
                XDestroyImage(w->image);
                w->image = NULL;
                w->shm_available = false;
            }
        } else {
            w->shm_available = false;
        }

        if (!w->shm_available) {
            fprintf(stderr, "Planex X11: XShm init failed, falling back to XPutImage\n");
        }
    }

    if (!w->image) {
        /* Stage 2 fallback: local BGRX buffer */
        w->image_data = (char*)malloc(buf_size);
        if (!w->image_data) {
            px_fb_free(w->fb);
            XFreeGC(w->display, w->gc);
            XDestroyWindow(w->display, w->window);
            XCloseDisplay(w->display);
            free(w);
            return NULL;
        }
        memset(w->image_data, 0, buf_size);

        w->image = XCreateImage(
            w->display, vis, depth,
            ZPixmap, 0,
            w->image_data,
            img_w, img_h,
            32, img_w * 4);
        if (!w->image) {
            fprintf(stderr, "Planex X11: XCreateImage failed\n");
            free(w->image_data);
            px_fb_free(w->fb);
            XFreeGC(w->display, w->gc);
            XDestroyWindow(w->display, w->window);
            XCloseDisplay(w->display);
            free(w);
            return NULL;
        }
    }

    /* Subscribe to WM_DELETE_WINDOW */
    Atom wm_delete_window = XInternAtom(w->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(w->display, w->window, &wm_delete_window, 1);

    /* Stage 9: Initialize XIM (X Input Method) for IME support.
     * This is best-effort — if XIM fails (no IME server, no locale),
     * we just fall back to plain XLookupString for ASCII. */
    w->xim = NULL;
    w->xic = NULL;
    w->ime_active = false;

    /* Set locale (required for XIM to work with CJK) */
    if (!setlocale(LC_CTYPE, "")) {
        /* try common CJK locales */
        setlocale(LC_CTYPE, "en_US.UTF-8");
    }

    w->xim = XOpenIM(w->display, NULL, NULL, NULL);
    if (w->xim) {
        XIMStyle style = 0;
        XIMStyles* styles = NULL;
        if (XGetIMValues(w->xim, XNQueryInputStyle, &styles, NULL) == NULL && styles) {
            /* Prefer Root preedit — simplest, works everywhere */
            for (int i = 0; i < styles->count_styles; i++) {
                if (styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing)) {
                    style = styles->supported_styles[i];
                    break;
                }
            }
            XFree(styles);
        }
        if (style == 0) {
            /* No root style — close IM, fall back to non-IME */
            XCloseIM(w->xim);
            w->xim = NULL;
        } else {
            w->xic = XCreateIC(w->xim,
                                XNInputStyle,    style,
                                XNClientWindow, w->window,
                                XNFocusWindow,   w->window,
                                NULL);
            if (!w->xic) {
                XCloseIM(w->xim);
                w->xim = NULL;
            }
        }
    }
    /* If XIM init failed, w->xic is NULL — KeyPress handler falls
     * back to XLookupString for ASCII. */

    /* Stage 6: window is resizable. Set a reasonable minimum size. */
    XSizeHints hints = {0};
    hints.flags = PMinSize;
    hints.min_width  = 64;
    hints.min_height = 48;
    XSetNormalHints(w->display, w->window, &hints);

    return w;
}

void px_window_on_resize(px_window* w, px_resize_callback cb, void* user) {
    if (!w) return;
    w->on_resize_cb   = cb;
    w->on_resize_user = user;
}

int px_window_width(px_window* w)  { return w ? w->width  : 0; }
int px_window_height(px_window* w) { return w ? w->height : 0; }
double px_window_scale(px_window* w) { return w ? w->scale : 1.0; }
int px_window_fb_width(px_window* w) { return w ? phys_w(w) : 0; }
int px_window_fb_height(px_window* w) { return w ? phys_h(w) : 0; }

void px_window_free(px_window* w) {
    if (!w) return;
    if (w->image) {
        if (w->shm_available) {
            XShmDetach(w->display, &w->shm_info);
            shmdt(w->shm_info.shmaddr);
            w->image->data = NULL;
            XDestroyImage(w->image);
        } else {
            w->image->data = NULL;
            XDestroyImage(w->image);
            free(w->image_data);
        }
    }
    /* Stage 9: destroy XIC + XIM */
    if (w->xic) XDestroyIC(w->xic);
    if (w->xim) XCloseIM(w->xim);
    if (w->fb) px_fb_free(w->fb);
    if (w->gc) XFreeGC(w->display, w->gc);
    if (w->window) XDestroyWindow(w->display, w->window);
    if (w->display) XCloseDisplay(w->display);
    free(w);
}

/* ============================================================
 * Resize handling (Stage 6)
 *
 * Called from ConfigureNotify event. Rebuilds XImage + shared memory
 * segment + fb at the new size. Then fires on_resize_cb if set.
 * ============================================================ */

static void rebuild_image(px_window* w, int new_log_w, int new_log_h) {
    if (!w || new_log_w <= 0 || new_log_h <= 0) return;
    if (new_log_w == w->width && new_log_h == w->height) return;

    /* Save new logical size */
    w->width  = new_log_w;
    w->height = new_log_h;

    /* Compute physical size */
    int new_w = phys_w(w);
    int new_h = phys_h(w);

    /* Tear down old image */
    if (w->shm_available) {
        XShmDetach(w->display, &w->shm_info);
        shmdt(w->shm_info.shmaddr);
        w->image->data = NULL;
        XDestroyImage(w->image);
        w->image = NULL;
    } else {
        w->image->data = NULL;
        XDestroyImage(w->image);
        free(w->image_data);
        w->image_data = NULL;
        w->image = NULL;
    }

    /* Allocate new image at physical size */
    Visual* vis = DefaultVisual(w->display, w->screen);
    int depth = DefaultDepth(w->display, w->screen);

    if (w->shm_available) {
        w->image = XShmCreateImage(w->display, vis, depth, ZPixmap,
                                    NULL, &w->shm_info, new_w, new_h);
        if (w->image) {
            w->shm_info.shmid = shmget(IPC_PRIVATE,
                                        (size_t)w->image->bytes_per_line * w->image->height,
                                        IPC_CREAT | 0777);
            if (w->shm_info.shmid >= 0) {
                w->shm_info.shmaddr = (char*)shmat(w->shm_info.shmid, NULL, 0);
                if (w->shm_info.shmaddr != (char*)-1) {
                    w->shm_info.readOnly = False;
                    w->image->data = w->shm_info.shmaddr;
                    w->image_data = w->shm_info.shmaddr;
                    if (XShmAttach(w->display, &w->shm_info) == 0) {
                        shmdt(w->shm_info.shmaddr);
                        shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                        XDestroyImage(w->image);
                        w->image = NULL;
                        w->shm_available = false;
                    } else {
                        XSync(w->display, False);
                        shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                    }
                } else {
                    shmctl(w->shm_info.shmid, IPC_RMID, NULL);
                    XDestroyImage(w->image);
                    w->image = NULL;
                    w->shm_available = false;
                }
            } else {
                XDestroyImage(w->image);
                w->image = NULL;
                w->shm_available = false;
            }
        } else {
            w->shm_available = false;
        }

        if (!w->shm_available) {
            fprintf(stderr, "Planex X11: XShm rebuild failed, falling back\n");
        }
    }

    if (!w->image) {
        size_t buf_size = (size_t)new_w * new_h * 4;
        w->image_data = (char*)malloc(buf_size);
        if (!w->image_data) return;
        memset(w->image_data, 0, buf_size);

        w->image = XCreateImage(w->display, vis, depth, ZPixmap, 0,
                                 w->image_data, new_w, new_h, 32, new_w * 4);
        if (!w->image) {
            free(w->image_data);
            w->image_data = NULL;
            return;
        }
    }

    /* Resize the framebuffer to physical size */
    px_fb_resize(w->fb, new_w, new_h);

    /* Fire the application's resize callback with logical size */
    if (w->on_resize_cb) {
        w->on_resize_cb(w, new_log_w, new_log_h, w->on_resize_user);
    }
}

px_fb* px_window_fb(px_window* w) {
    return w ? w->fb : NULL;
}

int px_window_show(px_window* w) {
    if (!w) return -1;
    XMapWindow(w->display, w->window);
    XFlush(w->display);
    w->visible = true;

    XEvent ev;
    while (w->visible) {
        XNextEvent(w->display, &ev);
        if (ev.type == Expose) break;
    }
    return 0;
}

/* ============================================================
 * Events
 * ============================================================ */

static char keycode_to_char(Display* dpy, KeyCode keycode) {
    char buf[16];
    KeySym keysym = NoSymbol;
    XComposeStatus compose;
    int n = XLookupString(&(XKeyEvent){
        .type = KeyPress,
        .display = dpy,
        .keycode = keycode,
        .state = 0,
    }, buf, sizeof(buf), &keysym, &compose);
    if (n > 0) return buf[0];
    /* v0.8: keys that produce no text but still carry a canonical
     * char for routing. Tab under Shift maps to ISO_Left_Tab and
     * XLookupString yields nothing — report '\t' anyway so the
     * KEY_DOWN event carries the key identity; modifiers tell the
     * shift apart. */
    if (keysym == XK_Tab || keysym == XK_ISO_Left_Tab) return '\t';
    if (keysym == XK_Return) return '\r';
    return 0;
}

/* Stage 9: Try to get UTF-8 text via XIC (X Input Context).
 * Returns:
 *   -1 = buffer too small or no text
 *   0  = no text (key was a modifier / arrow / etc.)
 *   >0 = number of bytes written to out_buf (not including NUL)
 *
 * Sets *status to:
 *   XLookupNone     — no input generated
 *   XLookupChars    — text available (regular or IME commit)
 *   XLookupKeySym   — only keysym, no text
 *   XLookupBoth     — both text and keysym */
static int xic_lookup_string(XIC xic, XKeyEvent* ev, char* out_buf, int buf_size,
                              int* out_status) {
    if (!xic) {
        *out_status = XLookupNone;
        return 0;
    }
    KeySym keysym = 0;
    Status status = 0;
    int n = XmbLookupString(xic, ev, out_buf, buf_size - 1, &keysym, &status);
    out_buf[n] = 0;
    *out_status = (int)status;
    return n;
}

px_event px_window_poll_event(px_window* w) {
    px_event ev = { .kind = PX_EV_NONE };
    if (!w) return ev;

    if (XPending(w->display) == 0) {
        return ev;
    }

    XEvent xev;
    XNextEvent(w->display, &xev);

    switch (xev.type) {
        case Expose:
            break;
        case ConfigureNotify: {
            /* Window resized (or moved). Rebuild fb + XImage if size changed. */
            int new_w = xev.xconfigure.width;
            int new_h = xev.xconfigure.height;
            if (new_w > 0 && new_h > 0 &&
                (new_w != w->width || new_h != w->height)) {
                rebuild_image(w, new_w, new_h);
                ev.kind = PX_EV_NONE;  /* internal event; app re-renders via on_resize_cb */
            }
            break;
        }
        case MotionNotify:
            ev.kind = PX_EV_MOUSE_MOVE;
            ev.x = xev.xmotion.x;
            ev.y = xev.xmotion.y;
            break;
        case ButtonPress:
            /* v0.6: X11 wheel events arrive as Button4/Button5 presses.
             * Map them to PX_EV_WHEEL instead of PX_EV_MOUSE_DOWN so the
             * semantic layer (scroll = continuous channel) stays distinct
             * from button clicks. */
            if (xev.xbutton.button == 4 || xev.xbutton.button == 5) {
                ev.kind = PX_EV_WHEEL;
                ev.x = xev.xbutton.x;
                ev.y = xev.xbutton.y;
                ev.wheel_dy = (xev.xbutton.button == 5) ? 1 : -1;
                break;
            }
            ev.kind = PX_EV_MOUSE_DOWN;
            ev.x = xev.xbutton.x;
            ev.y = xev.xbutton.y;
            ev.button = xev.xbutton.button;
            break;
        case ButtonRelease:
            /* Wheel releases are swallowed — the press already delivered
             * the full PX_EV_WHEEL event. */
            if (xev.xbutton.button == 4 || xev.xbutton.button == 5) {
                ev.kind = PX_EV_NONE;
                break;
            }
            ev.kind = PX_EV_MOUSE_UP;
            ev.x = xev.xbutton.x;
            ev.y = xev.xbutton.y;
            ev.button = xev.xbutton.button;
            break;
        case KeyPress: {
            /* v0.8: modifiers first — every key event carries them. */
            if (xev.xkey.state & ShiftMask)   ev.modifiers |= PX_MOD_SHIFT;
            if (xev.xkey.state & ControlMask) ev.modifiers |= PX_MOD_CTRL;
            if (xev.xkey.state & Mod1Mask)    ev.modifiers |= PX_MOD_ALT;

            /* Stage 9: use XIC (X Input Context) for IME + UTF-8 support.
             * If XIC available, XmbLookupString returns UTF-8 / locale-encoded text.
             * If commit happens (IME finalized), fire PX_EV_IME_COMMIT.
             * Otherwise (regular ASCII keypress), fall back to PX_EV_KEY_DOWN. */
            char buf[64];
            int status = 0;
            int n = xic_lookup_string(w->xic, &xev.xkey, buf, sizeof(buf), &status);
            if (n > 0 && (status == XLookupChars || status == XLookupBoth)) {
                /* Text was produced — could be ASCII or IME commit */
                if (n == 1 && (unsigned char)buf[0] < 0x80) {
                    /* ASCII single char — emit as KEY_DOWN for backward compat */
                    ev.kind = PX_EV_KEY_DOWN;
                    ev.key = xev.xkey.keycode;
                    ev.key_char = buf[0];
                } else {
                    /* Multi-byte (UTF-8) — IME commit or non-ASCII text */
                    ev.kind = PX_EV_IME_COMMIT;
                    ev.key = xev.xkey.keycode;
                    strncpy(ev.ime_text, buf, sizeof(ev.ime_text) - 1);
                    ev.ime_text[sizeof(ev.ime_text) - 1] = 0;
                }
            } else {
                /* No text — modifier / arrow / function key */
                ev.kind = PX_EV_KEY_DOWN;
                ev.key = xev.xkey.keycode;
                ev.key_char = keycode_to_char(w->display, xev.xkey.keycode);
            }
            break;
        }
        case KeyRelease:
            ev.kind = PX_EV_KEY_UP;
            ev.key = xev.xkey.keycode;
            if (xev.xkey.state & ShiftMask)   ev.modifiers |= PX_MOD_SHIFT;
            if (xev.xkey.state & ControlMask) ev.modifiers |= PX_MOD_CTRL;
            if (xev.xkey.state & Mod1Mask)    ev.modifiers |= PX_MOD_ALT;
            ev.key_char = keycode_to_char(w->display, xev.xkey.keycode);
            break;
        case ClientMessage: {
            Atom wm_delete_window = XInternAtom(w->display, "WM_DELETE_WINDOW", False);
            if ((Atom)xev.xclient.data.l[0] == wm_delete_window) {
                ev.kind = PX_EV_CLOSE;
                w->should_close = true;
            }
            break;
        }
        default:
            break;
    }

    return ev;
}

/* ============================================================
 * Present (copy framebuffer to window)
 *
 * Stage 5: uses XShmPutImage when shm_available, else XPutImage.
 * Both paths convert RGBA → BGRX into image_data first.
 *
 * Performance comparison (1024x768 @ 60fps):
 *   Stage 2 (XPutImage): RGBA→BGRX (3MB) + XPutImage IPC copy (3MB) = 6MB/frame
 *   Stage 5 (XShm):      RGBA→BGRX (3MB) + XShm read from shm =     3MB/frame
 *
 * For smaller windows (< 320x200) the difference is negligible.
 * ============================================================ */

void px_window_present(px_window* w) {
    if (!w || !w->fb || !w->image) return;

    const uint32_t* src = px_fb_pixels(w->fb);
    uint32_t*       dst = (uint32_t*)w->image_data;
    size_t n = (size_t)w->width * w->height;

    /* RGBA → BGRX: drop alpha, swap R and B */
    for (size_t i = 0; i < n; i++) {
        uint32_t rgba = src[i];
        uint32_t r = (rgba >> 16) & 0xFF;
        uint32_t g = (rgba >> 8)  & 0xFF;
        uint32_t b =  rgba        & 0xFF;
        dst[i] = 0xFF000000u | (b << 16) | (g << 8) | r;
    }

    if (w->shm_available) {
        /* XShmPutImage with send_event=False: X server reads from
         * shared memory when it processes the request. We XSync to
         * ensure the X server has finished reading before we overwrite
         * the shared buffer on the next frame (prevents tearing).
         *
         * This is slower than async (no XSync), but async risks
         * tearing. Stage 5.5+ could use double-buffering for async.
         *
         * Note: on Xvfb (virtual X server), XShm vs XPutImage
         * performance is similar because both write to in-memory
         * buffers. On real X servers (Xorg on hardware), XShm is
         * typically 2-5x faster because it avoids the IPC copy. */
        XShmPutImage(w->display, w->window, w->gc, w->image,
                     0, 0, 0, 0, w->width, w->height, False);
        XSync(w->display, False);
    } else {
        XPutImage(w->display, w->window, w->gc, w->image,
                  0, 0, 0, 0, w->width, w->height);
        XFlush(w->display);
    }
}

void px_window_close(px_window* w) {
    if (!w) return;
    w->should_close = true;
}

bool px_window_should_close(px_window* w) {
    return w ? w->should_close : true;
}
