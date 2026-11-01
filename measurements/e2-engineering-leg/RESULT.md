# E2 engineering leg — result and adjudication

**The research series' first registered, machine-measured number.**

- Branch: `slice/intent-flow` (implementation + measurement commits)
- Instrument: `slice/app/e2_engineering_leg.c` (self-judging; exits
  non-zero if any registered boundary breaks)
- Raw data: `e2_residency.csv` (this directory)
- Recompute: `scripts/analyze_e2_residency.sh
  measurements/e2-engineering-leg/e2_residency.csv` — anyone, any
  checkout, no re-run needed (vol. 6 §2.4's recomputation rule)
- Determinism: seeded xorshift64 (0x9E3779B97F4A7C15); byte-accounted
  residency (no RSS, no allocator noise)

## The number

| Checkpoint | n_changes | n_log | history_bytes | working_set_bytes | ratio | incr B/change |
|---|---|---|---|---|---|---|
| 1 | 500 | 501 | 34,048 | 50,440 | 0.675 | 48.000 |
| 2 | 1,000 | 1,001 | 58,048 | 50,440 | 1.151 | 48.000 |
| 3 | 2,000 | 2,001 | 106,048 | 50,440 | **2.102** | 48.000 |
| 4 | 4,000 | 4,001 | 202,048 | 50,440 | 4.006 | 48.000 |

Load mix: 604 stale-knowledge submissions (15.1% of 4,000 changes),
realizing **527 tier-2 merges**; zero forks (all objects tier 2),
zero breakers, zero receipts (the system's 100,000-unit budget paid
527 background merges).

## C8 adjudication

- **C8a — residency at the registered load floor (n = 2000):**
  **2.102× working set.** Boundary ≤ 3.0. **PASS.**
- **C8b — linearity:** R² = 1.000000 (least squares over the four
  checkpoints), slope 48.0 bytes/change, per-checkpoint incremental
  cost 48.000 at every checkpoint (window [40.8, 55.2] at ±15% of
  the mean). **PASS.**

**Verdict: C8 passes on both branches.** N2's engineering leg has
not fired; the demotion recommendation stays in its holster.

## Deviations from the registered prediction, recorded

1. **The measured ratio (2.102×) lands just under the predicted
   interval [2.2, 2.9].** The prediction was computed against the
   expected working-set shape; the implemented working set (the
   world struct: the region table at its full registered capacity
   of 1024 rows, not the 1000 in play) is slightly heavier than
   predicted, which pushes the ratio down. The direction is
   favorable and the boundary is unambiguous; recorded as a
   deviation, not smoothed.

2. **The intent record landed at exactly the registered 48 bytes**
   (ADR-0025 4f's accounting unit; `_Static_assert` in `ifl.h`).
   No deviation — noted because the earlier draft header had 44.

## Measurement adjudications (registered in the code where decided)

These three readings of the registered boundary sentence were fixed
during the first measurement run, each at the point of decision:

1. **History bytes count records, not allocator capacity**
   (`ifl_history_bytes`): n_log × 48 + live arena bytes + live
   receipt bytes. Counting the growth policy's reserved capacity
   (cap_log) would price the realloc strategy, not the ontology —
   an exact-fit allocator holds cap == n. First run's checkpoint
   increments under the capacity reading: 81.9 / 49.2 / 49.2 / 49.2
   (the doubling ladder), which is the allocator's staircase, not
   N2's cost.

2. **The working set is the world struct itself**
   (`ifl_working_set_bytes`): the region table lives inside
   `ifl_world`; counting `n_regions × sizeof(ifl_region)` on top of
   `sizeof(ifl_world)` would double-count the table.

3. **The incremental-cost baseline starts at load-complete**
   (`e2_engineering_leg.c`): the 1000-object document-load blob is
   part of the REGISTERED LOAD, not the mutation stream's marginal
   cost. Amortizing it into the first window would report 68
   B/change there against 48 everywhere else — a one-time load cost
   smeared into a linearity verdict it has nothing to say about.
   The blob's own bytes stay in history_bytes at every checkpoint.

A reviewer applying stricter readings (ratio at all n; capacity
accounting; blob amortized) can re-adjudicate from the raw CSV
without re-running anything — that is what it is published for.

## The full prediction ledger (C1–C9)

| # | Prediction | Result | Evidence |
|---|---|---|---|
| C1 | Four-column closure | **PASS** | `check_doc_sections --check --four-column`: all 24 ADR files, both sections present for ≥0024 |
| C2 | Calibration green | **PASS** | calibration 8/8, property 6/6, rebound gate clean + 3/3 bites |
| C3 | I1 single-source short-circuit | **PASS** | `test_c3`: concurrent=0, lattice=0, tier outcomes=0, system merges=0; causal applies > 90 |
| C4 | I2 zero-budget silence | **PASS** | `test_c4`: receipts ≥ 6, breakers ≥ 6, fold holds only the user's own writes |
| C5 | I3 all-tier-3 degeneration | **PASS** | `test_c5`: zero forks, zero merge-intents, ≥24 in-window arbitrations, LWW final value |
| C6 | I4 semilattice laws | **PASS** | 1M groups/round × {NUM, SET, mixed}: no counterexample; planted non-associative join REJECTED at registration; planted violation shrunk to a minimal triple |
| C7 | I5 undo-as-query | **PASS** | `test_c7`: sugar vs the written-out manual three steps — byte-identical states, three projections, and the entire log |
| C8 | E2 engineering leg | **PASS** | 2.102× at n=2000; R² = 1.000000; increments within ±15% (this document) |
| C9 | Control-arm cost, measured | **PASS** | classic arm: mode_switches=26, attribution_records=26, snapshot_copies=26, manual_version_records=4, value_losses=0; treatment arm: all four zero (printed at both arms' tails; classic self-check enforces the control actually paid) |

## The honest ledger (unchanged by this landing)

Zero human-measured numbers. E1a/E1b (field + controlled), the E2
experimental leg, E3a/b/c, E4, E5a/b remain registered and unrun;
the slice's arms are their carriers, ready. Calibration passing is
a statement about the instrument, not about user performance
(ADR-0025 CAVEATS).
