#!/usr/bin/env bash
# scripts/check_build_sync.sh — CI-ready linter for build-system sync.
#
# Falsifiability gate (Gate 13). Machine-enforces CONTRIBUTING.md rule 1
# ("Build system sync rule"), whose LESSON records 20+ commits of CI
# breakage because demos were updated in one build file but not the
# other. Until now the rule was a human checklist; the 2026-10 external
# audit found it had drifted in BOTH directions at once:
#   - examples/hover_drag_interaction.c was in the Makefile but missing
#     from CMakeLists STDOUT_DEMOS (invisible to the CMake CI jobs);
#   - eight stdout demos (animation_demo, async_demo, confidence_demo,
#     integration_4abs, v3_prototype_{actor,perlocution,interpretant,
#     breakdown}) were in CMakeLists but missing from the Makefile
#     (invisible to the make CI jobs);
#   - 17 `run-*` convenience targets referenced demos deleted in the
#     v0.3 refactor (the PLATFORMS.md demo-table rot class, in the
#     Makefile itself).
# None of the 12 existing gates could see any of this: no gate was
# designed to answer "do the two build files agree with examples/?".
#
# Contract (what this gate falsifies):
#   1. Every examples/*.c is declared in BOTH build systems:
#      - Makefile:        EXAMPLES_NO_X11 ∪ EXAMPLES_WINDOWED
#      - CMakeLists.txt:  STDOUT_DEMOS     ∪ WINDOWED_DEMOS
#   2. Every demo named in those lists has an examples/<name>.c file
#      (no phantom declarations pointing at deleted sources).
#   3. Every Makefile `run-*` convenience target points at a demo that
#      exists (no run-targets for deleted demos).
#   4. Every src/*.c is referenced by BOTH build systems (core lists,
#      backend selections, or the conditional font_ttf.c). Found in the
#      wild: actor.c/breakdown.c shipped in CMakeLists CORE_SOURCES but
#      never added to the Makefile CORE_SRCS — the root cause behind
#      the eight CMake-only demos (they could not link under make).
#
# Usage:
#   scripts/check_build_sync.sh --check    # CI mode: exit 1 on any drift
#   scripts/check_build_sync.sh --report   # print drift, always exit 0
#
# Pure shell (no toolchain) — same constraint class as the docs.yml
# gates, so it can run in any job.

set -u

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

mode="check"
case "${1:---check}" in
    --check)  mode="check" ;;
    --report) mode="report" ;;
    "")       mode="check" ;;
    -h|--help)
        sed -n '2,38p' "$0"
        exit 0
        ;;
    *)
        echo "Usage: $0 [--check|--report]" >&2
        exit 2
        ;;
esac

fail=0

# ---------------------------------------------------------------
# Inventory: what actually exists on disk
# ---------------------------------------------------------------
on_disk="$(ls examples/*.c 2>/dev/null | sed 's|examples/||; s|\.c$||' | sort -u)"

# ---------------------------------------------------------------
# Makefile lists (join backslash continuations, split on whitespace,
# strip # comments) — mirrors how make itself parses them
# ---------------------------------------------------------------
makefile_lists() {
    sed -e ':a' -e '/\\$/N' -e 's/\\\n[ \t]*/ /g' -e 'ta' Makefile \
      | awk -F'= ' '/^EXAMPLES_NO_X11[ ]*=/ || /^EXAMPLES_WINDOWED[ ]*=/ {print $2}' \
      | tr ' ' '\n' | sed 's/#.*//' | rg -v '^$' | sort -u
}
mk_listed="$(makefile_lists)"

# ---------------------------------------------------------------
# CMakeLists lists (set(STDOUT_DEMOS ... ) / set(WINDOWED_DEMOS ...))
# ---------------------------------------------------------------
cmake_lists() {
    awk '
        /set\(STDOUT_DEMOS/  {inlist=1; next}
        /set\(WINDOWED_DEMOS/{inlist=1; next}
        inlist {
            sub(/#.*/, "")                       # strip trailing comments FIRST:
                                                 # entries carry annotations like
                                                 # "# Phase 1 API validation (9 tests)"
                                                 # whose ")" must not end the set()
            if ($0 ~ /^[ \t]*\)/) { inlist=0; next }
            n = split($0, w, /[ \t]+/)
            for (i = 1; i <= n; i++)
                if (w[i] ~ /^[a-z][a-z0-9_]+$/) print w[i]
        }
    ' CMakeLists.txt | sort -u
}
cm_listed="$(cmake_lists)"

both_listed="$(printf '%s\n%s\n' "$mk_listed" "$cm_listed" | sort -u)"

# ---------------------------------------------------------------
# Check 1a: on disk but not in Makefile
# ---------------------------------------------------------------
missing_mk="$(comm -23 <(echo "$on_disk") <(echo "$mk_listed"))"
if [ -n "$missing_mk" ]; then
    echo "BUILD-SYNC FAIL: in examples/ but NOT in Makefile (EXAMPLES_NO_X11/EXAMPLES_WINDOWED):"
    echo "$missing_mk" | sed 's/^/    /'
    fail=1
fi

# ---------------------------------------------------------------
# Check 1b: on disk but not in CMakeLists
# ---------------------------------------------------------------
missing_cm="$(comm -23 <(echo "$on_disk") <(echo "$cm_listed"))"
if [ -n "$missing_cm" ]; then
    echo "BUILD-SYNC FAIL: in examples/ but NOT in CMakeLists.txt (STDOUT_DEMOS/WINDOWED_DEMOS):"
    echo "$missing_cm" | sed 's/^/    /'
    fail=1
fi

# ---------------------------------------------------------------
# Check 2: declared in a build list but no source file
# ---------------------------------------------------------------
phantom="$(comm -13 <(echo "$on_disk") <(echo "$both_listed"))"
if [ -n "$phantom" ]; then
    echo "BUILD-SYNC FAIL: declared in a build list but examples/<name>.c does not exist:"
    echo "$phantom" | sed 's/^/    /'
    fail=1
fi

# ---------------------------------------------------------------
# Check 3: run-* convenience targets pointing at missing demos
# ---------------------------------------------------------------
dead_runs=""
while IFS= read -r target; do
    [ -f "examples/${target}.c" ] || dead_runs="$dead_runs${target}\n"
done < <(rg -o '^run-[a-z0-9_]+' Makefile | sed 's/^run-//' | sort -u)
if [ -n "$dead_runs" ]; then
    echo "BUILD-SYNC FAIL: Makefile run-* targets with no examples/<name>.c:"
    printf "$dead_runs" | sed 's/^/    /'
    fail=1
fi

# ---------------------------------------------------------------
# Check 4: every src/*.c referenced by BOTH build systems
# (any list: core sources, backend selections, or the optional
#  font_ttf.c — a plain filename mention in each file is enough;
#  the falsifiable claim is "the file is known to this build")
# ---------------------------------------------------------------
for f in src/*.c; do
    base="$(basename "$f")"
    if ! rg -q "$base" Makefile; then
        echo "BUILD-SYNC FAIL: $f is not referenced by the Makefile (missing from CORE_SRCS / BACKEND_SRCS?)"
        fail=1
    fi
    if ! rg -q "$base" CMakeLists.txt; then
        echo "BUILD-SYNC FAIL: $f is not referenced by CMakeLists.txt (missing from CORE_SOURCES / BACKEND_SOURCES?)"
        fail=1
    fi
done

# ---------------------------------------------------------------
# Verdict
# ---------------------------------------------------------------
if [ "$fail" -eq 0 ]; then
    n_disk=$(echo "$on_disk"   | wc -l)
    n_mk=$(echo "$mk_listed"   | wc -l)
    n_cm=$(echo "$cm_listed"   | wc -l)
    n_src=$(ls src/*.c | wc -l)
    echo "check_build_sync: PASS — ${n_disk} example(s) on disk, ${n_mk} in Makefile, ${n_cm} in CMakeLists, ${n_src} src file(s) all referenced by both build systems, all run-* targets resolve."
    exit 0
fi

if [ "$mode" = "report" ]; then
    echo "check_build_sync: REPORT mode — drift above, exit 0."
    exit 0
fi
echo "check_build_sync: FAIL — build lists are out of sync with examples/."
echo "Fix per CONTRIBUTING.md rule 1: update CMakeLists.txt AND Makefile (and"
echo "examples/README.md) in the same commit. Run: scripts/check_build_sync.sh --report"
exit 1
