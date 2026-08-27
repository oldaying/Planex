/* v4/src/perlocution.c — essence #5: Searle perlocutionary effect (NEW)
 *
 * Perlocution is the *effect* of the system's utterance on the
 * actor's mental state — distinct from illocution (Closure, what
 * the system is doing) and from operational status (which is
 * DERIVED from perlocution, not stored separately).
 *
 *   "Saved successfully"              -> PX_PERLOC_INFORM
 *   "Saved. 3 fields were auto-corrected." -> PX_PERLOC_INFORM (+ surprise)
 *   "Validation failed: email required"   -> PX_PERLOC_ALERT
 *   "Working on it..."                -> PX_PERLOC_REASSURE (status=RUNNING)
 *   "Connection lost. Try again?"     -> PX_PERLOC_FRUSTRATE
 *
 * v4: NEW abstraction. v3 Path B bolted it onto Closure as a sub-API.
 * v4 makes it first-class.
 *
 * v4 BREAK: Closure's px_closure_status / px_closure_promise /
 * px_closure_declare / px_closure_fail are GONE. Operational status
 * is now derived from the perlocution.
 */

#include "planex/planex.h"
#include <stdlib.h>
#include <string.h>

struct px_perlocution {
    px_closure*         closure;   /* weak ref */
    px_actor*           actor;     /* weak ref */
    px_perlocution_kind kind;
    char*               text;
};

px_perlocution* px_perlocution_new(px_closure* c, px_actor* actor) {
    px_perlocution* p = (px_perlocution*)calloc(1, sizeof(px_perlocution));
    if (!p) return NULL;
    p->closure = c;
    p->actor = actor;
    p->kind = PX_PERLOC_UNSPECIFIED;
    p->text = NULL;
    return p;
}

void px_perlocution_free(px_perlocution* p) {
    if (!p) return;
    free(p->text);
    free(p);
}

void px_perlocution_set(px_perlocution* p,
                         px_perlocution_kind kind,
                         const char* outcome_text) {
    if (!p) return;
    p->kind = kind;
    free(p->text);
    if (outcome_text) {
        p->text = (char*)malloc(strlen(outcome_text) + 1);
        if (p->text) strcpy(p->text, outcome_text);
    } else {
        p->text = NULL;
    }
}

px_perlocution_kind px_perlocution_kind_get(const px_perlocution* p) {
    return p ? p->kind : PX_PERLOC_UNSPECIFIED;
}

const char* px_perlocution_text(const px_perlocution* p) {
    return p ? p->text : NULL;
}

const char* px_perlocution_kind_str(px_perlocution_kind k) {
    switch (k) {
        case PX_PERLOC_UNSPECIFIED: return "UNSPECIFIED";
        case PX_PERLOC_INFORM:      return "INFORM";
        case PX_PERLOC_PERSUADE:    return "PERSUADE";
        case PX_PERLOC_REASSURE:    return "REASSURE";
        case PX_PERLOC_ALERT:       return "ALERT";
        case PX_PERLOC_FRUSTRATE:   return "FRUSTRATE";
        case PX_PERLOC_SURPRISE:    return "SURPRISE";
        default:                    return "(unknown)";
    }
}

px_operational_status px_perlocution_status(const px_perlocution* p) {
    if (!p || p->kind == PX_PERLOC_UNSPECIFIED) return PX_STATUS_IDLE;
    switch (p->kind) {
        case PX_PERLOC_REASSURE:
            return PX_STATUS_RUNNING;   /* "working on it" -> not terminal */
        case PX_PERLOC_INFORM:
        case PX_PERLOC_PERSUADE:
        case PX_PERLOC_SURPRISE:
            return PX_STATUS_DONE;      /* terminal, positive */
        case PX_PERLOC_ALERT:
        case PX_PERLOC_FRUSTRATE:
            return PX_STATUS_FAILED;    /* terminal, negative */
        default:
            return PX_STATUS_IDLE;
    }
}

const char* px_status_str(px_operational_status s) {
    switch (s) {
        case PX_STATUS_IDLE:    return "IDLE";
        case PX_STATUS_RUNNING: return "RUNNING";
        case PX_STATUS_DONE:    return "DONE";
        case PX_STATUS_FAILED:  return "FAILED";
        default:                return "(unknown)";
    }
}
