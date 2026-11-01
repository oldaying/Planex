#!/usr/bin/env bash
#
# check_slice_rebound.sh — the slice's and v5's rebound signal-1
# static gate (ADR-0025 Decision 1; ADR-0026 Decision 6 extends it
# to the v5 tree; ADR-0023's rebound question, first automated
# enforcement: can a "retired" concept sneak back in as a special
# case?).
#
# What it forbids in slice/src/*.c, slice/include/, v5/src/*.c and
# v5/include/:
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
# What it ADDITIONALLY forbids in v5/ only (ADR-0026 Decision 3c,
# the S15 seat's no-borrowing clause):
#   5. visual-field vocabulary — rect, glyph, font, color, colour,
#      pixel, rgba as whole words in v5's headers and core. The
#      semantic tree must carry the interface's semantic content
#      WITHOUT reproduction primacy: a tree that needed a
#      rectangle to exist would be the dead reproducibility claim
#      sneaking back in as a field name (vol. 7 ch. 5's re-review
#      condition, statically enforced).
#
# Then the synthetic-bite demonstration: the same patterns are
# injected into a COPY of each tree, and the gate MUST fail on the
# copy (a gate that cannot bite is decoration — ADR-0024's
# Validation precedent).
#
# Usage: scripts/check_slice_rebound.sh   (from the repo root or
#        anywhere; paths resolve from the script's location)
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SLICE="$REPO_ROOT/slice"
V5="$REPO_ROOT/v5"

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

# --- the v5 section (ADR-0026 Decision 6) ---------------------
# The same four bans over the derivation tree, plus the S15
# visual-field ban. The ban's registered scope is the CORE
# (v5/src + v5/include), where the ontology lives; the tests
# exercise the ban from outside.

v5_patterns=(
    '(\.|->)kind *==|kind *!='                # 1. kind dispatch
    '\<passive\>'                             # 2. a quiet-system MODE
    'n_sources *== *1'                         # 3. single-source case
    '\<is_(person|agent|process)\>'           # 4. kind predicates
    '\<(rect|glyph|font|color|colour|pixel|rgba)\>'  # 5. S15 visual fields
)

v5_names=(
    "kind dispatch (I1: kind is metadata, never dispatches)"
    "a 'passive' mode (I2: quietness is the zero-budget configuration)"
    "single-source special-casing (I1: the short-circuit is emergent)"
    "kind predicates (the classic rebound shape)"
    "a visual field in the semantic core (S15: no reproduction primacy)"
)

for i in "${!v5_patterns[@]}"; do
    hits=$(scan "${v5_patterns[$i]}" "$V5/src" "$V5/include")
    if [ -n "$hits" ]; then
        echo "REBOUND SIGNAL (v5): ${v5_names[$i]}"
        echo "$hits"
        echo
        fail=1
    fi
done

# --- the mechanism-layer ban (ADR-0027 Decision 6) --------------
# The px_interaction invariant, statically enforced at its v5
# seat: if5_process_sample's body may not submit, append, or
# debit — samples fan out to nothing, ever. The body is extracted
# with awk (from the function's opening line to the first
# column-0 closing brace) and scanned with the same GNU grep -E
# the repo's gates use.
sample_body=$(awk '/^bool if5_process_sample/,/^}/' "$V5/src/if5_process.c" 2>/dev/null)
if printf '%s' "$sample_body" | grep -Eq 'if5_submit|if5__append|if5__post_debit|if5_debit'; then
    echo "REBOUND SIGNAL (v5/process): if5_process_sample submits — samples must fan out to nothing (ADR-0027 2g)"
    fail=1
fi

# --- the R3 bans (ADR-0028 Decision 6) ---------------------------
# Two new boundaries, both about the client zone v5/client/:
#
#   6. DEPENDENCY DIRECTION — the core (v5/src + v5/include) may
#      not reference any if5c_ symbol. The client links the core;
#      the core never links the client. The S15 visual-field ban's
#      scan paths stay exactly v5/src + v5/include (the client
#      zone is outside BY CONSTRUCTION — ADR-0028 registered the
#      map so a future "just extend the scan" edit is a visible
#      ADR-level change, not a gate tweak).
#
#   7. CLIENT PURITY — the client modules (v5/client/src/if5c_*.c)
#      may not call if5_submit / if5__append / if5__post_debit /
#      if5_debit / if5_undo_nth / if5_view_adopt: the client
#      renders, it does not speak. The APPLICATION (the demo in
#      v5/client/apps/, any program) submits through the ordinary
#      channel — the ban scopes to the client library's modules,
#      mirroring how the process ban scopes to one function body.

if [ -d "$V5/client" ]; then
    dep_hits=$(scan 'if5c_' "$V5/src" "$V5/include")
    if [ -n "$dep_hits" ]; then
        echo "REBOUND SIGNAL (v5/direction): the core references the client —"
        echo "  the client links the core, never the other way (ADR-0028 Decision 1)"
        echo "$dep_hits"
        echo
        fail=1
    fi

    purity_hits=$(scan 'if5_submit|if5__append|if5__post_debit|if5_debit|if5_undo_nth|if5_view_adopt' \
        "$V5/client/src")
    if [ -n "$purity_hits" ]; then
        echo "REBOUND SIGNAL (v5/client): a client module speaks — the client"
        echo "  renders, it does not submit (ADR-0028 Decision 2)"
        echo "$purity_hits"
        echo
        fail=1
    fi
fi

if [ "$fail" -ne 0 ]; then
    echo "gate: FAIL — a retired concept rebounded as a special case"
    exit 1
fi
echo "v5 rebound gate: clean (+ no visual field in the semantic core, samples pure, core/client one-way, client silent)"

# --- synthetic bite -------------------------------------------
# Inject each forbidden pattern into a scratch copy; the gate must
# catch every one. If any injection survives, the gate is
# toothless and that is itself a failure.

BITE_DIR="$(mktemp -d)"
BITE5_DIR="$(mktemp -d)"
trap 'rm -rf "$BITE_DIR" "$BITE5_DIR"' EXIT
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
echo "synthetic bite: all planted slice rebounds caught ($bites_caught patterns matched)"

# --- the v5 bite (ADR-0026 Decision 6) -------------------------
# Plant a kind dispatch AND a visual field in a copy of v5; the
# v5 patterns must catch both (the visual-field ban gets its own
# bite — a new pattern with no bite is exactly the toothless-gate
# shape this demo exists to prevent).

cp -r "$V5/src" "$BITE5_DIR/src"
cp -r "$V5/include" "$BITE5_DIR/include"

printf '\nstatic int bite4(const if5_world* w, if5_sid s)\n{\n    if (w->sources[s].kind == IF5_KIND_PERSON) return 1;\n    return 0;\n}\n' \
    >> "$BITE5_DIR/src/if5_core.c"
printf '\nstruct bite5 { unsigned rect; unsigned color; };\nstatic int bite5fn(void)\n{\n    struct bite5 b = { 1, 2 };\n    return (int)(b.rect + b.color);\n}\n' \
    >> "$BITE5_DIR/src/if5_tree.c"

# The mechanism-layer bite (ADR-0027 Decision 6): a submit
# smuggled into the sample path — the body scan must catch it, or
# the new ban is decorative exactly where the ADR relies on it.
python3 - "$BITE5_DIR/src/if5_process.c" << 'PYBITE'
import sys
p = sys.argv[1]
s = open(p).read()
needle = "    p->latest = value;"
if needle in s and "if5__append(p->w" not in s:
    s = s.replace(needle, "    (void)if5__append(p->w, 0);\n" + needle, 1)
    open(p, 'w').write(s)
    print("planted process bite")
else:
    print("bite already present or needle missing")
PYBITE

v5_bites=0
for i in "${!v5_patterns[@]}"; do
    hits=$(scan "${v5_patterns[$i]}" "$BITE5_DIR/src" "$BITE5_DIR/include")
    if [ -n "$hits" ]; then
        v5_bites=$((v5_bites + 1))
    fi
done

# pattern 1 (kind dispatch) and pattern 5 (visual fields) are
# planted; patterns 2-4 share identifier families with 1. Require
# at least 2 distinct planted catches.
if [ "$v5_bites" -lt 2 ]; then
    echo "v5 synthetic bite: FAILED — the gate missed a planted rebound ($v5_bites/2 caught)"
    exit 1
fi
echo "v5 synthetic bite: planted rebounds caught ($v5_bites patterns matched)"

bite_body=$(awk '/^bool if5_process_sample/,/^}/' "$BITE5_DIR/src/if5_process.c" 2>/dev/null)
if ! printf '%s' "$bite_body" | grep -Eq 'if5_submit|if5__append|if5__post_debit|if5_debit'; then
    echo "process synthetic bite: FAILED — a submit inside if5_process_sample survived the scan"
    exit 1
fi
echo "process synthetic bite: the smuggled submit was caught"

# --- the R3 bites (ADR-0028 Decision 6) ---------------------------
# Plant BOTH new violations in copies: an if5c_ reference inside
# the core, and a submit inside a client module. A new ban with
# no bite is exactly the toothless-gate shape this demo exists
# to prevent (the R1 lesson, applied twice more).

if [ -d "$V5/client" ]; then
    BITE_R3="$(mktemp -d)"
    cp -r "$V5/src" "$BITE_R3/src"
    cp -r "$V5/client/src" "$BITE_R3/client_src"

    printf '\n#include "planex/if5c.h" /* bite: the core reaching its client */\nstatic int bite_r3_dir(void) { return (int)if5c_fb_hash(NULL); }\n' \
        >> "$BITE_R3/src/if5_tree.c"
    printf '\n/* bite: a client module that speaks */\nstatic uint32_t bite_r3_pure(if5_world* w)\n{\n    return if5_submit(w, 0, 1, 0, 1, NULL);\n}\n' \
        >> "$BITE_R3/client_src/if5c_hit.c"

    r3_dir_caught=0
    if [ -n "$(scan 'if5c_' "$BITE_R3/src")" ]; then
        r3_dir_caught=1
    fi
    r3_pure_caught=0
    if [ -n "$(scan 'if5_submit|if5__append|if5__post_debit|if5_debit|if5_undo_nth|if5_view_adopt' \
        "$BITE_R3/client_src")" ]; then
        r3_pure_caught=1
    fi
    rm -rf "$BITE_R3"

    if [ "$r3_dir_caught" -ne 1 ] || [ "$r3_pure_caught" -ne 1 ]; then
        echo "R3 synthetic bite: FAILED — direction=$r3_dir_caught purity=$r3_pure_caught (need 1/1)"
        exit 1
    fi
    echo "R3 synthetic bite: the core->client reference and the client submit both caught"
fi

echo "rebound gate: PASS"
exit 0
