#!/usr/bin/env bash
# scripts/compression_metric.sh — Planex Compression Metric (PCM) v0.1
#
# Implements the metric defined in docs/concepts/state/compression-metric.md.
# Two sub-metrics:
#   A. Per-example Abstraction-Event Leverage (AEL):
#      AEL(X) = code_LOC(X) / distinct_px_API_calls(X)
#      WARN if AEL > 10.0; FAIL if AEL > 25.0 (catastrophic).
#      Antipattern demos (antipattern_*.c) and hover_drag_4abs.c (a
#      hack-pain measurement demo per ADR-0011) are MEASURED but
#      exempt from FAIL.
#
#   B. Aggregate Library-App Leverage (LLE):
#      LLE = abstraction_layer_LOC / application_layer_LOC
#      WARN if LLE < 1.0; FAIL if LLE < 0.3 (catastrophic).
#
# Threshold design: the v0.4 baseline is intentionally lenient. The
# metric's primary purpose is *drift detection* — when a new example
# pushes AEL above 25.0, or when LLE collapses below 0.3, that is a
# catastrophic signal the abstraction set is decompressing caller code.
# WARN-level signals (the v0.4 baseline has many) are tracked but not
# CI-blocked; they are the expected state of a v0.4 library whose
# abstractions are not yet mature.
#
# Exit codes:
#   0 — PASS (no critical threshold violated)
#   1 — FAIL (AEL > 25.0 on non-exempt example, OR LLE < 0.3)
#
# Usage:
#   scripts/compression_metric.sh                # run + print table
#   scripts/compression_metric.sh --check        # same; CI-friendly exit
#   scripts/compression_metric.sh --report       # always exit 0; full report
#
# Falsifiability gate (see abstraction-form.md Prerequisite 3 Layer 4):
# the metric is computed fresh from the source tree on every run. The
# WARN band is tracked but not CI-blocked; the FAIL band is CI-blocked.
# Drift in either band is a CI signal that the abstraction set may be
# decompressing caller code relative to a component-library baseline.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# code_LOC(file) = wc -l, minus blank lines, minus comment-only lines.
# A comment-only line matches ^\s*$ (blank), ^\s*// (C++ style),
# ^\s*\* (block-comment continuation), or ^\s*/\* (block-comment start).
code_LOC() {
    local f="$1"
    # 1. wc -l gives total physical lines
    # 2. grep -vcE counts lines NOT matching the comment/blank pattern
    wc -l < "$f" | awk '{print $1}'
    # Above is total LOC. For "code LOC" we subtract blank+comment lines.
    # But the file may have block-comment multi-line; we approximate by
    # treating any line that matches ^\s*$ | ^\s*// | ^\s*\* | ^\s*/\* | ^\s*\*/ as a non-code line.
    true
}

# A more careful code_LOC counter using awk:
code_LOC_careful() {
    local f="$1"
    awk '
BEGIN { code = 0; in_block = 0 }
{
    line = $0
    if (in_block) {
        if (match(line, /\*\//)) {
            in_block = 0
            sub(/^.*\*\//, "", line)
        } else {
            next
        }
    }
    gsub(/\/\*[^*]*\*\//, "", line)
    if (match(line, /\/\*/)) {
        in_block = 1
        sub(/\/\*.*/, "", line)
    }
    sub(/\/\/.*/, "", line)
    gsub(/^[ \t]+|[ \t]+$/, "", line)
    if (length(line) > 0) code++
}
END { print code + 0 }
' "$f"
}

# distinct_px_calls(file) = number of distinct px_* identifiers in the file.
# We use grep -oE 'px_[a-z_][a-z_0-9]*' to extract tokens, then sort -u.
distinct_px_calls() {
    local f="$1"
    grep -oE 'px_[a-z_][a-z_0-9]*' "$f" 2>/dev/null | sort -u | wc -l
}

# ---------------------------------------------------------------------------
# Build file lists
# ---------------------------------------------------------------------------

EXAMPLES_DIR="$REPO_ROOT/examples"
SRC_DIR="$REPO_ROOT/src"

# Application layer = all examples/*.c
mapfile -t APP_FILES < <(find "$EXAMPLES_DIR" -maxdepth 1 -name '*.c' -type f | sort)

# Abstraction layer = src/*.c EXCLUDING backend adapters
# (x11.c, win32.c, cocoa.c, headless.c, app.c are platform glue)
BACKEND_RE='(x11|win32|cocoa|headless|app)\.c$'
mapfile -t LIB_FILES < <(find "$SRC_DIR" -maxdepth 1 -name '*.c' -type f | grep -vE "$BACKEND_RE" | sort)

# ---------------------------------------------------------------------------
# Compute per-example AEL
# ---------------------------------------------------------------------------

# Output table
printf "%-40s %10s %12s %10s %s\n" "Example" "code_LOC" "px_calls" "AEL" "Verdict"
printf "%-40s %10s %12s %10s %s\n" "---------------------------------------" "---------" "----------" "--------" "---------------------------"

declare -a SUMMARY_LINES
GATE_FAIL=0
GATE_WARN=0

for f in "${APP_FILES[@]}"; do
    base="$(basename "$f" .c)"
    loc=$(code_LOC_careful "$f")
    calls=$(distinct_px_calls "$f")
    if [[ $calls -eq 0 ]]; then
        # No px_ calls -> not a Planex example; skip the verdict line
        ael="-"
        verdict="(skip) no px_ calls"
    else
        # AEL = code_LOC / distinct_px_calls (integer division ok for threshold checks)
        ael=$(awk -v loc="$loc" -v c="$calls" 'BEGIN { printf "%.1f", loc / c }')
        # Antipattern demos + hover_drag_4abs (hack-pain demo per ADR-0011)
        # are exempt from FAIL but still measured.
        if [[ "$base" == antipattern_* ]] || [[ "$base" == "hover_drag_4abs" ]]; then
            verdict="(exempt) by-design decompressed demo"
        elif awk -v a="$ael" 'BEGIN { exit !(a > 25.0) }'; then
            verdict="FAIL (catastrophic >25.0)"
            GATE_FAIL=$((GATE_FAIL + 1))
        elif awk -v a="$ael" 'BEGIN { exit !(a > 10.0) }'; then
            verdict="WARN (decompressing >10.0)"
            GATE_WARN=$((GATE_WARN + 1))
        else
            verdict="PASS (compressed)"
        fi
    fi
    SUMMARY_LINES+=("$base|$loc|$calls|$ael|$verdict")
    printf "%-40s %10s %12s %10s %s\n" "$base" "$loc" "$calls" "$ael" "$verdict"
done

# ---------------------------------------------------------------------------
# Compute aggregate LLE
# ---------------------------------------------------------------------------

LIB_LOC=0
APP_LOC=0
for f in "${LIB_FILES[@]}"; do
    LIB_LOC=$((LIB_LOC + $(code_LOC_careful "$f")))
done
for f in "${APP_FILES[@]}"; do
    APP_LOC=$((APP_LOC + $(code_LOC_careful "$f")))
done

if [[ $APP_LOC -eq 0 ]]; then
    LLE="-"
    lle_verdict="(skip) no app LOC"
else
    LLE=$(awk -v l="$LIB_LOC" -v a="$APP_LOC" 'BEGIN { printf "%.2f", l / a }')
    if awk -v l="$LLE" 'BEGIN { exit !(l < 0.3) }'; then
        lle_verdict="FAIL (catastrophic <0.3)"
        GATE_FAIL=$((GATE_FAIL + 1))
    elif awk -v l="$LLE" 'BEGIN { exit !(l < 1.0) }'; then
        lle_verdict="WARN (weak leverage <1.0)"
        GATE_WARN=$((GATE_WARN + 1))
    else
        lle_verdict="PASS (positive leverage)"
    fi
fi

echo
printf "%-40s %10s\n" "Abstraction layer LOC:" "$LIB_LOC"
printf "%-40s %10s\n" "Application layer LOC:" "$APP_LOC"
printf "%-40s %10s %s\n" "Aggregate LLE:" "$LLE" "$lle_verdict"
echo
echo "Summary: $GATE_FAIL FAIL, $GATE_WARN WARN"

# ---------------------------------------------------------------------------
# Exit code
# ---------------------------------------------------------------------------

if [[ $GATE_FAIL -gt 0 ]]; then
    echo
    echo "compression_metric: FAIL ($GATE_FAIL critical threshold violation(s))."
    echo "See docs/concepts/state/compression-metric.md for the metric definition."
    exit 1
fi

echo
echo "compression_metric: PASS (no critical threshold violated; $GATE_WARN warning(s) tracked but not blocked)."
exit 0
