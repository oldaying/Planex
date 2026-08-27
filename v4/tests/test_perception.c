/* v4/tests/test_perception.c — essence #2: Representamen (sign vehicle)
 *
 * Verifies: perception is a pure function, multi-channel capable,
 * returns a representamen. Multiple perceptions can coexist.
 *
 * v4 BREAK verified: no set_intended_interpretant on Perception
 * (that's on Interpretant now — see test_interpretant.c).
 */

#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
    else { printf("ok: %s\n", msg); } \
} while (0)

/* A perceive fn that returns a string representamen (static buffer).
 * Pure: same inputs -> same output. */
static void* make_text_representamen(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    static char buf[64];
    if (n <= 0) {
        snprintf(buf, sizeof(buf), "(no inputs)");
    } else {
        double v = px_estimate_now(inputs[0]);
        snprintf(buf, sizeof(buf), "count = %.0f", v);
    }
    return buf;
}

/* A second perceive fn that returns an a11y-style string. */
static void* make_a11y_representamen(px_estimate* const* inputs, int n, void* user) {
    (void)user;
    static char buf[64];
    if (n <= 0) {
        snprintf(buf, sizeof(buf), "status: empty");
    } else {
        double v = px_estimate_now(inputs[0]);
        snprintf(buf, sizeof(buf), "aria-live=polite: value is %.0f", v);
    }
    return buf;
}

int main(void) {
    px_estimate* counter = px_estimate_new(7.0, 1.0);
    px_estimate* srcs[] = { counter };

    int before = px_perception_count();

    px_perception* p_text = px_perception_new("text", make_text_representamen,
                                                srcs, 1, NULL);
    ASSERT(p_text != NULL, "perception_new text");
    ASSERT(px_perception_count() == before + 1, "registry count incremented");

    px_perception* p_a11y = px_perception_new("a11y", make_a11y_representamen,
                                                srcs, 1, NULL);
    ASSERT(p_a11y != NULL, "perception_new a11y");
    ASSERT(px_perception_count() == before + 2, "registry count incremented again");

    /* invoke both — they read same input but produce different representamens */
    char* r_text = (char*)px_perception_invoke(p_text);
    char* r_a11y = (char*)px_perception_invoke(p_a11y);
    ASSERT(r_text != NULL, "text representamen produced");
    ASSERT(r_a11y != NULL, "a11y representamen produced");
    ASSERT(strstr(r_text, "count = 7") != NULL, "text representamen content");
    ASSERT(strstr(r_a11y, "aria-live") != NULL, "a11y representamen content");

    /* change source -> same perception fn produces different output */
    px_estimate_set(counter, 42.0, 1.0);
    r_text = (char*)px_perception_invoke(p_text);
    ASSERT(strstr(r_text, "count = 42") != NULL, "representamen updates with input");

    /* inputs introspection */
    int n_in = 0;
    px_estimate** in = px_perception_inputs(p_text, &n_in);
    ASSERT(n_in == 1, "input count");
    ASSERT(in != NULL && in[0] == counter, "input[0] == counter");

    /* name */
    ASSERT(strcmp(px_perception_name(p_text), "text") == 0, "name");

    px_perception_free(p_text);
    px_perception_free(p_a11y);
    ASSERT(px_perception_count() == before, "registry count decremented after free");

    px_estimate_free(counter);
    printf("test_perception: ALL PASS\n");
    return 0;
}
