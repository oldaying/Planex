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
#
# Required ADR sections (every ADR under docs/decisions/{proposed,accepted,deferred,deprecated,superseded}/):
#   - "## Status"            (lifecycle field)
#   - "## Context"           (why this decision was made)
#   - "## Decision"          (the actual decision)
#   - "## Consequences"      (downstream effects)
#   - "## Alternatives Considered"
#   - "## CAVEATS"            (what this decision does NOT promise — distinct from alternatives)
#   - "## HISTORY"           (per-ADR state transition log, mirrors the project-level changelog)
#   - "## References"         (code paths, related ADRs, external prior art)
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
case "${1:---check}" in
    --check)  mode="check" ;;
    --report) mode="report" ;;
    "")       mode="check" ;;
    -h|--help)
        sed -n '2,30p' "$0"
        exit 0
        ;;
    *)
        echo "Usage: $0 [--check|--report]" >&2
        exit 2
        ;;
esac

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

# Find every ADR markdown file under the 5 lifecycle subdirs.
adr_files=()
for state in proposed accepted deferred deprecated superseded; do
    if [[ -d "$ADR_ROOT/$state" ]]; then
        while IFS= read -r f; do
            adr_files+=("$f")
        done < <(find "$ADR_ROOT/$state" -maxdepth 1 -name 'ADR-*.md' -type f | sort)
    fi
done

if [[ ${#adr_files[@]} -eq 0 ]]; then
    echo "check_doc_sections: no ADR files found under $ADR_ROOT/{proposed,accepted,deferred,deprecated,superseded}/" >&2
    exit 2
fi

missing_total=0
declare -a report_lines=()

for f in "${adr_files[@]}"; do
    base=$(basename "$f")
    for sect in "${mandatory[@]}"; do
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
    echo
    echo "See: docs/doc-organization.md Part IX (Principle 11) for the rationale."
else
    echo "check_doc_sections: all ${#adr_files[@]} ADR file(s) have all ${#mandatory[@]} mandatory sections."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ $missing_total -eq 0 ]] || exit 1
fi
