#!/usr/bin/env bash
# scripts/check_terms.sh — CI-ready linter for glossary term linking.
#
# Falsifiability gate (see doc-organization.md Part VI, Acceptance Check 3 —
# "Term consistency scan"). Catches the drift Principle 3 is designed to
# prevent: when a glossary term is mentioned in a doc without ANY link to
# the glossary, readers can't navigate to the canonical definition.
#
# Strategy:
#   1. Extract glossary anchor IDs from `docs/reference/glossary.md`
#      (patterns: `<a id="X"></a>`).
#   2. For each term X, walk every non-glossary markdown file in the
#      repo. Find files that mention the term (case-insensitive,
#      word-boundary) but contain ZERO markdown links to
#      `glossary.md#X`. These are "saturation gaps" — files that use
#      the term without linking to its definition even once.
#   3. Report per-file-per-term gaps. We do NOT require EVERY occurrence
#      to link (that's an unbounded criterion); we require each file to
#      link at least once per term it uses.
#
# Allowlist (terms exempt from the linking requirement, even when they
# appear in the glossary):
#   - "px-rel"        — too noisy as a substring; canonical form is PX_REL_*
#   - "depends-on"    — same; canonical form is DEPENDS_ON
#   - "triggers"      — same; canonical form is TRIGGERS
#   - "declare"       — overloaded by `px_closure_declare` API surface
#   - "fail"          — too noisy as a substring (matches "failure", "failed")
#   - "intent"        — overloaded by PX_INTENT_* enum, linked via code spans
#   - "promise"       — overloaded by px_closure_promise API
#   - "execution"     — generic English word
#   - "interpretation" — generic English word
#   - "behavior"      — generic English word
#   - "goal"          — generic English word
#   - "action"        — generic English word (Closure stage 3, but most uses
#                       are verb-noun not technical)
#   - "confidence"    — Estimate field but generic English
#   - "evaluation"    — generic English (Closure stage 7)
#
# The remaining checked terms (closure, estimate, perception, relation,
# semilattice) are Planex-specific enough to require glossary linking
# when used in canonical/ docs (the Wave 4.1 scope).
#
# Scope: by default, only files under docs/concepts/canonical/ are
# checked. Pass --scope=all to check every markdown file. The narrow
# default is the doc-organization.md Part VI Acceptance Check 3 scope;
# --scope=all is opt-in pending doc-wide glossary saturation.
#
# Allowlist is intentionally conservative; adding a term to it means
# "we accept that this term appears unlinked in docs." Review the
# allowlist whenever glossary.md gains a new term.
#
# Usage:
#   scripts/check_terms.sh                # exit non-zero on unlinked terms
#   scripts/check_terms.sh --check         # same, prints drift summary first
#   scripts/check_terms.sh --report        # always exit 0, print human report
#
# Exit codes:
#   0 — no unlinked glossary terms (allowlist aside)
#   1 — at least one unlinked term occurrence (CI fail)
#   2 — usage error

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

GLOSSARY="docs/reference/glossary.md"

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

if [[ ! -f "$GLOSSARY" ]]; then
    echo "check_terms: glossary not found at $GLOSSARY" >&2
    exit 2
fi

# Allowlist of term IDs that are exempt from the linking requirement.
allowlist_rx='^(px-rel|depends-on|triggers|declare|fail|intent|promise|execution|interpretation|behavior|goal|action|confidence|evaluation)$'

# Default scope: only canonical/ docs (the Wave 4.1 acceptance scope).
# Override with --scope=all to check every markdown file.
scope="canonical"
case "${2:-}" in
    --scope=canonical) scope="canonical" ;;
    --scope=all)       scope="all" ;;
    "")                ;;
    *) ;;
esac

# Step 1: extract glossary anchor IDs.
declare -a terms=()
while IFS= read -r t; do
    [[ -z "$t" ]] && continue
    if [[ "$t" =~ $allowlist_rx ]]; then
        continue
    fi
    terms+=("$t")
done < <(grep -oE '<a id="[^"]+"></a>' "$GLOSSARY" | sed -E 's/^<a id="([^"]+)"><\/a>$/\1/' | sort -u)

if [[ ${#terms[@]} -eq 0 ]]; then
    echo "check_terms: no glossary anchor IDs found in $GLOSSARY" >&2
    exit 2
fi

# Step 2: walk non-glossary markdown files in scope.
md_files=()
if [[ "$scope" == "canonical" ]]; then
    while IFS= read -r f; do
        [[ -z "$f" ]] && continue
        md_files+=("$f")
    done < <(find docs/concepts/canonical -name '*.md' -type f | sort)
else
    while IFS= read -r f; do
        md_files+=("$f")
    done < <(
        find docs -name '*.md' -type f | grep -v "^$GLOSSARY\$" | sort
        find . -maxdepth 1 -name '*.md' -type f | sed 's|^\./||' | sort
    )
fi

if [[ ${#md_files[@]} -eq 0 ]]; then
    echo "check_terms: no markdown files in scope ($scope)" >&2
    exit 2
fi

# Step 3: for each term, find unlinked occurrences.
unlinked_total=0
declare -a report_lines=()

for term in "${terms[@]}"; do
    # Pattern: case-insensitive word-boundary match.
    # \b in grep -E works for ASCII alphanumerics and the term IDs here
    # (e.g. estimate, perception) — but IDs like "px-rel" contain a hyphen,
    # which is itself a word boundary. Handle the hyphenated case specially
    # by treating the term as a literal and checking word boundaries on the
    # alphanumerics at each end.
    if [[ "$term" == *-* ]]; then
        # Use a literal match — no \b on the hyphen side.
        pattern="${term}"
    else
        pattern="\b${term}\b"
    fi

    # Find files mentioning the term (case-insensitive).
    mapfile -t hits < <(grep -liE "$pattern" "${md_files[@]}" 2>/dev/null || true)

    for f in "${hits[@]}"; do
        # Does the file contain ANY markdown link to glossary.md#$term?
        link_rx="glossary\.md#${term}([[:space:]]|[)]|\")"
        if grep -qE -- "$link_rx" "$f" 2>/dev/null; then
            continue
        fi
        # Find the first line that mentions the term for the report.
        # Use grep -m1 to avoid SIGPIPE under `set -o pipefail` (head -1 + pipe).
        first_hit=$(grep -niEm1 -- "$pattern" "$f" 2>/dev/null | cut -d: -f1)
        unlinked_total=$((unlinked_total + 1))
        report_lines+=("$f:$first_hit :: term '$term' used but never linked to glossary")
    done
done

# Print report.
if [[ ${#report_lines[@]} -gt 0 ]]; then
    echo "check_terms: $unlinked_total glossary term(s) used but unlinked somewhere across ${#md_files[@]} markdown file(s):"
    # Show first 50 to keep the report tractable; full list available in --report mode.
    if [[ "$mode" == "report" ]]; then
        printf '  %s\n' "${report_lines[@]}"
    else
        printf '  %s\n' "${report_lines[@]}" | head -50
        if [[ ${#report_lines[@]} -gt 50 ]]; then
            echo "  ... and $(( ${#report_lines[@]} - 50 )) more (run with --report for full list)"
        fi
    fi
    echo
    echo "Each glossary term used in a doc should be linked to"
    echo "  docs/reference/glossary.md#<term> at least once in that doc."
    echo "Allowlist (exempt terms): $(echo "$allowlist_rx" | sed -E 's/\^//;s/\$$//;s/\|/ /g')"
    echo
    echo "See: doc-organization.md Part VI (Acceptance Check 3 — Term consistency scan)."
else
    echo "check_terms: every glossary term used in ${#md_files[@]} markdown file(s) is linked at least once."
fi

if [[ "$mode" == "report" ]]; then
    exit 0
else
    [[ $unlinked_total -eq 0 ]] || exit 1
fi
