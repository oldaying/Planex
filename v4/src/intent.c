/* v4/src/intent.c — open symbol system for intent kinds
 *
 * v4 BREAK: px_intent_kind was an enum in v0.4. In v4 it is
 * const char*. Domains needing custom illocutionary forces
 * (legal UI: AUTHORIZE/WITNESS; medical UI: PRESCRIBE/DIAGNOSE;
 * deliberative UI: PROPOSE/OBJECT) pass any string.
 *
 * Built-ins provided as extern const. Comparison via strcmp
 * (px_intent_kind_eq), not ==.
 */

#include "planex/planex.h"
#include <string.h>

/* Definition of the extern declarations from planex.h. */
const px_intent_kind PX_INTENT_ASSERT   = "ASSERT";
const px_intent_kind PX_INTENT_REQUEST  = "REQUEST";
const px_intent_kind PX_INTENT_PROMISE   = "PROMISE";
const px_intent_kind PX_INTENT_DECLARE  = "DECLARE";
const px_intent_kind PX_INTENT_EXPRESS  = "EXPRESS";

bool px_intent_kind_eq(px_intent_kind a, px_intent_kind b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

const char* px_intent_kind_str(px_intent_kind k) {
    return k ? k : "(null)";
}
