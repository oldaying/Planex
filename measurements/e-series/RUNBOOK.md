# The E-Series Commissioning Runbook

> **Status: READY — NOT RUN.**
>
> Zero legs claimed run. Zero participants recruited. Zero data
> files collected. This runbook is the preparation work ADR-0027
> §7 registered and ADR-0028 Decision 7 delivered: the kit a
> maintainer commissions the E-series human legs FROM. The legs
> themselves require real participants, a consent process, and an
> environment this repository cannot and does not provide.
>
> The account above is load-bearing: F12 verifies it against the
> repository state at every commit (no `measurements/e-series/data/`
> directory exists, no results are claimed anywhere in this file),
> and the Regression Clause of ADR-0028 makes the check permanent
> — a successor may add legs' DATA when they are commissioned, but
> may never backfill claims of runs that did not happen.

## What is already in place (and where)

| Instrument | Location | State |
|---|---|---|
| The frozen instrument (two arms) | `slice/` (ADR-0025; merged, CI-green) | frozen — do not touch |
| The registered E2 engineering leg | `measurements/e2-engineering-leg/` | RUN (2.102×, R² = 1.000000, 48 B/change) |
| The E2 recompute script | `scripts/analyze_e2_residency.sh` | the convention a leg's analyzer follows: it lands WITH the data |
| The v5 presentation surface | `v5/client/` (ADR-0028; golden-tested) | ready — a participant-facing binary CAN be built on it when a leg is registered for v5; no such leg is registered today |
| The demotion clauses | ADR-0026 §5 (quoting vol. 6 ch. 3) | pre-committed; revisions may only tighten (vol. 6 §9.1) |
| This runbook | `measurements/e-series/RUNBOOK.md` | ready-not-run |

## The legs

Every entry below has the five fields a commissioner needs. All
content is DERIVED from the registered sources (vol. 6 ch. 3,
quoted in ADR-0026 §5; vol. 6 §7.2; vol. 2 table 3-2) — nothing
here is invented, and nothing here has been measured.

### E1a — single-intent share (the N1 strong-form leg)

- **Question:** do agent workflows show a single-intent absolute
  majority?
- **Instrument:** the frozen slice's processing arm, driven by
  participants on the registered scenario.
- **Primary metric / boundary:** the share of workflows
  decomposable to a single intent, against the registered
  majority boundary.
- **Demotion clause (verbatim, ADR-0026 §5):** "agent workflows
  show single-intent absolute majority → N1's strong form and the
  killer-domain choice fall together (vol. 6 §7.2's binding)."
- **Data schema:** `data/e1a.csv` — header
  `participant,session,workflow_id,step,step_kind,intent_count`,
  one row per workflow step; the analyzer recomputes the per-
  workflow majority from the raw steps (never from a summary).

### E1b — the audit family (global addresses)

- **Question:** are the world-level cut addresses (the registry)
  the granularity real audit questions need?
- **Instrument:** the frozen slice's processing arm; the audit
  tasks of vol. 6's protocol.
- **Primary metric / boundary:** task completion and error rates
  on registry-addressed audit questions, per the registered
  equivalence margins.
- **Demotion clause:** none of its own — E1b feeds the promotion
  gate's evidence base (ADR-0026 §5 preamble); its registered
  boundary is the protocol's three-level evidence rule.
- **Data schema:** `data/e1b.csv` — header
  `participant,task,task_kind,address_used,correct`,
  one row per audit task.

### E2 experimental — remote recovery (the N2 leg)

- **Question:** on the REMOTE recovery subset, is query-style undo
  TOST-equivalent to or worse than the stack button?
- **Instrument:** the frozen slice's two arms (the undo surfaces
  under comparison), participants on the registered recovery
  tasks.
- **Primary metric / boundary:** TOST equivalence bounds as
  registered in vol. 6 ch. 3 (the ± margin on the recovery
  metric); the engineering leg ALREADY PASSED at 2.102× ≤ 3.0 —
  this is the human leg the OR in vol. 2 table 3-2 gives equal
  force.
- **Demotion clause (verbatim):** "on the REMOTE recovery subset,
  query-style undo TOST-equivalent to or worse than the stack
  button → N2 demotes to scenario option."
- **Data schema:** `data/e2_experimental.csv` — header
  `participant,task,recovery_style,time_ms,errors`, one row per
  recovery task; the TOST is recomputed from raw pairs by the
  leg's analyzer.

### E3a — the tier-3 census (the N3 strong leg)

- **Question:** what share of regions across the registered
  domains actually live at tier 3?
- **Instrument:** implementation census over the registered
  domain set.
- **Primary metric / boundary:** the tier-3 share, against the
  registered ≥ 90% boundary.
- **Demotion clause (verbatim):** fires only JOINTLY with E3b —
  "E3a + E3b both: tier-3 share ≥ 90% across domains AND all
  three residual-value metrics < 1% → the three tiers collapse
  to one."
- **Data schema:** `data/e3a.csv` — header
  `domain,region_id,tier_observed`, one row per region.

### E3b — the residual-value metrics (the N3 companion leg)

- **Question:** what residual value do the non-strict tiers
  carry, on the three registered metrics?
- **Instrument:** the census's residual-value instruments (the
  three metrics as registered in vol. 6 ch. 3).
- **Primary metric / boundary:** all three residual-value
  metrics, against the registered < 1% boundaries.
- **Demotion clause (verbatim):** fires only JOINTLY with E3a —
  see the E3a entry; both must fire for the tiers to collapse.
- **Data schema:** `data/e3b.csv` — header
  `domain,metric,value`, one row per metric per domain.

### E3c — the implementer-cost leg (the merge algebra's)

- **Question:** what do implementers say the semilattice costs
  them?
- **Instrument:** the implementer-cost instrument (the
  registered acceptance scale).
- **Primary metric / boundary:** the semilattice-cost acceptance
  median, against the registered < 4 boundary.
- **Demotion clause (verbatim):** "E3c: implementer
  semilattice-cost acceptance median < 4 → the merge algebra
  demotes to implementation detail." (Independent of E3a/E3b.)
- **Data schema:** `data/e3c.csv` — header
  `implementer,acceptance_score` (1..7, the registered scale).

### E4 — budgeted attention (the N4 leg)

- **Question:** is budgeted attention management worse than free
  notifications on EITHER primary metric?
- **Instrument:** the frozen slice's budget arm against a
  notifications-unbudgeted configuration, participants on the
  registered interruption workload.
- **Primary metric / boundary:** throughput and satisfaction, the
  two registered primaries, with the registered equivalence
  margin.
- **Demotion clause (verbatim):** "budgeted attention management
  worse than free notifications on BOTH primary metrics
  (throughput and satisfaction) beyond the equivalence margin →
  N4 demotes to an optional constraint."
- **Data schema:** `data/e4.csv` — header
  `participant,condition,throughput,satisfaction_score`.

### E5a — the calibration correlation (the N5 leg)

- **Question:** does calibration performance correlate with
  ledger quality?
- **Instrument:** the calibration instrument across the
  registered product set.
- **Primary metric / boundary:** the calibration-performance
  rank correlation |ρ| 95% CI upper bound, against the
  registered 0.1 boundary.
- **Demotion clause (verbatim):** "E5a: calibration-performance
  rank correlation |ρ| 95% CI upper bound < 0.1 with consistent
  direction across products → N5 is a candidate for revocation."
- **Data schema:** `data/e5a.csv` — header
  `product,participant,calibration_rank,ledger_quality_rank`.

### E5b — the bookkeeping cost-benefit (the N5 companion leg)

- **Question:** does the ledger's bookkeeping cost exceed its
  governance benefit over a deployment window?
- **Instrument:** a deployment-window measurement of breaker
  events against bookkeeping costs (the receipts stream is the
  queryable surface — vol. 6 §7's design).
- **Primary metric / boundary:** the cost-benefit comparison
  over the registered window.
- **Demotion clause (verbatim):** "E5b: ledger bookkeeping cost
  exceeding governance benefit over the deployment window → N5
  demotes to the protocol's metric family."
- **Data schema:** `data/e5b.csv` — header
  `window_start,window_end,breaker_events,bookkeeping_cost,governance_benefit`
  (the units as registered in vol. 6 ch. 3).

## Commissioning procedure (the maintainer's half)

1. **Pick the leg.** Any entry above; the promotion gate
   (ADR-0026 §5) adjudicates on the full set, but each leg stands
   alone as a measurement.
2. **Register the run.** The leg's protocol (participants,
   environment, consent, exclusions) follows vol. 6 ch. 3's
   registered defaults; any deviation may only TIGHTEN (vol. 6
   §9.1's revision rule) and is recorded in the data file's
   header comment.
3. **Collect.** Raw rows only — the schemas above. No summaries
   in the data files; the analyzer recomputes everything (the
   `analyze_e2_residency.sh` convention: the recompute script
   lands in the same commit as the data).
4. **Analyze.** Write `scripts/analyze_e<leg>.sh` (or `.py`)
   alongside the data; it must recompute the primary metric from
   the raw rows and print the boundary comparison, not a verdict
   alone.
5. **Adjudicate.** If a demotion clause fires, the consequence is
   ADR-0026 §5's, verbatim: the promotion gate closes, R4+ stalls
   (per ADR-0028's inheritance of the same clause), and the raw
   data plus the demotion recommendation go to the maintainer.
   ADR-0014's lifecycle authority for adjudicating on evidence
   short of the list remains the maintainer's.

## What this kit deliberately does not provide

- **A participant-facing interactive layer on the slice.** The
  slice is the frozen instrument (ADR-0026 Decision 2); a leg
  needing interactive drive requires an instrument ADR of its
  own. The v5 client (`v5/client/`) is the surface a FUTURE,
  separately-registered v5-based leg could use — readiness, not
  an instrument.
- **Pre-written analyzers.** Their shape depends on data the legs
  do not yet have; pre-writing them would be predicting results.
- **Any claim about any human, anywhere.** The engineering
  numbers that exist (E2's 2.102×, V1–V9, M1–M9, F1–F12) are
  machine assertions about bytes and clocks; the human questions
  are exactly as open as they were before this kit existed.
