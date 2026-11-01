/*
 * if5c_hit.c — the hit test (ADR-0028 Decision 2c): the inverse
 * of the layout.
 *
 * ADR-0027 mapping (f) deferred geometric hit-testing to "the
 * client, forever, by S15's contract"; this module is the
 * discharge. It is only well-defined BECAUSE the layout is a
 * pure function — the inverse of a deterministic map is a map.
 */
#include "planex/if5c.h"

uint16_t if5c_hit(const if5c_layout* l, int x, int y)
{
    uint32_t i;
    if (!l || !l->cells) return 0;
    /* the tree order is the scan order; cells never overlap (the
     * grid construction), so the first containment is THE
     * containment */
    for (i = 0; i < l->n; i++) {
        const if5c_cell* c = &l->cells[i];
        if (x >= c->x && x < c->x + c->w &&
            y >= c->y && y < c->y + c->h)
            return c->rid;
    }
    return 0;
}
