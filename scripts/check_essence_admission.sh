#!/usr/bin/env bash
# scripts/check_essence_admission.sh — essence-justified abstraction admission
# enforcement (Part 3 of ADR-0014).
#
# Implements ADR-0011's three-criterion admission gate for essence-justified
# abstractions, closing two of the three sub-criteria automatically:
#   1. Tradition citation (paper / book / URL / figurehead / tradition name)
#   2. Non-strawman Alternatives Considered section (real alternative with body)
#   3. Negative Consequences sub-section (per TEMPLATE-GUIDE.md's
#      "Zero-negative-consequences" failure mode)
#
# The third sub-criterion of ADR-0011 (denotational semantics) is NOT
# automated here; it remains reviewer-applied per REVIEW-RUBRIC.md
# criterion 3 (Compressiveness). The orthogonality half is enforced by
# tests/test_orthogonality.c (shipping v0.4) and tests/test_v4_orthogonality.c
# (v4 proposals), independent of this script.
#
# Scope: an ADR is in scope if it has a `## Essence Check` H2 heading
# (case-sensitive, exactly two leading hashes). ADRs without that heading
# (purely-engineering ADRs, observation ADRs, convention-naming ADRs) are
# not subject to this script per ADR-0014's counterexamples 1-3.
#
# Not retroactive: existing ADRs in `accepted/`, `superseded/`,
# `deprecated/`, `deferred/` are grandfathered. The CI `--check` mode
# scans only `docs/decisions/{proposed,validated}/`.
#
# Usage:
#   scripts/check_essence_admission.sh FILE [FILE...]   # check specific ADR(s)
#   scripts/check_essence_admission.sh --check          # CI: scan proposed/+validated/
#   scripts/check_essence_admission.sh --report         # always exit 0, print report
#
# Exit codes:
#   0 — every in-scope ADR passes all three criteria
#   1 — at least one in-scope ADR fails a criterion (CI fail)
#   2 — usage error
#
# ADR-0014 self-referential contract: this script is the demonstration
# required for ADR-0014 to progress from `Validated` to `Accepted`. The
# synthetic violation case in tests/synthetic_adr_0015.md is encoded as
# `--synthetic` mode, which runs the check on the synthetic file and
# expects non-zero exit (the lint correctly fires).

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
ADR_ROOT="$REPO_ROOT/docs/decisions"
SYNTHETIC_CASE="$REPO_ROOT/tests/synthetic_adr_0015.md"

# Figureheads from the 6-tradition bibliography in
# docs/concepts/canonical/why-four-abstractions.md. Any of these surnames
# in Essence Check or Context satisfies criterion 1 (tradition citation).
TRADITION_FIGUREHEADS='Alexander|Elliott|Friston|Gibson|Harel|Hutchins|Norman|Sutherland|Suchman|Winograd|Flores|Peirce|Searle|Heidegger|Hoare|Simmel|Dourish|Bateson|Maturana|Shoup|Rehg|Alexander|Conal|Karl'

# Tradition names from the 6-tradition survey.
TRADITION_NAMES='phenomenolog|semiotic|cybernetic|speech.act|predictive.coding|free.energy|functional.reactive|FRP|statechart|CSP|embodied|situated|affordance|denotational'

mode="check"
files=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --check)    mode="check"; shift ;;
        --report)   mode="report"; shift ;;
        --synthetic) mode="synthetic"; shift ;;
        -h|--help)
            sed -n '2,40p' "$0"
            exit 0
            ;;
        -*)
            echo "check_essence_admission: unknown option: $1" >&2
            echo "usage: $0 [--check|--report|--synthetic] [FILE...]" >&2
            exit 2
            ;;
        *)
            files+=("$1")
            shift
            ;;
    esac
done

# In --check / --report / --synthetic modes, ignore explicit file args
# (they would only be used in direct mode).
if [[ "$mode" == "check" || "$mode" == "report" ]]; then
    if [[ ${#files[@]} -gt 0 ]]; then
        # Direct mode: check the named files.
        :
    else
        # CI mode: scan proposed/ + validated/ ADRs.
        files=()
        for state in proposed validated; do
            dir="$ADR_ROOT/$state"
            if [[ -d "$dir" ]]; then
                while IFS= read -r f; do
                    files+=("$f")
                done < <(find "$dir" -maxdepth 1 -name 'ADR-*.md' -type f | sort)
            fi
        done
    fi
elif [[ "$mode" == "synthetic" ]]; then
    if [[ ! -f "$SYNTHETIC_CASE" ]]; then
        echo "check_essence_admission: synthetic case file missing: $SYNTHETIC_CASE" >&2
        exit 2
    fi
    files=("$SYNTHETIC_CASE")
fi

if [[ ${#files[@]} -eq 0 ]]; then
    echo "check_essence_admission: no ADR files to check" >&2
    if [[ "$mode" == "report" ]]; then
        exit 0
    fi
    exit 2
fi

# Extract the body of an H2 section from a markdown file, skipping fenced
# code blocks (``` ... ```). Prints the body to stdout.
#
# Args: $1 = file path, $2 = section title (e.g., "Context").
extract_section() {
    local file="$1"
    local section="$2"
    awk -v want="## $section" '
        BEGIN { in_code = 0; in_section = 0 }
        /^```/ { in_code = !in_code; next }
        in_code { next }
        /^## / {
            if (in_section) {
                # End of section (next H2).
                in_section = 0
            }
            if ($0 == want) {
                in_section = 1
            }
            next
        }
        in_section { print }
    ' "$file"
}

# Check whether a section body contains a tradition citation.
# Matches: any URL, any tradition figurehead surname, or any tradition name
# (case-insensitive). Returns 0 if found, 1 if not.
has_tradition_cite() {
    local body="$1"
    # URLs (http://... or https://...)
    if grep -qiE 'https?://[a-zA-Z0-9._/-]+' <<<"$body"; then
        return 0
    fi
    # Figurehead surnames
    if grep -qiE "\b($TRADITION_FIGUREHEADS)\b" <<<"$body"; then
        return 0
    fi
    # Tradition names
    if grep -qiE "\b($TRADITION_NAMES)\b" <<<"$body"; then
        return 0
    fi
    return 1
}

# Check whether the Alternatives Considered section has at least one real
# alternative (an H3 sub-heading with substantive body, not a one-line
# strawman). Returns 0 if at least one substantive alternative found, 1 if not.
has_real_alternatives() {
    local file="$1"
    # Extract the Alternatives Considered section (skipping code blocks).
    local body
    body=$(extract_section "$file" "Alternatives Considered")
    [[ -n "$body" ]] || return 1

    # Walk the body, splitting on H3 sub-headings; for each, count chars
    # in its body. A "real" alternative has >= 80 chars of body content
    # (excludes one-line strawman like "Do nothing" with no rationale).
    # The threshold 80 is calibrated: ADR-0009's shortest alternative
    # body is ~150 chars; ADR-0014's shortest is ~200 chars; synthetic
    # ADR-0015 has zero alternatives, so trivially fails.
    awk '
        BEGIN { current = ""; current_len = 0; found_real = 0 }
        /^### / {
            # Flush previous alternative.
            if (current_len >= 80) { found_real = 1 }
            current = $0
            current_len = 0
            next
        }
        # Skip blank lines but count non-blank content.
        /^[[:space:]]*$/ { next }
        current != "" { current_len += length($0) + 1 }
        END {
            if (current_len >= 80) { found_real = 1 }
            exit (found_real ? 0 : 1)
        }
    ' <<<"$body"
}

# Check whether the Consequences section has a Non-empty Negative sub-section.
# Returns 0 if found, 1 if not.
has_negative_consequences() {
    local file="$1"
    # Extract the Consequences section.
    local body
    body=$(extract_section "$file" "Consequences")
    [[ -n "$body" ]] || return 1

    # Within Consequences, find the ### Negative sub-section and verify
    # it has substantive body content (>= 40 chars; excludes the bare
    # heading with no body).
    awk '
        BEGIN { in_neg = 0; neg_len = 0 }
        /^### / {
            if ($0 ~ /^### Negative[[:space:]]*$/) {
                in_neg = 1
            } else {
                if (in_neg && neg_len >= 40) { found = 1; exit 0 }
                in_neg = 0
            }
            next
        }
        in_neg && /^[[:space:]]*$/ { next }
        in_neg { neg_len += length($0) + 1 }
        END {
            if (in_neg && neg_len >= 40) { found = 1; exit 0 }
            exit (found ? 0 : 1)
        }
    ' <<<"$body"
}

# Check whether an ADR is in scope: it has a `## Essence Check` H2 heading
# (skipping fenced code blocks). Returns 0 if in scope, 1 if not.
in_scope() {
    local file="$1"
    # Use awk to find `## Essence Check` heading OUTSIDE any fenced block.
    awk '
        BEGIN { in_code = 0; found = 0 }
        /^```/ { in_code = !in_code; next }
        in_code { next }
        /^## Essence Check[[:space:]]*$/ { found = 1; exit }
        END { exit (found ? 0 : 1) }
    ' "$file"
}

# Extract ADR number from filename (e.g., "ADR-0014-validated-...md" → "0014").
get_adr_num() {
    local file="$1"
    basename "$file" | grep -oE 'ADR-[0-9]+' | head -1 | sed 's/ADR-//'
}

# Extract ADR title (first H1 line, minus the "# ADR-NNNN: " prefix).
get_adr_title() {
    local file="$1"
    grep -m1 '^# ' "$file" | sed 's/^# //; s/^ADR-[0-9]*:[[:space:]]*//'
}

# Run the three criteria on a single ADR file.
# Returns 0 if all pass, 1 if any fail. Prints PASS/FAIL lines.
check_adr() {
    local file="$1"
    local num title context_body essence_body
    num=$(get_adr_num "$file")
    title=$(get_adr_title "$file")

    # Scope check: ADR must have a `## Essence Check` section.
    if ! in_scope "$file"; then
        echo "check_essence_admission: ADR-$num ($title) OUT OF SCOPE — no \`## Essence Check\` section; skipping."
        return 0
    fi

    # Criterion 1: tradition citation in Context OR Essence Check.
    context_body=$(extract_section "$file" "Context")
    essence_body=$(extract_section "$file" "Essence Check")
    local c1_fail=0
    if ! has_tradition_cite "$context_body" && ! has_tradition_cite "$essence_body"; then
        c1_fail=1
    fi

    # Criterion 2: non-strawman Alternatives Considered.
    local c2_fail=0
    if ! has_real_alternatives "$file"; then
        c2_fail=1
    fi

    # Criterion 3: Negative Consequences sub-section.
    local c3_fail=0
    if ! has_negative_consequences "$file"; then
        c3_fail=1
    fi

    local overall=0
    if [[ $c1_fail -eq 1 || $c2_fail -eq 1 || $c3_fail -eq 1 ]]; then
        overall=1
        echo "check_essence_admission: ADR-$num ($title) FAILS the essence-justified admission gate:"
        if [[ $c1_fail -eq 1 ]]; then
            echo "  criterion 1 (tradition citation): no tradition source cited in Essence Check or Context."
            echo "    ADR-0011 requires a specific paper or book citation, not a general vibe."
        fi
        if [[ $c2_fail -eq 1 ]]; then
            echo "  criterion 2 (real alternatives): no \`### Alternative\` sub-heading with substantive body found."
            echo "    ADR-0014 requires at least one non-strawman alternative in \`## Alternatives Considered\`."
        fi
        if [[ $c3_fail -eq 1 ]]; then
            echo "  criterion 3 (negative consequences): no \`### Negative\` sub-section with substantive body found."
            echo "    TEMPLATE-GUIDE.md's \"Zero-negative-consequences\" failure mode requires at least one."
        fi
    else
        echo "check_essence_admission: ADR-$num ($title) PASSES all 3 essence-justified admission criteria."
    fi
    return $overall
}

# Main driver.
fail_count=0
pass_count=0
out_of_scope=0
declare -a fail_files=()

for f in "${files[@]}"; do
    if [[ ! -f "$f" ]]; then
        echo "check_essence_admission: file not found: $f" >&2
        fail_count=$((fail_count + 1))
        continue
    fi
    if in_scope "$f"; then
        if check_adr "$f"; then
            pass_count=$((pass_count + 1))
        else
            fail_count=$((fail_count + 1))
            fail_files+=("$f")
        fi
    else
        out_of_scope=$((out_of_scope + 1))
        num=$(get_adr_num "$f")
        title=$(get_adr_title "$f")
        echo "check_essence_admission: ADR-$num ($title) OUT OF SCOPE — no \`## Essence Check\` section; skipping (not subject to ADR-0014)."
    fi
done

echo
echo "check_essence_admission: $pass_count PASS, $fail_count FAIL, $out_of_scope OUT OF SCOPE across ${#files[@]} ADR file(s)."

if [[ "$mode" == "synthetic" ]]; then
    # Synthetic mode: the synthetic ADR-0015 is expected to fail.
    if [[ $fail_count -eq 1 && $pass_count -eq 0 ]]; then
        echo "OK: synthetic case correctly triggered the lint (expected fail, got fail)."
        exit 0
    else
        echo "ERROR: synthetic case did not trigger the lint as expected."
        echo "ADR-0014 self-referential contract is broken — the lint does not fire on its synthetic violation."
        exit 1
    fi
fi

if [[ "$mode" == "report" ]]; then
    exit 0
fi

if [[ $fail_count -gt 0 ]]; then
    echo
    echo "Failing ADRs:"
    for ff in "${fail_files[@]}"; do
        echo "  - $ff"
    done
    exit 1
fi

exit 0
