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
 * root, one FRAME (the window anchor — orca's focus machinery
 * resolves focused elements into the active window, and the frame
 * holds the initial focus), one region object PER ELEMENT THAT HAS
 * HELD FOCUS (a lazily-grown name-keyed table — see flush), and one
 * ALERT object that carries announcements via the "announcement"
 * signal. The mirror objects are a minimal AtkObject SUBCLASS
 * (PxMirrorObject) with a persistent state set and explicit
 * children — the atk_no_op objects the v0.7 bridge used lose state
 * changes on read-back (their ref_state_set hands out a fresh
 * empty set per call).
 * Known limits, recorded rather than hidden:
 *   - VALUES ride in the accessible description ("value: 3") until
 *     a value-carrying subclass lands (AtkValue needs a GObject
 *     implementing the interface);
 *   - region objects materialize ON FOCUS — elements that never
 *     held focus are not in the tree;
 *   - the app must call flush() regularly (the demo: ~60 Hz) — the
 *     flush pumps the bridge's D-Bus traffic, and an app that
 *     stops flushing looks hung to clients;
 *   - the very first focus move after a client attaches may be
 *     announced late or not at all: the client's own keyboard
 *     handling races the event (query latency vs its locus
 *     advance) — observed once with orca, every later move
 *     announced;
 *   - detach does NOT unregister the application from the
 *     accessibility bus (the GTK exit semantic — atk-bridge 2.56's
 *     cleanup path is unsafe for non-GTK embedders with a live
 *     client; see the detach); re-attaching after detach is not
 *     supported (single bridge per process).
 * The end-to-end orca run (v0.8 Cross-cutting A) drove TWELVE
 * rounds of real fixes into this file — the AtkUtil root provider,
 * the registration completion (children-changed), the D-Bus pump,
 * the frame + active-window projection, the ENABLED->SENSITIVE and
 * FOCUSED->FOCUSABLE projections, the mirror subclass, the eager
 * listener activation, the per-region objects, the announcement
 * signal, the focus-gated materialization, and the clean teardown
 * (the defunct sweep + the frame's true initial child + dropping
 * atk_bridge_adaptor_cleanup, whose cache teardown crashed non-GTK
 * embedders at detach — see px_mirror_mark_defunct and the detach)
 * — each annotated
 * where it lives; a compile probe cannot see any of them, which is
 * exactly why the observed pass was the roadmap's success
 * criterion.
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

/* ---- PxMirrorObject: the minimal mirror AtkObject ----------------
 *
 * Found by the orca run, round six: atk_no_op_object_new's
 * ref_state_set returns a FRESH EMPTY set on every call, so the
 * state changes atk_object_notify_state_change() makes land on a
 * throwaway set — clients querying GetState always saw an empty
 * state set (orca read every enabled element as "grayed", and the
 * active-window resolution failed because the frame's ACTIVE state
 * read false). The mirror therefore needs its own subclass with:
 *   - a PERSISTENT AtkStateSet (notify mutations stick, exactly
 *     like a GTK widget accessible);
 *   - EXPLICIT CHILDREN (get_n_children/ref_child), so clients can
 *     enumerate the tree (window resolution walks it).
 * This succeeds the "zero-subclass" no-op posture; the known limit
 * that REMAINS is that values ride in the description (no AtkValue
 * interface is implemented). */

typedef struct {
    AtkObject   parent;
    AtkStateSet* states;      /* persistent — the point of the class */
    GPtrArray*   children;    /* AtkObject*, borrowed refs            */
} PxMirrorObject;

typedef struct {
    AtkObjectClass parent_class;
} PxMirrorObjectClass;

static GType px_mirror_object_get_type(void);

static AtkStateSet* px_mirror_ref_state_set(AtkObject* obj) {
    PxMirrorObject* m = (PxMirrorObject*)obj;
    return (AtkStateSet*)g_object_ref(m->states);
}

static gint px_mirror_get_n_children(AtkObject* obj) {
    PxMirrorObject* m = (PxMirrorObject*)obj;
    return (gint)m->children->len;
}

static AtkObject* px_mirror_ref_child(AtkObject* obj, gint i) {
    PxMirrorObject* m = (PxMirrorObject*)obj;
    if (i < 0 || (guint)i >= m->children->len) return NULL;
    return (AtkObject*)g_object_ref(m->children->pdata[i]);
}

/* Chain up to AtkObject's own finalize (it frees name/description)
 * via the class peeked at registration time. */
static GObjectClass* s_atk_gobject_class = NULL;

static void px_mirror_object_finalize(GObject* gobj) {
    PxMirrorObject* m = (PxMirrorObject*)gobj;
    g_object_unref(m->states);
    g_ptr_array_free(m->children, TRUE);   /* the children themselves
                                            * are unref'd by detach */
    if (s_atk_gobject_class && s_atk_gobject_class->finalize) {
        s_atk_gobject_class->finalize(gobj);
    }
}

static void px_mirror_object_class_init(AtkObjectClass* klass,
                                         gpointer class_data) {
    (void)class_data;
    klass->ref_state_set  = px_mirror_ref_state_set;
    klass->get_n_children = px_mirror_get_n_children;
    klass->ref_child      = px_mirror_ref_child;
    s_atk_gobject_class   = G_OBJECT_CLASS(
        g_type_class_peek(ATK_TYPE_OBJECT));
    G_OBJECT_CLASS(klass)->finalize = px_mirror_object_finalize;
}

static void px_mirror_object_init(PxMirrorObject* m,
                                   gpointer class_data) {
    (void)class_data;
    m->states   = atk_state_set_new();
    m->children = g_ptr_array_new();
}

static GType px_mirror_object_get_type(void) {
    static GType type = 0;
    if (!type) {
        static const GTypeInfo info = {
            sizeof(PxMirrorObjectClass),
            NULL, NULL,
            (GClassInitFunc)px_mirror_object_class_init,
            NULL, NULL,
            sizeof(PxMirrorObject), 0,
            (GInstanceInitFunc)px_mirror_object_init, NULL
        };
        type = g_type_register_static(ATK_TYPE_OBJECT,
                                      "PxMirrorObject", &info, 0);
    }
    return type;
}

static PxMirrorObject* px_mirror_object_new(void) {
    return (PxMirrorObject*)g_object_new(
        px_mirror_object_get_type(), NULL);
}

/* The GTK pattern for states: the OBJECT owns a real state set, and
 * notify only ANNOUNCES changes (atk_object_notify_state_change
 * emits the signal — it does not write any state, which the probe
 * runs of round six established: even with our persistent set,
 * notify alone leaves it empty, and every client GetState reads
 * empty). The bridge therefore routes ALL state changes through
 * this apply: mutate the persistent set first, notify only when
 * the set actually changed — the delta discipline, now enforced by
 * the set itself. */
static void px_mirror_apply_state(AtkObject* obj, AtkStateType t,
                                  bool v) {
    PxMirrorObject* m = (PxMirrorObject*)obj;
    bool changed = v ? atk_state_set_add_state(m->states, t)
                     : atk_state_set_remove_state(m->states, t);
    if (changed) {
        atk_object_notify_state_change(obj, t, v);
    }
}

/* Children are REGISTERED (parent pointer + enumerable), and the
 * children-changed::add signal rides along — atk-bridge needs the
 * signal to export the child; clients need the enumeration to walk
 * the tree. The signal index is the child's ENUMERATED position
 * (computed here, append semantics) — round twelve found the orca
 * run's indices drifting from the array: regions announced at
 * hash-table positions while the array already held [NULL, alert]
 * (the attach bug below), so clients saw ChildrenChanged events
 * whose indices disagreed with GetChildAtIndex. One source of
 * truth: the position IS the array slot. */
static void px_mirror_add_child(PxMirrorObject* parent,
                                AtkObject* child) {
    gint index = (gint)parent->children->len;
    g_ptr_array_add(parent->children, child);
    atk_object_set_parent(child, (AtkObject*)parent);
    g_signal_emit_by_name((AtkObject*)parent, "children-changed::add",
                          index, (gpointer)child);
}

/* The teardown half of the GTK contract (found by the orca run,
 * round twelve): a dying widget's accessible is marked DEFUNCT —
 * `state-changed:defunct` is the one signal atk-bridge treats as
 * "this object is gone": its state listener runs the full
 * deregistration per object (the "object-deregistered" emit → the
 * cache-out: weak-unref with the CORRECT arguments + refmap entry
 * removal; the register's own weak-unref; the D-Bus path teardown).
 * Without the sweep, atk_bridge_adaptor_cleanup() destroyed the
 * cache's refmap with our weak refs still installed — its teardown
 * foreach weak-unrefs the refmap VALUES (always NULL — an
 * atk-bridge 2.56 defect, the values never hold the objects), so
 * the unrefs fail and the refs go stale — and freeing the mirror
 * afterwards fired the stale weak notifies into the dead table:
 * g_hash_table_remove on an empty hash, a GLib fatal abort (or a
 * plain SEGV, heap-layout dependent). Marking defunct FIRST is the
 * GTK teardown path, exercised by every GTK app that ever closed a
 * window; the sweep makes the mirror's exit as clean as its run. */
static void px_mirror_mark_defunct(gpointer key, gpointer value,
                                   gpointer user) {
    (void)key;
    (void)user;
    px_mirror_apply_state((AtkObject*)value, ATK_STATE_DEFUNCT, true);
}

struct px_a11y_bridge {
    px_a11y*   a;            /* the query side we mirror          */
    AtkObject* root;         /* APPLICATION root                   */
    AtkObject* frame;        /* the WINDOW mirror (see attach)     */
    GHashTable* elements;    /* NAME -> AtkObject, one per region
                              * that has held focus (round nine)   */
    AtkObject* element;      /* the CURRENT region's object        */
    AtkObject* alert;        /* announcements (orca reads alerts)  */
    char       last_announcement[160];
    bool       bridge_up;    /* atk-bridge adaptor initialized     */
    /* Flush-side change guards: atk_object_set_name/role/descrip-
     * tion may emit property-change on EVERY call (not only on
     * changes), and apps that flush per frame would spam the bus.
     * The mirror fires only on real changes — the same delta
     * discipline as push_states. On an element TRANSITION the
     * guards are bypassed once: the new object must be fully
     * populated regardless of what the previous one carried. */
    char       last_element_desc[160];
    AtkRole    last_element_role;
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

/* Map the px state bits onto the mirror's state set via
 * px_mirror_apply_state (mutate + notify-on-change; the set itself
 * is the delta discipline — no shadow prev tracking left). */
static void push_states(AtkObject* obj, unsigned state) {
    static const struct { unsigned px; AtkStateType atk; } k_map[] = {
        { PX_A11Y_STATE_ENABLED,  ATK_STATE_ENABLED  },
        { PX_A11Y_STATE_FOCUSED,  ATK_STATE_FOCUSED  },
        { PX_A11Y_STATE_CHECKED,  ATK_STATE_CHECKED  },
        { PX_A11Y_STATE_PRESSED,  ATK_STATE_PRESSED  },
        { PX_A11Y_STATE_EXPANDED, ATK_STATE_EXPANDED },
        { PX_A11Y_STATE_SELECTED, ATK_STATE_SELECTED },
    };
    for (size_t i = 0; i < sizeof(k_map) / sizeof(k_map[0]); i++) {
        bool now = (state & k_map[i].px) != 0;
        px_mirror_apply_state(obj, k_map[i].atk, now);
        /* The orca pass found orca announcing every enabled element
         * as "grayed": orca's availability check reads the AT-SPI
         * SENSITIVE state (atk-bridge maps ENABLED and SENSITIVE
         * independently). PX has one ENABLED bit; project it onto
         * BOTH states — an enabled Planex element is both enabled
         * and sensitive. */
        if (k_map[i].px == PX_A11Y_STATE_ENABLED) {
            px_mirror_apply_state(obj, ATK_STATE_SENSITIVE, now);
        }
        /* Round eight: orca's focused-event presentation drops a
         * focused object that lacks FOCUSABLE ("is focused but
         * lacks state focusable" — GTK focusables always carry
         * both). PX has no focusable bit; an element that receives
         * focus was focusable by construction — project the
         * state alongside FOCUSED and never clear it (the mirror
         * serves focusable elements by design). */
        if (k_map[i].px == PX_A11Y_STATE_FOCUSED && now) {
            px_mirror_apply_state(obj, ATK_STATE_FOCUSABLE, true);
        }
    }
}

/* ---- the AtkUtil root provider (the non-GTK integration point) ---
 *
 * Found by the first end-to-end orca run (v0.8 Cross-cutting A):
 * atk_bridge_adaptor_init() registers the app subtree ROOTED AT
 * atk_get_root() and bails with `runtime check failed: (root)`
 * when that is NULL — plain ATK's default. GTK installs its root
 * via the AtkUtil class vtable; a non-GTK toolkit must do the
 * same BEFORE the adaptor init, or the app never reaches the
 * accessibility bus (the compile probe cannot see this — only
 * the observed pass can). Single bridge per process, like the
 * rest of Planex: the mirror root is kept in a static. */
static AtkObject* s_mirror_root = NULL;

static AtkObject* bridge_util_get_root(void) {
    return s_mirror_root;
}
static const gchar* bridge_util_toolkit_name(void) {
    return "Planex";
}
static const gchar* bridge_util_toolkit_version(void) {
    /* PLANEX_VERSION_* are non-string macros; map by hand — the
     * toolkit version is diagnostic, not a contract. */
    return "0.8";
}

/* ---- lifecycle --------------------------------------------------- */

px_a11y_bridge* px_a11y_bridge_atspi_attach(px_a11y* a,
                                            const char* app_name) {
    if (!a) return NULL;

    px_a11y_bridge* b =
        (px_a11y_bridge*)calloc(1, sizeof(px_a11y_bridge));
    if (!b) return NULL;
    b->a = a;

    /* The mirror tree: application root + FRAME + current element
     * + alert, all PxMirrorObjects (persistent state sets, explicit
     * children — see the class note above; the no-op objects the
     * v0.7 bridge used lose states on read-back, found by the orca
     * run, round six).
     *
     * The FRAME (found by the orca run, round four): orca's focus
     * machinery needs an ACTIVE WINDOW to resolve focused objects
     * into — without one it kept "Active window is None", reset the
     * locus to None mid-presentation, and dropped later focus
     * events as orphans. The frame carries the window identity and
     * fires state-changed:active at attach, which orca's default
     * script treats as window activation. */
    PxMirrorObject* root  = px_mirror_object_new();
    PxMirrorObject* frame = px_mirror_object_new();
    PxMirrorObject* alert = px_mirror_object_new();
    if (!root || !frame || !alert) {
        fprintf(stderr, "Planex a11y bridge: AtkObject allocation "
                        "failed\n");
        if (alert) g_object_unref(G_OBJECT(alert));
        if (frame) g_object_unref(G_OBJECT(frame));
        if (root)  g_object_unref(G_OBJECT(root));
        free(b);
        return NULL;
    }
    b->root     = (AtkObject*)root;
    b->frame    = (AtkObject*)frame;
    b->alert    = (AtkObject*)alert;
    /* Round nine: one mirror object per REGION that has held focus
     * (a lazily-grown table keyed by the element name — the query
     * side's identity). See the flush for why the flat one-object
     * mirror could not express a focus move. */
    b->elements = g_hash_table_new_full(g_str_hash, g_str_equal,
                                        g_free,
                                        (GDestroyNotify)g_object_unref);
    if (!b->elements) {
        g_object_unref(G_OBJECT(alert));
        g_object_unref(G_OBJECT(frame));
        g_object_unref(G_OBJECT(root));
        free(b);
        return NULL;
    }
    b->element = NULL;

    atk_object_set_role(b->root, ATK_ROLE_APPLICATION);
    atk_object_set_name(b->root,
                        app_name ? app_name : "Planex application");

    atk_object_set_role(b->frame, ATK_ROLE_FRAME);
    atk_object_set_name(b->frame,
                        app_name ? app_name : "Planex application");
    atk_object_set_role(b->alert, ATK_ROLE_ALERT);
    atk_object_set_name(b->alert, "");

    /* Install the root provider BEFORE the adaptor init: the
     * adaptor registers the app at atk_get_root() (see the note
     * above — the first orca run found this the hard way). */
    s_mirror_root = b->root;
    AtkUtilClass* uclass = g_type_class_ref(ATK_TYPE_UTIL);
    uclass->get_root            = bridge_util_get_root;
    uclass->get_toolkit_name    = bridge_util_toolkit_name;
    uclass->get_toolkit_version = bridge_util_toolkit_version;

    /* Export the tree over D-Bus to the AT-SPI2 bus. The adaptor
     * returns -1 when it cannot reach the bus (no session D-Bus,
     * accessibility disabled) — the found-by-inspection round: the
     * v0.7 code ignored that and dereferenced half-initialized
     * bridge state (a crash when the bus is absent). The honest
     * contract is the stub's: NULL, the query side keeps working. */
    if (atk_bridge_adaptor_init(NULL, NULL) != 0) {
        fprintf(stderr, "Planex a11y bridge: AT-SPI2 bus unavailable "
                        "â attach returns NULL (the query "
                        "side is unaffected)\n");
        g_hash_table_destroy(b->elements);
        g_object_unref(G_OBJECT(b->alert));
        g_object_unref(G_OBJECT(b->frame));
        g_object_unref(G_OBJECT(b->root));
        free(b);
        return NULL;
    }
    b->bridge_up = true;

    /* Eager listener activation (found by the orca run, round seven):
     * atk-bridge registers its ATK->D-Bus event listeners LAZILY —
     * only once an AT client has been seen (spi_atk_activate, gated
     * on the async client-discovery chain). GTK apps run a GLib main
     * loop that completes that chain; an app pumping its own loop
     * races it — and ours lost consistently: property/state signals
     * fired on the AtkObjects but the bridge never forwarded them
     * (clients saw the app, queried it, and heard nothing until
     * process death flushed the queue). The activation symbols are
     * exported by the bridge precisely for toolkit integrators; a
     * bridge that exists to serve clients activates eagerly. The
     * lazy path re-firing later is idempotent (it warns once). */
    {
        extern void spi_atk_activate(void);
        spi_atk_activate();
    }

    /* Complete the app registration (found by the orca run, round
     * two): atk-bridge DEFERS the D-Bus registration until the
     * root emits children-changed::add — GTK fires that when its
     * first toplevel appears, and until then the registration
     * stays pending (`_atk_bridge_schedule_application_registra-
     * tion` in the bridge): zero apps on the desktop, zero events
     * to clients. The mirror is complete at attach time, so
     * assemble the tree now — px_mirror_add_child fires the signal
     * AND registers the child for enumeration. The signal handler
     * is installed by the adaptor init above — emit after it. */
    px_mirror_add_child(root, b->frame);
    /* Round twelve: the frame's initial child is the ALERT alone —
     * the old code also added b->element here, which was NULL at
     * this point (the element becomes the frame two lines below),
     * seeding the children array with a NULL slot that every client
     * enumeration tripped over (g_object_ref(NULL) criticals in the
     * orca log) and desynchronizing the announced child indices
     * from the array positions. Region objects join under the frame
     * when they materialize on focus; the frame is NOT its own
     * child. */
    px_mirror_add_child(frame, b->alert);
    /* The FRAME announces itself as the active window (round four):
     * orca's default script reads state-changed:active on a frame
     * as window activation and adopts it as the active window —
     * the anchor focus events resolve into. It also holds the
     * initial FOCUS (round eleven): orca's startup locus IS the
     * frame, and making the mirror say so gives the first region
     * transition a real exit event (focused=false on the frame) —
     * without it the first focus move finds the locus already set
     * and is not announced. */
    px_mirror_apply_state(b->frame, ATK_STATE_ACTIVE, true);
    px_mirror_apply_state(b->frame, ATK_STATE_SHOWING, true);
    px_mirror_apply_state(b->frame, ATK_STATE_VISIBLE, true);
    px_mirror_apply_state(b->frame, ATK_STATE_FOCUSABLE, true);
    px_mirror_apply_state(b->frame, ATK_STATE_FOCUSED, true);
    b->element = b->frame;   /* the frame is the initial "region" */
    /* The bridge defers work onto the default GLib main context;
     * Planex apps do not run one, so pump whatever the init just
     * scheduled (non-blocking: only pending iterations). */
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }

    return b;
}

void px_a11y_bridge_atspi_detach(px_a11y_bridge* b) {
    if (!b) return;
    if (b->bridge_up) {
        /* The defunct sweep (round twelve; see px_mirror_mark_defunct):
         * every mirror object announces state-changed:defunct — the
         * GTK widget-destruction signal — and the bridge deregisters
         * each one it knows (the cache weak ref + refmap entry via
         * the object-deregistered emit, the register weak ref, the
         * D-Bus path). Child-first, the GTK destruction order. */
        if (b->elements) {
            g_hash_table_foreach(b->elements, px_mirror_mark_defunct,
                                 NULL);
        }
        px_mirror_apply_state(b->alert, ATK_STATE_DEFUNCT, true);
        px_mirror_apply_state(b->frame, ATK_STATE_DEFUNCT, true);
        px_mirror_apply_state(b->root, ATK_STATE_DEFUNCT, true);

        /* Deliberately NO atk_bridge_adaptor_cleanup() — the GTK exit
         * semantic (GTK apps never call it), adopted after the orca
         * run traced two upstream defects in it (atk-bridge 2.56):
         * (1) its cache teardown weak-unrefs the refmap VALUES —
         * always NULL, never the objects — so every entry's weak ref
         * survives as stale; (2) the teardown itself re-registers
         * objects (tidy_windows marshals Window:destroy references
         * through the registration path), seeding fresh weak refs
         * milliseconds before it frees the cache's hash tables. Both
         * leave weak notifies pointing into freed memory; the mirror
         * frees below then fired them (a detach-time SEGV under
         * orca, heap-layout dependent). Without the cleanup the
         * cache's tables stay ALIVE for the process lifetime: any
         * weak notify a mirror object still carries — stale or fresh
         * — removes itself from a live table and cannot crash. The
         * honest cost, recorded in a11y.h: the app stays registered
         * on the accessibility bus until process exit, exactly like
         * any GTK application. */
    }
    /* The root provider must not outlive the root it serves. */
    s_mirror_root = NULL;
    /* AtkObjects are GObjects; unref what we created. The element
     * table owns its objects (created lazily by flush). */
    if (b->elements) g_hash_table_destroy(b->elements);
    if (b->alert)   g_object_unref(G_OBJECT(b->alert));
    if (b->frame)   g_object_unref(G_OBJECT(b->frame));
    if (b->root)    g_object_unref(G_OBJECT(b->root));
    free(b);
}

/* ---- the flush: query side -> mirror -> D-Bus --------------------- */

void px_a11y_bridge_atspi_flush(px_a11y_bridge* b) {
    if (!b || !b->a) return;

    /* 1. Resolve the current element's REGION OBJECT (round nine).
     *    The name is the query side's identity; the table grows one
     *    object per region that has held focus. A flat single
     *    object cannot express a focus move: clients deduplicate
     *    same-object focus events, so the exit (focused=false) is
     *    always obsoleted by the entry (focused=true), the locus
     *    never transitions, and nothing is announced (the GTK
     *    control run proved orca announces per-object focus moves
     *    instantly — this reproduces that exact pattern). */
    unsigned state = px_a11y_get_state(b->a);
    const char* name = px_a11y_get_name(b->a);
    const char* value = px_a11y_get_value(b->a);
    const char* safe_name = name ? name : "";
    AtkRole role = map_role(px_a11y_get_role(b->a));

    /* Region objects materialize ON FOCUS (round eleven): before
     * the first focus the query side describes the window itself,
     * and a pre-focus region object gives clients a focusable to
     * pre-empt the locus with (orca's keyboard handler advanced to
     * it, and the first focused event then found the locus already
     * set — no announcement). The FRAME carries the window
     * identity; regions join the tree when they actually hold
     * focus. Focus LOSS still resolves (the old object's state
     * updates via push_states). */
    AtkObject* obj = NULL;
    if (state & PX_A11Y_STATE_FOCUSED) {
        obj = (AtkObject*)g_hash_table_lookup(b->elements, safe_name);
    } else if (b->element) {
        obj = b->element;   /* focus lost: states only */
    } else {
        goto pump;          /* nothing focused yet, nothing to mirror */
    }
    bool transition = (obj != b->element);
    if (!obj) {
        /* focused but never seen: materialize the region object */
        PxMirrorObject* m = px_mirror_object_new();
        if (!m) return;
        obj = (AtkObject*)m;
        atk_object_set_name(obj, safe_name);
        g_hash_table_insert(b->elements, g_strdup(safe_name), obj);
        /* published under the frame — clients see the region join
         * the window (children-changed rides along; the index is
         * the array slot, see px_mirror_add_child) */
        px_mirror_add_child((PxMirrorObject*)b->frame, obj);
    }
    if (transition) {
        /* the OLD region exits first — exactly the GTK pattern: a
         * focused=false on the outgoing object, distinct from the
         * incoming one, so no client-side dedup can eat it. */
        if (b->element && (px_a11y_get_state(b->a) &
                           PX_A11Y_STATE_FOCUSED)) {
            px_mirror_apply_state(b->element, ATK_STATE_FOCUSED,
                                  false);
        }
        b->element = obj;
    }

    /* 1b. Project the properties. Change-guarded on the same
     *     object (per-frame flushes must not spam the bus); after
     *     a transition the new object is populated unconditionally
     *     (the guards track the CURRENT object only). */
    if (transition || role != b->last_element_role) {
        b->last_element_role = role;
        atk_object_set_role(obj, role);
    }
    if (transition) {
        atk_object_set_name(obj, safe_name);
    }
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
    snprintf(desc, sizeof(desc), "value: %s%s",
             value ? value : "",
             (state & PX_A11Y_STATE_DRAGGABLE) ? " draggable" : "");
    if (transition || strcmp(desc, b->last_element_desc) != 0) {
        snprintf(b->last_element_desc,
                 sizeof(b->last_element_desc), "%s", desc);
        atk_object_set_description(obj, desc);
    }

    /* 2. State projection onto the region object's persistent set
     *    (the set notifies on real changes only). On a transition
     *    this carries the entry's focused=true — the GTK pattern. */
    push_states(obj, state);

    /* 3. Drain the announcement ring onto the ALERT object via the
     *    ANNOUNCEMENT signal (round ten): atk-bridge forwards the
     *    "announcement" signal on any AtkObject as an AT-SPI
     *    object:announcement event carrying the text, and orca's
     *    default script presents the text verbatim
     *    (on_announcement -> presentMessage). The earlier round-five
     *    translation (alert name change + SHOWING false->true pair)
     *    arrives but is NOT spoken — orca's alert presentation keys
     *    on other conditions. The alert's name still tracks the
     *    newest message for tree inspection. Identical consecutive
     *    messages announce once (documented). */
    int count = px_a11y_announcement_count(b->a);
    if (count > 0) {
        const char* newest =
            px_a11y_announcement(b->a, count - 1);
        if (newest && strcmp(newest, b->last_announcement) != 0) {
            atk_object_set_name(b->alert, newest);
            snprintf(b->last_announcement,
                     sizeof(b->last_announcement), "%s", newest);
            g_signal_emit_by_name(b->alert, "announcement", newest);
        }
    }

pump:
    /* 4. Service the bridge's D-Bus traffic (found by the orca
     *    run, round 3): GDBus dispatches INCOMING method calls —
     *    AT clients reading the mirror (GetName, GetRole, GetState,
     *    GetChildren...) — on the default GLib main context.
     *    Planex apps do not run a GLib loop; without this pump a
     *    client's read stalls until its timeout, the app is marked
     *    `The process appears to be hung`, and every subsequent
     *    event from it is dropped as coming from a DEAD app.
     *    Apps that present over the bridge should call flush
     *    regularly (the demo does it from on_tick, ~60 Hz); the
     *    pump is non-blocking — it only drains what is pending. */
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
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
