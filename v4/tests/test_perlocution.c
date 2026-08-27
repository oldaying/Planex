/* v4/tests/test_perlocution.c — essence #5: Searle perlocutionary effect (NEW)
 *
 * Verifies: 6 perlocutionary kinds, set/get, status derivation.
 *
 * Operational status is DERIVED from perlocution (not stored separately):
 *   IDLE    = no perlocution set
 *   RUNNING = REASSURE without terminal
 *   DONE    = INFORM / PERSUADE / SURPRISE
 *   FAILED  = ALERT / FRUSTRATE
 *
 * This abstraction is NEW in v4. In v3 it was a sub-API of Closure.
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

int main(void) {
    /* build a closure + actor to bind perlocution to */
    px_closure* c = px_closure_new("save document", PX_INTENT_REQUEST,
                                      NULL, NULL, NULL);
    px_actor* alice = px_actor_new("alice", NULL);

    px_perlocution* p = px_perlocution_new(c, alice);
    ASSERT(p != NULL, "perlocution_new");

    /* fresh perlocution: IDLE */
    ASSERT(px_perlocution_status(p) == PX_STATUS_IDLE, "fresh = IDLE");
    ASSERT(px_perlocution_kind_get(p) == PX_PERLOC_UNSPECIFIED, "kind UNSPECIFIED");

    /* "Working on it..." -> REASSURE -> RUNNING */
    px_perlocution_set(p, PX_PERLOC_REASSURE, "Working on it...");
    ASSERT(px_perlocution_kind_get(p) == PX_PERLOC_REASSURE, "kind REASSURE");
    ASSERT(strcmp(px_perlocution_text(p), "Working on it...") == 0,
           "outcome text retrievable");
    ASSERT(px_perlocution_status(p) == PX_STATUS_RUNNING, "REASSURE => RUNNING");

    /* "Saved successfully" -> INFORM -> DONE */
    px_perlocution_set(p, PX_PERLOC_INFORM, "Saved successfully");
    ASSERT(px_perlocution_status(p) == PX_STATUS_DONE, "INFORM => DONE");

    /* "Validation failed: email required" -> ALERT -> FAILED */
    px_perlocution_set(p, PX_PERLOC_ALERT, "Validation failed: email required");
    ASSERT(px_perlocution_status(p) == PX_STATUS_FAILED, "ALERT => FAILED");

    /* "Saved. 3 fields were auto-corrected." -> SURPRISE -> DONE */
    px_perlocution_set(p, PX_PERLOC_SURPRISE,
                        "Saved. 3 fields were auto-corrected.");
    ASSERT(px_perlocution_status(p) == PX_STATUS_DONE, "SURPRISE => DONE");

    /* "Connection lost. Try again?" -> FRUSTRATE -> FAILED */
    px_perlocution_set(p, PX_PERLOC_FRUSTRATE, "Connection lost. Try again?");
    ASSERT(px_perlocution_status(p) == PX_STATUS_FAILED, "FRUSTRATE => FAILED");

    /* "You should believe X" -> PERSUADE -> DONE */
    px_perlocution_set(p, PX_PERLOC_PERSUADE, "Trust me, this is safe.");
    ASSERT(px_perlocution_status(p) == PX_STATUS_DONE, "PERSUADE => DONE");

    /* kind_str human-readable */
    ASSERT(strcmp(px_perlocution_kind_str(PX_PERLOC_INFORM), "INFORM") == 0,
           "kind_str INFORM");
    ASSERT(strcmp(px_status_str(PX_STATUS_DONE), "DONE") == 0,
           "status_str DONE");

    px_perlocution_free(p);
    px_actor_free(alice);
    px_closure_free(c);
    printf("test_perlocution: ALL PASS\n");
    return 0;
}
