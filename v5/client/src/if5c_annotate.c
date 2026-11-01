/*
 * if5c_annotate.c — the annotation table (ADR-0028 Decision 2e):
 * cancel reasons, keyed by the cut registry's ids.
 *
 * ADR-0027 mapping (g) registered the cancel-reason string as
 * "a successor-ADR question" — attaching a reason word would be
 * a new record field, and the 48-byte density is registered.
 * This module is the discharge that changes no record: the CUT
 * is the semantic content (the history change); the REASON is
 * presentation (why a human asked for it), and it lives here, in
 * client session state, keyed by the address it explains.
 *
 * NOT CARRIED: durability. Annotations are session state — a
 * reloaded world has its cuts and their NAMES (world-side), and
 * fresh annotations (ADR-0028 CAVEATS; a measured need for
 * durable annotations is a successor-ADR question).
 */
#include "planex/if5c.h"

#include <stdlib.h>
#include <string.h>

#define IF5C_ANNOT_MAX 128

struct if5c_annot {
    struct {
        uint8_t  cut_id;
        char     reason[IF5C_ANNOT_MAX];
    } rows[IF5C_MAX_ANNOTATIONS];
    uint32_t n;
};

if5c_annot* if5c_annot_new(void)
{
    return (if5c_annot*)calloc(1, sizeof(if5c_annot));
}

void if5c_annot_free(if5c_annot* an)
{
    free(an);
}

bool if5c_annotate(if5c_annot* an, uint8_t cut_id, const char* reason)
{
    uint32_t i;
    size_t nl;
    if (!an || !reason || cut_id == 0) return false;
    nl = strlen(reason);
    if (nl >= IF5C_ANNOT_MAX) nl = IF5C_ANNOT_MAX - 1;

    for (i = 0; i < an->n; i++) {
        if (an->rows[i].cut_id == cut_id) {
            memset(an->rows[i].reason, 0, IF5C_ANNOT_MAX);
            memcpy(an->rows[i].reason, reason, nl);
            return true;
        }
    }
    if (an->n >= IF5C_MAX_ANNOTATIONS) return false;
    an->rows[an->n].cut_id = cut_id;
    memcpy(an->rows[an->n].reason, reason, nl);
    an->n++;
    return true;
}

const char* if5c_annotation_of(const if5c_annot* an, uint8_t cut_id)
{
    uint32_t i;
    if (!an || cut_id == 0) return NULL;
    for (i = 0; i < an->n; i++)
        if (an->rows[i].cut_id == cut_id) return an->rows[i].reason;
    return NULL;
}

uint32_t if5c_annotation_count(const if5c_annot* an)
{
    return an ? an->n : 0;
}
