/*
 * if5c_calc.c — the calculator demo (ADR-0028 Decision 7): the
 * shipping tree's `examples/calculator_denotative.c` validation
 * ("(c) route works at calculator complexity"), re-derived on
 * the intent-flow ontology.
 *
 * What it validates:
 *   1. A multi-region state machine (display / accumulator /
 *      operator / flag + a keypad subtree) is expressible as
 *      REGIONS whose folds carry the state.
 *   2. The client renders the whole machine from the semantic
 *      tree at a view — pure function, committed golden.
 *   3. Buttons hit-test back to their regions (F4 at work: the
 *      demo's input path IS if5c_hit), and the app submits
 *      through the ordinary channel — the client never speaks.
 *   4. Undo is a view: after '=', one undo re-renders the world
 *      as it stood before the submission (N2's addressable time,
 *      drawn).
 *
 * What it deliberately does NOT do:
 *   - It does not touch the frozen slice (the registered
 *     instrument stays untouched; this is readiness, not an
 *     instrument — ADR-0028 Decision 7).
 *   - It claims nothing about humans. A scripted session drives
 *     it; the E-series legs are the maintainer's to commission.
 *
 * Usage:
 *   if5c_calc            render the registered session to
 *                        if5c_calc_session.bmp (a FIXED name — the
 *                        shipping demos' convention: no path ever
 *                        crosses from argv into a file call)
 *   if5c_calc --undo     render the one-undo variant to
 *                        if5c_calc_undo.bmp instead
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "planex/if5c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * The machine, as regions
 * ============================================================ */

typedef struct {
    if5_world* w;
    if5_sid u;
    uint16_t disp, acc, op, flag;      /* the state registers */
    uint16_t keys;                     /* the keypad root */
    uint16_t btn[16];                  /* 0-9, +,-,*,/, =, C */
    char btn_label[16];
} calc;

static void calc_set(const calc* c, uint16_t rid, const char* s)
{
    uint64_t v = 0;
    int i;
    for (i = 0; s[i] && i < 8; i++)
        v |= (uint64_t)(uint8_t)s[i] << (8 * i);
    assert(if5_user_act(c->w, c->u, rid, IF5_T_STR, v));
}

static void calc_get(const calc* c, uint16_t rid, char* out, size_t cap)
{
    uint64_t v;
    if5_view now;
    int i, n = 0;
    int limit = (int)cap - 1;      /* room for the terminator */
    if (limit > 8) limit = 8;
    if (limit < 0) limit = 0;
    out[0] = 0;
    now = if5_now(c->w);
    v = if5_state_at(c->w, rid, &now, NULL);
    for (i = 0; i < 8; i++) {
        uint8_t b = (uint8_t)(v >> (8 * i));
        if (!b) break;
        if (n < limit) {
            out[n] = (char)b;
            n++;
        }
    }
    out[n] = 0;                    /* n <= limit < cap, by construction */
}

static void press_digit(calc* c, char d)
{
    char disp[16], flag[16];
    calc_get(c, c->flag, flag, sizeof flag);
    calc_get(c, c->disp, disp, sizeof disp);
    if (flag[0] == '1') {                     /* fresh after '=' */
        calc_set(c, c->flag, "0");
        calc_set(c, c->acc, "0");
        calc_set(c, c->op, " ");
        disp[0] = 0;
    } else if (flag[0] == '2') {               /* fresh after an op */
        calc_set(c, c->flag, "0");
        disp[0] = 0;
    }
    if (strlen(disp) >= 8) return;             /* display capacity */
    {
        char nd[20];
        if (disp[0] == '0' && disp[1] == 0) disp[0] = 0;  /* no leading 0 */
        snprintf(nd, sizeof nd, "%s%c", disp, d);
        calc_set(c, c->disp, nd);
    }
}

static void press_op(calc* c, char o)
{
    char disp[16], acc[16];
    calc_get(c, c->disp, disp, sizeof disp);
    calc_get(c, c->acc, acc, sizeof acc);
    if (acc[0] == '0' || !acc[0]) {
        calc_set(c, c->acc, disp);              /* first operand */
    } else {
        /* chain: apply the pending op first */
        char prev[16];
        calc_get(c, c->op, prev, sizeof prev);
        if (prev[0] == '+' || prev[0] == '-' || prev[0] == '*' ||
            prev[0] == '/') {
            long a = atol(acc), b = atol(disp), r = b;
            if (prev[0] == '+') r = a + b;
            if (prev[0] == '-') r = a - b;
            if (prev[0] == '*') r = a * b;
            if (prev[0] == '/' && b != 0) r = a / b;
            if (prev[0] == '/' && b == 0) r = 0;
            {
                char buf[24];
                snprintf(buf, sizeof buf, "%ld", r);
                calc_set(c, c->acc, buf);
            }
        }
    }
    calc_set(c, c->op, (char[2]){ o, 0 });
    calc_set(c, c->flag, "2");
}

static void press_eq(calc* c)
{
    char disp[16], acc[16], op[16];
    long a, b, r = 0;
    calc_get(c, c->acc, acc, sizeof acc);
    calc_get(c, c->disp, disp, sizeof disp);
    calc_get(c, c->op, op, sizeof op);
    a = atol(acc);
    b = atol(disp);
    if (op[0] == '+') r = a + b;
    else if (op[0] == '-') r = a - b;
    else if (op[0] == '*') r = a * b;
    else if (op[0] == '/' && b != 0) r = a / b;
    else if (op[0] == '/' && b == 0) r = 0;
    else r = b;                                  /* no op: identity */
    {
        char buf[24];
        snprintf(buf, sizeof buf, "%ld", r);
        calc_set(c, c->disp, buf);
    }
    calc_set(c, c->flag, "1");
}

static void press_clear(calc* c)
{
    calc_set(c, c->disp, "0");
    calc_set(c, c->acc, "0");
    calc_set(c, c->op, " ");
    calc_set(c, c->flag, "0");
}

/* The button dispatch — THE client's hit test is the input path:
 * the caller passes PIXELS, the layout answers the region, the
 * region answers the button. */
static void press_at(calc* c, const if5c_layout* l, int x, int y)
{
    uint16_t rid = if5c_hit(l, x, y);
    int i;
    if (rid == c->disp || rid == c->acc || rid == c->op ||
        rid == c->flag || rid == 0)
        return;                                  /* not a button */
    for (i = 0; i < 16; i++)
        if (c->btn[i] == rid) {
            char label = c->btn_label[i];
            if (label >= '0' && label <= '9') press_digit(c, label);
            else if (label == '=') press_eq(c);
            else if (label == 'C') press_clear(c);
            else press_op(c, label);
            return;
        }
}

/* ============================================================
 * The construction and the registered session
 * ============================================================ */

static void build_calc(calc* c)
{
    if5_world* w = if5_world_create();
    if5_payload_type t;
    if5_source s;
    static const char labels[16] = {
        '7', '8', '9', '/',
        '4', '5', '6', '*',
        '1', '2', '3', '-',
        '0', 'C', '=', '+'
    };
    int i;

    assert(w);
    memset(&t, 0, sizeof t);
    t.name = "seq";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == 2);
    memset(&t, 0, sizeof t);
    t.name = "str";
    t.value_size = 8;
    assert(if5_register_payload_type(w, &t) == IF5_T_STR);

    memset(&s, 0, sizeof s);
    memcpy(s.id, "u", 1);
    s.kind = IF5_KIND_PERSON;
    s.trust = 3;
    s.authority = 0xFFFFFFFFu;
    c->u = if5_add_source(w, &s);
    c->w = w;

    /* the state registers, under a calculator root */
    {
        uint16_t root = if5_region_create(w, c->u, "calc", 0,
                                          IF5_T_NUM, IF5_TIER_MERGE,
                                          NULL, 0);
        c->disp = if5_region_create(w, c->u, "display", root,
                                    IF5_T_STR, IF5_TIER_STRICT,
                                    NULL, 0);
        c->acc  = if5_region_create(w, c->u, "acc", root,
                                    IF5_T_STR, IF5_TIER_STRICT,
                                    NULL, 0);
        c->op   = if5_region_create(w, c->u, "op", root,
                                    IF5_T_STR, IF5_TIER_STRICT,
                                    NULL, 0);
        c->flag = if5_region_create(w, c->u, "flag", root,
                                    IF5_T_STR, IF5_TIER_STRICT,
                                    NULL, 0);
        c->keys = if5_region_create(w, c->u, "keypad", root,
                                    IF5_T_NUM, IF5_TIER_MERGE,
                                    NULL, 0);
        assert(root && c->disp && c->acc && c->op && c->flag &&
               c->keys);
    }

    /* the keypad subtree: sixteen buttons, declaration order = the
     * grid's row-major order (the layout's pre-order columns) */
    for (i = 0; i < 16; i++) {
        char name[8];
        snprintf(name, sizeof name, "k%c", labels[i]);
        c->btn[i] = if5_region_create(w, c->u, name, c->keys,
                                      IF5_T_STR, IF5_TIER_STRICT,
                                      NULL, 0);
        c->btn_label[i] = labels[i];
        assert(c->btn[i]);
    }

    /* the initial state */
    calc_set(c, c->disp, "0");
    calc_set(c, c->acc, "0");
    calc_set(c, c->op, " ");
    calc_set(c, c->flag, "0");
}

/* The registered session: 7 8 + 6 = (then, optionally, one undo) */
static void run_session(calc* c, const if5c_policy* p, const char* path,
                        uint32_t undo_from_end)
{
    if5c_layout* l = if5c_layout_new();
    if5c_fb* fb;
    int i;

    /* the presses, driven through the hit test: the session's
     * next character names the button, the button's cell center
     * is the "tap", and if5c_hit is the entire input path */
    const char session[] = "78+6=";
    for (i = 0; session[i]; i++) {
        if5c_layout* li = if5c_layout_new();
        int k;
        uint32_t j;
        int tapped = 0;
        assert(if5c_layout_at(li, c->w, NULL, p) > 0);
        for (k = 0; k < 16 && !tapped; k++) {
            if (c->btn_label[k] != session[i]) continue;
            for (j = 0; j < li->n; j++) {
                if (li->cells[j].rid == c->btn[k]) {
                    press_at(c, li,
                             li->cells[j].x + li->cells[j].w / 2,
                             li->cells[j].y + li->cells[j].h / 2);
                    tapped = 1;
                    break;
                }
            }
        }
        assert(tapped);
        if5c_layout_free(li);
    }

    /* the optional undo: time is addressable, so "before the ="
     * is a view, and the world re-renders as it stood */
    if (undo_from_end) {
        uint32_t target = if5_log_count(c->w);
        uint32_t k;
        for (k = 0; k < undo_from_end; k++) {
            /* walk back to the k-th-from-end submission by the
             * display register's newest contributor */
            target = if5_now(c->w).cut - k - 1;
            (void)target;
        }
        /* undo the last submission to the display (= pressed):
         * find it, then rewind */
        {
            if5_view now = if5_now(c->w);
            uint32_t iid = 0, j;
            for (j = now.cut; j >= 1; j--) {
                const if5_intent* r = if5_log_at(c->w, j);
                if (r->region == c->disp) { iid = j; break; }
            }
            if (iid) {
                /* the undo happens; the FINAL frame below then
                 * renders the world as it stood before '=' — one
                 * golden, the undone state, no side file */
                assert(if5_undo_nth(c->w, c->u, iid) != 0);
            }
        }
    }

    /* the golden frame */
    assert(if5c_layout_at(l, c->w, NULL, p) > 0);
    fb = if5c_fb_new(l->width, l->height);
    assert(fb);
    assert(if5c_render(fb, l, p) == 0);
    assert(if5c_fb_save_bmp(fb, path) == 0);
    if5c_fb_free(fb);
    if5c_layout_free(l);
}

/* The demo's own policy: compact cells, a narrow attention
 * margin — the demo's presentation choice, pinned by its
 * goldens (ADR-0028 CAVEATS: a policy change moves them). */
static if5c_policy calc_policy(void)
{
    if5c_policy p = if5c_policy_default();
    p.cell_w = 44;
    p.cell_h = 16;
    p.step_x = 48;
    p.row_h = 20;
    p.base_x = 8;
    p.base_y = 8;
    p.mark_w = 40;
    p.mark_h = 10;
    p.margin_w = 56;
    return p;
}

int main(int argc, char** argv)
{
    calc c;
    if5c_policy p = calc_policy();
    const char* out = "if5c_calc_session.bmp";
    uint32_t undo = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--undo") == 0)
            { undo = 1; out = "if5c_calc_undo.bmp"; }
    }

    memset(&c, 0, sizeof c);
    build_calc(&c);
    run_session(&c, &p, out, undo);

    {
        char disp[16];
        const char* want = undo ? "6" : "84";
        /* without undo: 78 + 6 = 84. With one undo: the '=' is
         * revoked and the display re-renders as it stood — "6" —
         * which is N2's addressable time, drawn, not a failure */
        calc_get(&c, c.disp, disp, sizeof disp);
        printf("session 7 8 + 6 =%s -> display \"%s\"\n",
               undo ? " (then one undo)" : "", disp);
        if (strcmp(disp, want) != 0) {
            fprintf(stderr, "DEMO FAIL: display is \"%s\", want "
                    "\"%s\"\n", disp, want);
            return 1;
        }
    }
    if5_world_free(c.w);
    return 0;
}
