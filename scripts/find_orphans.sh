#!/usr/bin/env bash
# scripts/find_orphans.sh — CI-ready scanner for unreferenced markdown docs.
#
# Falsifiability gate (see doc-organization.md Part VI, Acceptance Check 2 —
# "Orphan doc scan"). Catches the Wave 5+ failure mode: a doc that lands
# in `staging/` (or anywhere else) and is never graduated / linked from
# anywhere in the docs graph.
#
# Walks every `.md` file under docs/ + repo-root *.md, builds a set of all
# files referenced by any other markdown link, then reports files NOT in
# that set. Orphans are surfaced as failures unless allowlisted below.
#
# Allowlist (files that are NOT orphans by definition):
#   - docs/staging/*           — holding pen by design (Principle 5)
#   - docs/changelog.md        — chronological, not referenced
#   - docs/faq.md               — entry-point, not referenced
#   - docs/README.md            — top-level index, discovered by navigation
#   - README.md, CONTRIBUTING.md, TESTING.md, UPGRADING.md, PLATFORMS.md
#                                — repo-root entry points, discovered by
#                                  convention not by inter-doc links
#   - docs/decisions/README.md  — auto-generated ADR index (gen_adr_index.sh)
#   - docs/decisions/TEMPLATE.md, TEMPLATE-GUIDE.md, REVIEW-RUBRIC.md
#                                — templates, reached by convention
#   - docs/concepts/{canonical,state,history,background,speculation}/README.md
#                                — section dividers, conventional entry
#   - docs/how-to/README.md, docs/reference/README.md, docs/research/README.md,
#   - docs/tutorials/README.md  — same: section dividers
#
# Usage:
#   scripts/find_orphans.sh                # exit non-zero on orphans
#   scripts/find_orphans.sh --check         # same, prints orphan list first
#   scripts/find_orphans.sh --report        # always exit 0, print human report
#
# Exit codes:
#   0 — no orphans (allowlist aside)
#   1 — at least one orphan found (CI fail)
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

# Allowlist — patterns matched against the repo-relative path of each file.
# Order: longer/more specific patterns first to keep the bash case branches tidy.
allowlist=(
    "docs/staging/"                                # holding pen by design
    "docs/changelog.md"                            # chronological
    "docs/faq.md"                                  # entry-point
    "docs/README.md"                               # top-level index
    "docs/doc-organization.md"                     # meta-doc, navigated separately
    "docs/decisions/README.md"                     # auto-generated ADR index
    "docs/decisions/TEMPLATE.md"                    # template, convention
    "docs/decisions/TEMPLATE-GUIDE.md"             # template companion
    "docs/decisions/REVIEW-RUBRIC.md"               # review rubric, convention
    "docs/concepts/README.md"
    "docs/concepts/canonical/README.md"
    "docs/concepts/state/README.md"
    "docs/concepts/history/README.md"
    "docs/concepts/background/README.md"
    "docs/concepts/speculation/README.md"
    "docs/how-to/README.md"
    "docs/reference/README.md"
    "docs/research/README.md"
    "docs/tutorials/README.md"
    "README.md"                                    # repo root
    "CONTRIBUTING.md"
    "TESTING.md"
    "UPGRADING.md"
    "PLATFORMS.md"
)

# Discover all markdown files (repo-relative paths).
md_files=()
while IFS= read -r f; do
    md_files+=("$f")
done < <(
    find docs -name '*.md' -type f | sort
    find . -maxdepth 1 -name '*.md' -type f | sed 's|^\./||' | sort
)

# Build set of referenced files. For each markdown file, extract its link
# targets (after stripping code spans + fenced blocks), resolve each
# relative to the link source's directory, normalize, and record.
declare -A referenced=()

for src in "${md_files[@]}"; do
    dir_of_src=$(dirname "$src")
    while IFS= read -r target; do
        [[ -z "$target" ]] && continue
        # Strip fragment.
        path_part="${target%%#*}"
        [[ -z "$path_part" ]] && continue
        # Skip external.
        case "$path_part" in
            http://*|https://*|mailto:*|ftp://*|git://*|ssh://*|tel:*) continue ;;
        esac
        # Resolve relative to source.
        candidate=$(realpath -m "$dir_of_src/$path_part" 2>/dev/null || echo "")
        [[ -z "$candidate" ]] && continue
        # Convert to repo-relative.
        rel=${candidate#$REPO_ROOT/}
        # If the resolved path is a directory, mark the directory's *.md
        # children as referenced (e.g. links to docs/staging/ mark every
        # file there as referenced) — no, that's too lenient. We only mark
        # the directory itself as referenced.
        if [[ -d "$candidate" ]]; then
            referenced["$rel/"]=1
        else
            referenced["$rel"]=1
        fi
    done < <(
        awk '/^```/{c=!c;next}c{next}{gsub(/`[^`]*`/,"");print}' "$src" \
            | grep -oE '\]\([^)]+\)' \
            | sed -E 's/^\]\((.*)\)$/\1/'
    )
done

# Now check each markdown file: is it in `referenced` (or under an
# allowlisted prefix)?
orphans=()
for f in "${md_files[@]}"; do
    # Allowlist check.
    skip=0
    for pat in "${allowlist[@]}"; do
        if [[ "$f" == "$pat" ]]; then
            skip=1
            break
        fi
    done
    [[ $skip -eq 1 ]] && continue
    # Allowlist prefix check (e.g. docs/staging/*).
    case "$f" in
        docs/staging/*) skip=1 ;;
    esac
    [[ $skip -eq 1 ]] && continue

    # Is it referenced by anyone?
    if [[ -n "${referenced[$f]:-}" ]]; then
        continue
    fi
    orphans+=("$f")
done

# Print report.
if [[ ${#orphans[@]} -gt 0 ]]; then
    echo "find_orphans: ${#orphans[@]} unreferenced markdown file(s):"
    printf '  %s\n' "${orphans[@]}"
    echo
    echo "Every markdown doc must be linked from at least one other doc."
    echo "See: doc-organization.md Part VI (Acceptance Check 2 — Orphan doc scan)."
else
    echo "find_orphans: all markdown files are referenced (allowlist aside)."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ ${#orphans[@]} -eq 0 ]] || exit 1
fi
