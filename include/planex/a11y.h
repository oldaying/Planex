/*
 * a11y.h - Accessibility layer for Planex (Stage 16)
 *
 * Goal: let screen readers (NVDA, JAWS, VoiceOver, Orca) announce
 * UI state changes to visually impaired users.
 *
 * Design: minimal cross-platform API that maps to:
 * - Linux: AT-SPI2 (via D-Bus, stub for now)
 * - Windows: UI Automation (UiaProvider, stub for now)
 * - macOS: Accessibility API (AXUIElement protocol)
 *
 * The API is intentionally simple — just announce:
 *   - Element role (button, text, slider, etc.)
 *   - Element name (label text)
 *   - Element value (current value as string)
 *   - Element state (enabled/disabled, focused, checked)
 *
 * Usage:
 *   px_a11y* a = px_a11y_new(window);
 *   px_a11y_set_role(a, PX_A11Y_ROLE_BUTTON);
 *   px_a11y_set_name(a, "Increment counter");
 *   px_a11y_set_value(a, "42");
 *   px_a11y_set_state(a, PX_A11Y_STATE_FOCUSED | PX_A11Y_STATE_ENABLED);
 *   px_a11y_announce(a, "Counter incremented to 43");
 *   px_a11y_free(a);
 *
 * If the platform doesn't support accessibility, all functions
 * are no-ops (safe to call unconditionally).
 */
#ifndef PLANEX_A11Y_H
#define PLANEX_A11Y_H

#include "planex/window.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Types
 * ============================================================ */

typedef struct px_a11y px_a11y;

typedef enum {
    PX_A11Y_ROLE_NONE = 0,
    PX_A11Y_ROLE_WINDOW,
    PX_A11Y_ROLE_BUTTON,
    PX_A11Y_ROLE_TEXT,
    PX_A11Y_ROLE_CHECKBOX,
    PX_A11Y_ROLE_RADIO,
    PX_A11Y_ROLE_SLIDER,
    PX_A11Y_ROLE_COMBOBOX,
    PX_A11Y_ROLE_LIST,
    PX_A11Y_ROLE_LIST_ITEM,
    PX_A11Y_ROLE_PROGRESS_BAR,
    PX_A11Y_ROLE_TEXT_INPUT,
} px_a11y_role;

typedef enum {
    PX_A11Y_STATE_NONE     = 0,
    PX_A11Y_STATE_ENABLED  = 1 << 0,
    PX_A11Y_STATE_FOCUSED   = 1 << 1,
    PX_A11Y_STATE_CHECKED   = 1 << 2,
    PX_A11Y_STATE_PRESSED   = 1 << 3,
    PX_A11Y_STATE_EXPANDED  = 1 << 4,
    PX_A11Y_STATE_SELECTED  = 1 << 5,
} px_a11y_state;

/* ============================================================
 * Lifecycle
 * ============================================================ */

/* Create an accessibility context for a window.
 * Returns NULL if accessibility is not available on this platform.
 * Safe to call — NULL return just means no-op. */
px_a11y* px_a11y_new(px_window* w);

/* Free the accessibility context. */
void     px_a11y_free(px_a11y* a);

/* ============================================================
 * Properties (set the current element being announced)
 * ============================================================ */

/* Set the role of the current element. */
void     px_a11y_set_role(px_a11y* a, px_a11y_role role);

/* Set the accessible name (label) of the current element. */
void     px_a11y_set_name(px_a11y* a, const char* name);

/* Set the value of the current element (as a string). */
void     px_a11y_set_value(px_a11y* a, const char* value);

/* Set the state flags (bitmask of px_a11y_state). */
void     px_a11y_set_state(px_a11y* a, unsigned state);

/* ============================================================
 * Actions
 * ============================================================ */

/* Announce a text change to the screen reader.
 * E.g., "Counter incremented to 43". */
void     px_a11y_announce(px_a11y* a, const char* message);

/* Notify that focus moved to the current element. */
void     px_a11y_focus(px_a11y* a);

/* Get human-readable role name (for debugging). */
const char* px_a11y_role_str(px_a11y_role role);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_A11Y_H */
