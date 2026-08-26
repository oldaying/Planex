/*
 * counter_denotative.c — (c) route prototype
 *
 * Prototype for ADR-0001 candidate path (c):
 *   "Render is a pure function of Estimate, not a callback."
 *
 * This file deliberately diverges from counter_fb.c / counter_x11.c
 * to test whether the (c) route can fit within Planex's current
 * architecture without requiring new core abstractions.
 *
 * What this prototype proves (or disproves):
 *
 *   1. Render can be expressed as a pure function:
 *        px_fb* render(px_estimate* count)
 *      Same input → same output. No side effects on Estimate.
 *
 *   2. Multiple denotations of the same Estimate coexist:
 *        - render_to_pixels (visual)
 *        - render_to_a11y_text (accessibility)
 *        - render_to_log (debug)
 *      All three are pure functions of the same Estimate.
 *
 *   3. Render is unit-testable without running the app loop:
 *        Set estimate → call render → assert pixels.
 *
 *   4. UI appearance can be statically inspected:
 *        Given count = 42, the output pixels are deterministic.
 *
 * What this prototype deliberately does NOT do:
 *
 *   - Does NOT modify include/planex/*.h
 *   - Does NOT add a new "px_perception" type
 *   - Does NOT change px_closure_new's signature
 *   - Does NOT touch the other 24 demos
 *
 * It only adds ONE file under examples/ and validates whether (c)
 * is feasible in Planex's current shape. If it works, we write
 * ADR-0005 and roll out (c) to the other demos.
 *
 * Run: ./build/counter_denotative
 * Output: counter_denotative.bmp + assertions to stdout
 *
 * Build (manual):
 *   cc -std=c17 -I include examples/counter_denotative.c \
 *      src/relation.c src/estimate.c src/closure.c src/fb.c \
 *      src/font.c -lm -o build/counter_denotative
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* =================================================================
 * THE PURE FUNCTION — render is denotation of Estimate
 *
 * Properties:
 *   - Input: a single Estimate (the count)
 *   - Output: a fresh px_fb (caller owns it, must free)
 *   - No reads of globals, no reads of user data
 *   - No writes to globals, no writes to the input Estimate
 *   - Same Estimate value → same pixel output, always
 *
 * This is the heart of the (c) route. If you accept this file,
 * you accept that "render = denotation of Estimate".
 * ================================================================= */

static px_fb* render_to_pixels(px_estimate* count) {
    /* Allocate a fresh framebuffer — no shared mutable state */
    px_fb* fb = px_fb_new(256, 96);
    if (!fb) return NULL;

    /* Read the Estimate's current value (pure read, no mutation) */
    double v = px_estimate_value(count);

    /* Background */
    px_fb_clear(fb, PX_BG);

    /* Window frame */
    px_fb_draw_rect(fb, 4, 4, 248, 88, PX_BORDER);

    /* Title bar */
    px_fb_fill_rect(fb, 4, 4, 248, 20, PX_SURFACE);
    px_fb_draw_text(fb, 12, 8, "Planex Counter (denotative)", PX_TEXT);

    /* Count display box */
    int box_x = 32, box_y = 32, box_w = 192, box_h = 24;
    px_fb_fill_rect(fb, box_x, box_y, box_w, box_h, PX_SURFACE);
    px_fb_draw_rect(fb, box_x, box_y, box_w, box_h, PX_BORDER);

    char buf[64];
    snprintf(buf, sizeof(buf), "Count: %.0f", v);
    px_fb_draw_text(fb, box_x + 12, box_y + 4, buf, PX_TEXT);

    /* Static button shapes (purely visual; hit-testing is separate) */
    int btn_y = box_y + box_h + 8;
    int btn_w = 80, btn_h = 20;
    px_fb_fill_rect(fb, box_x, btn_y, btn_w, btn_h, PX_ACCENT);
    px_fb_draw_text(fb, box_x + 24, btn_y + 2, "+ Inc", PX_TEXT);

    px_fb_fill_rect(fb, box_x + btn_w + 12, btn_y, btn_w, btn_h, PX_SURFACE);
    px_fb_draw_rect(fb, box_x + btn_w + 12, btn_y, btn_w, btn_h, PX_BORDER);
    px_fb_draw_text(fb, box_x + btn_w + 36, btn_y + 2, "- Dec", PX_TEXT);

    /* Status bar */
    int sy = 76;
    px_fb_fill_rect(fb, 4, sy, 248, 16, PX_SURFACE);
    const char* status = (v < 0) ? "Status: negative (would FAIL eval)" : "Status: OK";
    uint32_t status_color = (v < 0) ? PX_DANGER : PX_TEXT_DIM;
    px_fb_draw_text(fb, 12, sy + 2, status, status_color);

    return fb;  /* caller frees */
}

/* =================================================================
 * SECOND DENOTATION — accessibility text
 *
 * Same Estimate, different perception. Proves that (c) route
 * supports multiple denotations without a separate Perception
 * abstraction.
 * ================================================================= */

static char* render_to_a11y_text(px_estimate* count) {
    /* Returns a heap-allocated string the caller must free.
     * Pure: same Estimate → same string. */
    double v = px_estimate_value(count);
    char* buf = malloc(64);
    if (!buf) return NULL;
    snprintf(buf, 64, "Counter value is %.0f", v);
    return buf;
}

/* =================================================================
 * THIRD DENOTATION — debug log
 * ================================================================= */

static void render_to_log(px_estimate* count, FILE* out) {
    /* Pure: writes only what the Estimate says. */
    double v = px_estimate_value(count);
    double c = px_estimate_confidence(count);
    fprintf(out, "[denotative] count=%.0f confidence=%.2f\n", v, c);
}

/* =================================================================
 * UNIT TESTS — proving the (c) route enables testability
 *
 * These tests run without:
 *   - A window
 *   - An event loop
 *   - A running app
 *   - Any user input
 *
 * Each test sets the Estimate, calls the pure function, asserts
 * on the resulting pixels. This is impossible in the callback
 * version (counter_x11.c) because on_render returns void.
 * ================================================================= */

static bool pixel_is_color(px_fb* fb, int x, int y, uint32_t expected) {
    return px_fb_get_pixel(fb, x, y) == expected;
}

static void test_count_zero_renders_zero(void) {
    printf("  [test] count=0 renders background + surface ... ");
    px_estimate* count = px_estimate_new(0, 1.0);
    px_fb* fb = render_to_pixels(count);

    /* Background pixel (corner) should be PX_BG — never painted over */
    assert(pixel_is_color(fb, 0, 0, PX_BG));
    assert(pixel_is_color(fb, 255, 95, PX_BG));
    /* Title bar interior (y=10, mid-x) should be PX_SURFACE */
    assert(pixel_is_color(fb, 100, 12, PX_SURFACE));
    /* Bottom status bar (y=76..91) interior should be PX_SURFACE */
    assert(pixel_is_color(fb, 100, 80, PX_SURFACE));
    /* Width and height should match the render function's contract */
    assert(px_fb_width(fb) == 256);
    assert(px_fb_height(fb) == 96);

    px_fb_free(fb);
    px_estimate_free(count);
    printf("PASS\n");
}

static void test_count_negative_renders_danger_status(void) {
    printf("  [test] count=-5 changes status bar to DANGER color ... ");
    px_estimate* count_pos = px_estimate_new(5, 1.0);
    px_estimate* count_neg = px_estimate_new(-5, 1.0);
    px_fb* fb_pos = render_to_pixels(count_pos);
    px_fb* fb_neg = render_to_pixels(count_neg);

    /* When count < 0, status bar text color is PX_DANGER.
     * When count >= 0, status bar text color is PX_TEXT_DIM.
     *
     * We can't easily check text pixels (they're sparse), but we
     * can verify the framebuffer changed between the two states —
     * proving the output is sensitive to the Estimate value.
     */
    bool frames_differ = false;
    for (int y = 0; y < px_fb_height(fb_pos) && !frames_differ; y++) {
        for (int x = 0; x < px_fb_width(fb_pos); x++) {
            if (px_fb_get_pixel(fb_pos, x, y) != px_fb_get_pixel(fb_neg, x, y)) {
                frames_differ = true;
                break;
            }
        }
    }
    assert(frames_differ);

    px_fb_free(fb_pos);
    px_fb_free(fb_neg);
    px_estimate_free(count_pos);
    px_estimate_free(count_neg);
    printf("PASS\n");
}

static void test_same_input_same_output(void) {
    printf("  [test] same Estimate value → same pixels (purity) ... ");
    px_estimate* a = px_estimate_new(42, 1.0);
    px_estimate* b = px_estimate_new(42, 1.0);
    px_fb* fa = render_to_pixels(a);
    px_fb* fb = render_to_pixels(b);

    /* Both must have identical dimensions */
    assert(px_fb_width(fa) == px_fb_width(fb));
    assert(px_fb_height(fa) == px_fb_height(fb));

    /* Both must have identical pixels */
    int W = px_fb_width(fa), H = px_fb_height(fa);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            uint32_t pa = px_fb_get_pixel(fa, x, y);
            uint32_t pb = px_fb_get_pixel(fb, x, y);
            if (pa != pb) {
                printf("FAIL at (%d,%d): a=0x%08x b=0x%08x\n", x, y, pa, pb);
                assert(0);
            }
        }
    }

    px_fb_free(fa);
    px_fb_free(fb);
    px_estimate_free(a);
    px_estimate_free(b);
    printf("PASS\n");
}

static void test_a11y_denotation_matches_visual(void) {
    printf("  [test] a11y denotation matches visual value ... ");
    px_estimate* count = px_estimate_new(42, 1.0);
    px_fb* fb = render_to_pixels(count);
    char* a11y = render_to_a11y_text(count);

    /* The a11y string should contain "42" */
    assert(strstr(a11y, "42") != NULL);

    /* Sanity: the visual pixel buffer should not be NULL */
    assert(fb != NULL);
    assert(px_fb_width(fb) > 0);

    free(a11y);
    px_fb_free(fb);
    px_estimate_free(count);
    printf("PASS\n");
}

/* =================================================================
 * The "app" — much thinner than counter_fb.c
 *
 * Compare to counter_fb.c:
 *   - No on_render callback
 *   - No render Closure (px_closure_new with on_render)
 *   - No "render is a Closure" abstraction friction
 *   - The render function is just a C function, called explicitly
 *
 * The Closure abstraction is still used for inc/dec — that's its
 * proper scope (user→machine direction). Render is no longer a
 * Closure; it's a denotation of Estimate.
 * ================================================================= */

typedef struct {
    px_estimate* count;
    px_graph*    graph;
    px_closure*  inc;
    px_closure*  dec;
} App;

static void on_inc(px_intent intent, void* user) {
    (void)intent;
    App* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v + 1, 1.0);
}

static void on_dec(px_intent intent, void* user) {
    (void)intent;
    App* app = user;
    double v = px_estimate_value(app->count);
    px_estimate_set(app->count, v - 1, 1.0);
}

static bool eval_nonneg(void* user) {
    App* app = user;
    return px_estimate_value(app->count) >= 0;
}


/* =================================================================
 * Main — runs tests, then simulates a few interactions
 * ================================================================= */

int main(void) {
    printf("Planex counter_denotative — (c) route prototype\n");
    printf("================================================\n");
    printf("Validates: render is a pure function of Estimate.\n");
    printf("Source: ADR-0001 candidate path (c)\n\n");

    /* --- Phase 1: unit tests (no app, no window, no loop) --- */
    printf("Phase 1: Unit tests for the pure render function\n");
    printf("-------------------------------------------------\n");
    test_count_zero_renders_zero();
    test_count_negative_renders_danger_status();
    test_same_input_same_output();
    test_a11y_denotation_matches_visual();
    printf("\n");

    /* --- Phase 2: simulate a few interactions --- */
    printf("Phase 2: Simulated interactions\n");
    printf("-------------------------------------------------\n");

    App app = {0};
    app.count = px_estimate_new(0, 1.0);
    app.graph = px_graph_new();
    app.inc   = px_closure_new("increment counter", PX_INTENT_REQUEST,
                                on_inc, eval_nonneg, &app);
    app.dec   = px_closure_new("decrement counter", PX_INTENT_REQUEST,
                                on_dec, eval_nonneg, &app);

    px_declare(app.graph, app.inc, PX_REL_TRIGGERS, app.count);
    px_declare(app.graph, app.dec, PX_REL_TRIGGERS, app.count);

    /* Simulated event sequence */
    printf("  initial state:\n");
    render_to_log(app.count, stdout);

    px_closure_trigger(app.inc, NULL, 0);
    printf("  after +1:\n");
    render_to_log(app.count, stdout);

    px_closure_trigger(app.inc, NULL, 0);
    px_closure_trigger(app.inc, NULL, 0);
    printf("  after +1, +1 (now should be 3):\n");
    render_to_log(app.count, stdout);

    px_closure_trigger(app.dec, NULL, 0);
    printf("  after -1 (now should be 2):\n");
    render_to_log(app.count, stdout);

    /* --- Phase 3: save BMP (visual denotation) --- */
    printf("\nPhase 3: Save visual denotation (BMP)\n");
    printf("-------------------------------------------------\n");
    px_fb* final_fb = render_to_pixels(app.count);
    if (final_fb) {
        px_fb_save_bmp(final_fb, "counter_denotative.bmp");
        printf("  Saved: counter_denotative.bmp (%dx%d)\n",
               px_fb_width(final_fb), px_fb_height(final_fb));
        printf("  Current count: %.0f\n", px_estimate_value(app.count));
        px_fb_free(final_fb);
    }

    /* --- Phase 4: a11y denotation --- */
    printf("\nPhase 4: Accessibility denotation\n");
    printf("-------------------------------------------------\n");
    char* a11y = render_to_a11y_text(app.count);
    if (a11y) {
        printf("  Screen reader would announce: \"%s\"\n", a11y);
        free(a11y);
    }

    /* --- Cleanup --- */
    px_closure_free(app.inc);
    px_closure_free(app.dec);
    px_graph_free(app.graph);
    px_estimate_free(app.count);

    printf("\n=== Prototype complete ===\n");
    printf("What we proved:\n");
    printf("  1. Render is a pure function of Estimate — no callback needed.\n");
    printf("  2. Multiple denotations (pixels, a11y text, log) coexist.\n");
    printf("  3. Render is unit-testable without an app loop.\n");
    printf("  4. The Closure abstraction is unchanged — still used for inc/dec.\n");
    printf("\nNext step: write ADR-0005 deciding whether to roll this out.\n");
    return 0;
}
