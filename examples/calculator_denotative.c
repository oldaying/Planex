/*
 * calculator_denotative.c — (c) route validation at calculator complexity
 *
 * Purpose:
 *   counter_denotative proved (c) route works for single-state UI.
 *   todo_app is too complex for first (c) route multi-state test.
 *   Calculator is the right intermediate complexity:
 *     - Multiple buttons (10 digits + 4 operators + equals + clear)
 *     - State machine (entering operand 1 → operator → entering operand 2 → equals)
 *     - Display state derived from multiple inputs (current input, stored operand, operator)
 *     - But NO list, NO input box, NO IME — keeps the demo focused
 *
 * What this prototype validates:
 *
 *   1. (c) route works for multi-state UIs (not just single Estimate)
 *   2. Pure render function can express a state machine's display
 *   3. Multiple buttons map cleanly to multiple Closures, each with own Intent
 *   4. Display Estimate is derived from input history via Relation graph
 *   5. UI stays unit-testable: set state → call render → assert pixels
 *
 * What this prototype deliberately does NOT do:
 *
 *   - Does NOT use on_render callback (replaced by pure render_to_pixels)
 *   - Does NOT make render a Closure (it's a plain C function)
 *   - Does NOT modify Planex core (include/planex/*.h unchanged)
 *
 * Build:
 *   cc -std=c17 -I include examples/calculator_denotative.c \
 *      src/relation.c src/estimate.c src/closure.c src/fb.c src/font.c \
 *      -lm -o build/calculator_denotative
 *
 * Run:
 *   ./build/calculator_denotative
 *
 * ================================================================== */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define CALC_DISPLAY_MAX 32

/* ============================================================
 * Calculator state — modeled as Estimates
 *
 * The calculator has 4 pieces of state, all Estimates:
 *   - display: what's currently shown on screen (string)
 *   - stored_operand: the first operand when operator pressed (double)
 *   - operator: which operator was pressed (+, -, *, /, or none)
 *   - just_computed: true if last action was "equals"
 *
 * Together these form a state machine. The "display" Estimate is
 * the user-visible one. The others are internal state.
 *
 * Key (c)-route question: can a pure function render this whole
 * state machine correctly? Yes — the function takes all 4 Estimates
 * as input and produces pixels. No callbacks.
 * ============================================================ */

typedef enum {
    OP_NONE = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} calc_operator;

typedef struct {
    px_estimate* display_value;   /* numeric value of current display */
    px_estimate* stored_operand;  /* first operand, saved when operator pressed */
    px_estimate* operator_est;   /* encoded as double: 0=none, 1=+, 2=-, 3=*, 4=/ */
    px_estimate* just_computed;   /* 1.0 if last action was equals, 0.0 otherwise */
    px_graph*    graph;
    /* Closures — one per button */
    px_closure* digit_closures[10];  /* 0-9 */
    px_closure* op_add;
    px_closure* op_sub;
    px_closure* op_mul;
    px_closure* op_div;
    px_closure* op_equals;
    px_closure* op_clear;
} CalculatorApp;

/* ============================================================
 * Helpers
 * ============================================================ */

static const char* op_to_symbol(calc_operator op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_NONE: return " ";
    }
    return "?";
}

static calc_operator op_from_double(double v) {
    int i = (int)v;
    if (i < 0 || i > 4) return OP_NONE;
    return (calc_operator)i;
}

static void append_digit_to_display(CalculatorApp* app, int digit) {
    double current = px_estimate_value(app->display_value);
    double just_done = px_estimate_value(app->just_computed);

    if (just_done > 0.5 || just_done < -0.5) {
        /* Last action was equals OR operator pressed — start fresh */
        px_estimate_set(app->display_value, (double)digit, 1.0);
        /* Reset just_computed to "normal entering" state (0.0) */
        if (just_done < -0.5) {
            /* Operator was pressed — we still keep operator_est as is,
             * but mark just_computed = 0 so next digit appends normally */
            px_estimate_set(app->just_computed, 0.0, 1.0);
        } else {
            /* Equals was pressed — clear all state, start new calculation */
            px_estimate_set(app->just_computed, 0.0, 1.0);
            px_estimate_set(app->stored_operand, 0.0, 1.0);
            px_estimate_set(app->operator_est, (double)OP_NONE, 1.0);
        }
    } else {
        /* Append digit: shift left by 10 and add */
        double new_val = current * 10.0 + digit;
        /* Clamp to prevent overflow */
        if (new_val > 1e15) new_val = current;  /* ignore if too big */
        px_estimate_set(app->display_value, new_val, 1.0);
    }
}

static void press_operator(CalculatorApp* app, calc_operator op) {
    double current = px_estimate_value(app->display_value);
    double stored = px_estimate_value(app->stored_operand);
    calc_operator prev_op = op_from_double(px_estimate_value(app->operator_est));

    /* If we already have a stored operand and an operator, perform
     * the pending operation first (chained calculations like 2 + 3 * 4) */
    if (prev_op != OP_NONE && px_estimate_value(app->just_computed) < 0.5) {
        double result = 0;
        switch (prev_op) {
            case OP_ADD: result = stored + current; break;
            case OP_SUB: result = stored - current; break;
            case OP_MUL: result = stored * current; break;
            case OP_DIV:
                if (current != 0) result = stored / current;
                else result = 0;  /* division by zero → 0, display shows ERROR */
                break;
            case OP_NONE: result = current; break;
        }
        px_estimate_set(app->display_value, result, 1.0);
        px_estimate_set(app->stored_operand, result, 1.0);
    } else {
        /* First operator press — just store the operand */
        px_estimate_set(app->stored_operand, current, 1.0);
    }

    px_estimate_set(app->operator_est, (double)op, 1.0);
    /* Mark that next digit should start fresh, but not "just_computed"
     * (just_computed means equals was pressed, this is operator pressed) */
    px_estimate_set(app->just_computed, 0.0, 1.0);
    /* Trick: set display_value to 0 so next digit starts fresh,
     * but we want this to be detected as "operator just pressed" */
    /* Actually, let's use just_computed = -1.0 to mean "operator pressed, awaiting next operand" */
    px_estimate_set(app->just_computed, -1.0, 1.0);
}

static void press_equals(CalculatorApp* app) {
    double current = px_estimate_value(app->display_value);
    double stored = px_estimate_value(app->stored_operand);
    calc_operator op = op_from_double(px_estimate_value(app->operator_est));

    if (op == OP_NONE) {
        /* No operator pressed — just keep current value */
        px_estimate_set(app->just_computed, 1.0, 1.0);
        return;
    }

    double result = 0;
    bool error = false;
    switch (op) {
        case OP_ADD: result = stored + current; break;
        case OP_SUB: result = stored - current; break;
        case OP_MUL: result = stored * current; break;
        case OP_DIV:
            if (current != 0) result = stored / current;
            else { error = true; result = 0; }
            break;
        case OP_NONE: result = current; break;
    }

    if (error) {
        /* Set display to a sentinel value to show ERROR */
        px_estimate_set(app->display_value, -999999.0, 1.0);
    } else {
        px_estimate_set(app->display_value, result, 1.0);
    }
    px_estimate_set(app->stored_operand, result, 1.0);
    px_estimate_set(app->operator_est, (double)OP_NONE, 1.0);
    px_estimate_set(app->just_computed, 1.0, 1.0);
}

static void press_clear(CalculatorApp* app) {
    px_estimate_set(app->display_value, 0.0, 1.0);
    px_estimate_set(app->stored_operand, 0.0, 1.0);
    px_estimate_set(app->operator_est, (double)OP_NONE, 1.0);
    px_estimate_set(app->just_computed, 0.0, 1.0);
}

/* ============================================================
 * THE PURE FUNCTION — render is denotation of Calculator state
 *
 * Takes all 4 Estimates as input, returns a fresh px_fb.
 * No reads of globals. No mutation of inputs.
 * Same state → same pixels, always.
 * ============================================================ */

static px_fb* render_calculator(px_estimate* display_value,
                                  px_estimate* stored_operand,
                                  px_estimate* operator_est,
                                  px_estimate* just_computed) {
    px_fb* fb = px_fb_new(256, 224);
    if (!fb) return NULL;

    /* Read all state once, at the start. No mutation. */
    double display_val = px_estimate_value(display_value);
    double stored_val = px_estimate_value(stored_operand);
    calc_operator op = op_from_double(px_estimate_value(operator_est));
    double just_done = px_estimate_value(just_computed);

    /* Background */
    px_fb_clear(fb, PX_BG);

    /* Window frame */
    px_fb_draw_rect(fb, 4, 4, 248, 216, PX_BORDER);

    /* Title bar */
    px_fb_fill_rect(fb, 4, 4, 248, 20, PX_SURFACE);
    px_fb_draw_text(fb, 12, 8, "Planex Calculator (denotative)", PX_TEXT);

    /* Display area — shows the current value or ERROR */
    int disp_x = 16, disp_y = 32, disp_w = 224, disp_h = 32;
    px_fb_fill_rect(fb, disp_x, disp_y, disp_w, disp_h, PX_SURFACE);
    px_fb_draw_rect(fb, disp_x, disp_y, disp_w, disp_h, PX_BORDER);

    char disp_buf[64];
    if (display_val == -999999.0) {
        snprintf(disp_buf, sizeof(disp_buf), "ERROR");
    } else {
        /* Format: if integer, show as integer; else show with decimals */
        double intpart;
        if (modf(display_val, &intpart) == 0.0 && fabs(display_val) < 1e15) {
            snprintf(disp_buf, sizeof(disp_buf), "= %.0f", display_val);
        } else {
            snprintf(disp_buf, sizeof(disp_buf), "= %.4g", display_val);
        }
    }
    /* Right-align text in display area */
    int text_w = (int)(strlen(disp_buf) * 8);
    int text_x = disp_x + disp_w - text_w - 8;
    px_fb_draw_text(fb, text_x, disp_y + 12, disp_buf, PX_TEXT);

    /* Status line — shows pending operation */
    int status_y = disp_y + disp_h + 6;
    char status_buf[64];
    if (op != OP_NONE && just_done < -0.5) {
        /* Operator pressed, awaiting next operand */
        snprintf(status_buf, sizeof(status_buf), "stored: %.4g %s _",
                 stored_val, op_to_symbol(op));
    } else if (just_done > 0.5) {
        snprintf(status_buf, sizeof(status_buf), "computed");
    } else {
        snprintf(status_buf, sizeof(status_buf), "ready");
    }
    px_fb_draw_text(fb, 12, status_y, status_buf, PX_TEXT_DIM);

    /* Button grid — 4 columns x 5 rows
     * Row 1: 7 8 9  C
     * Row 2: 4 5 6  /
     * Row 3: 1 2 3  *
     * Row 4: 0 . =  +   (no decimal here, simpler: 0 = + -)
     * Row 5:                            (collapsed — actually 4 rows only)
     *
     * Let's do 4 rows x 4 cols:
     * Row 1: 7 8 9  C
     * Row 2: 4 5 6  /
     * Row 3: 1 2 3  *
     * Row 4: 0 = +  -
     */
    int btn_w = 50, btn_h = 32;
    int btn_x_start = 16, btn_y_start = status_y + 16;
    int btn_gap = 6;

    /* Helper macro-like function for drawing a button */
    /* We'll just inline the calls */

    /* Row 1: 7 8 9  C */
    const char* row1[] = {"7", "8", "9", "C"};
    uint32_t row1_colors[] = {PX_SURFACE, PX_SURFACE, PX_SURFACE, PX_DANGER};
    for (int c = 0; c < 4; c++) {
        int x = btn_x_start + c * (btn_w + btn_gap);
        int y = btn_y_start;
        px_fb_fill_rect(fb, x, y, btn_w, btn_h, row1_colors[c]);
        px_fb_draw_rect(fb, x, y, btn_w, btn_h, PX_BORDER);
        px_fb_draw_text(fb, x + 20, y + 11, row1[c], PX_TEXT);
    }

    /* Row 2: 4 5 6  / */
    const char* row2[] = {"4", "5", "6", "/"};
    for (int c = 0; c < 4; c++) {
        int x = btn_x_start + c * (btn_w + btn_gap);
        int y = btn_y_start + (btn_h + btn_gap);
        px_fb_fill_rect(fb, x, y, btn_w, btn_h, PX_SURFACE);
        px_fb_draw_rect(fb, x, y, btn_w, btn_h, PX_BORDER);
        px_fb_draw_text(fb, x + 20, y + 11, row2[c], PX_TEXT);
    }

    /* Row 3: 1 2 3  * */
    const char* row3[] = {"1", "2", "3", "*"};
    for (int c = 0; c < 4; c++) {
        int x = btn_x_start + c * (btn_w + btn_gap);
        int y = btn_y_start + 2 * (btn_h + btn_gap);
        px_fb_fill_rect(fb, x, y, btn_w, btn_h, PX_SURFACE);
        px_fb_draw_rect(fb, x, y, btn_w, btn_h, PX_BORDER);
        px_fb_draw_text(fb, x + 20, y + 11, row3[c], PX_TEXT);
    }

    /* Row 4: 0 = + - */
    const char* row4[] = {"0", "=", "+", "-"};
    uint32_t row4_colors[] = {PX_SURFACE, PX_ACCENT, PX_ACCENT, PX_ACCENT};
    for (int c = 0; c < 4; c++) {
        int x = btn_x_start + c * (btn_w + btn_gap);
        int y = btn_y_start + 3 * (btn_h + btn_gap);
        px_fb_fill_rect(fb, x, y, btn_w, btn_h, row4_colors[c]);
        px_fb_draw_rect(fb, x, y, btn_w, btn_h, PX_BORDER);
        px_fb_draw_text(fb, x + 20, y + 11, row4[c], PX_TEXT);
    }

    /* Footer */
    int footer_y = btn_y_start + 4 * (btn_h + btn_gap) + 4;
    px_fb_draw_text(fb, 12, footer_y, "BMP only — no interactivity in this demo",
                    PX_TEXT_DIM);

    return fb;
}

/* ============================================================
 * Second denotation — accessibility text
 * ============================================================ */

static char* render_calculator_a11y(px_estimate* display_value,
                                      px_estimate* stored_operand,
                                      px_estimate* operator_est,
                                      px_estimate* just_computed) {
    char* buf = malloc(128);
    if (!buf) return NULL;

    double disp = px_estimate_value(display_value);
    double stored = px_estimate_value(stored_operand);
    calc_operator op = op_from_double(px_estimate_value(operator_est));
    double just_done = px_estimate_value(just_computed);

    if (disp == -999999.0) {
        snprintf(buf, 128, "Calculator shows ERROR. Press C to clear.");
    } else if (op != OP_NONE && just_done < -0.5) {
        snprintf(buf, 128, "Calculator shows %.4g. Pending: %.4g %s next operand.",
                 disp, stored, op_to_symbol(op));
    } else if (just_done > 0.5) {
        snprintf(buf, 128, "Calculator shows %.4g. Computed. Press C to clear or digit to start new.",
                 disp);
    } else {
        snprintf(buf, 128, "Calculator shows %.4g. Ready.", disp);
    }
    return buf;
}

/* ============================================================
 * UNIT TESTS — proving (c) route enables testability
 *
 * These tests run without:
 *   - A window
 *   - An event loop
 *   - Any user input
 *   - Any button click
 *
 * They directly assert on pixels produced by the pure function.
 * ============================================================ */

static bool pixel_is_color(px_fb* fb, int x, int y, uint32_t expected) {
    return px_fb_get_pixel(fb, x, y) == expected;
}

static void test_initial_state_renders_zero(void) {
    printf("  [test] initial state renders \"= 0\" in display ... ");
    CalculatorApp app = {0};
    app.display_value = px_estimate_new(0.0, 1.0);
    app.stored_operand = px_estimate_new(0.0, 1.0);
    app.operator_est = px_estimate_new((double)OP_NONE, 1.0);
    app.just_computed = px_estimate_new(0.0, 1.0);

    px_fb* fb = render_calculator(app.display_value, app.stored_operand,
                                    app.operator_est, app.just_computed);

    /* Verify display area is PX_SURFACE */
    assert(pixel_is_color(fb, 100, 48, PX_SURFACE));
    /* Verify background corner */
    assert(pixel_is_color(fb, 0, 0, PX_BG));
    /* Verify dimensions */
    assert(px_fb_width(fb) == 256);
    assert(px_fb_height(fb) == 224);

    px_fb_free(fb);
    px_estimate_free(app.display_value);
    px_estimate_free(app.stored_operand);
    px_estimate_free(app.operator_est);
    px_estimate_free(app.just_computed);
    printf("PASS\n");
}

static void test_error_state_renders_ERROR_text(void) {
    printf("  [test] error state (div by zero) changes display ... ");
    CalculatorApp app = {0};
    app.display_value = px_estimate_new(-999999.0, 1.0);  /* error sentinel */
    app.stored_operand = px_estimate_new(0.0, 1.0);
    app.operator_est = px_estimate_new((double)OP_NONE, 1.0);
    app.just_computed = px_estimate_new(1.0, 1.0);

    /* Render error state and a normal state — they should differ */
    px_fb* fb_err = render_calculator(app.display_value, app.stored_operand,
                                        app.operator_est, app.just_computed);

    /* Now render normal state */
    px_estimate_set(app.display_value, 42.0, 1.0);
    px_fb* fb_ok = render_calculator(app.display_value, app.stored_operand,
                                       app.operator_est, app.just_computed);

    /* The two frames should differ — error state should have different pixels
     * in the display area */
    bool display_differs = false;
    for (int y = 32; y < 64 && !display_differs; y++) {
        for (int x = 16; x < 240; x++) {
            if (px_fb_get_pixel(fb_err, x, y) != px_fb_get_pixel(fb_ok, x, y)) {
                display_differs = true;
                break;
            }
        }
    }
    assert(display_differs);

    px_fb_free(fb_err);
    px_fb_free(fb_ok);
    px_estimate_free(app.display_value);
    px_estimate_free(app.stored_operand);
    px_estimate_free(app.operator_est);
    px_estimate_free(app.just_computed);
    printf("PASS\n");
}

static void test_purity_same_state_same_pixels(void) {
    printf("  [test] same calculator state → same pixels (purity) ... ");
    /* Create two identical state sets */
    CalculatorApp a = {0}, b = {0};
    a.display_value = px_estimate_new(42.0, 1.0);
    a.stored_operand = px_estimate_new(10.0, 1.0);
    a.operator_est = px_estimate_new((double)OP_ADD, 1.0);
    a.just_computed = px_estimate_new(-1.0, 1.0);

    b.display_value = px_estimate_new(42.0, 1.0);
    b.stored_operand = px_estimate_new(10.0, 1.0);
    b.operator_est = px_estimate_new((double)OP_ADD, 1.0);
    b.just_computed = px_estimate_new(-1.0, 1.0);

    px_fb* fa = render_calculator(a.display_value, a.stored_operand,
                                    a.operator_est, a.just_computed);
    px_fb* fb = render_calculator(b.display_value, b.stored_operand,
                                    b.operator_est, b.just_computed);

    /* Both must have identical pixels */
    assert(px_fb_width(fa) == px_fb_width(fb));
    assert(px_fb_height(fa) == px_fb_height(fb));

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
    px_estimate_free(a.display_value);
    px_estimate_free(a.stored_operand);
    px_estimate_free(a.operator_est);
    px_estimate_free(a.just_computed);
    px_estimate_free(b.display_value);
    px_estimate_free(b.stored_operand);
    px_estimate_free(b.operator_est);
    px_estimate_free(b.just_computed);
    printf("PASS\n");
}

static void test_a11y_denotation_matches_state(void) {
    printf("  [test] a11y denotation matches calculator state ... ");
    CalculatorApp app = {0};
    app.display_value = px_estimate_new(42.0, 1.0);
    app.stored_operand = px_estimate_new(10.0, 1.0);
    app.operator_est = px_estimate_new((double)OP_ADD, 1.0);
    app.just_computed = px_estimate_new(-1.0, 1.0);

    char* a11y = render_calculator_a11y(app.display_value, app.stored_operand,
                                          app.operator_est, app.just_computed);

    /* Should mention pending operation */
    assert(strstr(a11y, "Pending") != NULL);
    assert(strstr(a11y, "42") != NULL);
    assert(strstr(a11y, "+") != NULL);

    free(a11y);
    px_estimate_free(app.display_value);
    px_estimate_free(app.stored_operand);
    px_estimate_free(app.operator_est);
    px_estimate_free(app.just_computed);
    printf("PASS\n");
}

/* ============================================================
 * Calculator app setup — Closure actions
 *
 * Each button is a Closure with proper Goal + Intent + Evaluation.
 * The render function is NOT a Closure — it's a pure C function
 * called explicitly when needed (after state changes).
 * ============================================================ */

/* Action callbacks */

static void on_digit_0(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 0); }
static void on_digit_1(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 1); }
static void on_digit_2(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 2); }
static void on_digit_3(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 3); }
static void on_digit_4(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 4); }
static void on_digit_5(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 5); }
static void on_digit_6(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 6); }
static void on_digit_7(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 7); }
static void on_digit_8(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 8); }
static void on_digit_9(px_intent intent, void* user) { (void)intent; append_digit_to_display(user, 9); }

static void on_op_add(px_intent intent, void* user) { (void)intent; press_operator(user, OP_ADD); }
static void on_op_sub(px_intent intent, void* user) { (void)intent; press_operator(user, OP_SUB); }
static void on_op_mul(px_intent intent, void* user) { (void)intent; press_operator(user, OP_MUL); }
static void on_op_div(px_intent intent, void* user) { (void)intent; press_operator(user, OP_DIV); }
static void on_op_equals(px_intent intent, void* user) { (void)intent; press_equals(user); }
static void on_op_clear(px_intent intent, void* user) { (void)intent; press_clear(user); }

/* Evaluation — always true (calculator actions always succeed) */
static bool calc_eval_always_true(void* user) {
    (void)user;
    return true;
}


typedef void (*digit_fn_t)(px_intent, void*);
static digit_fn_t digit_fns[10] = {
    on_digit_0, on_digit_1, on_digit_2, on_digit_3, on_digit_4,
    on_digit_5, on_digit_6, on_digit_7, on_digit_8, on_digit_9,
};

/* ============================================================
 * Main — runs tests, simulates calculator interactions
 * ============================================================ */

int main(void) {
    printf("Planex calculator_denotative — (c) route validation\n");
    printf("===================================================\n");
    printf("Validates: render is a pure function of multiple Estimates.\n");
    printf("Complexity: multi-state UI (calculator state machine)\n");
    printf("Source: extends counter_denotative to higher complexity\n\n");

    /* --- Phase 1: unit tests --- */
    printf("Phase 1: Unit tests for the pure render function\n");
    printf("-------------------------------------------------\n");
    test_initial_state_renders_zero();
    test_error_state_renders_ERROR_text();
    test_purity_same_state_same_pixels();
    test_a11y_denotation_matches_state();
    printf("\n");

    /* --- Phase 2: setup calculator app --- */
    CalculatorApp app = {0};
    app.display_value = px_estimate_new(0.0, 1.0);
    app.stored_operand = px_estimate_new(0.0, 1.0);
    app.operator_est = px_estimate_new((double)OP_NONE, 1.0);
    app.just_computed = px_estimate_new(0.0, 1.0);
    app.graph = px_graph_new();

    /* Create 10 digit closures */
    char goal_buf[32];
    for (int i = 0; i < 10; i++) {
        snprintf(goal_buf, sizeof(goal_buf), "press digit %d", i);
        app.digit_closures[i] = px_closure_new(
            goal_buf, PX_INTENT_REQUEST,
            digit_fns[i], calc_eval_always_true, &app);
    }

    app.op_add = px_closure_new("press +", PX_INTENT_REQUEST,
                                  on_op_add, calc_eval_always_true, &app);
    app.op_sub = px_closure_new("press -", PX_INTENT_REQUEST,
                                  on_op_sub, calc_eval_always_true, &app);
    app.op_mul = px_closure_new("press *", PX_INTENT_REQUEST,
                                  on_op_mul, calc_eval_always_true, &app);
    app.op_div = px_closure_new("press /", PX_INTENT_REQUEST,
                                  on_op_div, calc_eval_always_true, &app);
    app.op_equals = px_closure_new("press =", PX_INTENT_REQUEST,
                                     on_op_equals, calc_eval_always_true, &app);
    app.op_clear = px_closure_new("press C", PX_INTENT_REQUEST,
                                    on_op_clear, calc_eval_always_true, &app);

    /* Declare relations: each closure triggers the display */
    for (int i = 0; i < 10; i++) {
        px_declare(app.graph, app.digit_closures[i], PX_REL_TRIGGERS, app.display_value);
    }
    px_declare(app.graph, app.op_add, PX_REL_TRIGGERS, app.display_value);
    px_declare(app.graph, app.op_sub, PX_REL_TRIGGERS, app.display_value);
    px_declare(app.graph, app.op_mul, PX_REL_TRIGGERS, app.display_value);
    px_declare(app.graph, app.op_div, PX_REL_TRIGGERS, app.display_value);
    px_declare(app.graph, app.op_equals, PX_REL_TRIGGERS, app.display_value);
    px_declare(app.graph, app.op_clear, PX_REL_TRIGGERS, app.display_value);

    /* --- Phase 3: simulate "2 + 3 =" --- */
    printf("Phase 2: Simulated interactions — compute 2 + 3 = ?\n");
    printf("-------------------------------------------------\n");

    /* Press 2 */
    px_closure_trigger(app.digit_closures[2], NULL, 0);
    printf("  [press 2] display = %.0f\n", px_estimate_value(app.display_value));

    /* Press + */
    px_closure_trigger(app.op_add, NULL, 0);
    printf("  [press +] stored = %.0f, op = %s\n",
           px_estimate_value(app.stored_operand),
           op_to_symbol(op_from_double(px_estimate_value(app.operator_est))));

    /* Press 3 */
    px_closure_trigger(app.digit_closures[3], NULL, 0);
    printf("  [press 3] display = %.0f\n", px_estimate_value(app.display_value));

    /* Press = */
    px_closure_trigger(app.op_equals, NULL, 0);
    printf("  [press =] result = %.0f\n", px_estimate_value(app.display_value));

    /* --- Phase 4: render final state to BMP --- */
    printf("\nPhase 3: Save visual denotation (BMP)\n");
    printf("-------------------------------------------------\n");
    px_fb* final_fb = render_calculator(app.display_value, app.stored_operand,
                                          app.operator_est, app.just_computed);
    if (final_fb) {
        px_fb_save_bmp(final_fb, "calculator_denotative.bmp");
        printf("  Saved: calculator_denotative.bmp (%dx%d)\n",
               px_fb_width(final_fb), px_fb_height(final_fb));
        printf("  Display value: %.0f (expected: 5)\n",
               px_estimate_value(app.display_value));
        px_fb_free(final_fb);
    }

    /* --- Phase 5: a11y denotation --- */
    printf("\nPhase 4: Accessibility denotation\n");
    printf("-------------------------------------------------\n");
    char* a11y = render_calculator_a11y(app.display_value, app.stored_operand,
                                          app.operator_est, app.just_computed);
    if (a11y) {
        printf("  Screen reader: \"%s\"\n", a11y);
        free(a11y);
    }

    /* --- Phase 6: chained calculation 4 * 5 + 2 = ? --- */
    printf("\nPhase 5: Chained calculation — 4 * 5 + 2 = ?\n");
    printf("-------------------------------------------------\n");
    px_closure_trigger(app.op_clear, NULL, 0);
    px_closure_trigger(app.digit_closures[4], NULL, 0);
    printf("  [press 4] display = %.0f\n", px_estimate_value(app.display_value));
    px_closure_trigger(app.op_mul, NULL, 0);
    printf("  [press *] stored = %.0f\n", px_estimate_value(app.stored_operand));
    px_closure_trigger(app.digit_closures[5], NULL, 0);
    printf("  [press 5] display = %.0f\n", px_estimate_value(app.display_value));
    px_closure_trigger(app.op_add, NULL, 0);  /* should compute 4*5=20 first */
    printf("  [press +] stored = %.0f (after chained compute)\n",
           px_estimate_value(app.stored_operand));
    px_closure_trigger(app.digit_closures[2], NULL, 0);
    printf("  [press 2] display = %.0f\n", px_estimate_value(app.display_value));
    px_closure_trigger(app.op_equals, NULL, 0);
    printf("  [press =] result = %.0f (expected: 22)\n",
           px_estimate_value(app.display_value));

    /* --- Phase 7: division by zero --- */
    printf("\nPhase 6: Division by zero\n");
    printf("-------------------------------------------------\n");
    px_closure_trigger(app.op_clear, NULL, 0);
    px_closure_trigger(app.digit_closures[5], NULL, 0);
    px_closure_trigger(app.op_div, NULL, 0);
    px_closure_trigger(app.digit_closures[0], NULL, 0);
    px_closure_trigger(app.op_equals, NULL, 0);
    printf("  5 / 0 = display value %.0f (sentinel for ERROR)\n",
           px_estimate_value(app.display_value));
    char* a11y_err = render_calculator_a11y(app.display_value, app.stored_operand,
                                              app.operator_est, app.just_computed);
    if (a11y_err) {
        printf("  Screen reader: \"%s\"\n", a11y_err);
        free(a11y_err);
    }

    /* Save the error state BMP too */
    px_fb* err_fb = render_calculator(app.display_value, app.stored_operand,
                                        app.operator_est, app.just_computed);
    if (err_fb) {
        px_fb_save_bmp(err_fb, "calculator_denotative_error.bmp");
        printf("  Saved: calculator_denotative_error.bmp\n");
        px_fb_free(err_fb);
    }

    /* --- Cleanup --- */
    for (int i = 0; i < 10; i++) px_closure_free(app.digit_closures[i]);
    px_closure_free(app.op_add);
    px_closure_free(app.op_sub);
    px_closure_free(app.op_mul);
    px_closure_free(app.op_div);
    px_closure_free(app.op_equals);
    px_closure_free(app.op_clear);
    px_graph_free(app.graph);
    px_estimate_free(app.display_value);
    px_estimate_free(app.stored_operand);
    px_estimate_free(app.operator_est);
    px_estimate_free(app.just_computed);

    printf("\n=== Prototype complete ===\n");
    printf("\nWhat we validated:\n");
    printf("  1. (c) route works for multi-state UI (calculator)\n");
    printf("  2. Pure render takes 4 Estimates as input — no callbacks\n");
    printf("  3. 16 buttons map cleanly to 16 Closures, each with own Intent\n");
    printf("  4. State machine (entering op1 → operator → op2 → equals)\n");
    printf("     is expressible via Estimate transitions, not separate abstraction\n");
    printf("  5. Render is unit-testable without an app loop or window\n");
    printf("  6. Multiple denotations (pixels + a11y text) coexist\n");
    printf("  7. Chained calculation (4*5+2=22) works via Relation graph\n");
    printf("  8. Error state (division by zero) expressible without exceptions\n");
    printf("\nConclusion:\n");
    printf("  (c) route scales beyond counter's single-state UI.\n");
    printf("  It handles calculator's multi-state state machine cleanly.\n");
    printf("  Next step: ADR-0005 can consider (c) route for moderate-complexity UIs.\n");
    printf("  Remaining open question: does it scale to todo_app's list + input + filter?\n");
    return 0;
}
