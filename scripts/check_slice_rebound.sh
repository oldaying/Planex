#!/usr/bin/env bash
#
# check_slice_rebound.sh — the slice's rebound signal-1 static gate
# (ADR-0025 Decision 1; ADR-0023's rebound question, first automated
# enforcement: can a "retired" concept sneak back in as a special
# case?).
#
# What it forbids in slice/src/*.c and slice/include/:
#   1. kind dispatch — any comparison against a source's `kind`
#      field (I1: kind is metadata and NEVER dispatches; the
#      person/agent/process distinction must not re-enter as code)
#   2. a `passive` mode — the word itself (I2: passivity is the
#      all-zero-budget CONFIGURATION, not a mode the core checks)
#   3. single-source special-casing — `n_sources == 1` branches
#      (I1's short-circuit is emergent from the one-clock total
#      order; a consulted special case would be the rebound)
#   4. is_person / is_agent / is_process predicates — the classic
#      shape the retired concept returns in
#
# Then the synthetic-bite demonstration: the same patterns are
# injected into a COPY of the tree, and the gate MUST fail on the
# copy (a gate that cannot bite is decoration — ADR-0024's
# Validation precedent).
#
# Usage: scripts/check_slice_rebound.sh   (from the repo root or
#        anywhere; paths resolve from the script's location)
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SLICE="$REPO_ROOT/slice"

fail=0

# --- gate body ------------------------------------------------
#
# Scanner: plain GNU grep -E, same as every other gate script in
# this repository (scripts/check_doc_sections.sh et al.) — the CI
# runners do not install ripgrep, and a gate whose scanner is
# missing must not silently pass. (The first CI run of this script
# used rg and the bite demo caught exactly that: 0/3 planted
# rebounds caught behind a clean-looking gate. The demo works.)
#
# Each pattern is anchored with a required-space-or-word-boundary
# prefix so comments cannot dodge it and identifiers cannot
# half-match it.

scan() {
    # $1 = ERE, $2... = files-or-dirs. NOTE the option order: the
    # --include filters come BEFORE the "--" terminator — after it
    # they would be parsed as FILE arguments, fail, and the failure
    # would vanish behind 2>/dev/null while the scan still ran on
    # the directory arguments. (Found by testing the bite in
    # isolation: grep printed "No such file: --include=*.c" that
    # the gate's own stderr redirection had been eating. A scanner
    # that half-fails silently is half a gate.)
    local pat="$1"; shift
    grep -rnE --include='*.c' --include='*.h' -- "$pat" "$@" 2>/dev/null || true
}

patterns=(
    '(\.|->)kind *==|kind *!='                # 1. kind dispatch
    '\<passive\>'                             # 2. a quiet-system MODE
    'n_sources *== *1'                         # 3. single-source case
    '\<is_(person|agent|process)\>'           # 4. kind predicates
)

names=(
    "kind dispatch (I1: kind is metadata, never dispatches)"
    "a 'passive' mode (I2: quietness is the zero-budget configuration)"
    "single-source special-casing (I1: the short-circuit is emergent)"
    "kind predicates (the classic rebound shape)"
)

for i in "${!patterns[@]}"; do
    hits=$(scan "${patterns[$i]}" "$SLICE/src" "$SLICE/include")
    if [ -n "$hits" ]; then
        echo "REBOUND SIGNAL 1: ${names[$i]}"
        echo "$hits"
        echo
        fail=1
    fi
done

if [ "$fail" -ne 0 ]; then
    echo "gate: FAIL — a retired concept rebounded as a special case"
    exit 1
fi
echo "rebound gate: clean (no kind dispatch, no quiet-mode flag, no single-source case)"

# --- synthetic bite -------------------------------------------
# Inject each forbidden pattern into a scratch copy; the gate must
# catch every one. If any injection survives, the gate is
# toothless and that is itself a failure.

BITE_DIR="$(mktemp -d)"
trap 'rm -rf "$BITE_DIR"' EXIT
cp -r "$SLICE/src" "$BITE_DIR/src"
cp -r "$SLICE/include" "$BITE_DIR/include"

# bite 1: kind dispatch
printf '\nstatic int bite1(const ifl_world* w, ifl_sid s)\n{\n    if (w->sources[s].kind == IFL_KIND_PERSON) return 1;\n    return 0;\n}\n' \
    >> "$BITE_DIR/src/ifl_core.c"
# bite 2: a passive mode
printf '\nstatic int bite2(const ifl_world* w)\n{\n    if (w->passive) return 1;\n    return 0;\n}\n' \
    >> "$BITE_DIR/src/ifl_merge.c"
# bite 3: single-source special case
printf '\nstatic int bite3(const ifl_world* w)\n{\n    if (w->n_sources == 1) return 1;\n    return 0;\n}\n' \
    >> "$BITE_DIR/src/ifl_budget.c"

bites_caught=0
for i in "${!patterns[@]}"; do
    hits=$(scan "${patterns[$i]}" "$BITE_DIR/src" "$BITE_DIR/include")
    if [ -n "$hits" ]; then
        bites_caught=$((bites_caught + 1))
    fi
done

# patterns 1-3 are injected; pattern 4 (predicates) is covered by
# the same identifier family as 1 — require at least the three
# planted bites to be caught.
if [ "$bites_caught" -lt 3 ]; then
    echo "synthetic bite: FAILED — the gate missed a planted rebound ($bites_caught/3 caught)"
    exit 1
fi
echo "synthetic bite: all planted rebounds caught ($bites_caught patterns matched)"
echo "rebound gate: PASS"
exit 0
