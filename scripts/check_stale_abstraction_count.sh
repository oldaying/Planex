#!/usr/bin/env bash
# scripts/check_stale_abstraction_count.sh
#
# CI-ready enforcement of CONTRIBUTING.md rule 5 (Documentation sync).
# Fails CI when v0.5-current docs contain stale abstraction-count
# references — the exact failure mode ADR-0008 (v0.4, added px_loop
# as 5th abstraction) created and CONTRIBUTING.md rule 5 grep patterns
# surface manually today.
#
# Two detection patterns:
#   1. Literal "4 abstractions" or "four abstractions" (case-insensitive)
#      — stale because v0.4 promoted the abstraction count to 5.
#   2. Literal "Relation + Estimate + Closure + Perception" NOT followed
#      (on the same line) by "+ px_loop" — catches the missing-5th form.
#
# Code blocks (```fenced``` and `inline spans`) are skipped so the
# grep commands in CONTRIBUTING.md rule 5 (which mention the literal
# stale strings as detection targets) do not false-positive.
#
# Historical files are exempt entirely (they are accurate snapshots
# of their era and "4 abstractions" is correct in their context):
#   - docs/decisions/{proposed,accepted,validated,deferred,deprecated,superseded}/
#   - docs/changelog.md
#   - docs/research/
#   - docs/concepts/history/
#     (v0.4-roadmap.md moved there at v0.6.0; the blanket history/ exemption
#      now covers it — the former explicit state/ entry was removed)
#   - docs/concepts/background/   (background research; "four abstractions"
#                                  used as narrative reference to the
#                                  why-four-abstractions.md manifesto)
#   - docs/concepts/state/ui-pattern-coverage.md  (Applies to v0.4 snapshot)
#   - docs/concepts/state/roadmap-matrix.md       (Applies to v0.4 forward-looking)
#   - docs/concepts/canonical/why-four-abstractions.md
#                                                 (filename is historical;
#                                                  content explicitly
#                                                  narrates the 4->5
#                                                  transition per ADR-0008;
#                                                  "4 abstractions"
#                                                  references inside are
#                                                  explicit quotations of
#                                                  the historical framing)
#
# Inline marker: a line containing a stale match may be marked as
# intentional (e.g., a historical quotation, a description of a v0.4
# file, a link text matching the historical filename) with a trailing
# HTML comment on the same line:
#
#   Some text mentioning 4 abstractions <!-- stale-allow: reason -->
#
# The marker must be on the same line as the match. Reviewers seeing
# a marker in a PR diff should audit the reason; markers are not a
# blanket exemption — they are an auditable exemption.
#
# Usage:
#   scripts/check_stale_abstraction_count.sh                # CI mode
#   scripts/check_stale_abstraction_count.sh --check        # same
#   scripts/check_stale_abstraction_count.sh --report       # always exit 0
#
# Exit codes:
#   0 — no stale references in v0.5-current docs (allowlist/markers aside)
#   1 — at least one stale reference found (CI fail)
#   2 — usage error
#
# Closes the gap surfaced in commit aa752e7 (docs(spec): refresh stale
# 4-abstraction references in v0.5-current docs) by making the grep
# pattern self-enforcing instead of human-run-on-demand.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

mode="check"
case "${1:---check}" in
    --check)  mode="check" ;;
    --report) mode="report" ;;
    "")       mode="check" ;;
    -h|--help)
        sed -n '2,55p' "$0"
        exit 0
        ;;
    *)
        echo "Usage: $0 [--check|--report]" >&2
        exit 2
        ;;
esac

# Historical files (accurate snapshots of their era) — exempt entirely.
# Matched against repo-relative path with glob-style prefix.
exempt_paths=(
    "docs/decisions/"
    "docs/changelog.md"
    "docs/research/"
    "docs/concepts/history/"
    "docs/concepts/background/"
    "docs/concepts/state/ui-pattern-coverage.md"
    "docs/concepts/state/roadmap-matrix.md"
    "docs/concepts/canonical/why-four-abstractions.md"
)

is_exempt() {
    local path="$1"
    for pat in "${exempt_paths[@]}"; do
        # shellcheck disable=SC2254
        case "$path" in
            "$pat"*) return 0 ;;
        esac
    done
    return 1
}

# Discover all v0.5-current markdown files.
md_files=()
while IFS= read -r f; do
    is_exempt "$f" && continue
    md_files+=("$f")
done < <(
    {
        find docs -name '*.md' -type f
        find examples -name '*.md' -type f
        find . -maxdepth 1 -name '*.md' -type f | sed 's|^\./||'
    } | sort
)

# Single awk pass per file. Tracks fenced code blocks, strips inline
# code spans, applies both detection patterns, honors the inline
# stale-allow marker.
violations=()
for f in "${md_files[@]}"; do
    while IFS= read -r hit; do
        [[ -z "$hit" ]] && continue
        violations+=("$hit")
    done < <(
        awk -v file="$f" '
            BEGIN { in_code = 0 }
            # Match fenced code blocks allowing 0-3 leading spaces (CommonMark).
            /^[[:space:]]{0,3}```/ { in_code = !in_code; next }
            in_code { next }
            {
                line = $0
                # Strip inline code spans so grep command text inside
                # `inline spans` does not false-positive.
                gsub(/`[^`]*`/, "", line)
                # Link-text exemption: if "Four abstractions" appears inside
                # a markdown link text (e.g., [Why Four Abstractions](...)),
                # it is a reference to the why-four-abstractions.md filename,
                # not an abstraction-count claim. Skip the line entirely.
                # The pattern matches `[...Four abstractions...]` (any case)
                # with no `]` between the opening `[` and the matched word.
                if (line ~ /\[[^]]*[Ff]our [Aa]bstractions[^]]*\]/) next
                # Pattern 1: 4/four abstractions (case-insensitive).
                if (tolower(line) ~ /(^|[^a-z0-9])(4|four) abstractions([^a-z0-9]|$)/) {
                    if (line ~ /<!-- stale-allow:/) next
                    print file ":" FNR ": stale 4/four-abstractions reference: " $0
                    next
                }
                # Pattern 2: missing-px_loop form.
                if (line ~ /Relation \+ Estimate \+ Closure \+ Perception/) {
                    if (line ~ /\+ px_loop/) next
                    if (line ~ /<!-- stale-allow:/) next
                    print file ":" FNR ": missing px_loop in abstraction list: " $0
                    next
                }
            }
        ' "$f"
    )
done

# Report.
if [[ ${#violations[@]} -gt 0 ]]; then
    echo "check_stale_abstraction_count: ${#violations[@]} stale reference(s) in v0.5-current docs:"
    printf '  %s\n' "${violations[@]}"
    echo
    echo "v0.5-current docs must say \"5 abstractions\" and list"
    echo "\"Relation + Estimate + Closure + Perception + px_loop\" (with px_loop)."
    echo "If the reference is intentional (historical quotation, link text"
    echo "matching the why-four-abstractions.md filename, description of a"
    echo "v0.4 snapshot file), add an inline marker on the same line:"
    echo "  <!-- stale-allow: reason -->"
    echo
    echo "See: CONTRIBUTING.md rule 5 (Documentation sync) for the manual"
    echo "grep this script automates."
    echo "See: ADR-0008 (added px_loop as 5th abstraction, v0.4)."
else
    echo "check_stale_abstraction_count: no stale 4-abstraction references in v0.5-current docs."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
fi

[[ ${#violations[@]} -eq 0 ]] || exit 1
exit 0
