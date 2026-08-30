/*
 * x11_key_inject.c — XTEST key injector for the orca end-to-end
 * harness (scripts/verify_orca_e2e.sh)
 *
 * The harness needs REAL key events: the chain under test is
 *   X server -> Planex x11 backend -> px_app_run focus ring ->
 *   the a11y query side -> AT-SPI2 -> orca
 * and XSendEvent carries a synthetic flag the server does not. The
 * XTEST extension synthesizes events server-side — indistinguishable
 * from a physical keyboard.
 *
 * Usage:
 *   x11_key_inject DISPLAY "window title" "tok tok tok..." [gap_ms]
 *
 * Tokens: tab  stab (shift+tab)  return  space  q..z a..z 0..9
 *
 * The injector finds the window by title (XStoreName — the Planex
 * x11 backend stores the px_app_desc.title), focuses it (there is
 * no window manager under Xvfb, so focus must be set explicitly),
 * then replays the token stream with XTEST key presses.
 *
 * Build (no -dev packages needed beyond libX11 + libXtst runtime):
 *   cc -O1 x11_key_inject.c -o x11_key_inject -lX11 -lXtst
 * (the harness tries pkg-config first and falls back to
 *  -l:libXtst.so.6 so a runtime-only system works.)
 *
 * Part of the v0.8 Cross-cutting A verification tooling.
 */

#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if defined(__has_include)
#if __has_include(<X11/extensions/XTest.h>)
#include <X11/extensions/XTest.h>
#define PX_HAVE_XTEST_H 1
#endif
#endif
#ifndef PX_HAVE_XTEST_H
/* Runtime-only fallback: libxtst6 present, libxtst-dev not. The
 * XTEST fake-event entry point is declared locally (the extension
 * API has been stable since X11R6) so the harness compiles with
 * -l:libXtst.so.6 and no dev package. */
extern Bool XTestFakeKeyEvent(Display*, unsigned int, Bool, unsigned long);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* X headers tangle the feature macros; sleep via nanosleep with a
 * local helper rather than depending on usleep visibility. */
static void msleep(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static Window find_by_title(Display* dpy, Window root, const char* title) {
    Window parent, *children = NULL;
    unsigned int n = 0;
    Window found = None;
    if (XQueryTree(dpy, root, &root, &parent, &children, &n) && children) {
        for (unsigned int i = 0; i < n; i++) {
            char* name = NULL;
            if (XFetchName(dpy, children[i], &name) && name) {
                if (strcmp(name, title) == 0) found = children[i];
                XFree(name);
                if (found != None) break;
            }
        }
        XFree(children);
    }
    return found;
}

static void press(Display* dpy, KeyCode kc, Bool down) {
    XTestFakeKeyEvent(dpy, kc, down, CurrentTime);
    XFlush(dpy);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: %s DISPLAY \"window title\" \"tok tok ...\" [gap_ms]\n",
                argv[0]);
        return 2;
    }
    const char* dpy_name  = argv[1];
    const char* wtitle    = argv[2];
    const char* script    = argv[3];
    int gap_ms            = argc > 4 ? atoi(argv[4]) : 250;

    Display* dpy = XOpenDisplay(dpy_name);
    if (!dpy) {
        fprintf(stderr, "inject: cannot open display %s\n", dpy_name);
        return 1;
    }

    Window root = DefaultRootWindow(dpy);
    Window win = find_by_title(dpy, root, wtitle);
    if (win == None) {
        fprintf(stderr, "inject: window \"%s\" not found\n", wtitle);
        XCloseDisplay(dpy);
        return 1;
    }
    /* No WM under Xvfb: set input focus directly. */
    XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, win);
    XFlush(dpy);
    msleep(150);

    int events = 0;
    char* toks = strdup(script);
    char* save = NULL;
    for (char* t = strtok_r(toks, " \t", &save); t;
         t = strtok_r(NULL, " \t", &save)) {
        KeySym sym = NoSymbol;
        Bool shift = False;
        if (strcmp(t, "tab") == 0)        sym = XK_Tab;
        else if (strcmp(t, "stab") == 0) { sym = XK_Tab;    shift = True; }
        else if (strcmp(t, "return") == 0) sym = XK_Return;
        else if (strcmp(t, "space") == 0)  sym = XK_space;
        else if (strlen(t) == 1 && t[0] >= 'a' && t[0] <= 'z')
                                          sym = XStringToKeysym(t);
        else if (strlen(t) == 1 && t[0] >= '0' && t[0] <= '9')
                                          sym = XStringToKeysym(t);
        if (sym == NoSymbol) {
            fprintf(stderr, "inject: unknown token \"%s\"\n", t);
            continue;
        }
        KeyCode kc = XKeysymToKeycode(dpy, sym);
        if (kc == 0) {
            fprintf(stderr, "inject: no keycode for \"%s\"\n", t);
            continue;
        }
        KeyCode shift_kc = XKeysymToKeycode(dpy, XK_Shift_L);
        if (shift) press(dpy, shift_kc, True);
        msleep(30);
        press(dpy, kc, True);
        msleep(60);
        press(dpy, kc, False);
        if (shift) { msleep(30); press(dpy, shift_kc, False); }
        events++;
        msleep(gap_ms);
    }
    free(toks);

    printf("inject: %d events into \"%s\"\n", events, wtitle);
    XCloseDisplay(dpy);
    return 0;
}
