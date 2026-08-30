/*
 * a11y_bridge_atspi.c — AT-SPI2 adapter behind the v0.6 query-side
 * contract (v0.7 Line 4)
 *
 * The Lisp-machine lesson applied (v0.7 roadmap Line 4): never bet
 * the abstractions on a host condition that can die. The a11y query
 * side (getters, announcement ring, set_verbose) is the stable
 * contract; this file is a REPLACEABLE ADAPTER, not an ontology
 * commitment. Zero changes to the canonical abstractions' sources.
 *
 * Build with the bridge (Linux, needs atk + atk-bridge headers):
 *   make CFLAGS_EXTRA="-DPX_A11Y_ATSPI $(pkg-config --cflags atk atk-bridge-2.0)"
 *
 * Include-layout note (found by the CI probe's first real compile):
 * atk-bridge.h's on-disk location is distro-dependent (Ubuntu 22.04:
 * /usr/include/atk-bridge-2.0/, Ubuntu 24.04:
 * /usr/include/at-spi2-atk/2.0/). The source therefore includes
 * <atk-bridge.h> BARE and relies on the pkg-config cflags to resolve
 * the layout — never hard-code the directory prefix.
 *
 * Without -DPX_A11Y_ATSPI this file compiles to a stub whose attach()
 * returns NULL — the logging/query-side default, byte-for-byte the
 * v0.6 behavior (the flag is opt-in, so no build regresses).
 *
 * The provider path (how a non-GTK C app talks to AT-SPI2):
 *   - the app builds an AtkObject tree (the mirror);
 *   - atk-bridge exports that tree over D-Bus to the AT-SPI2 bus;
 *   - orca (or any AT-SPI2 client) reads it.
 * This adapter keeps the mirror MINIMAL and honest: one application
 * root, one "current element" object whose properties track the
 * query side, and one ALERT object that carries announcements (orca
 * reads alert name changes aloud). Known limits, recorded rather
 * than hidden:
 *   - VALUES ride in the accessible description ("value: 3") until
 *     a value-carrying AtkObject subclass lands (AtkValue needs a
 *     GObject implementing the interface; no_op objects do not);
 *   - the tree is flat (root + element + alert) — no per-widget
 *     object hierarchy yet; navigation sees one element at a time;
 *   - atk_no_op_object_new is the zero-subclass way to get AtkObjects
 *     from plain C; it accepts no interfaces, hence the two limits
 *     above.
 *
 * THREAD SAFETY: single-threaded, like the rest of Planex.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/a11y.h"
#include <stdio.h>

#if defined(PX_A11Y_ATSPI)

#include <atk/atk.h>
#include <atk-bridge.h>   /* bare: pkg-config atk-bridge-2.0 resolves
                           * the distro-dependent include layout */
#include <stdlib.h>
#include <string.h>

struct px_a11y_bridge {
    px_a11y*   a;            /* the query side we mirror          */
    AtkObject* root;         /* APPLICATION root                   */
    AtkObject* element;      /* mirror of the current element      */
    AtkObject* alert;        /* announcements (orca reads alerts)  */
    char       last_announcement[160];
    bool       bridge_up;    /* atk-bridge adaptor initialized     */
};

/* ---- role mapping: px_a11y_role -> AtkRole ---------------------- */

static AtkRole map_role(px_a11y_role r) {
    switch (r) {
        case PX_A11Y_ROLE_WINDOW:       return ATK_ROLE_FRAME;
        case PX_A11Y_ROLE_BUTTON:       return ATK_ROLE_PUSH_BUTTON;
        case PX_A11Y_ROLE_TEXT:         return ATK_ROLE_LABEL;
        case PX_A11Y_ROLE_CHECKBOX:     return ATK_ROLE_CHECK_BOX;
        case PX_A11Y_ROLE_RADIO:        return ATK_ROLE_RADIO_BUTTON;
        case PX_A11Y_ROLE_SLIDER:       return ATK_ROLE_SLIDER;
        case PX_A11Y_ROLE_COMBOBOX:     return ATK_ROLE_COMBO_BOX;
        case PX_A11Y_ROLE_LIST:         return ATK_ROLE_LIST;
        case PX_A11Y_ROLE_LIST_ITEM:    return ATK_ROLE_LIST_ITEM;
        case PX_A11Y_ROLE_PROGRESS_BAR: return ATK_ROLE_PROGRESS_BAR;
        case PX_A11Y_ROLE_TEXT_INPUT:   return ATK_ROLE_ENTRY;
        case PX_A11Y_ROLE_NONE:
        default:                        return ATK_ROLE_UNKNOWN;
    }
}

/* Map the px state bits onto AtkStateTypes; fires notify events for
 * the delta only (so repeated flushes do not spam the bus). */
static void push_states(AtkObject* obj, unsigned state,
                        unsigned* prev) {
    static const struct { unsigned px; AtkStateType atk; } k_map[] = {
        { PX_A11Y_STATE_ENABLED,  ATK_STATE_ENABLED  },
        { PX_A11Y_STATE_FOCUSED,  ATK_STATE_FOCUSED  },
        { PX_A11Y_STATE_CHECKED,  ATK_STATE_CHECKED  },
        { PX_A11Y_STATE_PRESSED,  ATK_STATE_PRESSED  },
        { PX_A11Y_STATE_EXPANDED, ATK_STATE_EXPANDED },
        { PX_A11Y_STATE_SELECTED, ATK_STATE_SELECTED },
    };
    for (size_t i = 0; i < sizeof(k_map) / sizeof(k_map[0]); i++) {
        unsigned bit = 1u << i;
        bool now  = (state & k_map[i].px) != 0;
        bool then = (*prev & bit) != 0;
        if (now != then) {
            atk_object_notify_state_change(obj, k_map[i].atk, now);
        }
    }
    *prev = state;
}

/* ---- lifecycle --------------------------------------------------- */

px_a11y_bridge* px_a11y_bridge_atspi_attach(px_a11y* a,
                                            const char* app_name) {
    if (!a) return NULL;

    px_a11y_bridge* b =
        (px_a11y_bridge*)calloc(1, sizeof(px_a11y_bridge));
    if (!b) return NULL;
    b->a = a;

    /* The mirror tree: application root + current element + alert.
     * atk_no_op_object_new validates its GObject argument (GTK-style
     * constructor), so pass a real carrier — created once, unref'd
     * after the three AtkObjects exist (the no-op objects keep their
     * own refs; the carrier is only constructor plumbing). */
    GObject* carrier = g_object_new(G_TYPE_OBJECT, NULL);
    if (!carrier) {
        free(b);
        return NULL;
    }
    b->root    = atk_no_op_object_new(carrier);
    b->element = atk_no_op_object_new(carrier);
    b->alert   = atk_no_op_object_new(carrier);
    g_object_unref(carrier);
    if (!b->root || !b->element || !b->alert) {
        fprintf(stderr, "Planex a11y bridge: AtkObject allocation "
                        "failed\n");
        if (b->alert)   g_object_unref(G_OBJECT(b->alert));
        if (b->element) g_object_unref(G_OBJECT(b->element));
        if (b->root)    g_object_unref(G_OBJECT(b->root));
        free(b);
        return NULL;
    }

    atk_object_set_role(b->root, ATK_ROLE_APPLICATION);
    atk_object_set_name(b->root,
                        app_name ? app_name : "Planex application");

    atk_object_set_parent(b->element, b->root);
    atk_object_set_parent(b->alert, b->root);
    atk_object_set_role(b->alert, ATK_ROLE_ALERT);
    atk_object_set_name(b->alert, "");

    /* Export the tree over D-Bus to the AT-SPI2 bus. */
    atk_bridge_adaptor_init(NULL, NULL);
    b->bridge_up = true;

    return b;
}

void px_a11y_bridge_atspi_detach(px_a11y_bridge* b) {
    if (!b) return;
    if (b->bridge_up) {
        atk_bridge_adaptor_cleanup();
    }
    /* AtkObjects are GObjects; unref what we created. */
    if (b->alert)   g_object_unref(G_OBJECT(b->alert));
    if (b->element) g_object_unref(G_OBJECT(b->element));
    if (b->root)    g_object_unref(G_OBJECT(b->root));
    free(b);
}

/* ---- the flush: query side -> mirror -> D-Bus --------------------- */

void px_a11y_bridge_atspi_flush(px_a11y_bridge* b) {
    if (!b || !b->a) return;

    /* 1. Mirror the current element's properties. atk_object_set_name
     *    emits the property-change notifications the bridge forwards
     *    and orca reacts to. */
    const char* name = px_a11y_get_name(b->a);
    const char* value = px_a11y_get_value(b->a);
    atk_object_set_role(b->element, map_role(px_a11y_get_role(b->a)));
    atk_object_set_name(b->element, name ? name : "");
    /* VALUES ride in the description (the documented known limit —
     * atk_no_op_object_new accepts no AtkValue interface). v0.8
     * (Line 2): DRAGGABLE rides there too — the atk AtkStateType
     * enum has no draggable state (verified against atk 2.x; the
     * AT-SPI2 D-Bus StateType does, but the bridge mirror is built
     * from AtkStateTypes), so the bit the graph derived
     * (px_region_affords_process -> PX_A11Y_STATE_DRAGGABLE) is
     * surfaced honestly as text until a value/state-carrying
     * AtkObject subclass lands (the same posture as values). */
    char desc[160];
    unsigned state = px_a11y_get_state(b->a);
    snprintf(desc, sizeof(desc), "value: %s%s",
             value ? value : "",
             (state & PX_A11Y_STATE_DRAGGABLE) ? " draggable" : "");
    atk_object_set_description(b->element, desc);

    /* 2. State delta (notify events only for changed bits). */
    static unsigned s_prev = 0; /* single bridge per process — like
                                  * the rest of Planex, single-app */
    push_states(b->element, state, &s_prev);

    /* 3. Drain the announcement ring: the newest message that has
     *    not been announced becomes the alert's name — orca reads
     *    alert name changes aloud. Identical consecutive messages
     *    announce once (documented). */
    int count = px_a11y_announcement_count(b->a);
    if (count > 0) {
        const char* newest =
            px_a11y_announcement(b->a, count - 1);
        if (newest && strcmp(newest, b->last_announcement) != 0) {
            atk_object_set_name(b->alert, newest);
            snprintf(b->last_announcement,
                     sizeof(b->last_announcement), "%s", newest);
        }
    }
}

#else  /* !PX_A11Y_ATSPI — the v0.6 default: stub, no D-Bus, no dep */

px_a11y_bridge* px_a11y_bridge_atspi_attach(px_a11y* a,
                                            const char* app_name) {
    (void)app_name;
    (void)a;
    fprintf(stderr,
            "Planex a11y: AT-SPI2 bridge not compiled in — rebuild "
            "with -DPX_A11Y_ATSPI (needs atk + atk-bridge headers; "
            "see PLATFORMS.md). The query-side contract is fully "
            "functional without it.\n");
    return NULL;
}

void px_a11y_bridge_atspi_detach(px_a11y_bridge* b) {
    (void)b;
}

void px_a11y_bridge_atspi_flush(px_a11y_bridge* b) {
    (void)b;
}

#endif /* PX_A11Y_ATSPI */
