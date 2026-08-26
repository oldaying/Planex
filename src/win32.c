/*
 * win32.c - Windows GDI backend for Planex (Stage 7+)
 *
 * Uses GDI for software rasterization (StretchDIBits to copy fb
 * pixels to the window). No Direct2D/Direct3D.
 *
 * Stage 14: IME support via IMM32 (Imm* API).
 * - Handles WM_IME_COMPOSITION with GCS_RESULTSTR flag
 * - Gets committed UTF-16 text via ImmGetCompositionStringW
 * - Converts to UTF-8 and fires PX_EV_IME_COMMIT
 *
 * Build:
 *   x86_64-w64-mingw32-gcc -std=c17 -Wall -Wextra -Iinclude \
 *     -DPLANEX_BACKEND_WIN32 \
 *     src/relation.c src/estimate.c src/closure.c src/fb.c src/font.c \
 *     src/font_ttf.c src/app.c src/win32.c examples/counter_perception_window.c \
 *     -o counter.exe -lgdi32 -luser32 -limm32
 */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "planex/window.h"
#include "planex/fb.h"

#define PLANEX_WIN32_CLASS_NAME "PlanexWindowClass"

struct px_window {
    HWND         hwnd;
    HDC          hdc;
    HIMC         himc;
    int          width;          /* logical width */
    int          height;         /* logical height */
    double       scale;          /* DPI scale (Stage 15) */
    px_fb*       fb;             /* allocated at physical size */
    bool         should_close;
    bool         visible;
    px_event     queued_event;
    bool         has_queued;
    px_resize_callback on_resize_cb;
    void*              on_resize_user;
    BITMAPINFO   bmi;
};

/* Stage 14: Convert UTF-16 (wide) to UTF-8.
 * Returns bytes written (not including NUL), or 0 on failure. */
static int utf16_to_utf8(const wchar_t* wstr, int wlen, char* out, int out_size) {
    if (!wstr || wlen <= 0 || !out || out_size <= 0) return 0;
    int n = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, out, out_size - 1, NULL, NULL);
    if (n <= 0) return 0;
    out[n] = 0;
    return n;
}

/* Stage 14: Handle IME composition.
 * Called from WM_IME_COMPOSITION when GCS_RESULTSTR is set.
 * Gets the committed text (UTF-16) from IMM32, converts to UTF-8,
 * and queues a PX_EV_IME_COMMIT event. */
static void handle_ime_composition(px_window* w, LPARAM lp) {
    if (!w || !w->himc) return;

    if (lp & GCS_RESULTSTR) {
        /* Committed text available */
        LONG len = ImmGetCompositionStringW(w->himc, GCS_RESULTSTR, NULL, 0);
        if (len > 0) {
            /* len is in bytes; convert to wchar count */
            int wlen = len / sizeof(wchar_t);
            wchar_t* wbuf = (wchar_t*)malloc((size_t)(wlen + 1) * sizeof(wchar_t));
            if (wbuf) {
                ImmGetCompositionStringW(w->himc, GCS_RESULTSTR, wbuf, len);
                wbuf[wlen] = 0;

                /* Convert to UTF-8 */
                char utf8[128];
                int n = utf16_to_utf8(wbuf, wlen, utf8, sizeof(utf8));
                if (n > 0) {
                    w->queued_event.kind = PX_EV_IME_COMMIT;
                    memset(w->queued_event.ime_text, 0, sizeof(w->queued_event.ime_text));
                    strncpy(w->queued_event.ime_text, utf8,
                            sizeof(w->queued_event.ime_text) - 1);
                    w->has_queued = true;
                }
                free(wbuf);
            }
        }
    }

    if (lp & GCS_COMPSTR) {
        /* Preedit text available (intermediate, not committed yet).
         * Stage 14: we could fire PX_EV_IME_COMPOSE here for
         * preedit rendering, but for simplicity we skip it.
         * Stage 15+ can add preedit rendering. */
    }

    /* Tell IMM32 we handled it */
    ImmReleaseContext(w->hwnd, w->himc);
}

static LRESULT CALLBACK planex_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    px_window* w = (px_window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CLOSE:
            if (w) w->should_close = true;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            if (w) {
                int new_w = LOWORD(lp);
                int new_h = HIWORD(lp);
                if (new_w > 0 && new_h > 0 &&
                    (new_w != w->width || new_h != w->height)) {
                    px_fb_resize(w->fb, new_w, new_h);
                    w->width = new_w;
                    w->height = new_h;
                    if (w->on_resize_cb) {
                        w->on_resize_cb(w, new_w, new_h, w->on_resize_user);
                    }
                }
            }
            return 0;
        case WM_LBUTTONDOWN:
            if (w) {
                w->queued_event.kind = PX_EV_MOUSE_DOWN;
                w->queued_event.x = (int)(short)LOWORD(lp);
                w->queued_event.y = (int)(short)HIWORD(lp);
                w->queued_event.button = 1;
                w->has_queued = true;
            }
            return 0;
        case WM_LBUTTONUP:
            if (w) {
                w->queued_event.kind = PX_EV_MOUSE_UP;
                w->queued_event.x = (int)(short)LOWORD(lp);
                w->queued_event.y = (int)(short)HIWORD(lp);
                w->queued_event.button = 1;
                w->has_queued = true;
            }
            return 0;
        case WM_MOUSEMOVE:
            if (w) {
                w->queued_event.kind = PX_EV_MOUSE_MOVE;
                w->queued_event.x = (int)(short)LOWORD(lp);
                w->queued_event.y = (int)(short)HIWORD(lp);
                w->queued_event.button = 0;
                w->has_queued = true;
            }
            return 0;
        case WM_KEYDOWN:
            /* WM_KEYDOWN gives virtual key codes (VK_*), not ASCII.
             * Don't set key_char here — let WM_CHAR handle printable chars.
             * Only handle special keys that WM_CHAR won't produce. */
            if (w && !w->has_queued) {
                /* Map special keys to ASCII equivalents */
                char c = 0;
                switch (wp) {
                    case VK_RETURN: c = '\n'; break;
                    case VK_BACK:   c = 127; break;  /* DEL = backspace */
                    case VK_TAB:     c = '\t'; break;
                    case VK_ESCAPE:  c = 27; break;
                    default: break;  /* Let WM_CHAR handle printable keys */
                }
                if (c) {
                    w->queued_event.kind = PX_EV_KEY_DOWN;
                    w->queued_event.key_char = c;
                    w->has_queued = true;
                }
            }
            return 0;
        case WM_CHAR:
            /* Stage 14: WM_CHAR gives us the actual character (ASCII or UTF-16).
             * This always overrides WM_KEYDOWN for printable keys.
             * For ASCII (0x20-0x7E), use as KEY_DOWN.
             * For non-ASCII, fire IME_COMMIT with UTF-8 conversion. */
            if (w) {
                wchar_t wc = (wchar_t)wp;
                if (wc >= 0x20 && wc < 0x80) {
                    /* ASCII printable — override any WM_KEYDOWN event */
                    w->queued_event.kind = PX_EV_KEY_DOWN;
                    w->queued_event.key_char = (char)wc;
                    w->has_queued = true;
                } else if (wc != 0) {
                    /* Non-ASCII char — convert to UTF-8 */
                    wchar_t wstr[2] = { wc, 0 };
                    char utf8[8];
                    int n = utf16_to_utf8(wstr, 1, utf8, sizeof(utf8));
                    if (n > 0) {
                        w->queued_event.kind = PX_EV_IME_COMMIT;
                        memset(w->queued_event.ime_text, 0,
                               sizeof(w->queued_event.ime_text));
                        strncpy(w->queued_event.ime_text, utf8,
                                sizeof(w->queued_event.ime_text) - 1);
                        w->queued_event.key_char = 0;
                        w->has_queued = true;
                    }
                }
            }
            return 0;
        case WM_IME_SETCONTEXT:
            /* IME is being activated/deactivated. Let DefWindowProc
             * handle the default behavior (show/hide IME UI). */
            break;
        case WM_IME_STARTCOMPOSITION:
            /* IME composition starting — could show preedit window here */
            break;
        case WM_IME_COMPOSITION:
            /* Stage 14: IME composition event.
             * GCS_RESULTSTR = committed text available
             * GCS_COMPSTR = preedit text (intermediate) */
            if (w) {
                w->himc = ImmGetContext(hwnd);
                if (w->himc) {
                    handle_ime_composition(w, lp);
                }
            }
            if (!(lp & GCS_RESULTSTR)) {
                /* If no result string, let DefWindowProc handle it */
                return DefWindowProc(hwnd, msg, wp, lp);
            }
            return 0;
        case WM_IME_ENDCOMPOSITION:
            /* IME composition ended */
            break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static bool s_class_registered = false;

const char* px_window_backend_name(void) {
    return "win32";
}

px_window* px_window_new(int width, int height, const char* title) {
    if (!s_class_registered) {
        WNDCLASSA wc = {0};
        wc.lpfnWndProc   = planex_wndproc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = PLANEX_WIN32_CLASS_NAME;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        if (!RegisterClassA(&wc)) {
            fprintf(stderr, "Planex Win32: RegisterClass failed\n");
            return NULL;
        }
        s_class_registered = true;
    }

    px_window* w = (px_window*)calloc(1, sizeof(px_window));
    if (!w) return NULL;
    w->width  = width;
    w->height = height;
    w->should_close = false;
    w->visible = false;
    w->has_queued = false;
    w->himc = NULL;

    /* Stage 15: enable DPI awareness + detect scale.
     * SetProcessDpiAwareness (Win 8.1+) or SetProcessDPIAware (Vista+).
     * GetDpiForWindow (Win 10 1607+) or GetDeviceCaps(LOGPIXELSX). */
    HMODULE user32 = GetModuleHandleA("user32.dll");
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContext_t)(HANDLE);
    typedef UINT (WINAPI *GetDpiForWindow_t)(HWND);
    SetProcessDpiAwarenessContext_t set_ctx =
        (SetProcessDpiAwarenessContext_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    GetDpiForWindow_t get_dpi =
        (GetDpiForWindow_t)GetProcAddress(user32, "GetDpiForWindow");
    if (set_ctx) {
        set_ctx((HANDLE)-4);  /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
    } else {
        SetProcessDPIAware();
    }

    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    w->hwnd = CreateWindowExA(
        0,
        PLANEX_WIN32_CLASS_NAME,
        title ? title : "Planex",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!w->hwnd) {
        fprintf(stderr, "Planex Win32: CreateWindow failed\n");
        free(w);
        return NULL;
    }

    /* Detect DPI scale */
    if (get_dpi) {
        UINT dpi = get_dpi(w->hwnd);
        w->scale = (double)dpi / 96.0;
    } else {
        HDC screen_dc = GetDC(NULL);
        int dpi = GetDeviceCaps(screen_dc, LOGPIXELSX);
        ReleaseDC(NULL, screen_dc);
        w->scale = (double)dpi / 96.0;
    }
    if (w->scale < 1.0) w->scale = 1.0;

    /* Allocate fb at physical size */
    int pw = (int)(w->width * w->scale);
    int ph = (int)(w->height * w->scale);
    w->fb = px_fb_new(pw, ph);
    if (!w->fb) {
        free(w);
        return NULL;
    }

    SetWindowLongPtr(w->hwnd, GWLP_USERDATA, (LONG_PTR)w);
    w->hdc = GetDC(w->hwnd);

    memset(&w->bmi, 0, sizeof(w->bmi));
    w->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    w->bmi.bmiHeader.biWidth = width;
    w->bmi.bmiHeader.biHeight = -height;
    w->bmi.bmiHeader.biPlanes = 1;
    w->bmi.bmiHeader.biBitCount = 32;
    w->bmi.bmiHeader.biCompression = BI_RGB;

    /* Stage 14: Associate IME context with the window.
     * ImmGetContext retrieves the default IMC; we don't need to
     * create a custom one for basic IME support. */
    w->himc = ImmGetContext(w->hwnd);
    if (w->himc) {
        /* Enable IME for this window */
        ImmAssociateContext(w->hwnd, w->himc);
    }

    return w;
}

void px_window_free(px_window* w) {
    if (!w) return;
    if (w->himc) ImmReleaseContext(w->hwnd, w->himc);
    if (w->hwnd) {
        SetWindowLongPtr(w->hwnd, GWLP_USERDATA, 0);
        ReleaseDC(w->hwnd, w->hdc);
    }
    if (w->fb) px_fb_free(w->fb);
    free(w);
}

void px_window_on_resize(px_window* w, px_resize_callback cb, void* user) {
    if (!w) return;
    w->on_resize_cb   = cb;
    w->on_resize_user = user;
}

int px_window_width(px_window* w)  { return w ? w->width  : 0; }
int px_window_height(px_window* w) { return w ? w->height : 0; }
double px_window_scale(px_window* w) { return w ? w->scale : 1.0; }
int px_window_fb_width(px_window* w) {
    return w ? (int)(w->width * w->scale) : 0;
}
int px_window_fb_height(px_window* w) {
    return w ? (int)(w->height * w->scale) : 0;
}

px_fb* px_window_fb(px_window* w) {
    return w ? w->fb : NULL;
}

int px_window_show(px_window* w) {
    if (!w) return -1;
    ShowWindow(w->hwnd, SW_SHOW);
    UpdateWindow(w->hwnd);
    w->visible = true;
    return 0;
}

px_event px_window_poll_event(px_window* w) {
    px_event ev = { .kind = PX_EV_NONE };
    if (!w) return ev;

    if (w->has_queued) {
        ev = w->queued_event;
        w->has_queued = false;
        return ev;
    }

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (w->has_queued) {
            ev = w->queued_event;
            w->has_queued = false;
            return ev;
        }
        if (w->should_close) {
            ev.kind = PX_EV_CLOSE;
            return ev;
        }
    }
    return ev;
}

void px_window_present(px_window* w) {
    if (!w || !w->fb) return;

    /* Stage 15: fb is at physical resolution (logical * scale).
     * BITMAPINFO must match fb dimensions, but StretchDIBits stretches
     * from physical fb size to logical window size. */
    int fb_w = px_fb_width(w->fb);
    int fb_h = px_fb_height(w->fb);

    if (w->bmi.bmiHeader.biWidth != fb_w ||
        w->bmi.bmiHeader.biHeight != -fb_h) {
        w->bmi.bmiHeader.biWidth = fb_w;
        w->bmi.bmiHeader.biHeight = -fb_h;
    }

    /* Source: physical fb pixels (fb_w x fb_h)
     * Dest: logical window client area (w->width x w->height)
     * StretchDIBits handles the scaling */
    const uint32_t* pixels = px_fb_pixels(w->fb);
    StretchDIBits(w->hdc,
                   0, 0, w->width, w->height,
                   0, 0, fb_w, fb_h,
                   pixels, &w->bmi,
                   DIB_RGB_COLORS, SRCCOPY);
}

void px_window_close(px_window* w) {
    if (!w) return;
    w->should_close = true;
    if (w->hwnd) PostMessage(w->hwnd, WM_CLOSE, 0, 0);
}

bool px_window_should_close(px_window* w) {
    return w ? w->should_close : true;
}

#endif /* _WIN32 */
