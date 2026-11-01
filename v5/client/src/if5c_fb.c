/*
 * if5c_fb.c — the client's framebuffer (ADR-0028 Decision 2a):
 * an RGBA surface, deterministic primitives, the derived 5x7
 * font, a canonical 24-bit BMP writer (the golden format), and
 * the hash/bytes/dump primitives the determinism tests consume.
 *
 * The same idea as the shipping tree's src/fb.c, re-derived so
 * v5 links nothing from v4 (the derivation stays
 * zero-dependency). No floating point, no wall clock, no
 * allocation at draw time — the surface is malloc'd once at
 * creation, and every primitive is a pure function of its
 * arguments and the current pixels.
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "if5c_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX open/fdopen for the 0644 creation discipline (CodeQL
 * cpp/world-writable-file-creation; the shipping src/fb.c
 * precedent — the feature macro above makes them visible under
 * strict -std=c17) */
#ifdef _WIN32
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define IF5C_GLYPH_W  5
#define IF5C_GLYPH_H  7
#define IF5C_ADVANCE  6

if5c_fb* if5c_fb_new(int width, int height)
{
    if5c_fb* fb;
    if (width <= 0 || height <= 0) return NULL;
    fb = (if5c_fb*)calloc(1, sizeof *fb);
    if (!fb) return NULL;
    fb->px = (uint32_t*)calloc((size_t)width * (size_t)height,
                               sizeof(uint32_t));
    if (!fb->px) { free(fb); return NULL; }
    fb->width = width;
    fb->height = height;
    return fb;
}

void if5c_fb_free(if5c_fb* fb)
{
    if (!fb) return;
    free(fb->px);
    free(fb);
}

int if5c_fb_width(const if5c_fb* fb)  { return fb ? fb->width : 0; }
int if5c_fb_height(const if5c_fb* fb) { return fb ? fb->height : 0; }

void if5c_fb_clear(if5c_fb* fb, uint32_t rgb)
{
    int i, n;
    if (!fb) return;
    n = fb->width * fb->height;
    for (i = 0; i < n; i++) fb->px[i] = rgb;
    fb->bg = rgb;
}

void if5c_fb_set(if5c_fb* fb, int x, int y, uint32_t rgb)
{
    if (!fb) return;
    if (x < 0 || y < 0 || x >= fb->width || y >= fb->height) return;
    fb->px[(size_t)y * (size_t)fb->width + (size_t)x] = rgb;
}

uint32_t if5c_fb_get(const if5c_fb* fb, int x, int y)
{
    if (!fb) return 0;
    if (x < 0 || y < 0 || x >= fb->width || y >= fb->height)
        return fb->bg;
    return fb->px[(size_t)y * (size_t)fb->width + (size_t)x];
}

void if5c_fb_fill_rect(if5c_fb* fb, int x, int y, int w, int h,
                       uint32_t rgb)
{
    int yy;
    if (!fb || w <= 0 || h <= 0) return;
    for (yy = y; yy < y + h; yy++) {
        int xx, x0 = x < 0 ? 0 : x, x1 = x + w;
        if (yy < 0 || yy >= fb->height) continue;
        if (x1 > fb->width) x1 = fb->width;
        for (xx = x0; xx < x1; xx++)
            fb->px[(size_t)yy * (size_t)fb->width + (size_t)xx] = rgb;
    }
}

void if5c_fb_frame_rect(if5c_fb* fb, int x, int y, int w, int h,
                        uint32_t rgb)
{
    if (!fb || w <= 0 || h <= 0) return;
    if5c_fb_fill_rect(fb, x, y, w, 1, rgb);
    if5c_fb_fill_rect(fb, x, y + h - 1, w, 1, rgb);
    if5c_fb_fill_rect(fb, x, y, 1, h, rgb);
    if5c_fb_fill_rect(fb, x + w - 1, y, 1, h, rgb);
}

int if5c_fb_draw_text(if5c_fb* fb, int x, int y, const char* s,
                      uint32_t fg, uint32_t bg)
{
    int cx = x;
    const char* p;
    if (!fb || !s) return 0;
    for (p = s; *p; p++) {
        const uint8_t* g = if5c__glyph(*p);
        int col, row;
        if (*p == ' ') {
            if5c_fb_fill_rect(fb, cx, y, IF5C_ADVANCE, IF5C_GLYPH_H, bg);
        } else {
            for (col = 0; col < IF5C_GLYPH_W; col++) {
                uint8_t bits = g[col];
                for (row = 0; row < IF5C_GLYPH_H; row++) {
                    uint32_t c = (bits >> row) & 1u ? fg : bg;
                    if5c_fb_set(fb, cx + col, y + row, c);
                }
            }
            /* the advance gutter */
            if5c_fb_fill_rect(fb, cx + IF5C_GLYPH_W, y, 1, IF5C_GLYPH_H,
                              bg);
        }
        cx += IF5C_ADVANCE;
    }
    return cx - x;
}

uint32_t if5c_fb_hash(const if5c_fb* fb)
{
    /* FNV-1a over the pixel words — same pixels, same hash, in
     * every process, on every host. */
    uint32_t h = 2166136261u;
    int i, n;
    if (!fb) return 0;
    n = fb->width * fb->height;
    for (i = 0; i < n; i++) {
        h ^= fb->px[i];
        h *= 16777619u;
    }
    return h;
}

int if5c_fb_bytes(const if5c_fb* fb, uint8_t* out, int cap)
{
    int i, n;
    if (!fb || !out) return -1;
    n = fb->width * fb->height;
    if (cap < n * 4) return n * 4;        /* the needed size */
    for (i = 0; i < n; i++) {
        uint32_t v = fb->px[i];
        out[i * 4 + 0] = (uint8_t)(v >> 16);       /* R */
        out[i * 4 + 1] = (uint8_t)(v >> 8);        /* G */
        out[i * 4 + 2] = (uint8_t)v;               /* B */
        out[i * 4 + 3] = 0;                        /* reserved */
    }
    return n * 4;
}

int if5c_fb_dump_ascii(const if5c_fb* fb, char* out, int cap)
{
    int x, y, pos = 0;
    uint32_t bg;
    if (!fb || !out || cap <= 0) return 0;
    bg = fb->bg;
    for (y = 0; y < fb->height; y++) {
        for (x = 0; x < fb->width; x++) {
            uint32_t v = fb->px[(size_t)y * (size_t)fb->width + (size_t)x];
            char ch;
            if (v == bg) ch = '.';
            else {
                uint32_t lum = ((v >> 16) & 0xFFu) * 30u
                             + ((v >> 8) & 0xFFu) * 59u
                             + (v & 0xFFu) * 11u;
                ch = lum > 230 * 100u ? '#' : lum > 120 * 100u ? '+' : '*';
            }
            if (pos + 2 > cap) { out[pos] = 0; return pos; }
            out[pos++] = ch;
        }
        if (pos + 2 > cap) { out[pos] = 0; return pos; }
        out[pos++] = '\n';
    }
    out[pos] = 0;
    return pos;
}

int if5c_fb_save_bmp(const if5c_fb* fb, const char* path)
{
    /*
     * The canonical golden format: 24-bit BI_RGB BMP. The header
     * is fully derived from the pixel dimensions — no timestamps,
     * no color tables, no padding surprises beyond the 4-byte row
     * alignment the format itself mandates (rows are padded to a
     * multiple of 4 bytes with zeros, a pure function of width).
     */
    FILE* f;
    uint8_t hdr[54];
    uint32_t row, rowsz, filesz;
    uint8_t* rowbuf = NULL;

    if (!fb || !path) return -1;
    rowsz = (uint32_t)(((size_t)fb->width * 3 + 3) & ~(size_t)3);
    filesz = 54 + rowsz * (uint32_t)fb->height;

    memset(hdr, 0, sizeof hdr);
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = (uint8_t)(filesz);        hdr[3] = (uint8_t)(filesz >> 8);
    hdr[4] = (uint8_t)(filesz >> 16);  hdr[5] = (uint8_t)(filesz >> 24);
    hdr[10] = 54;                      /* pixel array offset */
    hdr[14] = 40;                      /* info header size */
    hdr[18] = (uint8_t)fb->width;      hdr[19] = (uint8_t)(fb->width >> 8);
    hdr[20] = (uint8_t)(fb->width >> 16); hdr[21] = (uint8_t)(fb->width >> 24);
    {   int32_t hneg = -(int32_t)fb->height; /* top-down rows */
        hdr[22] = (uint8_t)hneg;       hdr[23] = (uint8_t)(hneg >> 8);
        hdr[24] = (uint8_t)(hneg >> 16); hdr[25] = (uint8_t)(hneg >> 24); }
    hdr[26] = 1;                       /* planes */
    hdr[28] = 24;                      /* bits per pixel */
    hdr[34] = (uint8_t)(rowsz * (uint32_t)fb->height);
    hdr[35] = (uint8_t)((rowsz * (uint32_t)fb->height) >> 8);
    hdr[36] = (uint8_t)((rowsz * (uint32_t)fb->height) >> 16);
    hdr[37] = (uint8_t)((rowsz * (uint32_t)fb->height) >> 24);
    hdr[46] = 255; hdr[47] = 255; hdr[48] = 255; hdr[49] = 255; hdr[50] = 255;
    hdr[51] = 255; hdr[52] = 255; hdr[53] = 255;

    rowbuf = (uint8_t*)calloc(rowsz, 1);
    if (!rowbuf) return -1;

#ifdef _WIN32
    /* Windows CRT: ACL-governed access; no umask dependency worth
     * modeling (the shipping fb.c precedent, verbatim in spirit) */
    f = fopen(path, "wb");
    if (!f) { free(rowbuf); return -1; }
#else
    /* Create with an explicit 0644, independent of the process
     * umask — fopen("wb") creates 0666 & ~umask, and a permissive
     * umask makes the golden world-writable (CodeQL
     * cpp/world-writable-file-creation; the shipping tree's
     * src/fb.c carries the same fix and the same comment) */
    {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { free(rowbuf); return -1; }
        f = fdopen(fd, "wb");
        if (!f) { close(fd); free(rowbuf); return -1; }
    }
#endif
    if (fwrite(hdr, 1, sizeof hdr, f) != sizeof hdr) goto fail;

    for (row = 0; row < (uint32_t)fb->height; row++) {
        int x;
        memset(rowbuf, 0, rowsz);
        for (x = 0; x < fb->width; x++) {
            uint32_t v = fb->px[(size_t)row * (size_t)fb->width
                                + (size_t)x];
            rowbuf[x * 3 + 0] = (uint8_t)(v & 0xFFu);        /* B */
            rowbuf[x * 3 + 1] = (uint8_t)((v >> 8) & 0xFFu); /* G */
            rowbuf[x * 3 + 2] = (uint8_t)((v >> 16) & 0xFFu);/* R */
        }
        if (fwrite(rowbuf, 1, rowsz, f) != rowsz) goto fail;
    }
    free(rowbuf);
    if (fclose(f) != 0) return -1;
    return 0;

fail:
    free(rowbuf);
    fclose(f);
    return -1;
}
