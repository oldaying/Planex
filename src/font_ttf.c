/*
 * font_ttf.c - TTF/OTF font rendering via FreeType (Stage 10+)
 *
 * - Loads TTF/OTF files at arbitrary pixel sizes
 * - Caches rasterized glyphs in a simple hash table (256 entries)
 * - Handles UTF-8 multi-byte sequences (CJK, emoji)
 * - Stage 11: fallback font chain (up to 4 fonts)
 * - Stage 12: color emoji support (FT_LOAD_COLOR + BGRA blit)
 * - Stage 13: fontconfig integration (find fonts by name)
 * - Falls back to built-in 8x16 bitmap font when:
 *   - px_font_default() is used (no TTF loaded)
 *   - FreeType fails to render a glyph
 *
 * Glyph cache: 256-entry direct-mapped (codepoint & 0xFF).
 * Collisions overwrite - simple, low memory, good for typical UI text.
 *
 * Build: needs libfreetype at link time:
 *   cc ... -lfreetype -lfontconfig
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/fb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define px_strdup _strdup
#else
#define px_strdup strdup
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef PLANEX_HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
/* ============================================================
 * Glyph cache entry
 * ============================================================ */

typedef struct {
    uint32_t  codepoint;     /* 0 = empty slot */
    int       width;
    int       height;
    int       bearing_x;
    int       bearing_y;
    int       advance_x;
    uint8_t*  bitmap;        /* owned:
                              *   grayscale: width * height bytes
                              *   color:     width * height * 4 bytes (BGRA) */
    bool      is_color;      /* Stage 12: true for color emoji (FT_PIXEL_MODE_BGRA) */
} px_glyph;

/* ============================================================
 * px_font
 *
 * Two modes:
 *   - is_default = true:  uses built-in 8x16 bitmap font (no FreeType)
 *   - is_default = false: uses FreeType face + glyph cache
 * ============================================================ */

#define PX_GLYPH_CACHE_SIZE 256
#define PX_FONT_MAX_FALLBACKS 4

struct px_font {
    bool             is_default;
    int              pixel_size;
    int              line_height;
    int              ascent;
    int              descent;
    /* TTF mode only */
    FT_Library       ft_lib;        /* shared across primary + fallbacks */
    FT_Face          ft_face;       /* primary face */
    px_glyph         cache[PX_GLYPH_CACHE_SIZE];  /* primary cache */
    /* Stage 11: Fallback chain */
    FT_Face          fallback_faces[PX_FONT_MAX_FALLBACKS];
    px_glyph         fallback_caches[PX_FONT_MAX_FALLBACKS][PX_GLYPH_CACHE_SIZE];
    int              n_fallbacks;
};

/* ============================================================
 * Default font (8x16 bitmap, ASCII only)
 *
 * Returns a singleton — px_font_free is a no-op for it.
 * ============================================================ */

extern const unsigned char* px_font8x16_glyph(char c);

static px_font s_default_font = {
    .is_default  = true,
    .pixel_size  = 16,
    .line_height = 16,
    .ascent      = 12,
    .descent     = 4,
};

px_font* px_font_default(void) {
    return &s_default_font;
}

/* ============================================================
 * TTF loading
 * ============================================================ */

px_font* px_font_load(const char* ttf_path, int pixel_size) {
    if (!ttf_path || pixel_size <= 0) return NULL;

    px_font* font = (px_font*)calloc(1, sizeof(px_font));
    if (!font) return NULL;
    font->is_default = false;
    font->pixel_size = pixel_size;
    font->n_fallbacks = 0;

    FT_Library lib;
    if (FT_Init_FreeType(&lib) != 0) {
        fprintf(stderr, "Planex font: FT_Init_FreeType failed\n");
        free(font);
        return NULL;
    }
    font->ft_lib = lib;

    FT_Face face;
    if (FT_New_Face(lib, ttf_path, 0, &face) != 0) {
        fprintf(stderr, "Planex font: cannot load %s\n", ttf_path);
        FT_Done_FreeType(lib);
        free(font);
        return NULL;
    }
    font->ft_face = face;

    /* Set pixel size — FreeType will compute metrics */
    if (FT_Set_Pixel_Sizes(face, 0, pixel_size) != 0) {
        fprintf(stderr, "Planex font: FT_Set_Pixel_Sizes failed\n");
        FT_Done_Face(face);
        FT_Done_FreeType(lib);
        free(font);
        return NULL;
    }

    /* Use face metrics for line height / ascent / descent */
    int units_per_em = face->units_per_EM;
    if (units_per_em > 0) {
        double scale = (double)pixel_size / (double)units_per_em;
        font->ascent      = (int)(face->ascender * scale + 0.5);
        font->descent     = (int)(-face->descender * scale + 0.5);
        font->line_height = font->ascent + font->descent;
    } else {
        font->line_height = pixel_size;
        font->ascent      = pixel_size;
        font->descent     = 0;
    }

    /* Select Unicode charmap */
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    /* Init primary cache */
    for (int i = 0; i < PX_GLYPH_CACHE_SIZE; i++) {
        font->cache[i].codepoint = 0;
        font->cache[i].bitmap = NULL;
    }
    /* Init fallback caches */
    for (int f = 0; f < PX_FONT_MAX_FALLBACKS; f++) {
        for (int i = 0; i < PX_GLYPH_CACHE_SIZE; i++) {
            font->fallback_caches[f][i].codepoint = 0;
            font->fallback_caches[f][i].bitmap = NULL;
        }
    }

    return font;
}

/* Stage 11: Add a fallback TTF to the chain.
 * The fallback inherits the primary's pixel_size.
 * Stage 12: handles fixed-size color fonts (e.g. NotoColorEmoji)
 * which reject FT_Set_Pixel_Sizes — falls back to FT_Select_Size
 * with closest strike. */
int px_font_add_fallback(px_font* font, const char* ttf_path) {
    if (!font || font->is_default || !ttf_path) return -1;
    if (font->n_fallbacks >= PX_FONT_MAX_FALLBACKS) {
        fprintf(stderr, "Planex font: max %d fallbacks reached\n", PX_FONT_MAX_FALLBACKS);
        return -1;
    }
    if (!font->ft_lib) return -1;

    FT_Face face;
    if (FT_New_Face(font->ft_lib, ttf_path, 0, &face) != 0) {
        fprintf(stderr, "Planex font: cannot load fallback %s\n", ttf_path);
        return -1;
    }

    /* Stage 12: color emoji fonts are fixed-size — try FT_Set_Pixel_Sizes
     * first (works for scalable fonts), fall back to FT_Select_Size
     * (for fixed-size fonts like NotoColorEmoji). */
    if (FT_Set_Pixel_Sizes(face, 0, font->pixel_size) != 0) {
        /* Fixed-size font — find closest strike */
        int best_idx = 0;
        FT_Long best_diff = 0x7FFFFFFF;
        for (int i = 0; i < face->num_fixed_sizes; i++) {
            FT_Bitmap_Size* sz = &face->available_sizes[i];
            FT_Long diff = sz->height - font->pixel_size;
            if (diff < 0) diff = -diff;
            if (diff < best_diff) {
                best_diff = diff;
                best_idx = i;
            }
        }
        if (face->num_fixed_sizes > 0) {
            if (FT_Select_Size(face, best_idx) != 0) {
                fprintf(stderr, "Planex font: cannot set size for %s\n", ttf_path);
                FT_Done_Face(face);
                return -1;
            }
        } else {
            fprintf(stderr, "Planex font: cannot set size for %s\n", ttf_path);
            FT_Done_Face(face);
            return -1;
        }
    }
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    font->fallback_faces[font->n_fallbacks] = face;
    font->n_fallbacks++;
    return 0;
}

void px_font_free(px_font* font) {
    if (!font || font->is_default) return;
    /* Free primary cache */
    for (int i = 0; i < PX_GLYPH_CACHE_SIZE; i++) {
        free(font->cache[i].bitmap);
    }
    /* Free fallback caches + faces */
    for (int f = 0; f < font->n_fallbacks; f++) {
        for (int i = 0; i < PX_GLYPH_CACHE_SIZE; i++) {
            free(font->fallback_caches[f][i].bitmap);
        }
        if (font->fallback_faces[f]) FT_Done_Face(font->fallback_faces[f]);
    }
    if (font->ft_face) FT_Done_Face(font->ft_face);
    if (font->ft_lib)  FT_Done_FreeType(font->ft_lib);
    free(font);
}

int px_font_line_height(px_font* font) { return font ? font->line_height : 0; }
int px_font_ascent(px_font* font)       { return font ? font->ascent      : 0; }
int px_font_descent(px_font* font)     { return font ? font->descent    : 0; }

/* ============================================================
 * Glyph rasterization (TTF mode)
 *
 * Looks up codepoint in cache; if miss, rasterize via FreeType
 * and store in cache. Returns NULL if rasterization fails.
 * ============================================================ */

/* Helper: rasterize a glyph from a specific face into a cache slot.
 * Returns the glyph, or NULL on failure.
 *
 * Stage 12: supports both grayscale (FT_PIXEL_MODE_GRAY) and
 * color (FT_PIXEL_MODE_BGRA) bitmaps. Color bitmaps are 4 bytes
 * per pixel (BGRA), grayscale are 1 byte per pixel (alpha only).
 *
 * For color bitmaps, we use FT_LOAD_COLOR flag to get BGRA data.
 */
static px_glyph* rasterize_into(FT_Face face, px_glyph* cache_slot, uint32_t codepoint) {
    /* Stage 12: Try color first, fall back to grayscale.
     * FT_LOAD_COLOR tells FreeType to render color bitmaps if available
     * (CBDT/COLR/sbix tables). If the font has no color table, FreeType
     * returns the same as FT_LOAD_RENDER (grayscale). */
    FT_Int32 load_flags = FT_LOAD_COLOR | FT_LOAD_RENDER;
    if (FT_Load_Char(face, codepoint, load_flags) != 0) {
        /* Fallback: try without color */
        if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
            return NULL;
        }
    }
    FT_GlyphSlot slot_data = face->glyph;
    FT_Bitmap* bmp = &slot_data->bitmap;

    /* Free old bitmap */
    free(cache_slot->bitmap);
    cache_slot->bitmap = NULL;

    int w = (int)bmp->width;
    int h = (int)bmp->rows;
    if (w <= 0 || h <= 0) {
        /* Empty glyph (e.g. space) — store advance only */
        cache_slot->codepoint = codepoint;
        cache_slot->width     = 0;
        cache_slot->height    = 0;
        cache_slot->bearing_x = 0;
        cache_slot->bearing_y = 0;
        cache_slot->advance_x = (int)(slot_data->advance.x >> 6);
        cache_slot->is_color  = false;
        cache_slot->bitmap    = NULL;
        return cache_slot;
    }

    /* Stage 12: detect color vs grayscale */
    bool is_color = (bmp->pixel_mode == FT_PIXEL_MODE_BGRA);
    size_t bytes_per_pixel = is_color ? 4 : 1;
    size_t buf_size = (size_t)w * h * bytes_per_pixel;

    uint8_t* bmp_copy = (uint8_t*)malloc(buf_size);
    if (!bmp_copy) return NULL;
    memcpy(bmp_copy, bmp->buffer, buf_size);

    cache_slot->codepoint = codepoint;
    cache_slot->width     = w;
    cache_slot->height    = h;
    cache_slot->bearing_x = slot_data->bitmap_left;
    cache_slot->bearing_y = slot_data->bitmap_top;
    cache_slot->advance_x = (int)(slot_data->advance.x >> 6);
    cache_slot->is_color  = is_color;
    cache_slot->bitmap    = bmp_copy;
    return cache_slot;
}

static px_glyph* get_or_rasterize(px_font* font, uint32_t codepoint) {
    if (!font || font->is_default) return NULL;

    /* Stage 11: check primary cache first, then fallback chain.
     * Each face has its own cache to avoid thrashing. */

    /* 1. Try primary cache / face */
    int slot = (int)(codepoint & 0xFF);
    px_glyph* g = &font->cache[slot];
    if (g->codepoint == codepoint) {
        return g;
    }
    /* Cache miss — rasterize from primary face */
    px_glyph* result = rasterize_into(font->ft_face, g, codepoint);
    if (result) {
        /* Check if primary has a real glyph for this codepoint
         * (FT_Load_Char returns .notdef as glyph index 0) */
        if (FT_Get_Char_Index(font->ft_face, codepoint) != 0) {
            return result;
        }
        /* Primary has no glyph (got .notdef) — try fallbacks */
        free(result->bitmap);
        result->bitmap = NULL;
        result->codepoint = 0;
    }

    /* 2. Try fallback chain */
    for (int f = 0; f < font->n_fallbacks; f++) {
        px_glyph* fb_cache = &font->fallback_caches[f][slot];
        if (fb_cache->codepoint == codepoint) {
            return fb_cache;
        }
        /* Check if this fallback has the glyph */
        if (FT_Get_Char_Index(font->fallback_faces[f], codepoint) == 0) {
            continue;  /* fallback doesn't have it either */
        }
        px_glyph* fb_result = rasterize_into(font->fallback_faces[f], fb_cache, codepoint);
        if (fb_result) {
            return fb_result;
        }
    }

    return NULL;  /* No font in the chain has this codepoint */
}

/* ============================================================
 * UTF-8 decoding
 *
 * Returns the next codepoint at *p, advances *p past it.
 * Returns 0xFFFFFFFF on invalid UTF-8.
 * ============================================================ */

static uint32_t utf8_next(const char** p) {
    const unsigned char* s = (const unsigned char*)*p;
    if (*s == 0) return 0xFFFFFFFF;
    if (s[0] < 0x80) {
        *p += 1;
        return s[0];
    }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        uint32_t c = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
        return c;
    }
    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        uint32_t c = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3;
        return c;
    }
    if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
        uint32_t c = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *p += 4;
        return c;
    }
    /* Invalid UTF-8 — skip one byte */
    *p += 1;
    return 0xFFFFFFFF;
}

/* ============================================================
 * Drawing
 *
 * For default font: uses px_font8x16_glyph (8x16 bitmap)
 * For TTF font: uses FreeType + cache
 * ============================================================ */

int px_fb_draw_glyph_utf8(px_fb* fb, int x, int y, uint32_t codepoint,
                            px_font* font, uint32_t fg_rgba) {
    if (!fb || !font) return 0;

    if (font->is_default) {
        /* Default font: ASCII only, render via 8x16 bitmap */
        char c = (codepoint < 128) ? (char)codepoint : '?';
        return px_fb_draw_char(fb, x, y, c, fg_rgba);
    }

    /* TTF font: get glyph, blit alpha-blended */
    px_glyph* g = get_or_rasterize(font, codepoint);
    if (!g) {
        /* Render '?' as fallback */
        return px_fb_draw_char(fb, x, y, '?', fg_rgba);
    }

    /* Extract RGB from fg_rgba */
    uint8_t fg_r = (fg_rgba >> 16) & 0xFF;
    uint8_t fg_g = (fg_rgba >> 8) & 0xFF;
    uint8_t fg_b =  fg_rgba        & 0xFF;

    /* Blit bitmap at (x + bearing_x, y + ascent - bearing_y) */
    int bx = x + g->bearing_x;
    int by = y + font->ascent - g->bearing_y;
    for (int row = 0; row < g->height; row++) {
        for (int col = 0; col < g->width; col++) {
            int px = bx + col;
            int py = by + row;
            if (px < 0 || px >= px_fb_width(fb)) continue;
            if (py < 0 || py >= px_fb_height(fb)) continue;

            if (g->is_color) {
                /* Stage 12: color bitmap (BGRA, 4 bytes/pixel) */
                size_t idx = (size_t)(row * g->width + col) * 4;
                uint8_t b = g->bitmap[idx + 0];
                uint8_t gc = g->bitmap[idx + 1];
                uint8_t r = g->bitmap[idx + 2];
                uint8_t a = g->bitmap[idx + 3];
                if (a == 0) continue;
                /* Alpha-blend BGRA onto RGBA framebuffer */
                uint32_t dst = px_fb_get_pixel(fb, px, py);
                uint8_t dr = (dst >> 16) & 0xFF;
                uint8_t dg = (dst >> 8) & 0xFF;
                uint8_t db =  dst        & 0xFF;
                uint8_t inv = 255 - a;
                uint8_t fr = (uint8_t)((r * a + dr * inv) / 255);
                uint8_t fg = (uint8_t)((gc * a + dg * inv) / 255);
                uint8_t fb_b = (uint8_t)((b * a + db * inv) / 255);
                px_fb_set_pixel(fb, px, py, PX_RGBA(fr, fg, fb_b, 255));
            } else {
                /* Grayscale: alpha-only, use fg_rgba color */
                uint8_t a = g->bitmap[row * g->width + col];
                if (a == 0) continue;
                uint32_t dst = px_fb_get_pixel(fb, px, py);
                uint8_t dr = (dst >> 16) & 0xFF;
                uint8_t dg = (dst >> 8) & 0xFF;
                uint8_t db =  dst        & 0xFF;
                uint8_t inv = 255 - a;
                uint8_t r = (uint8_t)((fg_r * a + dr * inv) / 255);
                uint8_t gc = (uint8_t)((fg_g * a + dg * inv) / 255);
                uint8_t bc = (uint8_t)((fg_b * a + db * inv) / 255);
                px_fb_set_pixel(fb, px, py, PX_RGBA(r, gc, bc, 255));
            }
        }
    }
    return g->advance_x;
}

int px_fb_draw_text_utf8(px_fb* fb, int x, int y, const char* utf8_str,
                          px_font* font, uint32_t fg_rgba) {
    if (!fb || !utf8_str || !font) return 0;

    int cx = x;
    const char* p = utf8_str;
    while (*p) {
        uint32_t cp = utf8_next(&p);
        if (cp == 0xFFFFFFFF) continue;
        cx += px_fb_draw_glyph_utf8(fb, cx, y, cp, font, fg_rgba);
    }
    return cx - x;
}

/* ============================================================
 * Stage 13: Fontconfig integration
 *
 * Resolve a font family name (e.g. "Noto Serif SC") to a file
 * path using fontconfig, then load via FreeType.
 *
 * If fontconfig is not compiled in, tries a hardcoded list of
 * common paths as a fallback.
 * ============================================================ */

#ifdef PLANEX_HAVE_FONTCONFIG

/* Use fontconfig to resolve family_name to a file path.
 * Returns a malloc'd string (caller must free) or NULL. */
static char* fontconfig_resolve(const char* family_name) {
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) return NULL;

    FcPattern* pattern = FcNameParse((const FcChar8*)family_name);
    if (!pattern) {
        FcConfigDestroy(config);
        return NULL;
    }

    FcConfigSubstitute(config, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern* match = FcFontMatch(config, pattern, &result);
    FcPatternDestroy(pattern);

    if (!match || result != FcResultMatch) {
        if (match) FcPatternDestroy(match);
        FcConfigDestroy(config);
        return NULL;
    }

    FcChar8* file_path = NULL;
    if (FcPatternGetString(match, FC_FILE, 0, &file_path) != FcResultMatch) {
        FcPatternDestroy(match);
        FcConfigDestroy(config);
        return NULL;
    }

    char* result_path = px_strdup((const char*)file_path);
    FcPatternDestroy(match);
    FcConfigDestroy(config);
    return result_path;
}

px_font* px_font_find(const char* family_name, int pixel_size) {
    if (!family_name || pixel_size <= 0) return NULL;

    char* path = fontconfig_resolve(family_name);
    if (!path) {
        fprintf(stderr, "Planex font: fontconfig could not find \"%s\"\n", family_name);
        return NULL;
    }

    px_font* font = px_font_load(path, pixel_size);
    if (font) {
        fprintf(stderr, "Planex font: found \"%s\" at %s\n", family_name, path);
    }
    free(path);
    return font;
}

int px_font_add_fallback_named(px_font* font, const char* family_name) {
    if (!font || !family_name) return -1;

    char* path = fontconfig_resolve(family_name);
    if (!path) {
        fprintf(stderr, "Planex font: fontconfig could not find fallback \"%s\"\n", family_name);
        return -1;
    }

    int rc = px_font_add_fallback(font, path);
    free(path);
    return rc;
}

#else /* !PLANEX_HAVE_FONTCONFIG */

/* Fallback: no fontconfig, try hardcoded paths.
 * This is fragile but better than failing completely. */
static const char* guess_font_path(const char* family_name) {
    /* Very simple name-based heuristics for common fonts */
    if (strstr(family_name, "Noto Serif SC") || strstr(family_name, "noto-serif-sc")) {
        return "/usr/share/fonts/truetype/noto-serif-sc/NotoSerifSC-Regular.ttf";
    }
    if (strstr(family_name, "Noto Color Emoji") || strstr(family_name, "noto-color-emoji")) {
        return "/usr/share/fonts/truetype/emoji/NotoColorEmoji.ttf";
    }
    if (strstr(family_name, "DejaVu Sans Mono") || strstr(family_name, "dejavu-sans-mono")) {
        return "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    }
    if (strstr(family_name, "Sarasa") || strstr(family_name, "sarasa")) {
        return "/usr/share/fonts/truetype/chinese/SarasaMonoSC-Regular.ttf";
    }
    if (strstr(family_name, "WenQuanYi") || strstr(family_name, "wqy")) {
        return "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc";
    }
    return NULL;
}

px_font* px_font_find(const char* family_name, int pixel_size) {
    if (!family_name || pixel_size <= 0) return NULL;
    const char* path = guess_font_path(family_name);
    if (!path) {
        fprintf(stderr, "Planex font: cannot find \"%s\" (no fontconfig, no hardcoded path)\n",
                family_name);
        return NULL;
    }
    return px_font_load(path, pixel_size);
}

int px_font_add_fallback_named(px_font* font, const char* family_name) {
    if (!font || !family_name) return -1;
    const char* path = guess_font_path(family_name);
    if (!path) return -1;
    return px_font_add_fallback(font, path);
}

#endif /* PLANEX_HAVE_FONTCONFIG */
