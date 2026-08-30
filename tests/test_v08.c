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

    printf("\n--------------------------------\n");
    printf("%d/%d passed\n", g_tests_pass, g_tests_run);
    return (g_tests_pass == g_tests_run) ? 0 : 1;
}
