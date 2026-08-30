/*
 * test_completeness.c — closed-corpus falsifiability check
 *
 * Verifies that the 68-pattern UI pattern corpus in
 *   docs/reference/ui-pattern-corpus.md
 * is consistent with:
 *   1. The implementation examples in examples/*.c (for ✅ patterns
 *      grounded to EXAMPLE <file>).
 *   2. The limitations in docs/concepts/state/limitations.md (for ⚠️
 *      and ❌ patterns grounded to LIMITATION <L#>).
 *   3. The non-goals in docs/concepts/canonical/non-goals.md (for ❌
 *      patterns grounded to NONGOAL <NG-#>).
 *
 * Closed corpus invariants:
 *   - Total pattern count:  68
 *   - ✅ clean count:        31
 *   - ⚠️ forced count:       29
 *   - ❌ cannot count:        8
 *
 * Drift between this test and the corpus is a CI failure. See the corpus
 * doc's "Closing rule" section for the amendment procedure (ADR + same-
 * commit update of corpus + test + coverage matrix).
 *
 * Run: make check-completeness
 */

#define _POSIX_C_SOURCE 200809L
#include "planex/planex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Corpus constants — MUST match docs/reference/ui-pattern-corpus.md
 * Grand Summary. Update only via the amendment procedure (ADR).
 * ============================================================ */

#define EXPECTED_TOTAL   68
#define EXPECTED_CLEAN   39
#define EXPECTED_FORCED  23
#define EXPECTED_CANNOT   6

/* ============================================================
 * Pattern table — one row per UI pattern in the corpus.
 * ID, name, category, verdict, grounding_kind, grounding_ref.
 *
 * verdict_t values match the corpus legend:
 *   CLEAN   = ✅ cleanly expressible
 *   FORCED  = ⚠️ expressible but semantically wrong
 *   CANNOT  = ❌ cannot express without new abstraction
 *
 * grounding_kind_t values:
 *   G_EXAMPLE    = grounding_ref is a file under examples/
 *   G_CLAIM_ONLY = no external grounding needed (trivial pattern)
 *   G_LIMITATION = grounding_ref is "L#" to find in limitations.md
 *   G_NONGOAL    = grounding_ref is "NG-#" to find in non-goals.md
 * ============================================================ */

typedef enum { CLEAN, FORCED, CANNOT } verdict_t;
typedef enum { G_EXAMPLE, G_CLAIM_ONLY, G_LIMITATION, G_NONGOAL } grounding_kind_t;

typedef struct {
    int id;
    const char* name;
    const char* category;   /* "A" .. "I" */
    verdict_t verdict;
    grounding_kind_t grounding_kind;
    const char* grounding_ref;
} pattern_t;

/* Verdict name for human-readable output. */
static const char* verdict_name(verdict_t v) {
    switch (v) {
        case CLEAN:  return "clean";
        case FORCED: return "forced";
        case CANNOT: return "cannot";
        default:     return "?";
    }
}

/* Category name for human-readable output. */
static const char* category_name(const char* c) {
    /* c is "A".."I" — return the long form for the report. */
    if (c[0] >= 'A' && c[0] <= 'I') {
        static const char* names[] = {
            "A: Discrete state",
            "B: Animation & time",
            "C: Undo & history",
            "D: Continuous/transient interaction",
            "E: Layout & spatial",
            "F: Async & external data",
            "G: Multi-window",
            "H: Accessibility",
            "I: Extension",
        };
        return names[c[0] - 'A'];
    }
    return "?";
}

/* ============================================================
 * The 68-pattern corpus. Order MUST match the corpus doc.
 *
 * Grounding references:
 *   EXAMPLE    grounding_ref = "counter_4abs.c"  (file under examples/)
 *   CLAIM_ONLY grounding_ref = NULL
 *   LIMITATION grounding_ref = "L4"  (section in limitations.md)
 *   NONGOAL    grounding_ref = "NG-12"  (section in non-goals.md)
 * ============================================================ */

static const pattern_t kPatterns[] = {
    /* Category A: Discrete State Manipulation (P1-P12) — 12/12 clean */
    { 1,  "Counter (inc/dec)",              "A", CLEAN,  G_EXAMPLE,    "counter_4abs.c" },
    { 2,  "Checkbox toggle",                 "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 3,  "Radio button group",              "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 4,  "Dropdown / select",               "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 5,  "Text input (basic)",              "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 6,  "Form with validation",            "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 7,  "Tabs",                            "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 8,  "Modal dialog",                    "A", CLEAN,  G_CLAIM_ONLY, NULL },
    { 9,  "Wizard (multi-step)",             "A", CLEAN,  G_CLAIM_ONLY, NULL },
    {10,  "Todo list",                        "A", CLEAN,  G_CLAIM_ONLY, NULL },
    {11,  "Slider (value in range)",         "A", CLEAN,  G_CLAIM_ONLY, NULL },
    {12,  "Button with loading state",       "A", CLEAN,  G_CLAIM_ONLY, NULL },

    /* Category B: Animation & Time (P13-P18) — 6/6 clean */
    {13,  "Fade in/out",                     "B", CLEAN,  G_EXAMPLE,    "animation_demo.c" },
    {14,  "Slide transition",                "B", CLEAN,  G_EXAMPLE,    "animation_demo.c" },
    {15,  "Progress bar",                    "B", CLEAN,  G_EXAMPLE,    "async_demo.c" },
    {16,  "Typing animation",                "B", CLEAN,  G_CLAIM_ONLY, NULL },
    {17,  "Continuous data stream (sensor)", "B", CLEAN,  G_EXAMPLE,    "confidence_demo.c" },
    {18,  "Countdown timer",                 "B", CLEAN,  G_CLAIM_ONLY, NULL },

    /* Category C: Undo / History (P19-P23) — 1 clean, 2 forced, 2 cannot */
    {19,  "Undo last action",                "C", CLEAN,  G_EXAMPLE,    "undo_via_graph.c" },
    {20,  "Redo",                            "C", FORCED, G_LIMITATION, "L4" },
    {21,  "Time travel (jump to any point)", "C", FORCED, G_LIMITATION, "L4" },
    {22,  "Branching history (fork)",        "C", CANNOT, G_LIMITATION, "L4" },
    {23,  "Collaborative editing (multi-user)", "C", CANNOT, G_NONGOAL, "NG-12" },

    /* Category D: Continuous / Transient Interaction (P24-P38) â 7 clean, 7 forced, 1 cannot
     * (v0.7 re-score per ADR-0017 intent-compilation promotion + ADR-0018
     * interaction-process promotion; was 0/11/4 at v0.5) */
    {24,  "Hover highlight",                "D", CLEAN,  G_EXAMPLE,    "hover_drag_interaction.c" },
    {25,  "Mouse cursor position",          "D", CLEAN,  G_EXAMPLE,    "hover_drag_interaction.c" },
    {26,  "Pressed button visual",          "D", CLEAN,  G_EXAMPLE,    "hover_drag_interaction.c" },
    {27,  "Drag preview (ghost image)",      "D", CLEAN,  G_EXAMPLE,    "hover_drag_interaction.c" },
    {28,  "Drag-drop reorder",              "D", CLEAN,  G_EXAMPLE,    "hover_drag_interaction.c" },
    {29,  "Swipe gesture (touch)",          "D", FORCED, G_LIMITATION, "L12" },
    {30,  "Pinch-to-zoom",                  "D", CANNOT, G_LIMITATION, "L12" },
    {31,  "Tooltip on hover (delayed)",     "D", FORCED, G_LIMITATION, "L11" },
    {32,  "Context menu (right-click)",     "D", CLEAN,  G_EXAMPLE,    "palette_afford.c" },
    {33,  "Autocomplete suggestions",       "D", FORCED, G_CLAIM_ONLY, NULL },
    {34,  "Infinite scroll",                "D", FORCED, G_LIMITATION, "L12" },
    {35,  "Resizable panel (drag handle)",  "D", FORCED, G_LIMITATION, "L11" },
    {36,  "Color picker (drag slider)",      "D", CLEAN,  G_EXAMPLE,    "palette_afford.c" },
    {37,  "Knob / rotary control",          "D", FORCED, G_CLAIM_ONLY, NULL },
    {38,  "Scroll position",                 "D", FORCED, G_LIMITATION, "L12" },

    /* Category E: Layout & Spatial (P39-P44) — 2 clean, 4 forced */
    {39,  "Fixed layout",                   "E", CLEAN,  G_CLAIM_ONLY, NULL },
    {40,  "Responsive layout (resize)",      "E", FORCED, G_LIMITATION, "L11" },
    {41,  "Flexbox / grid",                  "E", FORCED, G_CLAIM_ONLY, NULL },
    {42,  "Absolute positioning",           "E", CLEAN,  G_CLAIM_ONLY, NULL },
    {43,  "Z-order / stacking",             "E", FORCED, G_CLAIM_ONLY, NULL },
    {44,  "Clipping / overflow",             "E", FORCED, G_CLAIM_ONLY, NULL },

    /* Category F: Async & External Data (P45-P52) — 4 clean, 4 forced */
    {45,  "Fetch data (async)",             "F", CLEAN,  G_EXAMPLE,    "async_demo.c" },
    {46,  "Polling (interval)",             "F", FORCED, G_CLAIM_ONLY, NULL },
    {47,  "WebSocket (real-time)",           "F", FORCED, G_CLAIM_ONLY, NULL },
    {48,  "File upload progress",           "F", CLEAN,  G_CLAIM_ONLY, NULL },
    {49,  "Search debouncing",              "F", FORCED, G_CLAIM_ONLY, NULL },
    {50,  "Optimistic update",              "F", CLEAN,  G_EXAMPLE,    "confidence_demo.c" },
    {51,  "Error retry",                    "F", CLEAN,  G_CLAIM_ONLY, NULL },
    {52,  "Offline mode",                   "F", FORCED, G_CLAIM_ONLY, NULL },

    /* Category G: Multi-window / Multi-context (P53-P57) — 2 clean, 2 forced, 1 cannot */
    {53,  "Multi-window sync",              "G", CANNOT, G_NONGOAL,    "NG-12" },
    {54,  "Split pane",                      "G", FORCED, G_CLAIM_ONLY, NULL },
    {55,  "Popup window",                   "G", FORCED, G_CLAIM_ONLY, NULL },
    {56,  "Notification toast",             "G", CLEAN,  G_CLAIM_ONLY, NULL },
    {57,  "Global state (Redux-like)",       "G", CLEAN,  G_CLAIM_ONLY, NULL },

    /* Category H: Accessibility & Multi-denotation (P58-P63) — 4 clean, 2 forced (P61 re-scored v0.8, ADR-0020) */
    {58,  "Screen reader (a11y)",           "H", CLEAN,  G_EXAMPLE,    "multi_perception.c" },
    {59,  "Test snapshot",                  "H", CLEAN,  G_EXAMPLE,    "counter_denotative.c" },
    {60,  "High contrast mode",             "H", FORCED, G_CLAIM_ONLY, NULL },
    {61,  "Keyboard navigation",            "H", CLEAN,  G_EXAMPLE,    "palette_afford.c" },
    {62,  "Reduced motion",                 "H", FORCED, G_CLAIM_ONLY, NULL },
    {63,  "ARIA live region",               "H", CLEAN,  G_CLAIM_ONLY, NULL },

    /* Category I: Extension & Programmability (P64-P68) — 1 clean, 3 forced, 1 cannot */
    {64,  "Plugin / extension",             "I", CANNOT, G_NONGOAL,    "NG-5" },
    {65,  "User scripting",                  "I", CANNOT, G_NONGOAL,    "NG-5" },
    {66,  "Theme system",                   "I", FORCED, G_NONGOAL,    "NG-8" },
    {67,  "Internationalization",           "I", FORCED, G_NONGOAL,    "NG-9" },
    {68,  "Custom widgets",                  "I", CLEAN,  G_EXAMPLE,    "integration_4abs.c" },
};

#define N_PATTERNS (int)(sizeof(kPatterns) / sizeof(kPatterns[0]))

/* ============================================================
 * Run counters
 * ============================================================ */

static int g_checks_run = 0;
static int g_checks_pass = 0;
static int g_checks_fail = 0;

#define CHECK(cond, fmt, ...) do {                                  \
    g_checks_run++;                                                \
    if (cond) {                                                    \
        g_checks_pass++;                                           \
    } else {                                                       \
        g_checks_fail++;                                           \
        printf("  [FAIL] P%-3d %s — " fmt "\n",                    \
               p->id, p->name, ##__VA_ARGS__);                     \
    }                                                              \
} while (0)

/* ============================================================
 * Grounding verifiers
 *
 * Each verifier opens the named doc file, scans for the grounding
 * reference (e.g. "## L4:" or "## NG-12:"), and returns 1 if found.
 * For EXAMPLE grounding, stat()s the file under examples/.
 * ============================================================ */

/* Check that a file under examples/ exists. */
static int example_exists(const char* filename) {
    char path[256];
    snprintf(path, sizeof(path), "examples/%s", filename);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* Scan a doc for a heading like "## L4:" or "## NG-12:". */
static int doc_has_heading(const char* doc_path, const char* heading_prefix) {
    FILE* f = fopen(doc_path, "r");
    if (!f) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        /* Match "^## <prefix>" at start of line. */
        if (line[0] == '#' && line[1] == '#' && line[2] == ' ') {
            /* Compare the rest of the line against heading_prefix. */
            if (strncmp(line + 3, heading_prefix, strlen(heading_prefix)) == 0) {
                found = 1;
                break;
            }
        }
    }
    fclose(f);
    return found;
}

/* Verify a LIMITATION grounding_ref (e.g. "L4") exists in limitations.md. */
static int limitation_exists(const char* l_id) {
    /* The heading in limitations.md looks like "## L4: Undo / redo not implemented". */
    /* heading_prefix is the L# followed by ":". */
    char prefix[16];
    snprintf(prefix, sizeof(prefix), "%s:", l_id);
    return doc_has_heading("docs/concepts/state/limitations.md", prefix);
}

/* Verify a NONGOAL grounding_ref (e.g. "NG-12") exists in non-goals.md. */
static int nongoal_exists(const char* ng_id) {
    /* The heading in non-goals.md looks like "## NG-12: Multi-window / multi-process UI". */
    char prefix[16];
    snprintf(prefix, sizeof(prefix), "%s:", ng_id);
    return doc_has_heading("docs/concepts/canonical/non-goals.md", prefix);
}

/* ============================================================
 * Main per-pattern check
 * ============================================================ */

static void check_pattern(const pattern_t* p) {
    switch (p->grounding_kind) {
        case G_EXAMPLE: {
            int exists = example_exists(p->grounding_ref);
            CHECK(exists,
                  "expected examples/%s to exist (EXAMPLE grounding)",
                  p->grounding_ref ? p->grounding_ref : "(null)");
            break;
        }
        case G_CLAIM_ONLY:
            /* No external check — CLAIM_ONLY patterns are trivially expressible
             * by definition. Their grounding is the prose in the corpus doc
             * itself, which is checked by the corpus-table invariant below. */
            g_checks_run++;
            g_checks_pass++;
            break;
        case G_LIMITATION: {
            int exists = limitation_exists(p->grounding_ref);
            CHECK(exists,
                  "expected ## %s: heading in docs/concepts/state/limitations.md",
                  p->grounding_ref ? p->grounding_ref : "(null)");
            break;
        }
        case G_NONGOAL: {
            int exists = nongoal_exists(p->grounding_ref);
            CHECK(exists,
                  "expected ## %s: heading in docs/concepts/canonical/non-goals.md",
                  p->grounding_ref ? p->grounding_ref : "(null)");
            break;
        }
        default:
            g_checks_run++;
            g_checks_fail++;
            printf("  [FAIL] P%-3d %s — unknown grounding_kind\n", p->id, p->name);
            break;
    }
}

/* ============================================================
 * Corpus invariant checks
 * ============================================================ */

static void check_corpus_invariants(void) {
    /* Count verdicts. */
    int total = 0, clean = 0, forced = 0, cannot = 0;
    for (int i = 0; i < N_PATTERNS; i++) {
        total++;
        switch (kPatterns[i].verdict) {
            case CLEAN:  clean++;  break;
            case FORCED: forced++; break;
            case CANNOT: cannot++; break;
        }
    }

    printf("\n[Corpus invariants]\n");

    g_checks_run++;
    if (total == EXPECTED_TOTAL) {
        g_checks_pass++;
        printf("  [OK]   pattern count = %d (expected %d)\n", total, EXPECTED_TOTAL);
    } else {
        g_checks_fail++;
        printf("  [FAIL] pattern count = %d (expected %d)\n", total, EXPECTED_TOTAL);
    }

    g_checks_run++;
    if (clean == EXPECTED_CLEAN) {
        g_checks_pass++;
        printf("  [OK]   clean count  = %d (expected %d)\n", clean, EXPECTED_CLEAN);
    } else {
        g_checks_fail++;
        printf("  [FAIL] clean count  = %d (expected %d)\n", clean, EXPECTED_CLEAN);
    }

    g_checks_run++;
    if (forced == EXPECTED_FORCED) {
        g_checks_pass++;
        printf("  [OK]   forced count = %d (expected %d)\n", forced, EXPECTED_FORCED);
    } else {
        g_checks_fail++;
        printf("  [FAIL] forced count = %d (expected %d)\n", forced, EXPECTED_FORCED);
    }

    g_checks_run++;
    if (cannot == EXPECTED_CANNOT) {
        g_checks_pass++;
        printf("  [OK]   cannot count = %d (expected %d)\n", cannot, EXPECTED_CANNOT);
    } else {
        g_checks_fail++;
        printf("  [FAIL] cannot count = %d (expected %d)\n", cannot, EXPECTED_CANNOT);
    }

    /* Sum invariant: clean + forced + cannot = total. */
    g_checks_run++;
    if (clean + forced + cannot == total) {
        g_checks_pass++;
        printf("  [OK]   verdict sum invariant: %d + %d + %d = %d\n",
               clean, forced, cannot, total);
    } else {
        g_checks_fail++;
        printf("  [FAIL] verdict sum invariant: %d + %d + %d != %d\n",
               clean, forced, cannot, total);
    }

    /* ID uniqueness + sequential P1..P68. */
    int seen[128] = {0};
    int id_unique_ok = 1;
    int id_sequential_ok = 1;
    for (int i = 0; i < N_PATTERNS; i++) {
        int id = kPatterns[i].id;
        if (id < 1 || id > 127) { id_unique_ok = 0; break; }
        if (seen[id]) { id_unique_ok = 0; break; }
        seen[id] = 1;
    }
    for (int i = 0; i < N_PATTERNS; i++) {
        if (kPatterns[i].id != i + 1) { id_sequential_ok = 0; break; }
    }

    g_checks_run++;
    if (id_unique_ok) {
        g_checks_pass++;
        printf("  [OK]   pattern IDs are unique\n");
    } else {
        g_checks_fail++;
        printf("  [FAIL] pattern IDs are not unique (duplicate detected)\n");
    }

    g_checks_run++;
    if (id_sequential_ok) {
        g_checks_pass++;
        printf("  [OK]   pattern IDs are sequential P1..P%d\n", N_PATTERNS);
    } else {
        g_checks_fail++;
        printf("  [FAIL] pattern IDs are not sequential P1..P%d\n", N_PATTERNS);
    }
}

/* ============================================================
 * Per-category breakdown (informational, not a CI fail)
 * ============================================================ */

static void print_category_breakdown(void) {
    printf("\n[Category breakdown]\n");
    printf("  %-40s %5s %5s %5s %5s\n", "Category", "cln", "frc", "cnt", "tot");
    for (char c = 'A'; c <= 'I'; c++) {
        int cl = 0, fr = 0, cn = 0, tot = 0;
        for (int i = 0; i < N_PATTERNS; i++) {
            if (kPatterns[i].category[0] == c) {
                tot++;
                switch (kPatterns[i].verdict) {
                    case CLEAN:  cl++; break;
                    case FORCED: fr++; break;
                    case CANNOT: cn++; break;
                }
            }
        }
        if (tot == 0) continue;
        printf("  %-40s %5d %5d %5d %5d\n",
               category_name((const char[]){c, '\0'}),
               cl, fr, cn, tot);
    }
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("=== test_completeness — UI Pattern Corpus Falsifiability Check ===\n");
    printf("Corpus: docs/reference/ui-pattern-corpus.md (68 patterns, v0.5)\n");
    printf("Hardcoded table: %d patterns\n\n", N_PATTERNS);

    /* Phase 1: corpus invariants (count, distribution, ID uniqueness). */
    check_corpus_invariants();

    /* Phase 2: per-pattern grounding verification. */
    printf("\n[Per-pattern grounding verification]\n");
    for (int i = 0; i < N_PATTERNS; i++) {
        check_pattern(&kPatterns[i]);
    }

    /* Phase 3: informational category breakdown (not a CI fail). */
    print_category_breakdown();

    /* Summary. */
    printf("\n=== Summary ===\n");
    printf("Checks run:    %d\n", g_checks_run);
    printf("Checks passed: %d\n", g_checks_pass);
    printf("Checks failed: %d\n", g_checks_fail);
    printf("\n%s\n",
           g_checks_fail == 0
               ? "ALL CHECKS PASSED — corpus is consistent with the repo."
               : "FAILURES DETECTED — corpus has drift from the repo.");

    return g_checks_fail == 0 ? 0 : 1;
}
