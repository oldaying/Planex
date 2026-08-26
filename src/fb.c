/*
 * fb.c — Framebuffer + BMP output for Planex Stage 1
 *
 * Software rasterizer: 32bpp RGBA in memory, output as BMP.
 * No platform dependency, no window system.
 *
 * Render is *not* a new abstraction — it's a Closure that consumes
 * Relation graph + Estimates and produces pixels. This keeps Stage
 * 0.5's 3-abstraction completeness intact.
 */
#define _POSIX_C_SOURCE 200809L
#include "planex/fb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Font (defined in font.c, but we need to declare it here)
 * ============================================================ */

extern const unsigned char* px_font8x16_glyph(char c);

/* ============================================================
 * Framebuffer lifecycle
 * ============================================================ */

struct px_fb {
    int      width;
    int      height;
    uint32_t* pixels;   /* width * height, RGBA8888 */
};

px_fb* px_fb_new(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;
    px_fb* fb = (px_fb*)calloc(1, sizeof(px_fb));
    if (!fb) return NULL;
    fb->width  = width;
    fb->height = height;
    fb->pixels = (uint32_t*)calloc((size_t)width * height, sizeof(uint32_t));
    if (!fb->pixels) {
        free(fb);
        return NULL;
    }
    return fb;
}

void px_fb_free(px_fb* fb) {
    if (!fb) return;
    free(fb->pixels);
    free(fb);
}

int px_fb_width(const px_fb* fb)  { return fb ? fb->width  : 0; }
int px_fb_height(const px_fb* fb) { return fb ? fb->height : 0; }

int px_fb_resize(px_fb* fb, int width, int height) {
    if (!fb || width <= 0 || height <= 0) return -1;
    /* Skip if no change */
    if (width == fb->width && height == fb->height) return 0;
    /* Realloc pixel buffer */
    uint32_t* new_pixels = (uint32_t*)calloc((size_t)width * height, sizeof(uint32_t));
    if (!new_pixels) return -1;  /* fb stays at old size */
    free(fb->pixels);
    fb->pixels = new_pixels;
    fb->width  = width;
    fb->height = height;
    return 0;
}

/* ============================================================
 * Drawing primitives
 * ============================================================ */

void px_fb_clear(px_fb* fb, uint32_t rgba) {
    if (!fb) return;
    size_t n = (size_t)fb->width * fb->height;
    for (size_t i = 0; i < n; i++) fb->pixels[i] = rgba;
}

void px_fb_set_pixel(px_fb* fb, int x, int y, uint32_t rgba) {
    if (!fb) return;
    if (x < 0 || x >= fb->width)  return;
    if (y < 0 || y >= fb->height) return;
    fb->pixels[y * fb->width + x] = rgba;
}

uint32_t px_fb_get_pixel(const px_fb* fb, int x, int y) {
    if (!fb) return 0;
    if (x < 0 || x >= fb->width)  return 0;
    if (y < 0 || y >= fb->height) return 0;
    return fb->pixels[y * fb->width + x];
}

const uint32_t* px_fb_pixels(const px_fb* fb) {
    return fb ? fb->pixels : NULL;
}

void px_fb_fill_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba) {
    if (!fb || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w; if (x1 > fb->width)  x1 = fb->width;
    int y1 = y + h; if (y1 > fb->height) y1 = fb->height;

    for (int py = y0; py < y1; py++) {
        uint32_t* row = fb->pixels + py * fb->width;
        for (int px = x0; px < x1; px++) {
            row[px] = rgba;
        }
    }
}

void px_fb_draw_rect(px_fb* fb, int x, int y, int w, int h, uint32_t rgba) {
    if (!fb || w <= 0 || h <= 0) return;
    /* top + bottom */
    px_fb_fill_rect(fb, x, y, w, 1, rgba);
    px_fb_fill_rect(fb, x, y + h - 1, w, 1, rgba);
    /* left + right */
    px_fb_fill_rect(fb, x,             y, 1, h, rgba);
    px_fb_fill_rect(fb, x + w - 1,     y, 1, h, rgba);
}

void px_fb_draw_hline(px_fb* fb, int x, int y, int len, uint32_t rgba) {
    if (!fb || len <= 0) return;
    px_fb_fill_rect(fb, x, y, len, 1, rgba);
}

void px_fb_draw_vline(px_fb* fb, int x, int y, int len, uint32_t rgba) {
    if (!fb || len <= 0) return;
    px_fb_fill_rect(fb, x, y, 1, len, rgba);
}

/* ============================================================
 * Text rendering
 * ============================================================ */

int px_fb_draw_char(px_fb* fb, int x, int y, char c, uint32_t fg_rgba) {
    if (!fb) return 8;
    const unsigned char* glyph = px_font8x16_glyph(c);
    for (int row = 0; row < 16; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                px_fb_set_pixel(fb, x + col, y + row, fg_rgba);
            }
        }
    }
    return 8;
}

int px_fb_draw_text(px_fb* fb, int x, int y, const char* s, uint32_t fg_rgba) {
    if (!fb || !s) return 0;
    int cx = x;
    for (const char* p = s; *p; p++) {
        /* Translate newline to next line (16 px down) */
        if (*p == '\n') {
            cx = x;
            y += 16;
            continue;
        }
        /* Translate tab to 4 spaces (32 px) */
        if (*p == '\t') {
            cx += 32;
            continue;
        }
        px_fb_draw_char(fb, cx, y, *p, fg_rgba);
        cx += 8;
    }
    return cx - x;
}

int px_fb_draw_text_bg(px_fb* fb, int x, int y, const char* s,
                        uint32_t fg_rgba, uint32_t bg_rgba) {
    if (!fb || !s) return 0;
    /* Fill background for the string bounds */
    size_t len = strlen(s);
    int w = (int)len * 8;
    px_fb_fill_rect(fb, x, y, w, 16, bg_rgba);
    return px_fb_draw_text(fb, x, y, s, fg_rgba);
}

/* ============================================================
 * ASCII art dump (for debugging)
 * ============================================================ */

void px_fb_dump_ascii(px_fb* fb) {
    if (!fb) return;
    /* Sample every 4x8 pixels for terminal aspect ratio */
    int sx = 4, sy = 8;
    const char* ramp = " .:-=+*#%@";
    int n_levels = 10;
    for (int y = 0; y < fb->height; y += sy) {
        for (int x = 0; x < fb->width; x += sx) {
            int sum = 0, count = 0;
            for (int dy = 0; dy < sy && y + dy < fb->height; dy++) {
                for (int dx = 0; dx < sx && x + dx < fb->width; dx++) {
                    uint32_t c = fb->pixels[(y+dy)*fb->width + (x+dx)];
                    int r = (c >> 16) & 0xFF;
                    int g = (c >> 8)  & 0xFF;
                    int b =  c        & 0xFF;
                    sum += (r + g + b) / 3;
                    count++;
                }
            }
            int avg = sum / count;
            int level = avg * n_levels / 256;
            if (level >= n_levels) level = n_levels - 1;
            putchar(ramp[level]);
        }
        putchar('\n');
    }
}

/* ============================================================
 * BMP output (32bpp BI_BITFIELDS, no compression)
 * ============================================================ */

static void put_u16(FILE* f, uint16_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8) & 0xFF, f);
}
static void put_u32(FILE* f, uint32_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8)  & 0xFF, f);
    fputc((v >> 16) & 0xFF, f);
    fputc((v >> 24) & 0xFF, f);
}

int px_fb_save_bmp(px_fb* fb, const char* path) {
    if (!fb || !path) return -1;

    FILE* f = fopen(path, "wb");
    if (!f) return -1;

    /* BMP file header (14 bytes) */
    fwrite("BM", 1, 2, f);
    /* file size = 14 + 40 + 12 + width * height * 4 (BGRA) */
    uint32_t pixel_data_size = (uint32_t)fb->width * fb->height * 4;
    uint32_t file_size = 14 + 40 + 12 + pixel_data_size;
    put_u32(f, file_size);
    put_u16(f, 0); /* reserved */
    put_u16(f, 0); /* reserved */
    put_u32(f, 14 + 40 + 12);  /* offset to pixel data */

    /* DIB header BITMAPV3INFOHEADER (52 bytes total: 40 + 12) */
    put_u32(f, 40 + 12);                /* header size */
    put_u32(f, (uint32_t)fb->width);    /* width */
    put_u32(f, (uint32_t)fb->height);   /* height (positive = bottom-up) */
    put_u16(f, 1);                       /* planes */
    put_u16(f, 32);                      /* bpp */
    put_u32(f, 3);   /* BI_BITFIELDS */
    put_u32(f, pixel_data_size);         /* image size */
    put_u32(f, 2835);                    /* x ppm (72 dpi) */
    put_u32(f, 2835);                    /* y ppm */
    put_u32(f, 0);                        /* colors used */
    put_u32(f, 0);                        /* important colors */

    /* Bit masks (RGB, 8 bits each, alpha ignored by BMP) */
    put_u32(f, 0x00FF0000);   /* R */
    put_u32(f, 0x0000FF00);   /* G */
    put_u32(f, 0x000000FF);   /* B */
    put_u32(f, 0xFF000000);   /* A */

    /* Pixel data (bottom-up) */
    for (int y = fb->height - 1; y >= 0; y--) {
        for (int x = 0; x < fb->width; x++) {
            uint32_t c = fb->pixels[y * fb->width + x];
            /* BMP stores BGRA */
            fputc((c >> 0)  & 0xFF, f);  /* B */
            fputc((c >> 8)  & 0xFF, f);  /* G */
            fputc((c >> 16) & 0xFF, f);  /* R */
            fputc((c >> 24) & 0xFF, f);  /* A */
        }
    }

    fclose(f);
    return 0;
}
