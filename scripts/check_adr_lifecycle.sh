#!/usr/bin/env bash
# scripts/check_adr_lifecycle.sh — CI-ready linter for ADR lifecycle-state consistency.
#
# Falsifiability gate, in the spirit of RFC 7322+7841's "Status of This Memo"
# boilerplate per ADR (see doc-organization.md Part IX, Principle 10 —
# "Status-of-This-Memo boilerplate per ADR (immutable)").
#
# Verifies that every ADR file lives in the lifecycle directory that matches
# its declared Status. The lifecycle state is encoded by directory
# (Principle 1); this script catches the Wave 3 failure mode where an ADR's
# Status field was not updated when the file was moved between lifecycle dirs.
#
# Status field is extracted from the first non-blank line under the `## Status`
# heading. The script accepts the following declared forms (case-insensitive):
#
#   accepted   -> docs/decisions/accepted/ADR-*.md
#   proposed  -> docs/decisions/proposed/ADR-*.md
#   deferred  -> docs/decisions/deferred/ADR-*.md
#   deprecated-> docs/decisions/deprecated/ADR-*.md
#   superseded-> docs/decisions/superseded/ADR-*.md
#
# Usage:
#   scripts/check_adr_lifecycle.sh                # exit non-zero on mismatch
#   scripts/check_adr_lifecycle.sh --check        # same, prints drift summary first
#   scripts/check_adr_lifecycle.sh --report        # always exit 0, print human report
#
# Exit codes:
#   0 — all ADRs live in the directory matching their declared Status
#   1 — at least one ADR is in the wrong directory (CI fail)
#   2 — usage error

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
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

# Lifecycle directories and the Status tokens that map to them.
# Order matters: more specific tokens (superseded, deprecated) are tested
# before more general ones (accepted, proposed) — otherwise ADR-0001's
# `**Superseded by ...**` followed by `Accepted: 2026-08-24` would match
# `accepted` first and report a false mismatch.
declare -a LIFECYCLE_DIRS=(superseded deprecated deferred accepted proposed)

# Find every ADR markdown file across the 5 lifecycle subdirs.
adr_files=()
for state in "${LIFECYCLE_DIRS[@]}"; do
    dir="$ADR_ROOT/$state"
    if [[ -d "$dir" ]]; then
        while IFS= read -r f; do
            adr_files+=("$f")
        done < <(find "$dir" -maxdepth 1 -name 'ADR-*.md' -type f | sort)
    fi
done

if [[ ${#adr_files[@]} -eq 0 ]]; then
    echo "check_adr_lifecycle: no ADR files found under $ADR_ROOT/{${LIFECYCLE_DIRS[*]}}/" >&2
    exit 2
fi

mismatch_total=0
declare -a report_lines=()

for f in "${adr_files[@]}"; do
    # Resolve actual lifecycle directory from the file's parent.
    parent_dir=$(basename "$(dirname "$f")")
    base=$(basename "$f")

    # Extract the body of the `## Status` heading (lines until the next `## `).
    # The Status heading is anchored at start-of-line with exactly two hashes.
    # Only inspect the first non-empty, non-blockquote line — that's the
    # actual declared Status; later lines are date/superseded-by metadata.
    status_body=$(awk '
        /^## Status[[:space:]]*$/ { in_status=1; next }
        /^## / { if (in_status) in_status=0 }
        in_status && $0 !~ /^[[:space:]]*>/ && $0 !~ /^[[:space:]]*$/ { print; exit }
    ' "$f")

    if [[ -z "$status_body" ]]; then
        mismatch_total=$((mismatch_total + 1))
        report_lines+=("$base :: MISSING :: no '## Status' heading")
        continue
    fi

    # Determine declared lifecycle: the first line whose lead word matches a
    # known token (case-insensitive). The check is intentionally fuzzy — ADRs
    # frequently write "**Accepted** — 2026-08-28" or
    # "Superseded by [ADR-NNNN]..." — the lead token is what matters.
    declared=""
    for tok in "${LIFECYCLE_DIRS[@]}"; do
        # Case-insensitive whole-word match for the lifecycle token at the
        # start of any line in the Status body.
        if grep -qiE "^[[:space:]]*[\*]*[[:space:]]*${tok}\b" <<<"$status_body"; then
            declared="$tok"
            break
        fi
    done

    if [[ -z "$declared" ]]; then
        mismatch_total=$((mismatch_total + 1))
        report_lines+=("$base :: MISSING :: Status body has no recognized lifecycle token (looked for: ${LIFECYCLE_DIRS[*]})")
        continue
    fi

    if [[ "$declared" != "$parent_dir" ]]; then
        mismatch_total=$((mismatch_total + 1))
        report_lines+=("$base :: MISMATCH :: Status=$declared but located in $parent_dir/ (expected $declared/)")
    fi
done

# Print report.
if [[ ${#report_lines[@]} -gt 0 ]]; then
    echo "check_adr_lifecycle: $mismatch_total mismatch(es) across ${#adr_files[@]} ADR file(s):"
    printf '  %s\n' "${report_lines[@]}"
    echo
    echo "Each ADR's lifecycle state is encoded by its directory"
    echo "(Principle 1 of doc-organization.md). The Status field must match the directory."
    echo
    echo "Fix: either move the file (git mv docs/decisions/<old>/ADR-X.md docs/decisions/<new>/)"
    echo "     or correct the Status field in the ADR body to match the directory."
else
    echo "check_adr_lifecycle: all ${#adr_files[@]} ADR file(s) live in directories matching their declared Status."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ $mismatch_total -eq 0 ]] || exit 1
fi
