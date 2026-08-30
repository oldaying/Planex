#!/usr/bin/env bash
# scripts/check_limitations_freshness.sh
#
# CI-ready enforcement that limitations.md's "not yet implemented"
# / "has not been implemented" claims stay fresh — when a capability
# is actually implemented (file exists in examples/ or src/ or include/),
# the limitations.md claim of "not yet implemented" must be either
# updated to RESOLVED or marked inline with <!-- fresh-allow: reason -->.
#
# This script is the capability-status analogue of
# scripts/check_stale_abstraction_count.sh (which catches
# abstraction-count drift). Where that script catches "stale '4
# abstractions' references after ADR-0008 promoted to 5", this script
# catches "stale 'X has not been implemented' references after the
# X implementation landed in examples/ or src/".
#
# Two detection patterns (each pairs a stale-claim regex with a
# reality-check that verifies whether the claimed-missing capability
# actually exists in the repo):
#
#   Pattern 1: a line containing "undo-via-graph" or "undo_via_graph"
#              near "has not been implemented" / "not yet implemented"
#              / "hasn't been implemented" -> if examples/undo_via_graph.c
#              exists, the claim is stale (the proof is in the repo).
#
#   Pattern 2: a line containing "No anti-pattern tests" or
#              "no anti-pattern test exists" or "anti-pattern test column
#              is entirely red" -> if examples/antipattern_*.c exists
#              (glob has at least 1 match), the claim is stale.
#
# Code blocks (```fenced``` and `inline spans`) are skipped so that
# grep commands or code listings inside markdown don't false-positive.
#
# Historical files are exempt entirely (they are accurate snapshots
# of their era and the claims are correct in their original context):
#   - docs/decisions/{proposed,accepted,validated,deferred,deprecated,superseded}/
#   - docs/changelog.md
#   - docs/research/
#   - docs/concepts/history/
#     (v0.4-roadmap.md moved there at v0.6.0; the blanket history/ exemption
#      now covers it — the former explicit state/ entry was removed)
#   - docs/concepts/background/
#   - docs/concepts/state/ui-pattern-coverage.md  (Applies to v0.4 snapshot)
#   - docs/concepts/state/roadmap-matrix.md       (Applies to v0.4 forward-looking)
#   - docs/concepts/canonical/why-four-abstractions.md
#                                                 (filename is historical;
#                                                  content explicitly narrates
#                                                  the 4->5 transition per
#                                                  ADR-0008)
#
# Inline marker: a line containing a stale-claim match may be marked as
# intentional (e.g., a historical quotation, a description of a v0.4
# snapshot file, a link text matching the historical filename) with a
# trailing HTML comment on the same line:
#
#   Some text claiming X has not been implemented <!-- fresh-allow: reason -->
#
# The marker must be on the same line as the match. Reviewers seeing
# a marker in a PR diff should audit the reason; markers are not a
# blanket exemption — they are an auditable exemption.
#
# Usage:
#   scripts/check_limitations_freshness.sh                # CI mode
#   scripts/check_limitations_freshness.sh --check        # same
#   scripts/check_limitations_freshness.sh --report       # always exit 0
#
# Exit codes:
#   0 — no stale capability-claims in v0.5-current docs
#   1 — at least one stale capability-claim found (CI fail)
#   2 — usage error
#
# Closes the gap surfaced when limitations.md L2 ("undo_via_graph has
# not been implemented") + L3 ("No anti-pattern tests for any
# abstraction") + L4 ("No undo/redo system is shipped; no px_undo()
# API exists") were all found stale in the post-v0.5 audit — the
# implementations had landed but the limitations entries had not been
# updated. This script makes the freshness check self-enforcing instead
# of human-run-on-demand.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

mode="check"
case "${1:---check}" in
    --check)  mode="check" ;;
    --report) mode="report" ;;
    "")       mode="check" ;;
    -h|--help)
        sed -n '2,75p' "$0"
        exit 0
        ;;
    *)
        echo "Usage: $0 [--check|--report]" >&2
        exit 2
        ;;
esac

# Historical files (accurate snapshots of their era) — exempt entirely.
# Same exempt set as scripts/check_stale_abstraction_count.sh so the two
# fresh-checks apply to the same v0.5-current scope.
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

# Reality checks: does the claimed-missing capability actually exist?
has_undo_via_graph=0
[[ -f examples/undo_via_graph.c ]] && has_undo_via_graph=1

has_antipattern_tests=0
shopt -s nullglob
ap_files=(examples/antipattern_*.c)
shopt -u nullglob
[[ ${#ap_files[@]} -gt 0 ]] && has_antipattern_tests=1

has_undo_api=0
if [[ -f src/undo.c ]] && grep -q 'px_undo\b' include/planex/planex.h 2>/dev/null; then
    has_undo_api=1
fi

# Single awk pass per file. Tracks fenced code blocks, strips inline
# code spans, applies both detection patterns, honors the inline
# fresh-allow marker.
violations=()
for f in "${md_files[@]}"; do
    while IFS= read -r hit; do
        [[ -z "$hit" ]] && continue
        violations+=("$hit")
    done < <(
        awk -v file="$f" \
            -v has_undo_graph="$has_undo_via_graph" \
            -v has_antipattern="$has_antipattern_tests" \
            -v has_undo_api="$has_undo_api" '
            BEGIN { in_code = 0 }
            # Match fenced code blocks allowing 0-3 leading spaces (CommonMark).
            /^[[:space:]]{0,3}```/ { in_code = !in_code; next }
            in_code { next }
            {
                line = $0
                # Strip inline code spans so code-listings do not false-positive.
                gsub(/`[^`]*`/, "", line)
                # Inline-marker exemption: if the line has fresh-allow, skip.
                if (line ~ /<!-- fresh-allow:/) next

                # Pattern 1: undo-via-graph "has not been implemented" claim
                # + examples/undo_via_graph.c actually exists.
                if (has_undo_graph == 1 && \
                    line ~ /(undo-via-graph|undo_via_graph)/ && \
                    line ~ /(has not been implemented|hasn.t been implemented|not yet implemented|has not yet been implemented)/) {
                    print file ":" FNR ": stale undo-via-graph not-implemented claim; examples/undo_via_graph.c exists (CI runs it): " $0
                    next
                }

                # Pattern 1b: "no px_undo() API exists" claim + src/undo.c
                # actually has the px_undo symbol.
                if (has_undo_api == 1 && \
                    line ~ /no .?px_undo\(\).? API exists|no .?px_undo\(\).? or .?px_replay\(\).? API exists|no .?px_undo\(\).? exists/) {
                    print file ":" FNR ": stale no-px_undo-API claim; include/planex/planex.h declares px_undo (src/undo.c implements it): " $0
                    next
                }

                # Pattern 2: "No anti-pattern tests" claim +
                # examples/antipattern_*.c actually exists.
                if (has_antipattern == 1 && \
                    (line ~ /No anti-pattern tests?/ || \
                     line ~ /no anti-pattern test exists/ || \
                     line ~ /Anti-pattern test column is entirely .*🔴/ || \
                     line ~ /Anti-pattern test.*column.*🔴.*for all/)) {
                    print file ":" FNR ": stale no-anti-pattern-tests claim; examples/antipattern_*.c glob has " $0
                    next
                }
            }
        ' "$f"
    )
done

# Report.
if [[ ${#violations[@]} -gt 0 ]]; then
    echo "check_limitations_freshness: ${#violations[@]} stale capability-claim(s) in v0.5-current docs:"
    printf '  %s\n' "${violations[@]}"
    echo
    echo "limitations.md / canonical/ entries that claim a capability is"
    echo "not yet implemented must reflect reality — when the file lands"
    echo "in examples/ or src/ or the API is declared in include/planex/*.h,"
    echo "the corresponding limitations entry must be updated to RESOLVED"
    echo "(with strikethrough severity + Resolution section, per L1/L13"
    echo "pattern) or marked inline with:"
    echo "  <!-- fresh-allow: reason -->"
    echo
    echo "Reality checks the script performed:"
    echo "  examples/undo_via_graph.c exists: $([[ $has_undo_via_graph == 1 ]] && echo yes || echo no)"
    echo "  examples/antipattern_*.c exists: $([[ $has_antipattern_tests == 1 ]] && echo yes || echo no) (${#ap_files[@]} file(s))"
    echo "  px_undo() declared in include/planex/planex.h: $([[ $has_undo_api == 1 ]] && echo yes || echo no)"
    echo
    echo "See: limitations.md L1/L13 RESOLVED pattern for the resolution template."
    echo "See: ADR-0002's ## Resolution section for the ADR-side analogue."
else
    echo "check_limitations_freshness: no stale capability-claims in v0.5-current docs."
    echo "  reality: examples/undo_via_graph.c exists=$([[ $has_undo_via_graph == 1 ]] && echo yes || echo no); antipattern_*.c exists=$([[ $has_antipattern_tests == 1 ]] && echo yes || echo no); px_undo declared=$([[ $has_undo_api == 1 ]] && echo yes || echo no)."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
fi

[[ ${#violations[@]} -eq 0 ]] || exit 1
exit 0
