/*
 * if5c_layout.c — the layout engine (ADR-0028 Decision 2b):
 * the semantic tree at a view, assigned geometry DETERMINISTICALLY.
 *
 * The rule (registered in the ADR, stated precisely here):
 *   depth is the ROW      — y = base_y + depth * row_h;
 *   pre-order is the COLUMN — x = base_x + walk_index * step_x,
 *     so children keep declaration order left-to-right (the
 *     pre-order walk visits each parent before its children and
 *     each parent's children in declaration order — the column
 *     order IS the tree order, and the tree order IS the
 *     declaration order).
 * Every cell lands in a unique (row, column) grid slot — total,
 * deterministic, collision-free by construction. The S17
 * schedule renders as the attention margin on the right: one
 * marker per entry, kind-glyphed and severity-graded.
 *
 * The layout derives STRUCTURALLY from the tree (name, value by
 * payload family, tier emphasis, contribution count). The five
 * presentation relation kinds (BESIDE, AFFORDS, WITHDRAWS_FOR,
 * PRESENTS_FOR, INTERPRETS_AS) are NOT carried — not here, not
 * as client metadata: carrying asserted layout edges would
 * resurrect, one zone over, the asserted-edge pattern the
 * cause-graph replaced (ADR-0028 Decision 2b, the stage's
 * sharpest not-carried claim; a measured need is the refutation
 * signal).
 *
 * A pure query: reads the world, writes nothing of it (F1).
 */
#include "planex/if5c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

if5c_policy if5c_policy_default(void)
{
    if5c_policy p;
    memset(&p, 0, sizeof p);
    p.cell_w = 104;
    p.cell_h = 24;
    p.step_x = 112;                /* cell + 8 gutter */
    p.row_h  = 32;                 /* cell + 8 gutter */
    p.base_x = 12;
    p.base_y = 12;
    p.mark_w = 96;
    p.mark_h = 16;
    p.margin_w = 120;
    p.bg    = IF5C_RGB(0xF4, 0xF2, 0xEC);   /* warm paper */
    p.fg    = IF5C_RGB(0x1A, 0x1A, 0x1A);
    p.grid  = IF5C_RGB(0xDD, 0xD8, 0xCC);
    p.tier_rgb[0] = IF5C_RGB(0x2A, 0x6B, 0xB8);  /* fork  — blue  */
    p.tier_rgb[1] = IF5C_RGB(0x2E, 0x8B, 0x57);  /* merge — green */
    p.tier_rgb[2] = IF5C_RGB(0xB8, 0x3A, 0x2A);  /* strict— red   */
    p.mark_rgb[IF5_SCHED_SURPRISE] = IF5C_RGB(0xB8, 0x3A, 0x2A);
    p.mark_rgb[IF5_SCHED_BREAKER]  = IF5C_RGB(0x7A, 0x1F, 0x7A);
    p.mark_rgb[IF5_SCHED_TIER]     = IF5C_RGB(0x2A, 0x6B, 0xB8);
    p.mark_rgb[IF5_SCHED_ARBIT]    = IF5C_RGB(0x8A, 0x6B, 0x2A);
    p.mark_rgb[IF5_SCHED_MERGE]    = IF5C_RGB(0x2E, 0x8B, 0x57);
    return p;
}

if5c_layout* if5c_layout_new(void)
{
    if5c_layout* l = (if5c_layout*)calloc(1, sizeof *l);
    return l;
}

void if5c_layout_free(if5c_layout* l)
{
    if (!l) return;
    free(l->cells);
    free(l);
}

/* The value text, formatted by payload family — the three
 * built-in route formats mirrored client-side (NUM decimal, SET
 * population, STR bytes), and a hex form for registered
 * extensions. Deterministic, always NUL-terminated. */
static void format_value(char* out, size_t cap, uint8_t ptype,
                         uint64_t value)
{
    if (cap == 0) return;
    out[0] = 0;
    switch (ptype) {
    case IF5_T_NUM:
        snprintf(out, cap, "%u", (uint32_t)(value & 0xFFFFFFFFu));
        break;
    case IF5_T_SET: {
        /* the population of the tag set — the informative half
         * of a 64-bit bitset at cell width */
        uint64_t v = value;
        uint32_t n = 0;
        while (v) { n += (uint32_t)(v & 1u); v >>= 1; }
        snprintf(out, cap, "set:%u", n);
        break;
    }
    case IF5_T_SEQ:
        snprintf(out, cap, "seq:%u", (uint32_t)(value & 0xFFFFFFFFu));
        break;
    case IF5_T_STR: {
        /* the 8 payload bytes, printable or '.' */
        int i;
        size_t pos = 0;
        for (i = 0; i < 8 && pos + 2 < cap; i++) {
            uint8_t b = (uint8_t)(value >> (8 * i));
            out[pos++] = (b >= 32 && b <= 126) ? (char)b : '.';
        }
        out[pos] = 0;
        break;
    }
    default:
        snprintf(out, cap, "%016llx",
                 (unsigned long long)value);
        break;
    }
}

typedef struct walk_ctx {
    if5c_layout* l;
    const if5c_policy* p;
    uint32_t idx;
    uint32_t cap;
    int max_x, max_y;
} walk_ctx;

static bool on_node(const if5_node* n, void* user)
{
    walk_ctx* ctx = (walk_ctx*)user;
    if5c_cell* c;

    if (ctx->idx >= ctx->cap) return false;      /* stop the walk */
    c = &ctx->l->cells[ctx->idx];
    ctx->idx++;

    memcpy(c->name, n->name, IF5_NAME_MAX);
    c->value    = n->value;
    c->contrib  = n->contrib;
    c->last_iid = n->last_iid;
    c->rid      = n->rid;
    c->parent   = n->parent;
    c->depth    = n->depth;
    c->ptype    = n->ptype;
    c->tier     = n->tier;
    c->x = ctx->p->base_x + (int)ctx->idx * 0 /* placeholder */
         + (int)(ctx->idx - 1) * ctx->p->step_x;
    c->y = ctx->p->base_y + (int)n->depth * ctx->p->row_h;
    c->w = ctx->p->cell_w;
    c->h = ctx->p->cell_h;
    if (c->x + c->w > ctx->max_x) ctx->max_x = c->x + c->w;
    if (c->y + c->h > ctx->max_y) ctx->max_y = c->y + c->h;
    return true;
}

uint32_t if5c_layout_at(if5c_layout* out, if5_world* w, const if5_view* v,
                     const if5c_policy* p)
{
    walk_ctx ctx;
    if5_view def;
    if5c_policy pol;
    uint32_t total, placed;
    if5_sched_entry entries[IF5C_MAX_MARKS];
    uint32_t i;

    if (!out || !w) return 0;
    if (!p) { pol = if5c_policy_default(); p = &pol; }
    if (!v) { def = if5_now(w); v = &def; }

    /* count first (the walk's counting contract), allocate once */
    total = if5_tree_walk(w, v, NULL, NULL);
    if (!total) return 0;

    free(out->cells);
    out->cells = (if5c_cell*)calloc(total, sizeof(if5c_cell));
    if (!out->cells) { out->n = 0; return 0; }

    ctx.l = out;
    ctx.p = p;
    ctx.idx = 0;
    ctx.cap = total;
    ctx.max_x = 0;
    ctx.max_y = 0;
    (void)if5_tree_walk(w, v, on_node, &ctx);
    out->n = ctx.idx;

    /* the attention margin: the S17 schedule at the view, newest
     * first is NOT the rule — the schedule's own order (oldest
     * first, the log's order) is, because the timetable is a
     * history, not a stack. The margin is ONE fixed column at the
     * cells' right edge (the x is captured before any mark extends
     * the surface — a staircase of marks was the first draft's
     * geometry, and the golden caught it). */
    out->n_marks = 0;
    memset(entries, 0, sizeof entries);
    if (if5_schedule(w, v, IF5_SID_NULL, entries, IF5C_MAX_MARKS)) {
        int margin_x = ctx.max_x + 16;
        placed = if5_schedule(w, v, IF5_SID_NULL, entries, IF5C_MAX_MARKS);
        if (placed > IF5C_MAX_MARKS) placed = IF5C_MAX_MARKS;
        for (i = 0; i < placed; i++) {
            if5c_mark* m = &out->marks[i];
            m->iid      = entries[i].iid;
            m->magnitude= entries[i].magnitude;
            m->region   = entries[i].region;
            m->sid      = entries[i].sid;
            m->kind     = entries[i].kind;
            m->x = margin_x;
            m->y = p->base_y + (int)i * (p->mark_h + 4);
            m->w = p->mark_w;
            m->h = p->mark_h;
            if (m->x + m->w > ctx.max_x) ctx.max_x = m->x + m->w;
            if (m->y + m->h > ctx.max_y) ctx.max_y = m->y + m->h;
        }
        out->n_marks = placed;
    }

    out->width  = ctx.max_x + 12;
    out->height = ctx.max_y + 12;
    return out->n;
}

int if5c_render(if5c_fb* fb, const if5c_layout* l, const if5c_policy* p)
{
    uint32_t i;
    if5c_policy pol;
    if (!fb || !l || !l->cells) return -1;
    if (!p) { pol = if5c_policy_default(); p = &pol; }

    if5c_fb_clear(fb, p->bg);

    /* depth guides: one rule per tree row, spanning the cell
     * area — the reader's vertical anchor */
    {
        int d;
        int rows = 0;
        for (i = 0; i < l->n; i++)
            if (l->cells[i].depth + 1 > rows) rows = l->cells[i].depth + 1;
        for (d = 0; d < rows; d++)
            if5c_fb_fill_rect(fb, p->base_x,
                              p->base_y + d * p->row_h + p->cell_h + 4,
                              l->width - p->base_x * 2, 1, p->grid);
    }

    /* cells in tree order (deterministic draw order — z-order is
     * the tree order, so identical worlds paint identically) */
    for (i = 0; i < l->n; i++) {
        const if5c_cell* c = &l->cells[i];
        char line[24];
        uint32_t border = c->tier >= 1 && c->tier <= 3
                        ? p->tier_rgb[c->tier - 1] : p->fg;
        if5c_fb_fill_rect(fb, c->x, c->y, c->w, c->h,
                          IF5C_RGB(0xFF, 0xFF, 0xFF));
        if5c_fb_frame_rect(fb, c->x, c->y, c->w, c->h, border);
        if5c_fb_draw_text(fb, c->x + 4, c->y + 3, c->name, p->fg,
                          IF5C_RGB(0xFF, 0xFF, 0xFF));
        format_value(line, sizeof line, c->ptype, c->value);
        if5c_fb_draw_text(fb, c->x + 4, c->y + 13, line, p->fg,
                          IF5C_RGB(0xFF, 0xFF, 0xFF));
        /* the contribution count, right-aligned as a superscript
         * bar: a 4px tick per 16 contributions (capped) */
        {
            uint32_t ticks = (c->contrib + 15) / 16;
            uint32_t t;
            if (ticks > 8) ticks = 8;
            for (t = 0; t < ticks; t++)
                if5c_fb_fill_rect(fb, c->x + c->w - 4 - (int)t * 4,
                                  c->y + 2, 2, 3, border);
        }
    }

    /* the attention margin's markers */
    for (i = 0; i < l->n_marks; i++) {
        const if5c_mark* m = &l->marks[i];
        char glyph[2];
        uint32_t col = m->kind <= 5 ? p->mark_rgb[m->kind] : p->fg;
        if5c_fb_fill_rect(fb, m->x, m->y, m->w, m->h,
                          IF5C_RGB(0xFF, 0xFF, 0xFF));
        if5c_fb_frame_rect(fb, m->x, m->y, m->w, m->h, col);
        glyph[0] = m->kind == IF5_SCHED_SURPRISE ? 'S'
                 : m->kind == IF5_SCHED_BREAKER  ? 'B'
                 : m->kind == IF5_SCHED_TIER     ? 'T'
                 : m->kind == IF5_SCHED_ARBIT    ? 'A'
                 : m->kind == IF5_SCHED_MERGE    ? 'M' : '?';
        glyph[1] = 0;
        if5c_fb_draw_text(fb, m->x + 3, m->y + 4, glyph, col,
                          IF5C_RGB(0xFF, 0xFF, 0xFF));
        {
            /* the severity rung, one digit (clamped) */
            char rung[2];
            uint32_t r = m->magnitude > 9 ? 9 : m->magnitude;
            rung[0] = (char)('0' + r);
            rung[1] = 0;
            if5c_fb_draw_text(fb, m->x + m->w - 10, m->y + 4, rung,
                              p->fg, IF5C_RGB(0xFF, 0xFF, 0xFF));
        }
    }
    return 0;
}
