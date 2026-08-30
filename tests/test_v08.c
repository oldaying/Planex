/*
 * test_v08.c — v0.8 roadmap line-by-line verification suite
 *
 * One suite per v0.8-roadmap.md line, accumulated as lines ship:
 *
 *   A. Line 1 — the focus ring: the derived ring (regions that
 *      afford closures, in creation order), wraparound in both
 *      directions, exclusion of unfocusable regions, the
 *      Tab-from-nowhere start, and the empty ring.
 *
 *   B. Line 1 — the keyboard compile: px_afford_compile_focus as
 *      the window-free activation step, the px_key_intent value
 *      contract (label embedded — replay-safe after free), and
 *      the last-declared-first multi-edge rule matching the
 *      pointer channel's.
 *
 *   C. Line 1 — app-level keyboard routing: the exact decision
 *      px_app_run runs on PX_EV_KEY_DOWN when intent_graph is set
 *      (Tab/Shift-Tab move focus; Enter/Space compile the focused
 *      region's closure; everything else falls back to on_key).
 *
 *   D. Line 1 — channel orthogonality: ONE graph serves both
 *      channels; the pointer compile and the key compile resolve
 *      the same region to the same closure with different payload
 *      types — the A6 mechanism judge.
 *
 *   E. Line 2 — the process compile: px_afford_compile_process
 *      resolves a pointer-down to a px_interaction (the L15b
 *      retire); the px_drag_intent value contract; the pure
 *      drag-ability query.
 *
 *   F. Line 2 — form orthogonality: ONE relation, TWO resolution
 *      forms (closure + process) that never type-confuse; the
 *      dual-form rule (the process owns the down); the kind
 *      predicates; the Line 1 focus-ring pins holding.
 *
 *   G. Line 2 — process reuse + app-level routing: reset as the
 *      rearm (a stable edge target survives its second drag); the
 *      exact px_app_run pointer decision (down compiles the
 *      process, moves sample it, the release commits it); the
 *      supersede and app-cancel contracts.
 *
 * Build (or: make test_v08):
 *   cc -std=c17 -I include tests/test_v08.c \
 *      src/relation.c src/estimate.c src/closure.c src/perception.c \
 *      src/undo.c src/feedback.c src/interaction.c src/hit.c \
 *      src/a11y.c -lm -o build/test_v08
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include "planex/app.h"
#include "planex/window.h"   /* PX_MOD_* */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run  = 0;
static int g_tests_pass = 0;

#define TEST(name) do {                                  \
    g_tests_run++;                                       \
    printf("  [TEST] %-44s ", #name);                   \
    test_##name();                                       \
    printf("OK\n");                                      \
    g_tests_pass++;                                      \
} while (0)

/* ============================================================
 * Shared helpers
 * ============================================================ */

static bool eval_true(void* u) { (void)u; return true; }

/* Key-channel sink: records the last px_key_intent it received. */
typedef struct {
    px_estimate*  count;
    px_key_intent last_ki;
    int           fires;
    bool          got_payload;
} KeySink;

static void on_key_sink(px_intent i, void* u) {
    KeySink* s = (KeySink*)u;
    s->fires++;
    if (i.payload && i.payload_size == sizeof(px_key_intent)) {
        s->last_ki = *(const px_key_intent*)i.payload;
        s->got_payload = true;
        px_estimate_set(s->count,
                        px_estimate_value(s->count) + 1.0, 1.0);
    }
}

/* Focus-move recorder for the app-level routing tests. */
static char g_focus_log[8][PX_REGION_LABEL_MAX];
static int  g_focus_log_n = 0;

static void on_focus_log(const char* label, void* user) {
    (void)user;
    if (label && g_focus_log_n < 8) {
        strncpy(g_focus_log[g_focus_log_n], label, PX_REGION_LABEL_MAX - 1);
        g_focus_log[g_focus_log_n][PX_REGION_LABEL_MAX - 1] = 0;
        g_focus_log_n++;
    }
}

/* Raw-key fallback counter — the on_key stand-in. */
static int g_raw_keys = 0;
static bool on_raw_key(char key, void* user) {
    (void)key; (void)user;
    g_raw_keys++;
    return true;
}

/* A three-region fixture: two focusable buttons + an unfocusable
 * slider (affords nothing — the palette_afford boundary shape).
 * Creation order: ok > cancel > slider. Ring = [ok, cancel]. */
typedef struct {
    px_graph*   g;
    px_region*  ok;
    px_region*  cancel;
    px_region*  slider;
    px_closure* act;
    KeySink     sink;
} Ring3;

static void ring3_new(Ring3* f) {
    memset(f, 0, sizeof(*f));
    f->g = px_graph_new();
    f->sink.count = px_estimate_new(0, 1.0);
    f->act = px_closure_new("act", PX_INTENT_REQUEST,
                            on_key_sink, eval_true, &f->sink);
    f->ok     = px_region_new(px_rect_make(10, 10, 50, 20), "ok");
    f->cancel = px_region_new(px_rect_make(10, 40, 50, 20), "cancel");
    f->slider = px_region_new(px_rect_make(10, 70, 50, 20), "brightness");
    px_declare(f->g, f->ok,     PX_REL_AFFORDS, f->act);
    px_declare(f->g, f->cancel, PX_REL_AFFORDS, f->act);
    /* slider: NO afford edge — it is geometry + a drag process,
     * not a discrete act (the ADR-0017 boundary). */
}

static void ring3_free(Ring3* f) {
    px_region_free(f->ok);
    px_region_free(f->cancel);
    px_region_free(f->slider);
    px_closure_free(f->act);
    px_estimate_free(f->sink.count);
    px_graph_free(f->g);
}

/* ============================================================
 * A. Line 1 — the focus ring
 * ============================================================ */

static void test_a1_first_is_creation_order_head(void) {
    Ring3 f;
    ring3_new(&f);
    /* Creation order ok -> cancel -> slider; the ring reads the
     * registry backward, so the FIRST focusable is `ok`, not the
     * most recently declared `cancel`. */
    assert(px_afford_focus_first(f.g) == f.ok);
    ring3_free(&f);
}

static void test_a2_next_advances_and_wraps(void) {
    Ring3 f;
    ring3_new(&f);
    assert(px_afford_focus_next(f.g, f.ok) == f.cancel);
    assert(px_afford_focus_next(f.g, f.cancel) == f.ok);  /* wrap */
    ring3_free(&f);
}

static void test_a3_prev_reverses_and_wraps(void) {
    Ring3 f;
    ring3_new(&f);
    assert(px_afford_focus_prev(f.g, f.cancel) == f.ok);
    assert(px_afford_focus_prev(f.g, f.ok) == f.cancel);  /* wrap */
    ring3_free(&f);
}

static void test_a4_unfocusable_region_not_on_ring(void) {
    Ring3 f;
    ring3_new(&f);
    /* The slider affords nothing, so it is nowhere: next from it
     * normalizes to the ring head. */
    assert(px_afford_focus_next(f.g, f.slider) == f.ok);
    assert(px_afford_focus_prev(f.g, f.slider) == f.ok);
    ring3_free(&f);
}

static void test_a5_null_from_is_ring_head(void) {
    Ring3 f;
    ring3_new(&f);
    /* Tab-from-nowhere: the first Tab focuses the first focusable
     * region — the keyboard user starts at the top of the ring. */
    assert(px_afford_focus_next(f.g, NULL) == f.ok);
    assert(px_afford_focus_prev(f.g, NULL) == f.ok);
    ring3_free(&f);
}

static void test_a6_empty_ring_returns_null(void) {
    px_graph* g = px_graph_new();
    px_region* r = px_region_new(px_rect_make(0, 0, 10, 10), "lone");
    /* No AFFORDS edge anywhere: the ring is empty. */
    assert(px_afford_focus_first(g) == NULL);
    assert(px_afford_focus_next(g, NULL) == NULL);
    assert(px_afford_focus_next(g, r) == NULL);
    px_region_free(r);
    px_graph_free(g);
}

static void test_a7_freed_from_normalizes_to_head(void) {
    Ring3 f;
    ring3_new(&f);
    px_region* gone = px_region_new(px_rect_make(0, 0, 5, 5), "gone");
    px_declare(f.g, gone, PX_REL_AFFORDS, f.act);
    /* Ring is now [ok, cancel, gone] in creation order: `gone` is
     * the tail, so next wraps to the head. */
    assert(px_afford_focus_next(f.g, gone) == f.ok);
    px_region_free(gone);
    /* The pointer is dead; the walk must not dereference it — a
     * freed `from` is nowhere, and nowhere resolves to the head. */
    assert(px_afford_focus_next(f.g, gone) == f.ok);
    ring3_free(&f);
}

/* ============================================================
 * B. Line 1 — the keyboard compile (value contract)
 * ============================================================ */

static void test_b1_compile_focus_resolves_closure_and_payload(void) {
    Ring3 f;
    ring3_new(&f);
    px_key_intent ki;
    px_closure* c = px_afford_compile_focus(f.g, f.ok, '\r', &ki);
    assert(c == f.act);
    assert(strcmp(ki.region, "ok") == 0);
    assert(ki.key == '\r');   /* context, not a routing key */

    /* Trigger with the compiled payload — the act never saw a
     * coordinate or a window. */
    px_closure_trigger(c, &ki, sizeof(ki));
    assert(f.sink.fires == 1 && f.sink.got_payload);
    assert(strcmp(f.sink.last_ki.region, "ok") == 0);
    assert(px_estimate_value(f.sink.count) == 1.0);
    ring3_free(&f);
}

static void test_b2_compile_focus_miss_zeroes_payload(void) {
    Ring3 f;
    ring3_new(&f);
    px_key_intent ki;
    ki.region[0] = 'X'; ki.key = 99;   /* poisoned */
    assert(px_afford_compile_focus(f.g, NULL, '\r', &ki) == NULL);
    assert(ki.region[0] == 0 && ki.key == 0);   /* zeroed on miss */
    /* An unfocusable region is a miss too. */
    assert(px_afford_compile_focus(f.g, f.slider, ' ', &ki) == NULL);
    assert(ki.region[0] == 0 && ki.key == 0);
    ring3_free(&f);
}

static void test_b3_intent_survives_region_free(void) {
    Ring3 f;
    ring3_new(&f);
    px_key_intent ki;
    px_closure* c = px_afford_compile_focus(f.g, f.ok, '\r', &ki);
    assert(c == f.act);
    px_region_free(f.ok);          /* the region dies... */
    /* ...the intent is still a value: label embedded, replay-safe. */
    px_closure_trigger(c, &ki, sizeof(ki));
    assert(f.sink.fires == 1);
    assert(strcmp(f.sink.last_ki.region, "ok") == 0);
    f.ok = NULL;                   /* ring3_free must not double-free */
    px_region* shield = px_region_new(px_rect_make(0, 0, 1, 1), "shield");
    (void)shield;                  /* keep the registry non-empty */
    ring3_free(&f);
    px_region_free(shield);
}

static void test_b4_multi_edge_last_declared_first(void) {
    px_graph* g = px_graph_new();
    KeySink s1 = {0}, s2 = {0};
    s1.count = px_estimate_new(0, 1.0);
    s2.count = px_estimate_new(0, 1.0);
    px_closure* first  = px_closure_new("first",  PX_INTENT_REQUEST,
                                        on_key_sink, eval_true, &s1);
    px_closure* second = px_closure_new("second", PX_INTENT_REQUEST,
                                        on_key_sink, eval_true, &s2);
    px_region* r = px_region_new(px_rect_make(0, 0, 10, 10), "multi");
    px_declare(g, r, PX_REL_AFFORDS, first);
    px_declare(g, r, PX_REL_AFFORDS, second);   /* declared later */

    /* Same rule the pointer channel pins (test_v07 a7): the LAST
     * declaration wins — one ring, one rule, two channels. */
    px_key_intent ki;
    px_closure* c = px_afford_compile_focus(g, r, '\r', &ki);
    assert(c == second);

    /* The pointer compile agrees. */
    px_pointer_intent pi;
    assert(px_afford_compile(g, 5, 5, 1, &pi) == second);

    px_region_free(r);
    px_closure_free(first);
    px_closure_free(second);
    px_estimate_free(s1.count);
    px_estimate_free(s2.count);
    px_graph_free(g);
}

/* ============================================================
 * C. Line 1 — app-level keyboard routing
 *
 * The exact decision px_app_run runs on PX_EV_KEY_DOWN (v0.8
 * Line 1), verbatim: compile first (focus move or activation),
 * raw-key fallback otherwise. `focus` is the loop-local state.
 * ============================================================ */

#define APP_KEY_ROUTE(desc, k, mods)                                        \
    do {                                                                    \
        char _k = (k);                                                      \
        if ((desc)->intent_graph) {                                         \
            if (_k == '\t') {                                               \
                px_region* _n = ((mods) & PX_MOD_SHIFT)                     \
                    ? px_afford_focus_prev((desc)->intent_graph, focus)     \
                    : px_afford_focus_next((desc)->intent_graph, focus);    \
                if (_n && _n != focus) {                                    \
                    focus = _n;                                             \
                    if ((desc)->on_focus)                                   \
                        (desc)->on_focus(px_region_label(focus),            \
                                         (desc)->user);                     \
                    break;                                                  \
                }                                                           \
                if (_n && _n == focus && (desc)->on_focus) {                \
                    (desc)->on_focus(px_region_label(focus),                \
                                     (desc)->user);                         \
                    break;                                                  \
                }                                                           \
            } else if (_k == '\r' || _k == '\n' || _k == ' ') {             \
                px_key_intent _ki;                                          \
                px_closure* _c = px_afford_compile_focus(                   \
                    (desc)->intent_graph, focus, _k, &_ki);                 \
                if (_c) {                                                   \
                    px_closure_trigger(_c, &_ki, sizeof(_ki));              \
                    break;                                                  \
                }                                                           \
            }                                                               \
        }                                                                   \
        if ((desc)->on_key) on_raw_key(_k, (desc)->user);                   \
    } while (0)

static void test_c1_tab_walks_focus_forward(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc d = {0};
    d.intent_graph = f.g;
    d.on_focus = on_focus_log;
    g_focus_log_n = 0;

    px_region* focus = NULL;   /* the loop-local state */
    APP_KEY_ROUTE(&d, '\t', 0);   /* nowhere -> ok      */
    APP_KEY_ROUTE(&d, '\t', 0);   /* ok -> cancel       */
    APP_KEY_ROUTE(&d, '\t', 0);   /* cancel -> ok (wrap)*/

    assert(g_focus_log_n == 3);
    assert(strcmp(g_focus_log[0], "ok") == 0);
    assert(strcmp(g_focus_log[1], "cancel") == 0);
    assert(strcmp(g_focus_log[2], "ok") == 0);
    ring3_free(&f);
}

static void test_c2_shift_tab_reverses(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc d = {0};
    d.intent_graph = f.g;
    d.on_focus = on_focus_log;
    g_focus_log_n = 0;

    px_region* focus = NULL;
    APP_KEY_ROUTE(&d, '\t', 0);            /* -> ok     */
    APP_KEY_ROUTE(&d, '\t', 0);            /* -> cancel */
    APP_KEY_ROUTE(&d, '\t', PX_MOD_SHIFT); /* cancel -> ok (reverse) */

    assert(g_focus_log_n == 3);
    assert(strcmp(g_focus_log[2], "ok") == 0);
    ring3_free(&f);
}

static void test_c3_enter_compiles_focused_closure(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc d = {0};
    d.intent_graph = f.g;
    d.on_key = on_raw_key;
    g_raw_keys = 0;

    px_region* focus = NULL;
    APP_KEY_ROUTE(&d, '\t', 0);       /* focus ok            */
    APP_KEY_ROUTE(&d, '\r', 0);       /* Enter: compile+fire */

    assert(f.sink.fires == 1);
    assert(strcmp(f.sink.last_ki.region, "ok") == 0);
    assert(f.sink.last_ki.key == '\r');
    assert(g_raw_keys == 0);          /* never reached on_key */
    ring3_free(&f);
}

static void test_c4_space_activates_too(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc d = {0};
    d.intent_graph = f.g;

    px_region* focus = NULL;
    APP_KEY_ROUTE(&d, '\t', 0);
    APP_KEY_ROUTE(&d, ' ', 0);        /* Space: the other activation key */

    assert(f.sink.fires == 1);
    assert(f.sink.last_ki.key == ' ');
    ring3_free(&f);
}

static void test_c5_activation_without_focus_falls_back(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc d = {0};
    d.intent_graph = f.g;
    d.on_key = on_raw_key;
    g_raw_keys = 0;

    px_region* focus = NULL;          /* never Tabbed onto the ring */
    APP_KEY_ROUTE(&d, '\r', 0);       /* Enter with no focus */

    assert(f.sink.fires == 0);        /* nothing compiled  */
    assert(g_raw_keys == 1);          /* the raw fallback  */
    ring3_free(&f);
}

static void test_c6_legacy_dispatch_unchanged(void) {
    Ring3 f;
    ring3_new(&f);
    px_app_desc legacy = {0};         /* no intent_graph */
    legacy.on_key = on_raw_key;
    g_raw_keys = 0;

    px_region* focus = NULL;
    APP_KEY_ROUTE(&legacy, '\t', 0);
    APP_KEY_ROUTE(&legacy, '\r', 0);
    APP_KEY_ROUTE(&legacy, 'x', 0);

    /* Opt-out: every key takes the raw path, nothing compiles. */
    assert(g_raw_keys == 3);
    assert(f.sink.fires == 0);
    ring3_free(&f);
}

/* ============================================================
 * D. Line 1 — channel orthogonality (the A6 judge)
 * ============================================================ */

static void test_d1_one_graph_two_channels_one_closure(void) {
    px_graph* g = px_graph_new();
    KeySink ks = {0};  ks.count = px_estimate_new(0, 1.0);

    px_region* r = px_region_new(px_rect_make(0, 0, 40, 20), "dual");

    /* ONE closure serving BOTH channels: the app registers one act,
     * not one per channel. This is the whole A6 claim at mechanism
     * level — the channel is a projection, the ontology is one
     * graph, and the act sorts channels by payload shape. */
    px_closure* dual_act = px_closure_new("dual act", PX_INTENT_REQUEST,
                                          on_key_sink, eval_true, &ks);
    px_declare(g, r, PX_REL_AFFORDS, dual_act);

    /* Pointer channel compiles position -> (closure, px_pointer_intent). */
    px_pointer_intent pi;
    assert(px_afford_compile(g, 20, 10, 1, &pi) == dual_act);
    assert(strcmp(pi.region, "dual") == 0);

    /* Keyboard channel compiles focus -> (closure, px_key_intent). */
    px_key_intent ki;
    assert(px_afford_compile_focus(g, r, '\r', &ki) == dual_act);
    assert(strcmp(ki.region, "dual") == 0);

    /* Both intents carry the SAME routing key — the region label —
     * and different context payloads. One graph, two channels. */
    assert(strcmp(pi.region, ki.region) == 0);

    px_region_free(r);
    px_closure_free(dual_act);
    px_estimate_free(ks.count);
    px_graph_free(g);
}

/* ============================================================
 * E. Line 2 — the process compile (px_afford_compile_process)
 *
 * The L15b retire: a pointer-down on a region that affords a
 * PROCESS (an AFFORDS edge targeting a px_interaction) compiles
 * to that process — drag-ability becomes graph data.
 * ============================================================ */

/* A three-region drag fixture:
 *   chip    (0,0,60,24)    DUAL form   — select closure + chip_drag
 *   slider  (0,40,60,24)   process only — slider_drag (no closure)
 *   button  (0,80,60,24)   closure only — the v0.7 control
 * Creation order: chip > slider > button. */
typedef struct {
    px_graph*       g;
    px_region*      chip;
    px_region*      slider;
    px_region*      button;
    px_interaction* chip_drag;
    px_interaction* slider_drag;
    px_closure*     select;
    px_closure*     click;
    int             sel_fires;
    int             clk_fires;
    int             begins;
    int             commits;
    int             cancels;
} Drag3;

static void on_count_fire(px_intent i, void* u) {
    (void)i;
    (*(int*)u)++;
}

static void drag_phase_counter(px_interaction* it, px_int_phase ph, void* u) {
    (void)it;
    Drag3* f = (Drag3*)u;
    if (ph == PX_INT_BEGAN)          f->begins++;
    else if (ph == PX_INT_COMMITTED) f->commits++;
    else if (ph == PX_INT_CANCELLED) f->cancels++;
}

static void drag3_new(Drag3* f) {
    memset(f, 0, sizeof(*f));
    f->g = px_graph_new();
    f->chip   = px_region_new(px_rect_make(0, 0, 60, 24), "chip");
    f->slider = px_region_new(px_rect_make(0, 40, 60, 24), "brightness");
    f->button = px_region_new(px_rect_make(0, 80, 60, 24), "button");

    f->select = px_closure_new("select", PX_INTENT_REQUEST,
                               on_count_fire, eval_true, &f->sel_fires);
    f->click  = px_closure_new("click", PX_INTENT_REQUEST,
                               on_count_fire, eval_true, &f->clk_fires);

    f->chip_drag   = px_interaction_new("chip drag", 32);
    f->slider_drag = px_interaction_new("slider drag", 32);
    px_interaction_on_phase(f->chip_drag,   drag_phase_counter, f);
    px_interaction_on_phase(f->slider_drag, drag_phase_counter, f);

    /* Declaration order matters for the last-declared-first pins:
     * the process edges are declared AFTER the closure edges, so a
     * kind-blind resolver (the pre-v0.8 code) would miscast the
     * process pointer as a closure. The kind filter must hold. */
    px_declare(f->g, f->chip,   PX_REL_AFFORDS, f->select);
    px_declare(f->g, f->chip,   PX_REL_AFFORDS, f->chip_drag);
    px_declare(f->g, f->slider, PX_REL_AFFORDS, f->slider_drag);
    px_declare(f->g, f->button, PX_REL_AFFORDS, f->click);
}

static void drag3_free(Drag3* f) {
    px_region_free(f->chip);
    px_region_free(f->slider);
    px_region_free(f->button);
    px_interaction_free(f->chip_drag);
    px_interaction_free(f->slider_drag);
    px_closure_free(f->select);
    px_closure_free(f->click);
    px_graph_free(f->g);
}

static void test_e1_compile_process_resolves_region_to_process(void) {
    Drag3 f;
    drag3_new(&f);

    /* The press on the slider compiles to its process — the begin
     * seam is gone: which regions drag is a graph query, not app
     * branching. The payload embeds the label, position, button. */
    px_drag_intent di;
    px_interaction* p = px_afford_compile_process(f.g, 30, 52, 1, &di);
    assert(p == f.slider_drag);
    assert(strcmp(di.region, "brightness") == 0);
    assert(di.x == 30 && di.y == 52 && di.button == 1);

    /* The chip (dual form) compiles to its process too. */
    px_interaction* p2 = px_afford_compile_process(f.g, 30, 12, 1, &di);
    assert(p2 == f.chip_drag);
    assert(strcmp(di.region, "chip") == 0);

    drag3_free(&f);
}

static void test_e2_compile_process_miss_zeroes_payload(void) {
    Drag3 f;
    drag3_new(&f);

    /* Empty space: no region, no compile. */
    px_drag_intent di;
    memset(&di, 'x', sizeof(di));   /* stale garbage */
    px_interaction* p = px_afford_compile_process(f.g, 300, 300, 1, &di);
    assert(p == NULL);
    assert(di.region[0] == 0 && di.x == 0 && di.y == 0 && di.button == 0);

    /* The button affords only a closure: the process form misses,
     * zeroed payload — a stale value can never leak through the
     * fallback (the same contract as the pointer/key compiles). */
    memset(&di, 'x', sizeof(di));
    p = px_afford_compile_process(f.g, 30, 92, 1, &di);
    assert(p == NULL);
    assert(di.region[0] == 0 && di.x == 0 && di.y == 0 && di.button == 0);

    drag3_free(&f);
}

static void test_e3_multi_edge_last_declared_first(void) {
    px_graph* g = px_graph_new();
    px_region* r = px_region_new(px_rect_make(0, 0, 60, 24), "multi");
    px_interaction* p1 = px_interaction_new("p1", 8);
    px_interaction* p2 = px_interaction_new("p2", 8);

    /* Two process edges on one region: the LAST declared wins — the
     * same resolution rule the closure form pins (test_v07 a7);
     * one graph, one rule, two forms. */
    px_declare(g, r, PX_REL_AFFORDS, p1);
    px_declare(g, r, PX_REL_AFFORDS, p2);

    px_drag_intent di;
    assert(px_afford_compile_process(g, 30, 12, 1, &di) == p2);

    px_region_free(r);
    px_interaction_free(p1);
    px_interaction_free(p2);
    px_graph_free(g);
}

static void test_e4_intent_survives_region_free(void) {
    Drag3 f;
    drag3_new(&f);

    /* The value contract: the label is EMBEDDED, so the compile
     * product survives the region's death — same construction as
     * px_pointer_intent (test_v07 a3) and px_key_intent (b3). */
    px_drag_intent di;
    assert(px_afford_compile_process(f.g, 30, 52, 1, &di) == f.slider_drag);
    px_drag_intent copy = di;

    px_region_free(f.slider);   /* the label's owner dies */
    f.slider = NULL;

    assert(strcmp(copy.region, "brightness") == 0);

    /* The registry is unaffected: the process is still itself. */
    assert(px_is_interaction(f.slider_drag));

    drag3_free(&f);
}

static void test_e5_region_affords_process_query(void) {
    Drag3 f;
    drag3_new(&f);

    /* Drag-ability as a pure graph query — the reader the a11y
     * projection (PX_A11Y_STATE_DRAGGABLE) and the corpus evidence
     * use. */
    assert(px_region_affords_process(f.g, f.chip));    /* dual     */
    assert(px_region_affords_process(f.g, f.slider));  /* process  */
    assert(!px_region_affords_process(f.g, f.button)); /* closure  */
    assert(!px_region_affords_process(f.g, NULL));     /* nowhere  */
    assert(!px_region_affords_process(NULL, f.chip));  /* no graph */

    drag3_free(&f);
}

static int g_raw_clicks = 0;
static int g_raw_moves = 0;
static int g_raw_ups = 0;

static bool on_raw_click(int x, int y, void* u) {
    (void)x; (void)y; (void)u;
    g_raw_clicks++;
    return true;
}
static bool on_raw_move(int x, int y, void* u) {
    (void)x; (void)y; (void)u;
    g_raw_moves++;
    return true;
}
static bool on_raw_up(int x, int y, void* u) {
    (void)x; (void)y; (void)u;
    g_raw_ups++;
    return true;
}

#define APP_PTR_DOWN(desc, x_, y_, btn)                                     \
    do {                                                                    \
        if ((desc)->intent_graph) {                                         \
            px_drag_intent _di;                                             \
            px_interaction* _p = px_afford_compile_process(                 \
                (desc)->intent_graph, (double)(x_), (double)(y_),           \
                (btn), &_di);                                               \
            if (_p) {                                                       \
                if (active) {                                               \
                    px_interaction_cancel(active,                           \
                                          "superseded by a new press");     \
                    active = NULL;                                          \
                }                                                           \
                px_interaction_reset(_p);                                   \
                px_interaction_begin(_p);                                   \
                px_int_sample _s = { px_now_ms(), _di.x, _di.y, 0.0,        \
                                     _di.button, 0 };                       \
                px_interaction_sample(_p, &_s);                             \
                active = _p;                                                \
                break;                                                      \
            }                                                               \
            px_pointer_intent _pi;                                          \
            px_closure* _c = px_afford_compile(                             \
                (desc)->intent_graph, (double)(x_), (double)(y_),           \
                (btn), &_pi);                                               \
            if (_c) {                                                       \
                px_closure_trigger(_c, &_pi, sizeof(_pi));                  \
                break;                                                      \
            }                                                               \
        }                                                                   \
        if ((desc)->on_click) (desc)->on_click((x_), (y_), (desc)->user);   \
    } while (0)

#define APP_PTR_MOVE(desc, x_, y_)                                          \
    do {                                                                    \
        if (active) {                                                       \
            px_int_phase _ph = px_interaction_phase(active);                \
            if (_ph == PX_INT_COMMITTED || _ph == PX_INT_CANCELLED) {       \
                active = NULL;                                              \
            } else {                                                        \
                px_int_sample _s = { px_now_ms(), (double)(x_),             \
                                     (double)(y_), 0.0, 0, 0 };             \
                px_interaction_sample(active, &_s);                         \
                break;                                                      \
            }                                                               \
        }                                                                   \
        if ((desc)->on_mouse_move)                                          \
            (desc)->on_mouse_move((x_), (y_), (desc)->user);                \
    } while (0)

#define APP_PTR_UP(desc, x_, y_)                                            \
    do {                                                                    \
        if (active) {                                                       \
            px_int_phase _ph = px_interaction_phase(active);                \
            if (_ph != PX_INT_COMMITTED && _ph != PX_INT_CANCELLED) {       \
                px_int_sample _s = { px_now_ms(), (double)(x_),             \
                                     (double)(y_), 0.0, 0, 0 };             \
                px_interaction_sample(active, &_s);                         \
                px_interaction_commit(active);                              \
            }                                                               \
            active = NULL;                                                  \
            break;                                                          \
        }                                                                   \
        if ((desc)->on_mouse_up) (desc)->on_mouse_up((x_), (y_), (desc)->user); \
    } while (0)

/* ============================================================
 * F. Line 2 — form orthogonality (one graph, two forms)
 * ============================================================ */

static void test_f1_closure_compile_skips_process_targets(void) {
    Drag3 f;
    drag3_new(&f);

    /* The chip's AFFORDS edges: [select (closure), chip_drag
     * (process)] — newest-first, the PROCESS edge is on top. The
     * closure form must resolve select anyway: kind-filtered, the
     * forms never type-confuse. (The pre-v0.8 resolver cast the
     * first non-NULL target — exactly this layout would have
     * handed back the process pointer as a px_closure*.) */
    px_pointer_intent pi;
    assert(px_afford_compile(f.g, 30, 12, 1, &pi) == f.select);
    assert(strcmp(pi.region, "chip") == 0);

    /* And the process form resolves the process on the SAME edges. */
    px_drag_intent di;
    assert(px_afford_compile_process(f.g, 30, 12, 1, &di) == f.chip_drag);

    /* The slider affords ONLY a process: the closure form misses —
     * NULL, not a miscast process pointer. This is the slider's
     * pre-v0.8 "unresolved click" boundary, now precise. */
    assert(px_afford_at(f.g, 30, 52) == NULL);

    /* Odd declarations (an estimate on an AFFORDS edge) resolve
     * nothing in either form — safer than the blind cast too. */
    px_estimate* est = px_estimate_new(0, 1.0);
    px_declare(f.g, f.button, PX_REL_AFFORDS, est);
    px_closure* c = px_afford_compile(f.g, 30, 92, 1, &pi);
    assert(c == f.click);   /* the estimate edge is skipped, the
                             * closure edge still resolves */
    assert(px_afford_compile_process(f.g, 30, 92, 1, &di) == NULL);

    px_estimate_free(est);
    drag3_free(&f);
}

static void test_f2_dual_form_process_owns_the_down(void) {
    Drag3 f;
    drag3_new(&f);

    px_app_desc d = {0};
    d.intent_graph = f.g;
    px_interaction* active = NULL;   /* the loop-local state */

    /* The v0.7 control FIRST: the button affords only a closure —
     * the down triggers it immediately, no process ever begins. */
    APP_PTR_DOWN(&d, 30, 92, 1);
    assert(f.clk_fires == 1);
    assert(f.begins == 0 && active == NULL);

    /* The dual-form chip: the process owns the down. The press is
     * genuinely ambiguous (tap vs drag); only the trajectory
     * resolves it — the closure does NOT fire on the down. */
    APP_PTR_DOWN(&d, 30, 12, 1);
    assert(active == f.chip_drag);
    assert(f.begins == 1);
    assert(f.sel_fires == 0);   /* reachable via the commit bridge,
                                 * not via the down */

    drag3_free(&f);
}

static void test_f3_kind_predicates_discriminate(void) {
    Drag3 f;
    drag3_new(&f);

    /* Registry-backed identity: no type punning, no dereference. */
    assert(px_is_interaction(f.chip_drag));
    assert(px_is_interaction(f.slider_drag));
    assert(!px_is_interaction(f.select));       /* a closure is not  */
    assert(!px_is_interaction(NULL));
    assert(!px_is_interaction(f.g));            /* a graph is not    */

    assert(px_is_closure(f.select));
    assert(px_is_closure(f.click));
    assert(!px_is_closure(f.chip_drag));        /* a process is not  */
    assert(!px_is_closure(NULL));

    /* Estimates and regions are neither kind. */
    px_estimate* est = px_estimate_new(0, 1.0);
    assert(!px_is_interaction(est) && !px_is_closure(est));
    assert(!px_is_interaction(f.chip) && !px_is_closure(f.chip));

    /* Freed objects leave the registry: no stale identity. */
    px_interaction* tmp = px_interaction_new("tmp", 4);
    assert(px_is_interaction(tmp));
    px_interaction_free(tmp);
    assert(!px_is_interaction(tmp));

    px_estimate_free(est);
    drag3_free(&f);
}

static void test_f4_focus_ring_pins_hold_under_process_edges(void) {
    Drag3 f;
    drag3_new(&f);

    /* Line 1's pinned semantics, unchanged by the process form:
     * focusable = affords at least one CLOSURE. The slider
     * (process-only) stays off the ring — keyboard
     * process-activation does not exist yet (ADR-0021 CAVEATS). */
    assert(px_afford_focus_first(f.g) == f.chip);   /* creation order */
    assert(px_afford_focus_next(f.g, f.chip) == f.button);
    assert(px_afford_focus_next(f.g, f.button) == f.chip);  /* wrap,
                                        slider skipped by the ring */

    /* The ring is [chip, button]: walk it and confirm. */
    px_region* ring[2] = { f.chip, f.button };
    px_region* cur = NULL;
    for (int i = 0; i < 4; i++) {
        cur = px_afford_focus_next(f.g, cur);
        assert(cur == ring[i % 2]);
    }

    drag3_free(&f);
}

/* ============================================================
 * G. Line 2 — process reuse + app-level routing
 *
 * The exact decision px_app_run runs on pointer events when
 * intent_graph is set (v0.8 Line 2), replicated macro-for-macro:
 * the process form first (reset + begin + press sample), moves
 * sample the active process, the release commits it, and the
 * raw callbacks (on_click / on_mouse_move / on_mouse_up) are the
 * fallback — the fallback, not the path.
 * ============================================================ */

/* A generic phase counter for the reuse tests (user = PhaseCount). */
typedef struct { int begins, commits, cancels; } PhaseCount;

static void phase_count_hook(px_interaction* it, px_int_phase p, void* u) {
    (void)it;
    PhaseCount* c = (PhaseCount*)u;
    if (p == PX_INT_BEGAN)          c->begins++;
    else if (p == PX_INT_COMMITTED) c->commits++;
    else if (p == PX_INT_CANCELLED) c->cancels++;
}

static void test_g1_reset_rearms_after_terminal(void) {
    /* A process that AFFORDS a region is a stable edge target — the
     * slider must survive its second drag. begin() on a terminal
     * process is a no-op (the outcome is final); reset is the
     * rearm, and it KEEPS everything bound. */
    PhaseCount pc = {0, 0, 0};
    int bridge_fires = 0;
    px_interaction* it = px_interaction_new("reusable", 8);
    px_closure* bridge = px_closure_new("commit bridge", PX_INTENT_REQUEST,
                                        on_count_fire, eval_true,
                                        &bridge_fires);

    px_interaction_on_phase(it, phase_count_hook, &pc);
    px_interaction_on_commit(it, bridge, NULL, 0);

    px_int_sample s1 = { 100.0, 10, 10, 0, 1, 0 };
    px_int_sample s2 = { 110.0, 20, 10, 0, 0, 0 };
    px_interaction_sample(it, &s1);          /* auto-begins */
    px_interaction_sample(it, &s2);
    px_interaction_commit(it);
    assert(px_interaction_phase(it) == PX_INT_COMMITTED);
    assert(px_interaction_stored(it) == 2);
    assert(bridge_fires == 1);
    assert(pc.begins == 1 && pc.commits == 1);

    /* Terminal is final: begin is a no-op on COMMITTED. */
    px_interaction_begin(it);
    assert(px_interaction_phase(it) == PX_INT_COMMITTED);

    /* The rearm: IDLE, trajectory cleared, bindings kept. */
    px_interaction_reset(it);
    assert(px_interaction_phase(it) == PX_INT_IDLE);
    assert(px_interaction_stored(it) == 0);
    assert(px_interaction_total(it) == 0);

    /* The second gesture works on the SAME object — and the commit
     * bridge survived the reset (bound at bind time, not per
     * gesture). */
    px_interaction_begin(it);
    assert(px_interaction_phase(it) == PX_INT_BEGAN);
    assert(pc.begins == 2);          /* the hook survived the reset */
    px_interaction_sample(it, &s1);
    px_interaction_sample(it, &s2);
    px_interaction_commit(it);
    assert(px_interaction_phase(it) == PX_INT_COMMITTED);
    assert(px_interaction_stored(it) == 2);
    assert(bridge_fires == 2);   /* the bridge fired again */
    assert(pc.commits == 2);

    /* Reset also clears a cancel reason. */
    px_interaction_reset(it);
    px_interaction_begin(it);
    px_interaction_cancel(it, "escape");
    assert(strcmp(px_interaction_cancel_reason(it), "escape") == 0);
    px_interaction_reset(it);
    assert(px_interaction_cancel_reason(it)[0] == 0);  /* cleared,
                                          not NULL — the reason array
                                          lives with the process */

    px_closure_free(bridge);
    px_interaction_free(it);
}

static void test_g2_app_routing_down_move_up(void) {
    Drag3 f;
    drag3_new(&f);
    g_raw_clicks = g_raw_moves = g_raw_ups = 0;

    px_app_desc d = {0};
    d.intent_graph   = f.g;
    d.on_click       = on_raw_click;
    d.on_mouse_move  = on_raw_move;
    d.on_mouse_up    = on_raw_up;
    px_interaction* active = NULL;

    /* The press on the slider compiles to the process: reset +
     * begin + the press is the first trajectory sample. */
    APP_PTR_DOWN(&d, 30, 52, 1);
    assert(active == f.slider_drag);
    assert(px_interaction_phase(f.slider_drag) == PX_INT_ACTIVE);
    assert(px_interaction_stored(f.slider_drag) == 1);
    const px_int_sample* first =
        px_interaction_at(f.slider_drag, 0);
    assert(first->x == 30 && first->y == 52 && first->button == 1);

    /* Moves SAMPLE the process — the inert hot path. The raw move
     * callback does not fire: the process owns the gesture. */
    APP_PTR_MOVE(&d, 40, 52);
    APP_PTR_MOVE(&d, 50, 52);
    assert(px_interaction_stored(f.slider_drag) == 3);
    assert(g_raw_moves == 0);

    /* The release: last sample + COMMIT — the app's bridges fire
     * once, with the whole trajectory readable. */
    APP_PTR_UP(&d, 60, 52);
    assert(px_interaction_phase(f.slider_drag) == PX_INT_COMMITTED);
    assert(px_interaction_stored(f.slider_drag) == 4);
    assert(px_interaction_last(f.slider_drag)->x == 60.0);
    assert(f.commits >= 1);       /* the phase hook saw the commit */
    assert(active == NULL);        /* the stream is released       */
    assert(g_raw_ups == 0 && g_raw_clicks == 0);

    /* After the gesture, plain moves take the raw path again. */
    APP_PTR_MOVE(&d, 70, 52);
    assert(g_raw_moves == 1);

    drag3_free(&f);
}

static void test_g3_superseded_press_cancels_active(void) {
    Drag3 f;
    drag3_new(&f);

    px_app_desc d = {0};
    d.intent_graph = f.g;
    px_interaction* active = NULL;

    /* A drag begins on the slider... */
    APP_PTR_DOWN(&d, 30, 52, 1);
    assert(active == f.slider_drag);
    APP_PTR_MOVE(&d, 40, 52);

    /* ...and a new press lands mid-gesture: one pointer, one
     * gesture — the active process is CANCELLED with the reason,
     * its bridges fire, and the new press routes normally. */
    APP_PTR_DOWN(&d, 30, 12, 1);
    assert(px_interaction_phase(f.slider_drag) == PX_INT_CANCELLED);
    assert(strcmp(px_interaction_cancel_reason(f.slider_drag),
                  "superseded by a new press") == 0);
    assert(active == f.chip_drag);   /* the new gesture owns the
                                      * pointer stream now */

    drag3_free(&f);
}

static void test_g4_app_cancel_releases_the_stream(void) {
    Drag3 f;
    drag3_new(&f);
    g_raw_moves = g_raw_ups = 0;

    px_app_desc d = {0};
    d.intent_graph  = f.g;
    d.on_mouse_move = on_raw_move;
    d.on_mouse_up   = on_raw_up;
    px_interaction* active = NULL;

    /* The app owns the process object: it may cancel from its own
     * key handler, its timer, wherever. The framework's contract:
     * a move/up that finds the process terminal drops it and falls
     * through to normal routing — no zombie capture. */
    APP_PTR_DOWN(&d, 30, 52, 1);
    assert(active == f.slider_drag);

    px_interaction_cancel(f.slider_drag, "app escape");
    assert(px_interaction_phase(f.slider_drag) == PX_INT_CANCELLED);

    /* The next move sees the terminal process, releases the
     * stream, and takes the raw path (both moves below). */
    APP_PTR_MOVE(&d, 40, 52);
    APP_PTR_MOVE(&d, 50, 52);
    assert(g_raw_moves == 2);
    assert(px_interaction_phase(f.slider_drag) == PX_INT_CANCELLED);

    /* The up is raw too — no double terminal on a cancelled
     * process. */
    APP_PTR_UP(&d, 60, 52);
    assert(g_raw_ups == 1);
    assert(px_interaction_phase(f.slider_drag) == PX_INT_CANCELLED);

    drag3_free(&f);
}

/* ============================================================
 * main
 * ============================================================ */

int main(void) {
    printf("test_v08 — v0.8 roadmap line verification\n");
    printf("==========================================\n\n");

    printf("A. the focus ring (px_afford_focus_*)\n");
    TEST(a1_first_is_creation_order_head);
    TEST(a2_next_advances_and_wraps);
    TEST(a3_prev_reverses_and_wraps);
    TEST(a4_unfocusable_region_not_on_ring);
    TEST(a5_null_from_is_ring_head);
    TEST(a6_empty_ring_returns_null);
    TEST(a7_freed_from_normalizes_to_head);

    printf("\nB. the keyboard compile (px_afford_compile_focus)\n");
    TEST(b1_compile_focus_resolves_closure_and_payload);
    TEST(b2_compile_focus_miss_zeroes_payload);
    TEST(b3_intent_survives_region_free);
    TEST(b4_multi_edge_last_declared_first);

    printf("\nC. app-level keyboard routing (the px_app_run decision)\n");
    TEST(c1_tab_walks_focus_forward);
    TEST(c2_shift_tab_reverses);
    TEST(c3_enter_compiles_focused_closure);
    TEST(c4_space_activates_too);
    TEST(c5_activation_without_focus_falls_back);
    TEST(c6_legacy_dispatch_unchanged);

    printf("\nD. channel orthogonality (A6 at mechanism level)\n");
    TEST(d1_one_graph_two_channels_one_closure);

    printf("\nE. the process compile (px_afford_compile_process)\n");
    TEST(e1_compile_process_resolves_region_to_process);
    TEST(e2_compile_process_miss_zeroes_payload);
    TEST(e3_multi_edge_last_declared_first);
    TEST(e4_intent_survives_region_free);
    TEST(e5_region_affords_process_query);

    printf("\nF. form orthogonality (one graph, two forms)\n");
    TEST(f1_closure_compile_skips_process_targets);
    TEST(f2_dual_form_process_owns_the_down);
    TEST(f3_kind_predicates_discriminate);
    TEST(f4_focus_ring_pins_hold_under_process_edges);

    printf("\nG. process reuse + app-level pointer routing\n");
    TEST(g1_reset_rearms_after_terminal);
    TEST(g2_app_routing_down_move_up);
    TEST(g3_superseded_press_cancels_active);
    TEST(g4_app_cancel_releases_the_stream);

    printf("\n--------------------------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
