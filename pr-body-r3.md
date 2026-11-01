# ADR-0028: stage R3 — the presentation client, the named addresses, the snapshot, and the E-series kit (registered → implemented → measured)

## What this is

Stage R3 of the v5 derivation — the stage ADR-0027 §7 registered
and deliberately did not execute: "the presentation-and-persistence
stage." Two commits, two beats:

1. `7f875b2` — **ADR-0028 registered** (four-column compliant,
   predictions F1–F12 fixed before any `v5/client/` or
   `if5_snapshot.c` source existed)
2. `31b4479` — **stage R3 implemented and measured** (the
   Validation section is the landing runs' transcript, never
   asserted in advance)

## The decision, in one paragraph

S15's "vision is one projection" clause is executed as a CLIENT:
`v5/client/` sits OUTSIDE the visual-vocabulary ban's scan paths
BY CONSTRUCTION (the ban protects the ontology; the client is
where the ontology's projections meet a surface), and two new
gates keep it that way — the dependency-direction ban (the core
never references `if5c_`) and the client purity ban (the client
modules never submit). The cut registry gains the name column
the "64 named cuts" language always implied (the world grows by
exactly 768 bytes; the differential's working-set delta moves
12,568 → **13,336** with that as the registered reason). The
snapshot is a NEW canonical format — the log and arena VERBATIM,
seeds, receipts, the address book, the cursor — loaded by
verbatim install plus ONE rebuild pass that replays the
bookkeeping and never re-adjudicates (the records' flags, PROV,
and clocks ARE the decisions), with every rebuildable cache
cross-checked against the persisted values: a divergence is a
refused load, never a wrong world. The black-box export format —
a registered surface — is untouched (F7 asserts its bytes). And
the E-series commissioning kit lands **ready-not-run**: nine leg
entries, five fields each, zero runs, zero participants, zero
data — enforced against the repository state at every commit by
`scripts/check_e_series_runbook.sh`.

## What is NOT carried (the contract columns)

- The five presentation relation kinds (BESIDE, AFFORDS,
  WITHDRAWS_FOR, PRESENTS_FOR, INTERPRETS_AS) — not even as
  client metadata: the layout derives STRUCTURALLY from the
  tree. Asserted layout edges stay gone.
- Wall-clock frame timing: easing is a pure t-mapping over exact
  integer rationals — no floating point anywhere in the client.
- Annotation durability (session state by contract), fonts beyond
  the derived 5×7, sample-trajectory persistence (no measured
  need has appeared — the trigger stays registered).

## The measured numbers (F1–F12 all PASS)

| Prediction | Measured |
|---|---|
| F1 client purity | durable image byte-identical around every client call; submit counter +0; static bans clean |
| F2 determinism | byte-identical renders, in-process, re-render, and across save/load |
| F3 totality | one cell per region, tree order, depth-rows + pre-order-columns |
| F4 hit inversion | 200 randomized trees, 1,216 cells, 15,280 points, brute-force agreement |
| F5 easing | endpoints byte-exact, per-channel monotone, both families |
| F6 named addresses | id/address/name stable across the round trip; duplicates refused |
| F7 fidelity | full op alphabet round trip identical on every surface; the reloaded world CONTINUES identically |
| F8 canonicity | save-twice and re-save fixpoints hold |
| F9 rebuild | svc/vc_now/ledgers/caches/heads/stats/tick exact, field by field |
| F10 the V1 surface | differential PASS; slope 48 B/change; residency 1.661× ≤ 3.0; **delta 13,336 exactly** |
| F11 gates bite | both planted violations (core→client ref, client submit) caught |
| F12 the kit | nine legs, five fields, zero data — enforced in CI |

Implementation-era findings, all caught before landing and
registered in the ADR: (1) the ARBIT witness carries the
newcomer's path counters — the stats cross-check refused the
first draft's load with both totals printed; (2) a tier-1 fork
does not touch the working-set value cache (frozen slice
semantics — replicated, not repaired); (3) the tier-2 system
debit is flag-witnessed (CONCUR_MERGE proves it succeeded); (4)
standalone `if5_debit` successes are genuinely log-invisible —
the spent totals persist authoritatively and the loader refuses
when the visible portion exceeds them; (5) the attention margin's
first geometry was a diagonal staircase — the golden caught it.

## The honest account

Every F-prediction is a machine-verifiable engineering assertion.
Zero human-subject claims: the E-series legs remain
registered-not-run, the promotion gate remains vacant (ADR-0026
§5's clauses carry unchanged), and the runbook's ready-not-run
account is a CI-enforced fact, not a promise. The calculator demo
renders a scripted session (`78+6=` → `84`; one undo → `6`) —
readiness, not an instrument; the registered instrument stays the
frozen slice.

Regression battery at landing: root 39/39 under -Werror; slice +
v5 + client suites green; the differential PASS; ctest 21 → 27;
ASAN+UBSan clean everywhere (goldens byte-identical under
sanitizers); the new sources clean under -Wconversion
-Wsign-conversion -Wshadow; every shell gate green; fresh-clone
rehearsal green with exec bits intact.

Merge with **Rebase and merge** — the two beats are the discipline
itself and must stay distinct commits.
