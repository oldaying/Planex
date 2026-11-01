/*
 * if5c_ease.c — the easing (ADR-0028 Decision 2d): the
 * estimate-trajectory mapping's two-sample special case, executed
 * client-side as a PURE t-mapping.
 *
 * The registered boundary, restated from ADR-0027 mapping (e):
 * this is a mapping from a RATIONAL t to a frame, never a
 * wall-clock to a frame. "60 fps" is a deployment calibration
 * claim this module cannot make and does not carry. All
 * arithmetic is exact integer math on 64-bit intermediates — no
 * floating point anywhere in the client, which is what makes the
 * frame sequence a function (and therefore golden-testable, F5).
 */
#include "if5c_internal.h"

#include <stdlib.h>

void if5c_ease_t(uint32_t num, uint32_t den, if5c_ease_family f,
                 uint64_t* out_num, uint64_t* out_den)
{
    uint64_t n = num, d = den;
    if (!out_num || !out_den) return;
    if (den == 0) { *out_num = 0; *out_den = 1; return; }
    if (f == IF5C_EASE_LINEAR) {
        /* normalized over den so both families share a scale */
        *out_num = n * d * d;          /* num/den = num*den^2/den^3 */
        *out_den = d * d * d;
        return;
    }
    /* smoothstep: 3t^2 - 2t^3 over den^3 */
    {
        uint64_t n2 = n * n;
        uint64_t n3 = n2 * n;
        uint64_t three_t2 = 3ull * n2 * d;
        uint64_t two_t3 = 2ull * n3;
        *out_num = three_t2 >= two_t3 ? three_t2 - two_t3 : 0;
        *out_den = d * d * d;
    }
}

int if5c_ease_frame(if5c_fb* out, const if5c_fb* a, const if5c_fb* b,
                    uint32_t num, uint32_t den, if5c_ease_family f)
{
    uint64_t tn, td;
    int i, n;
    const uint32_t *pa, *pb;
    uint32_t *po;

    if (!out || !a || !b) return -1;
    if (if5c_fb_width(out)  != if5c_fb_width(a)  ||
        if5c_fb_height(out) != if5c_fb_height(a) ||
        if5c_fb_width(a)    != if5c_fb_width(b)  ||
        if5c_fb_height(a)   != if5c_fb_height(b))
        return -1;
    if (den == 0 || num > den) return -1;

    if5c_ease_t(num, den, f, &tn, &td);

    n = if5c_fb_width(a) * if5c_fb_height(a);
    pa = a->px; pb = b->px; po = out->px;
    for (i = 0; i < n; i++) {
        uint32_t va = pa[i], vb = pb[i];
        uint32_t ch;
        uint32_t mixed = 0;
        /* per channel: a + (b - a) * (tn/td), exact */
        for (ch = 0; ch < 3; ch++) {
            int shift = (int)(16 - ch * 8);
            int32_t ca = (int32_t)((va >> shift) & 0xFFu);
            int32_t cb = (int32_t)((vb >> shift) & 0xFFu);
            int64_t delta = (int64_t)cb - (int64_t)ca;
            int64_t v = (int64_t)ca + (delta * (int64_t)tn) / (int64_t)td;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            mixed |= (uint32_t)v << shift;
        }
        po[i] = mixed;
    }
    return 0;
}
