#!/usr/bin/env bash
#
# analyze_e2_residency.sh — recompute the E2 engineering leg's
# published ratios from the raw CSV (vol. 6 §2.4's recomputation
# rule: the published numbers must be recomputable by anyone, from
# the raw data alone, without re-running the instrument).
#
# Verifies the two registered boundaries (ADR-0025 Decision 4f,
# Reading B):
#   C8a: residency <= 3.0 x working set at n = 2000
#   C8b: linear fit R^2 >= 0.99 across all checkpoints, and
#        per-checkpoint incremental bytes-per-change within +/-15%
#        of the session mean
#
# Usage: scripts/analyze_e2_residency.sh measurements/e2-engineering-leg/e2_residency.csv
set -eu

if [ $# -ne 1 ]; then
    echo "usage: $0 <e2_residency.csv>" >&2
    exit 2
fi
CSV="$1"
if [ ! -f "$CSV" ]; then
    echo "no such file: $CSV" >&2
    exit 2
fi

awk -F, '
FNR == 1 && $1 ~ /^#/ { next }          # leading comment line(s)
!hdr_done && $1 == "checkpoint" {
    for (i = 1; i <= NF; i++) hdr[$i] = i
    hdr_done = 1
    next
}
!hdr_done { next }                      # anything before the header
{
    n_changes = $(hdr["n_changes"])
    hist      = $(hdr["history_bytes"])
    work      = $(hdr["working_set_bytes"])
    incr      = $(hdr["incr_bytes_per_change"])
    rows++
    cn[rows] = n_changes; ch[rows] = hist; cw[rows] = work; ci[rows] = incr
    if (n_changes == 2000) r2000 = hist / work
    sx += n_changes; sy += hist; sxx += n_changes * n_changes; sxy += n_changes * hist
    si += incr
}
END {
    if (rows < 2) { print "not enough rows"; exit 1 }

    # least-squares fit of history_bytes on n_changes
    slope = (rows * sxy - sx * sy) / (rows * sxx - sx * sx)
    intercept = (sy - slope * sx) / rows
    for (i = 1; i <= rows; i++) {
        fit = slope * cn[i] + intercept
        sst += (ch[i] - sy / rows) ^ 2
        sse += (ch[i] - fit) ^ 2
    }
    r2 = sst > 0 ? 1.0 - sse / sst : 0.0

    mean_incr = si / rows
    lo = mean_incr * 0.85; hi = mean_incr * 1.15
    incr_ok = 1
    for (i = 1; i <= rows; i++)
        if (ci[i] < lo || ci[i] > hi) incr_ok = 0

    printf "recomputed from %s (%d checkpoints)\n", FILENAME, rows
    printf "  residency at n=2000 : %.3f x working set  (boundary <= 3.0)\n", r2000
    printf "  linear fit R^2       : %.6f             (boundary >= 0.99)\n", r2
    printf "  slope                : %.1f bytes/change\n", slope
    printf "  incremental cost     : mean %.1f, window [%.1f, %.1f] %s\n", \
           mean_incr, lo, hi, incr_ok ? "(within +/-15%)" : "(OUTSIDE)"

    verdict = "PASS"
    if (r2000 == 0 || r2000 > 3.0)   verdict = "FAIL"
    if (r2 < 0.99)                   verdict = "FAIL"
    if (!incr_ok)                    verdict = "FAIL"
    printf "  verdict              : %s\n", verdict
    exit verdict == "PASS" ? 0 : 1
}
' "$CSV"
