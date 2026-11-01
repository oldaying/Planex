/*
 * if5c_internal.h — shared internals of the v5 presentation
 * client (ADR-0028). Not installed, not part of the public
 * contract; the modules in v5/client/src include it, nothing
 * else does (the core CERTAINLY does not — the
 * dependency-direction ban reads this file's absence from
 * v5/src and v5/include, and scripts/check_slice_rebound.sh
 * enforces it).
 */
#ifndef IF5C_INTERNAL_H
#define IF5C_INTERNAL_H

#include "planex/if5c.h"

#include <stdint.h>

struct if5c_fb {
    int       width, height;
    uint32_t  bg;              /* out-of-bounds reads answer this */
    uint32_t* px;              /* 0x00RRGGBB, row-major */
};

/* if5c_font.c */
const uint8_t* if5c__font5x7(uint32_t* count);
const uint8_t* if5c__glyph(char ch);

#endif /* IF5C_INTERNAL_H */
