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

#include "planex/planex.h"

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

/* v0.7 Line 3: set the value string FROM an estimate's schema —
 * kind-aware denotation through px_estimate_describe() instead of a
 * hand-formatted string. This is the seam the platform bridges
 * (Line 4) read: the value's meaning comes from the contract, not
 * from each call site's printf. */
void     px_a11y_set_value_estimate(px_a11y* a, const px_estimate* e);

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

/* ============================================================
 * v0.6: Query side — the a11y channel becomes assertable
 *
 * Until v0.6 the a11y channel was write-only logging (L9: "logging-
 * only"). These getters mirror the setters, and a bounded ring of
 * recent announcements is kept queryable. Two consumers:
 *   1. Tests: multi-channel consistency can now be asserted in C
 *      ("the a11y value string contains the visual count") instead
 *      of parsing stderr.
 *   2. Future platform bridges: AT-SPI2/UIA/NSAccessibility glue can
 *      drain the same state — the query side IS the bridge contract.
 * The real platform bridges remain stubbed (#if 0 in a11y.c).
 * ============================================================ */

px_a11y_role px_a11y_get_role(const px_a11y* a);
const char*  px_a11y_get_name(const px_a11y* a);
const char*  px_a11y_get_value(const px_a11y* a);
unsigned     px_a11y_get_state(const px_a11y* a);

/* Enable/disable the stderr logging (query side keeps working either
 * way — production apps can silence the log without losing the data). */
void         px_a11y_set_verbose(px_a11y* a, bool verbose);
bool         px_a11y_is_verbose(const px_a11y* a);

/* Bounded announcement history (ring, most recent last).
 * px_a11y_announcement_count: how many announcements are retained
 *   (0..PX_A11Y_ANNOUNCE_CAPACITY).
 * px_a11y_announcement(a, i): the i-th retained announcement, 0 =
 *   oldest. NULL if i is out of range. Pointers are owned by the a11y
 *   context and valid until the context is freed. */
#define PX_A11Y_ANNOUNCE_CAPACITY 16
int          px_a11y_announcement_count(const px_a11y* a);
const char*  px_a11y_announcement(const px_a11y* a, int i);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_A11Y_H */
