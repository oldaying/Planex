#!/usr/bin/env bash
# scripts/check_links.sh — CI-ready linter for Markdown link integrity.
#
# Falsifiability gate (see doc-organization.md Part VI, Acceptance Check 1 —
# "Bidirectional link integrity"). The Wave 1 reorganization rewrote ~285
# markdown links; this script catches the residual drift that the Wave 1
# rewrite pass missed, and catches all future broken links introduced by
# doc edits.
#
# Walks every `.md` file under docs/ + repo-root *.md (README, CONTRIBUTING,
# UPGRADING, TESTING, PLATFORMS), extracts `[text](path)` references to
# local files (skipping http(s)/mailto/anchors-only), and verifies each
# target exists. Fails on broken links.
#
# Allowed reference kinds:
#   [text](relative/path.md)               — file must exist
#   [text](relative/path.md#anchor)        — file must exist (anchor unchecked)
#   [text](./) [text](../foo)              — dir must exist
#   [text](https://...) [text](mailto:..)  — skipped (external)
#   [text](#anchor)                         — skipped (in-file anchor)
#
# Usage:
#   scripts/check_links.sh                # exit non-zero on any broken link
#   scripts/check_links.sh --check         # same, prints drift summary first
#   scripts/check_links.sh --report        # always exit 0, print human report
#
# Exit codes:
#   0 — all internal markdown links resolve
#   1 — at least one broken link found (CI fail)
#   2 — usage error

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

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

# Discover markdown files. Order matters only for the report.
md_files=()
while IFS= read -r f; do
    md_files+=("$f")
done < <(
    find docs -name '*.md' -type f | sort
    find . -maxdepth 1 -name '*.md' -type f | sort
)

if [[ ${#md_files[@]} -eq 0 ]]; then
    echo "check_links: no markdown files found" >&2
    exit 2
fi

broken_total=0
declare -a report_lines=()

for f in "${md_files[@]}"; do
    # Extract markdown link targets: `[text](target)`. Skip image links
    # `![alt](src)` only if they target external URLs — local image links
    # are checked too. Strip code fences first to avoid matching inside
    # ``` blocks.
    #
    # The link regex is intentionally permissive: matches `](` followed by
    # a non-whitespace, non-`)` run, terminated by `)`. Anchor and query
    # components are split off below.
    while IFS= read -r line; do
        # Skip empty captures.
        [[ -z "$line" ]] && continue

        # Split target from any fragment (`#anchor`) — file existence is
        # what we check; the anchor is a doc-organization Wave 4.1 concern.
        target="${line%%#*}"
        # If line was just `#anchor` (no path), target is empty — skip.
        [[ -z "$target" ]] && continue

        # Skip external links (http://, https://, mailto:, ftp://, etc.).
        case "$target" in
            http://*|https://*|mailto:*|ftp://*|git://*|ssh://*|tel:*) continue ;;
        esac

        # Skip inter-page anchors like `#essence` (no path, just fragment).
        [[ "$target" == "$line" && "$line" == \#* ]] && continue

        # Resolve relative to the directory of $f (a repo-relative path
        # because we cd'd to REPO_ROOT). Allow `~/` etc. are not in scope.
        # Compute the absolute candidate path.
        dir_of_f=$(dirname "$f")
        candidate="$dir_of_f/$target"
        # Normalize — collapse `./` and `../` segments. Use realpath -m
        # for "the path as it would be if it existed" so we can test
        # existence cleanly.
        candidate=$(realpath -m "$candidate" 2>/dev/null || echo "")

        if [[ -z "$candidate" || ! -e "$candidate" ]]; then
            broken_total=$((broken_total + 1))
            report_lines+=("$f :: BROKEN :: $line -> ${target}")
        fi
    done < <(
        # Strip fenced code blocks (``` ... ```) and inline code spans
        # (`...`) so we don't pick up example URLs from doc prose.
        # Then extract markdown link targets — the grep -oE pattern
        # captures the contents of `(...)` that follows a `]`.
        # Fragment-stripping (#anchor) happens in the main loop above.
        awk '/^```/{c=!c;next}c{next}{gsub(/`[^`]*`/,"");print}' "$f" \
            | grep -oE '\]\([^)]+\)' \
            | sed -E 's/^\]\((.*)\)$/\1/'
    )
done

# Print report.
if [[ ${#report_lines[@]} -gt 0 ]]; then
    echo "check_links: $broken_total broken internal link(s) across ${#md_files[@]} markdown file(s):"
    printf '  %s\n' "${report_lines[@]}"
    echo
    echo "Internal markdown links must resolve to existing files."
    echo "See: doc-organization.md Part VI (Acceptance Check 1 — Bidirectional link integrity)."
else
    echo "check_links: all internal links in ${#md_files[@]} markdown file(s) resolve."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ $broken_total -eq 0 ]] || exit 1
fi
