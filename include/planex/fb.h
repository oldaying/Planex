/*
 * fb.h — Framebuffer renderer for Planex Stage 1
 *
 * Design: Render is a *Closure*, not a new abstraction. It is the
 * "project to pixels" closure, fed by Relation graph + Estimates.
 *
 * This keeps Stage 0.5's abstraction completeness intact:
 *   Relation + Estimate + Closure (no 4th abstraction).
 *
 * Stage 1 framebuffer:
 *   - 32bpp RGBA, software rasterized
 *   - Fill rect / draw text (8x16 bitmap font, ASCII printable)
 *   - Output as BMP file (no platform dependency, no window system)
 *
 * Stage 2+ will add: X11 / Win32 / Cocoa / Wayland backends
 * as alternative Render closures (same Relation/Estimate input).
 */
#ifndef PLANEX_FB_H
#define PLANEX_FB_H

#include "planex/planex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Framebuffer
 * ============================================================ */

typedef struct px_fb px_fb;

px_fb* px_fb_new(int width, int height);
void   px_fb_free(px_fb* fb);

int    px_fb_width(const px_fb* fb);
int    px_fb_height(const px_fb* fb);

/* Resize the framebuffer. Existing pixel contents are lost
 * (cleared to 0). Returns 0 on success, -1 on failure (fb stays
 * at old size on failure).
 *
 * Stage 6: called by window backend on ConfigureNotify.
 * Application should re-read width/height on every render. */
int    px_fb_resize(px_fb* fb, int width, int height);

/* Clear to color (alpha ignored, treated as opaque bg). */
void   px_fb_clear(px_fb* fb, uint32_t rgba);

/* Fill axis-aligned rect. (x,y) is top-left, origin top-left. */
void   px_fb_fill_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba);

/* Draw rect outline (1px). */
void   px_fb_draw_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba);

/* Draw horizontal/vertical line. */
void   px_fb_draw_hline(px_fb* fb, int x, int y, int len, uint32_t rgba);
void   px_fb_draw_vline(px_fb* fb, int x, int y, int len, uint32_t rgba);

/* Set a single pixel. */
void   px_fb_set_pixel(px_fb* fb, int x, int y, uint32_t rgba);

/* Get raw pixel buffer (read-only access for backends).
 * Returns pointer to width*height uint32_t values, format 0xAARRGGBB.
 * Pointer is owned by fb; do not free. */
const uint32_t* px_fb_pixels(const px_fb* fb);

/* v0.6: Get raw pixel buffer (mutable access for backends / blitters).
 * Same lifetime and format rules as px_fb_pixels(); use when a bulk
 * copy needs to write pixels (e.g. row-wise memcpy of a perception's
 * framebuffer into a window's framebuffer). */
uint32_t* px_fb_pixels_mutable(px_fb* fb);

/* Read a single pixel. */
uint32_t px_fb_get_pixel(const px_fb* fb, int x, int y);

/* ============================================================
 * Text rendering (8x16 bitmap font)
 * ============================================================ */

/* Draw a single character at (x, y). Returns advance (8).
 * Color is foreground; background is transparent. */
int    px_fb_draw_char(px_fb* fb, int x, int y, char c, uint32_t fg_rgba);

/* Draw a string. Returns advance (length * 8). */
int    px_fb_draw_text(px_fb* fb, int x, int y, const char* s, uint32_t fg_rgba);

/* Draw a string with background fill. */
int    px_fb_draw_text_bg(px_fb* fb, int x, int y, const char* s,
                          uint32_t fg_rgba, uint32_t bg_rgba);

/* ============================================================
 * TTF font rendering (Stage 10)
 *
 * FreeType-based scalable font support. Allows loading arbitrary
 * TTF/OTF fonts at arbitrary sizes — including CJK fonts.
 *
 * Usage:
 *   px_font* font = px_font_load("/usr/share/fonts/.../NotoSerifSC-Regular.ttf", 16);
 *   if (font) {
 *       px_fb_draw_text_utf8(fb, 10, 10, "Hello world", font, PX_TEXT);
 *       px_font_free(font);
 *   } else {
 *       // fallback to default 8x16 bitmap (ASCII only)
 *       px_fb_draw_text(fb, 10, 10, "Hello", PX_TEXT);
 *   }
 *
 * Glyph cache: rendered glyphs are cached internally; subsequent
 * draws of the same char are fast (no re-rasterization).
 * ============================================================ */

typedef struct px_font px_font;

/* Load a TTF/OTF font at given pixel size. Returns NULL on failure.
 * Stage 10: requires FreeType2 at link time (-lfreetype).
 * Note: caller must provide the full filesystem path to the TTF.
 * For automatic font discovery by name, use px_font_find() instead. */
px_font* px_font_load(const char* ttf_path, int pixel_size);

/* Find and load a font by family name (e.g. "Noto Serif SC",
 * "DejaVu Sans", "Noto Color Emoji").
 *
 * Uses fontconfig to resolve the name to a file path, then loads
 * via FreeType. If fontconfig is not available at build time,
 * falls back to a hardcoded list of common paths.
 *
 * Returns NULL if the font cannot be found or loaded.
 * Stage 13: requires libfontconfig at link time (-lfontconfig). */
px_font* px_font_find(const char* family_name, int pixel_size);

/* Get the built-in 8x16 bitmap font as a px_font (no FreeType needed).
 * ASCII only (32..126). Always succeeds. */
px_font* px_font_default(void);

/* Free a font. Safe to call on default font (no-op). */
void     px_font_free(px_font* font);

/* Stage 11: Fallback font chain.
 *
 * When the primary font lacks a glyph for a codepoint, the renderer
 * tries each fallback in order. Useful for:
 *   - Primary: Noto Serif SC (CJK)        — covers Chinese
 *   - Fallback 1: Noto Color Emoji         — covers emoji
 *   - Fallback 2: DejaVu Sans Mono          — covers Latin fallback
 *
 * Fallback fonts inherit the primary's pixel size. Adding a fallback
 * to the default font is a no-op (default font uses bitmap rendering).
 *
 * px_font_add_fallback: takes a file path (Stage 11)
 * px_font_add_fallback_named: takes a family name, uses fontconfig (Stage 13)
 *
 * Returns 0 on success, -1 on failure (font load failed). */
int      px_font_add_fallback(px_font* font, const char* ttf_path);
int      px_font_add_fallback_named(px_font* font, const char* family_name);

/* Get font metrics: line height, ascent, descent. */
int      px_font_line_height(px_font* font);
int      px_font_ascent(px_font* font);
int      px_font_descent(px_font* font);

/* Draw a UTF-8 string at (x, y). Returns advance width in pixels.
 * Handles multi-byte sequences (CJK, emoji). Falls back to the
 * default font's '?' glyph for chars not in any font of the chain. */
int      px_fb_draw_text_utf8(px_fb* fb, int x, int y, const char* utf8_str,
                               px_font* font, uint32_t fg_rgba);

/* Draw a single Unicode codepoint. Returns advance. */
int      px_fb_draw_glyph_utf8(px_fb* fb, int x, int y, uint32_t codepoint,
                                 px_font* font, uint32_t fg_rgba);

/* ============================================================
 * Color helpers
 * ============================================================ */

#define PX_RGBA(r,g,b,a) (((uint32_t)(a)<<24)|((uint32_t)(r)<<16)| \
                          ((uint32_t)(g)<<8)|((uint32_t)(b)))

#define PX_RGB(r,g,b)    PX_RGBA(r,g,b,255)

/* Built-in palette */
#define PX_BLACK         PX_RGB(0x0c, 0x0d, 0x12)
#define PX_BG            PX_RGB(0x0c, 0x0d, 0x12)
#define PX_SURFACE       PX_RGB(0x1f, 0x20, 0x29)
#define PX_HOVER         PX_RGB(0x18, 0x19, 0x24)
#define PX_PRESSED       PX_RGB(0x14, 0x15, 0x19)
#define PX_TEXT          PX_RGB(0xeb, 0xeb, 0xf0)
#define PX_TEXT_DIM      PX_RGB(0x8c, 0x90, 0x99)
#define PX_BORDER        PX_RGB(0x33, 0x36, 0x42)
#define PX_ACCENT        PX_RGB(0x66, 0x90, 0xf0)
#define PX_ACCENT_HOVER  PX_RGB(0x80, 0xa4, 0xf5)
#define PX_SUCCESS       PX_RGB(0x4e, 0xc9, 0xb0)
#define PX_WARNING       PX_RGB(0xd1, 0x9a, 0x66)
#define PX_DANGER        PX_RGB(0xf4, 0x87, 0x87)

/* ============================================================
 * Output (BMP file, no platform dependency)
 * ============================================================ */

/* Save framebuffer as 32bpp BMP file. Returns 0 on success. */
int    px_fb_save_bmp(px_fb* fb, const char* path);

/* Dump to stdout as ASCII art (16 levels). */
void   px_fb_dump_ascii(px_fb* fb);

#ifdef __cplusplus
}
#endif

#endif /* PLANEX_FB_H */
