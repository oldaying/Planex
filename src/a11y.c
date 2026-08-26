/*
 * a11y.c - Accessibility layer implementation (Stage 16)
 *
 * Stage 16 provides the cross-platform API and a logging fallback.
 * Platform-specific implementations (AT-SPI2, UIA, AXUIElement)
 * are stubbed — they compile on all platforms but are no-ops
 * on Linux (no D-Bus dependency) and log to stderr everywhere.
 *
 * This is intentional: full AT-SPI2 requires libatspi + D-Bus
 * (heavy deps), full UIA requires COM + UiaProvider (complex),
 * full AXUIElement requires NSObject + NSAccessibility protocol.
 * Each is 200-500 LOC of platform glue.
 *
 * What this stage achieves:
 * - Defines the API that demos and apps can call unconditionally
 * - Provides a logging fallback for development/debugging
 * - Shows the integration points for future stages
 *
 * Future: each platform can add #ifdef-guarded real implementations
 * without changing the API.
 */
#include "planex/a11y.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct px_a11y {
    px_window*     window;
    px_a11y_role   role;
    char           name[128];
    char           value[128];
    unsigned       state;
    bool           verbose;   /* log to stderr when true */
};

static const char* const k_role_names[] = {
    "none", "window", "button", "text", "checkbox", "radio",
    "slider", "combobox", "list", "list_item",
    "progress_bar", "text_input",
};

const char* px_a11y_role_str(px_a11y_role role) {
    int n = sizeof(k_role_names) / sizeof(k_role_names[0]);
    if ((int)role < 0 || (int)role >= n) return "?";
    return k_role_names[role];
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

px_a11y* px_a11y_new(px_window* w) {
    px_a11y* a = (px_a11y*)calloc(1, sizeof(px_a11y));
    if (!a) return NULL;
    a->window  = w;
    a->role    = PX_A11Y_ROLE_NONE;
    a->state   = 0;
    a->verbose = true;  /* log during development; can be disabled later */
    a->name[0]  = 0;
    a->value[0] = 0;
    return a;
}

void px_a11y_free(px_a11y* a) {
    free(a);
}

/* ============================================================
 * Properties
 * ============================================================ */

void px_a11y_set_role(px_a11y* a, px_a11y_role role) {
    if (!a) return;
    if (a->role != role) {
        a->role = role;
        if (a->verbose) {
            fprintf(stderr, "[a11y] role=%s\n", px_a11y_role_str(role));
        }
    }
}

void px_a11y_set_name(px_a11y* a, const char* name) {
    if (!a || !name) return;
    if (strcmp(a->name, name) != 0) {
        strncpy(a->name, name, sizeof(a->name) - 1);
        a->name[sizeof(a->name) - 1] = 0;
        if (a->verbose) {
            fprintf(stderr, "[a11y] name=\"%s\"\n", a->name);
        }
    }
}

void px_a11y_set_value(px_a11y* a, const char* value) {
    if (!a || !value) return;
    if (strcmp(a->value, value) != 0) {
        strncpy(a->value, value, sizeof(a->value) - 1);
        a->value[sizeof(a->value) - 1] = 0;
        if (a->verbose) {
            fprintf(stderr, "[a11y] value=\"%s\"\n", a->value);
        }
    }
}

void px_a11y_set_state(px_a11y* a, unsigned state) {
    if (!a) return;
    if (a->state != state) {
        a->state = state;
        if (a->verbose) {
            fprintf(stderr, "[a11y] state=");
            if (state & PX_A11Y_STATE_ENABLED)  fprintf(stderr, "enabled ");
            if (state & PX_A11Y_STATE_FOCUSED)   fprintf(stderr, "focused ");
            if (state & PX_A11Y_STATE_CHECKED)   fprintf(stderr, "checked ");
            if (state & PX_A11Y_STATE_PRESSED)   fprintf(stderr, "pressed ");
            if (state & PX_A11Y_STATE_EXPANDED)  fprintf(stderr, "expanded ");
            if (state & PX_A11Y_STATE_SELECTED)  fprintf(stderr, "selected ");
            fprintf(stderr, "\n");
        }
    }
}

/* ============================================================
 * Actions
 * ============================================================ */

void px_a11y_announce(px_a11y* a, const char* message) {
    if (!a || !message) return;
    if (a->verbose) {
        fprintf(stderr, "[a11y] announce: \"%s\"\n", message);
    }

    /* ============================================================
     * Platform-specific announce implementations
     * (future: uncomment when deps are available)
     * ============================================================ */

#if 0 /* AT-SPI2 on Linux — needs libatspi + D-Bus */
    /* AtspiAccessible* accessible = ...;
     * atspi_accessible_set_description(accessible, message, NULL);
     * atspi_text_set_caret_offset(ATSPI_TEXT(accessible), 0, NULL);
     */
#endif

#if 0 /* UIA on Windows — needs COM + UIAutomationProvider.h */
    /* UiaRaiseAutomationEvent(peer, UIA_Text_TextChangedEventId);
     */
#endif

#if 0 /* NSAccessibility on macOS — needs Cocoa */
    /* NSAccessibilityPostNotification(window, NSAccessibilityAnnouncementRequestedNotification,
     *   @{NSAccessibilityAnnouncementKey: [NSString stringWithUTF8String:message]});
     */
#endif
}

void px_a11y_focus(px_a11y* a) {
    if (!a) return;
    if (a->verbose) {
        fprintf(stderr, "[a11y] focus: role=%s name=\"%s\"\n",
                px_a11y_role_str(a->role), a->name);
    }

    /* Platform-specific focus implementations would go here,
     * similar to px_a11y_announce above. */
}

/* ============================================================
 * Integration points for future stages
 *
 * To implement real accessibility:
 *
 * X11 (AT-SPI2):
 *   - Link: -latspi
 *   - Create AtspiAccessible root for the window
 *   - Implement AtspiComponent interface for hit testing
 *   - Call atspi_accessible_set_name/description/role
 *   - Estimated: 300 LOC
 *
 * Win32 (UI Automation):
 *   - Link: -luiautomationcore
 *   - Implement IRawElementProviderSimple
 * *   - Override UiaReturnRawElementProvider in wndproc
 *   - Estimated: 400 LOC
 *
 * Cocoa (NSAccessibility):
 *   - No extra link needed
 *   - Implement accessibilityRole/Title/Value on PlanexView
 *   - Call NSAccessibilityPostNotification on changes
 *   - Estimated: 200 LOC
 * ============================================================ */
