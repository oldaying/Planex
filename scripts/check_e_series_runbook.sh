#!/usr/bin/env bash
# check_e_series_runbook.sh — F12's enforcement (ADR-0028): the
# E-series commissioning kit's account must match the repository
# state, at every commit, forever (the Regression Clause made
# this check permanent).
#
# What it verifies:
#   1. The runbook exists and declares READY — NOT RUN.
#   2. All nine leg entries exist (E1a, E1b, E2-experimental,
#      E3a, E3b, E3c, E4, E5a, E5b), each with all five fields
#      (question, instrument, primary metric, demotion clause,
#      data schema).
#   3. The account is TRUE: no data directory exists, and no data
#      file is claimed anywhere under measurements/e-series/.
#
# Exit codes: 0 = the account holds; 1 = drift (CI fail).
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RUNBOOK="$REPO_ROOT/measurements/e-series/RUNBOOK.md"
DATA_DIR="$REPO_ROOT/measurements/e-series/data"

fail=0

if [ ! -f "$RUNBOOK" ]; then
    echo "e-series runbook: MISSING — $RUNBOOK"
    exit 1
fi

# 1. the status line
if ! grep -q 'Status: READY — NOT RUN' "$RUNBOOK"; then
    echo "e-series runbook: the status line no longer declares READY — NOT RUN."
    echo "  If legs have genuinely been commissioned, update the account AND"
    echo "  this check in the same commit (the Regression Clause's rule)."
    fail=1
fi

# 2. the nine leg entries, each with the five fields
legs="E1a E1b E3a E3b E3c E4 E5a E5b"
missing=""
for leg in $legs; do
    if ! grep -q "^### $leg " "$RUNBOOK"; then
        missing="$missing $leg"
    fi
done
# E2's experimental leg shares a heading with the family name
if ! grep -q '^### E2 experimental' "$RUNBOOK"; then
    missing="$missing E2-experimental"
fi
if [ -n "$missing" ]; then
    echo "e-series runbook: missing leg entries:$missing"
    fail=1
fi

for field in 'Question:' 'Instrument:' 'Primary metric' 'Demotion clause' 'Data schema'; do
    n=$(grep -c "\*\*${field}" "$RUNBOOK" || true)
    if [ "$n" -lt 9 ]; then
        echo "e-series runbook: field '$field' appears $n times (need >= 9 — one per leg)"
        fail=1
    fi
done

# 3. the account is true: zero data
if [ -e "$DATA_DIR" ]; then
    echo "e-series runbook: data exists at $DATA_DIR —"
    echo "  the runbook still claims zero data files. Update BOTH in one commit:"
    echo "  the account line, the status, and this check's expectation."
    fail=1
fi
if find "$REPO_ROOT/measurements/e-series" -name '*.csv' | grep -q .; then
    echo "e-series runbook: CSV files exist under measurements/e-series/"
    fail=1
fi

if [ "$fail" -ne 0 ]; then
    echo "e-series account: DRIFT — the kit's account and the repository disagree"
    exit 1
fi
echo "e-series account: holds (9 legs, 5 fields each, zero runs, zero data — F12)"
exit 0
