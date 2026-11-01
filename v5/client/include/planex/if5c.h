/*
 * if5c.h — the presentation client of the v5 semantic tree
 * (ADR-0028 Decision 2): the S15 "vision is one projection"
 * clause, executed as a CLIENT.
 *
 * The one-way rule this header lives by (ADR-0028 Decision 1):
 * the client LINKS the core; the core never links the client —
 * statically enforced (scripts/check_slice_rebound.sh's
 * dependency-direction ban). The client modules render, hit-test,
 * interpolate, and annotate; they never submit, append, or debit
 * (the client purity ban, same gate). The application — the demo
 * in v5/client/apps/, or any program — submits through the
 * ordinary channel and renders through this one.
 *
 * Determinism is the client's whole value proposition: the same
 * (world, view, policy) lays out and renders to byte-identical
 * pixels every time, in every process, across save/load round
 * trips (F2). No wall clock, no floating point anywhere in the
 * render path — easing maps a RATIONAL t (num/den) through exact
 * integer arithmetic, because a frame sequence that is a function
 * is a frame sequence that can be golden-tested (ADR-0028
 * Decision 2d).
 *
 * NOT CARRIED (ADR-0028 Decision 2's contract columns): fonts
 * beyond the derived 5x7, anti-aliasing, output formats beyond
 * BMP/ASCII, the five presentation relation kinds (BESIDE,
 * AFFORDS, WITHDRAWS_FOR, PRESENTS_FOR, INTERPRETS_AS — the
 * layout derives STRUCTURALLY from the tree; asserted layout
 * edges stay gone, client included), wall-clock frame timing (a
 * t-mapping, never a clock), and annotation durability (client
 * session state, not world state).
 */
#ifndef PLANEX_IF5C_H
#define PLANEX_IF5C_H

#include "planex/if5.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * The framebuffer (2a)
 * ============================================================ */

typedef struct if5c_fb if5c_fb;

if5c_fb* if5c_fb_new(int width, int height);
void     if5c_fb_free(if5c_fb* fb);
int      if5c_fb_width(const if5c_fb* fb);
int      if5c_fb_height(const if5c_fb* fb);

/* 0x00RRGGBB (the alpha byte is not part of the client's
 * vocabulary; BMP output is 24-bit). Out-of-bounds writes are
 * ignored; out-of-bounds reads return the background. */
#define IF5C_RGB(r, g, b) \
    ((uint32_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

void     if5c_fb_clear(if5c_fb* fb, uint32_t rgb);
void     if5c_fb_set(if5c_fb* fb, int x, int y, uint32_t rgb);
uint32_t if5c_fb_get(const if5c_fb* fb, int x, int y);
void     if5c_fb_fill_rect(if5c_fb* fb, int x, int y, int w, int h,
                           uint32_t rgb);
void     if5c_fb_frame_rect(if5c_fb* fb, int x, int y, int w, int h,
                            uint32_t rgb);

/* 5x7 font, 6px advance; returns the advance. The background is
 * opaque (text is drawn as a solid block — no blending, by the
 * no-floating-point rule). */
int      if5c_fb_draw_text(if5c_fb* fb, int x, int y, const char* s,
                           uint32_t fg, uint32_t bg);

/* The canonical BMP (24-bit BI_RGB, no timestamps — the golden
 * format). Returns 0 on success. */
int      if5c_fb_save_bmp(const if5c_fb* fb, const char* path);

/* Determinism primitives: FNV-1a over the pixel plane, and the
 * raw byte image (RGB, row-major) for exact memcmp. */
uint32_t if5c_fb_hash(const if5c_fb* fb);
int      if5c_fb_bytes(const if5c_fb* fb, uint8_t* out, int cap);

/* The ASCII dump (the text-assertable rendering — what a
 * screen-reader-shaped test consumes). Writes a NUL-terminated,
 * newline-terminated map; returns the chars written. */
int      if5c_fb_dump_ascii(const if5c_fb* fb, char* out, int cap);

/* ============================================================
 * The layout (2b): the tree at a view, given geometry
 * ============================================================ */

#define IF5C_MAX_MARKS 64

typedef struct if5c_cell {
    /* the node, verbatim from the tree walk */
    char     name[IF5_NAME_MAX];
    uint64_t value;
    uint32_t contrib;
    uint32_t last_iid;
    uint16_t rid;
    uint16_t parent;
    uint16_t depth;
    uint8_t  ptype;
    uint8_t  tier;
    /* the geometry the policy assigned (deterministic) */
    int      x, y, w, h;
} if5c_cell;

typedef struct if5c_mark {
    /* one S17 schedule entry, drawn in the attention margin */
    uint32_t iid;
    uint32_t magnitude;
    uint16_t region;
    uint8_t  sid;
    uint8_t  kind;          /* IF5_SCHED_* */
    int      x, y, w, h;
} if5c_mark;

typedef struct if5c_layout {
    if5c_cell* cells;       /* tree order: DFS pre-order */
    uint32_t   n;
    if5c_mark  marks[IF5C_MAX_MARKS];
    uint32_t   n_marks;     /* entries placed (truncation is
                             * visible: n_marks == IF5C_MAX_MARKS
                             * means "at least this many") */
    int width, height;      /* the composed surface */
} if5c_layout;

/* The policy: every geometric and chromatic constant in one
 * struct. Changing a constant moves the goldens — that is what
 * they are for. Tier emphasis: index 0..2 = tiers 1..3. Mark
 * colors: index 1..5 = IF5_SCHED_*. */
typedef struct if5c_policy {
    int      cell_w, cell_h;
    int      step_x;        /* column pitch (pre-order columns) */
    int      row_h;         /* depth rows */
    int      base_x, base_y;
    int      mark_w, mark_h;
    int      margin_w;      /* the attention margin's width */
    uint32_t bg;
    uint32_t fg;
    uint32_t grid;          /* the depth guide lines */
    uint32_t tier_rgb[3];
    uint32_t mark_rgb[6];
} if5c_policy;

if5c_policy  if5c_policy_default(void);
if5c_layout* if5c_layout_new(void);
void         if5c_layout_free(if5c_layout* l);

/* The pure function at the center of the stage: walk the tree at
 * the view, assign geometry (depth is the row; the pre-order
 * walk is the column, so children keep declaration order
 * left-to-right), place the schedule's entries in the attention
 * margin. Returns the cell count (0 = error). The world's
 * durable image is unchanged (F1) — the layout reads, it never
 * writes. */
uint32_t     if5c_layout_at(if5c_layout* out, if5_world* w,
                         const if5_view* v, const if5c_policy* p);

/* Render a layout onto a framebuffer (deterministic draw order:
 * background, depth guides, cells in tree order, marks in
 * order). Returns 0 on success. */
int          if5c_render(if5c_fb* fb, const if5c_layout* l,
                         const if5c_policy* p);

/* ============================================================
 * The hit test (2c): the inverse of the layout
 * ============================================================ */

/* The region whose cell contains the point, 0 if none. Well-
 * defined because the layout is a pure function (ADR-0028
 * Decision 2c). */
uint16_t     if5c_hit(const if5c_layout* l, int x, int y);

/* ============================================================
 * The easing (2d): the two-sample trajectory, as a t-mapping
 * ============================================================ */

typedef enum {
    IF5C_EASE_LINEAR    = 0,
    IF5C_EASE_SMOOTHSTEP = 1
} if5c_ease_family;

/* frame(t) = a + (b - a) * ease(t), per pixel channel, exact
 * integer arithmetic on the rational t = num/den (den > 0).
 * t = 0 renders a byte-exactly; t = 1 renders b byte-exactly;
 * both families are monotone in t per channel (F5). Frames must
 * share dimensions. Returns 0 on success. */
int          if5c_ease_frame(if5c_fb* out, const if5c_fb* a,
                            const if5c_fb* b, uint32_t num,
                            uint32_t den, if5c_ease_family f);

/* The evaluated t under a family (exposed for the tests'
 * monotonicity proof): linear -> num/den; smoothstep ->
 * (3t^2 - 2t^3) — both as exact rationals over 64-bit
 * intermediates, returned as (out_num, out_den) normalized to
 * out_den = den^3. */
void         if5c_ease_t(uint32_t num, uint32_t den,
                         if5c_ease_family f,
                         uint64_t* out_num, uint64_t* out_den);

/* ============================================================
 * The annotation table (2e): cancel reasons, keyed by address
 * ============================================================ */

#define IF5C_MAX_ANNOTATIONS 64

typedef struct if5c_annot if5c_annot;

if5c_annot*  if5c_annot_new(void);
void         if5c_annot_free(if5c_annot* an);

/* Attach a reason to a cut registry id (ADR-0027 mapping (g)'s
 * successor question, discharged WITHOUT a record field: the cut
 * is the semantic content, the reason is presentation). Later
 * writes to the same id replace. Returns false on a full table
 * or a bad id. */
bool         if5c_annotate(if5c_annot* an, uint8_t cut_id,
                           const char* reason);
const char*  if5c_annotation_of(const if5c_annot* an, uint8_t cut_id);
uint32_t     if5c_annotation_count(const if5c_annot* an);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_IF5C_H */
