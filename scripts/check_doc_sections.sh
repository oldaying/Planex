#!/usr/bin/env bash
# scripts/check_doc_sections.sh — CI-ready linter for Planex ADR mandatory sections.
#
# Falsifiability gate, in the spirit of Mathlib's `docBlame` linter (see
# doc-organization.md Part IX, Principle 11 — "Mandatory sections enforced by CI").
#
# Usage:
#   scripts/check_doc_sections.sh                # exit non-zero on any missing section
#   scripts/check_doc_sections.sh --check        # same, but prints drift summary first
#   scripts/check_doc_sections.sh --report       # always exit 0, print human report
#   scripts/check_doc_sections.sh --four-column  # --check plus the ADR-0024 discipline:
#                                                #   ADRs numbered >= 0024 must also carry
#                                                #   "## Refutation Criteria" and
#                                                #   "## Regression Clause". Default mode
#                                                #   is behavior-identical to pre-ADR-0024;
#                                                #   CI passes --four-column since ADR-0024
#                                                #   reached Validated (2026-11-01), per
#                                                #   that ADR's registered promotion
#                                                #   procedure.
#
# Required ADR sections (every ADR under docs/decisions/{proposed,validated,accepted,deferred,deprecated,superseded}/):
#   - "## Status"            (lifecycle field)
#   - "## Context"           (why this decision was made)
#   - "## Decision"          (the actual decision)
#   - "## Consequences"      (downstream effects)
#   - "## Alternatives Considered"
#   - "## CAVEATS"            (what this decision does NOT promise — distinct from alternatives)
#   - "## HISTORY"           (per-ADR state transition log, mirrors the project-level changelog)
#   - "## References"         (code paths, related ADRs, external prior art)
#
# Four-column sections (ADR-0024 discipline, "--four-column" mode only, ADRs >= 0024):
#   - "## Refutation Criteria" (the decision's expiry condition — what observable
#                               evidence would count against it)
#   - "## Regression Clause"   (the contract a successor ADR inherits if this one is
#                               superseded or deprecated)
#
# "Essence Check" is required only for abstraction-affecting decisions; this script
# does not enforce it (the TEMPLATE.md note explains the rule).
#
# Exit codes:
#   0 — all ADRs have all mandatory sections
#   1 — at least one ADR is missing a mandatory section (CI fail)
#   2 — usage error

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || echo "$(pwd)/..")"
ADR_ROOT="$REPO_ROOT/docs/decisions"

mode="check"
four_column=0
for arg in "$@"; do
    case "$arg" in
        --check)       mode="check" ;;
        --report)      mode="report" ;;
        --four-column) four_column=1 ;;
        "")            mode="check" ;;
        -h|--help)
            sed -n '2,36p' "$0"
            exit 0
            ;;
        *)
            echo "Usage: $0 [--check|--report] [--four-column]" >&2
            exit 2
            ;;
    esac
done

# Mandatory section headings (literal `## X` matches — anchored at line start,
# case-sensitive, exactly two leading hashes so `###` sub-headings don't count).
mandatory=(
    "## Status"
    "## Context"
    "## Decision"
    "## Consequences"
    "## Alternatives Considered"
    "## CAVEATS"
    "## Known issues"
    "## HISTORY"
    "## References"
)

# ADR-0024 four-column sections ("--four-column" mode only). Appended to the
# per-file requirement for ADRs whose number is >= the threshold below. The
# number threshold, not a content sniff, is the grandfathering mechanism —
# ADR-0001..0023 predate the discipline and are exempt by ADR-0024's own
# When-NOT-to-use section.
four_column_sections=(
    "## Refutation Criteria"
    "## Regression Clause"
)
four_column_threshold=24

# Find every ADR markdown file under the 6 lifecycle subdirs.
adr_files=()
for state in proposed validated accepted deferred deprecated superseded; do
    if [[ -d "$ADR_ROOT/$state" ]]; then
        while IFS= read -r f; do
            adr_files+=("$f")
        done < <(find "$ADR_ROOT/$state" -maxdepth 1 -name 'ADR-*.md' -type f | sort)
    fi
done

if [[ ${#adr_files[@]} -eq 0 ]]; then
    echo "check_doc_sections: no ADR files found under $ADR_ROOT/{proposed,validated,accepted,deferred,deprecated,superseded}/" >&2
    exit 2
fi

missing_total=0
declare -a report_lines=()

for f in "${adr_files[@]}"; do
    base=$(basename "$f")
    # Per-file required sections: the universal mandatory list, plus (in
    # --four-column mode) the two ADR-0024 sections for ADRs >= threshold.
    required=("${mandatory[@]}")
    if [[ $four_column -eq 1 ]]; then
        # 10# forces base-10: ADR numbers are zero-padded ("0024"), and bare
        # [[ -ge ]] would read 8/9-containing numbers as invalid octal — the
        # exact silent-escape bug the repo's own gate culture exists to catch.
        adr_num=$(printf '%s' "$base" | grep -oE 'ADR-[0-9]+' | head -n1 | grep -oE '[0-9]+' || printf '0')
        if (( 10#${adr_num:-0} >= four_column_threshold )); then
            required+=("${four_column_sections[@]}")
        fi
    fi
    for sect in "${required[@]}"; do
        # Use grep with line-anchored pattern. -E needed for the `^## ` literal.
        if ! grep -qE "^${sect}([[:space:]]|$)" "$f"; then
            missing_total=$((missing_total + 1))
            report_lines+=("$base :: MISSING :: $sect")
        fi
    done
done

# Print report.
if [[ ${#report_lines[@]} -gt 0 ]]; then
    echo "check_doc_sections: $missing_total missing mandatory section(s) across ${#adr_files[@]} ADR file(s):"
    printf '  %s\n' "${report_lines[@]}"
    echo
    echo "Mandatory sections (per docs/decisions/TEMPLATE.md):"
    for s in "${mandatory[@]}"; do echo "  - $s"; done
    if [[ $four_column -eq 1 ]]; then
        echo "  (+ ${four_column_sections[0]} and ${four_column_sections[1]} for ADR-$(printf "%04d" ${four_column_threshold}) and later — ADR-0024 four-column mode)"
    fi
    echo
    echo "See: docs/doc-organization.md Part IX (Principle 11) for the rationale."
else
    if [[ $four_column -eq 1 ]]; then
        echo "check_doc_sections: all ${#adr_files[@]} ADR file(s) have all mandatory sections (four-column mode: ADR-$(printf "%04d" ${four_column_threshold}) and later also checked for ${four_column_sections[0]} and ${four_column_sections[1]})."
    else
        echo "check_doc_sections: all ${#adr_files[@]} ADR file(s) have all ${#mandatory[@]} mandatory sections."
    fi
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ $missing_total -eq 0 ]] || exit 1
fi
