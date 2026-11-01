<!--
Document freshness — every ADR carries this metadata block (research basis:
Software Engineering at Google Ch. 10 "Documentation", the freshness:{owner,
reviewed} pattern). CI (planned: see doc-organization.md Part IX, Principle 13)
flags ADRs whose reviewed: date is older than 12 months for re-review.
-->
<!--
freshness: { owner: "oldaying", reviewed: "2026-11-01" }
-->

# ADR-0025: The intent-flow vertical slice — Program I's falsification instrument

## Status of This Memo

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: 2026-11-01

## When to use this decision

This decision applies to the `slice/` tree and everything under it: the
intent-flow core (`ifl_*`), the two arms (`slice_app`, `classic_arm`), the
calibration suites, the E2 engineering-leg harness, and the rebound-signal
gate. It governs how the vertical slice mandated by the 2026-10/11 research
series (vols. 1–7, this repository's ADR-0023/0024 being its in-repo
instruments) is to be built, calibrated, and measured. It also fixes the
operationalization of the E2 engineering-leg boundaries for as long as
this ADR stands.

## When NOT to use this decision

This decision does not touch the shipping API (`include/planex/`,
`src/`), does not promote or demote any of the 7 canonical abstractions,
and does not commit the repository to the intent-flow ontology as a
shipping direction — the slice is an *instrument*, not a product and not
a proposal (the "instrument, not demo" clause in Decision §4). Nothing
here authorizes Phase 4 (full rewrite); that is a separate decision with
its own ADR. The E-series experiments involving human subjects (E1b, E2
experimental leg, E4, E5a) are *registered* here but *run* only under
their own protocol; this ADR's landing measurements cover the calibration
layer and the E2 engineering leg only.

## Status

**Validated.** Date: 2026-11-01.

The enforcement this ADR ships is implemented and proven to bite:
`make -C slice rebound-check` runs `scripts/check_slice_rebound.sh`
and its synthetic-bite demonstration (the gate's own three
toothless forms were found and fixed during registration, each
registered in the script; planted violations are caught), and the
property registry rejects a synthetic non-associative `sq` — the
checker must bite, and does. C1–C9 are all PASS with the measured
outcome transcribed in `## Validation`. Promotion to **Accepted**
remains a maintainer ruling through REVIEW-RUBRIC.md; the E-series
human legs this ADR registers stay ready-not-run, and nothing in
this validation claims otherwise.

## Context

The research series (six theory volumes delivered 2026-10 through
2026-11, plus the rehabilitation volume) fixed a five-step sequence for
its final act: semantics draft → evaluation protocol → rehabilitation
book → **vertical slice** → (gated) full rewrite. The first three are
done. Volume 6 (*Falsifiable Evaluation Protocol*) registered five
experiment groups (E1–E5), eleven seed experiments, and seven invariant
test designs — with the honest count "zero measured numbers", because
registration must precede measurement (Nosek et al. 2018; the
preregistration discipline this repository already practiced once at
probe scale in [ADR-0023](../proposed/ADR-0023-semantic-drift-probe.md)).

Volume 6 chapter 6 then specified the instrument itself. The slice's
recurring one-line spec: **one interface, one person, two agents, three
intent flows, time-addressable undo** — Program I's falsification
instrument. Three things were handed forward:

1. **Why three sources** (vol. 6 §6.1): three is the smallest N that
   makes three structural phenomena *measurable at once*. With two
   sources, associativity of merge is trivial (only one bracketing),
   every disagreement is a 1:1 deadlock needing external arbitration,
   and no delegation chain of length two exists. With three, I4's
   semilattice laws become violable equations, 2:1 coalitions become a
   real question for tier decisions, and a person→agent→sub-agent
   delegation chain exercises nested receipts (T7) and sub-budgets.
   "Two sources demo; three sources falsify."
2. **Calibration before measurement** (vol. 6 §6.2): the slice's first
   job is not to run E-series but to pass the calibration layer —
   I1–I7, the rebound signals (within vol. 6 §4.3's reachability
   limits), and differential testing. An uncalibrated instrument's
   numbers are invalid by discipline.
3. **The control arm** (vol. 6 §6.3): same language, same engineering
   discipline, same task set as the treatment arm; the *only*
   difference is the ontology. The registered carrier is the frozen
   Planex shipping body itself — `px_interaction`'s three bridges,
   `px_undo_record`'s stack undo, the single event stream — which has
   passed 12 CI gates, 77+ commits of audit culture, and a probe
   campaign. If the new ontology beats its own former self in a fair
   fight, the win cannot be attributed to a weak control; if it loses,
   not to a strong one. The control arm is frozen at the branch point
   of this slice (main@8229080, v0.7.0) and must not evolve during
   measurement.

The rehabilitation volume (vol. 7 §8) handed the slice three more
obligations: the **semantic-tree-first projection discipline** (the
slice's primary projection is the semantic structure; visual, a11y, and
generative projections are its siblings, none of them privileged
*above* it), the control-arm design principle above, and the **S5
loading question** — "example-as-spec" was seated at the consumable
layer's creation interface with loading deferred to the slice's design
round, because loading it requires the slice's generative loop.

One lineage from the semantics draft rides along in the budget algebra
(vol. 5 §7.4): the pricing spectrum that grades system initiative is
the readiness/rupture phenomenology the series verified at the D5
stratum — tools recede when ready-to-hand and rupture into presence
when they demand attention, Heidegger's Zuhandenheit/Vorhandenheit as
routed through Dourish's *Where the Action Is* (MIT Press, 2001). The
slice's debit ladder (Decision §4e) is that spectrum priced in
integers; the pricing *values* are E4's to measure, the monotonicity
is the slice's to keep.

Finally, volume 6 §9.1 registered the slice's own falsification
condition: if the slice lands without referencing the experiment
registry — testing unregistered metrics, or substituting demonstration
for measurement — the sequence fails and the slice is "an instrument
without reading rules". This ADR is where that reference is made
binding (Decision §3).

The axioms under test, with their third-column refutation conditions
translated from the constitution volume (tables 3-1..3-4) and the
semantics draft (§8.1), are quoted verbatim in Decision §2 below. The
parameter N=3, the tier set, the budget algebra, the seven invariants
and the seven-slot intent record are all specified by the semantics
draft (vol. 5) — this ADR operationalizes, it does not re-design.

## Decision

Build the slice as a new self-contained tree `slice/` in this
repository, on a dedicated branch, in three temporally ordered commits:
**registration** (this ADR, with predictions fixed before any slice code
exists on the branch), **implementation** (core + arms + calibration
suites + gates + wiring), and **measurement** (recorded data +
adjudication). The one-paragraph shape:

### 1. What gets built

```
slice/
  include/planex/ifl.h     the intent-flow core: sources (6 fields),
                           intents (7 slots), vector clocks, append-only
                           log, causal slices, state@t projections,
                           regions + three tiers, merge adjudication,
                           budget ledger, delegation receipts, black box
  src/ifl_core.c           log, clocks, slices, projections, PROV links
  src/ifl_merge.c          the four-step adjudication flow, payload-type
                           registry (open extension channel), conflicts,
                           forks, merge-intents, tier dial
  src/ifl_budget.c         ledger, debit classes, monotone pricing,
                           circuit breaker, sub-budgets, receipts
  app/scenario.h           THE deterministic task script (TRIP-17),
                           shared by both arms — same ops, two encodings
  app/slice_app.c          treatment arm: the slice proper
  app/classic_arm.c        control arm: frozen px_* body, honest-best
                           classical encoding of the same script
  app/e2_engineering_leg.c the E2 harness: 1000 objects, checkpointed
                           residency measurement, self-judging
  tests/test_slice_calibration.c   I1,I2,I3,I5,I6,I7 + scenario walk
  tests/test_slice_property.c      I4: 1,000,000 random groups/round,
                           counterexample shrinking, seeded
scripts/check_slice_rebound.sh    rebound signal-1 static gate + a
                                  synthetic-bite demonstration
measurements/e2-engineering-leg/  raw data, recomputation script, result
```

Placement follows the `v4/` precedent: a parallel ontology tree with its
own namespace (`ifl_`), its own Makefile, wired into CMake as a separate
`planex_slice_lib` that is **not** linked into `planex_lib` — except the
control arm, which deliberately links the frozen shipping library
(`classic_arm` → `planex_lib`), because the frozen body *is* the control.

### 2. The axioms under test (third columns, quoted)

- **N1 (intent flow)**: "If cross-domain field measurement of agent
  workflows shows single-intent delegation in an overwhelming majority,
  stable over time, AND controlled comparison shows explicit multi-flow
  modeling is not superior to implicit handling, the strong form is
  falsified and demotes to the weak form." (vol. 2 table 3-1,
  translated)
- **N2 (time coordinate)**: "If, under controlled protocol, retroactive
  query-based undo is not superior to stack undo on user performance, OR
  its memory residency cost curve is unacceptable in typical
  applications, the axiom demotes to a scenario option." (vol. 2 table
  3-2, translated)
- **N3 (divergence)**: "If measured multi-intent workflows show
  disagreement almost always auto-resolves and explicit divergence
  structure has no residual value, the three tiers collapse into
  single-tier auto-merge." (vol. 2 table 3-3, translated)
- **N4 (budget)**: "If, in controlled experiment, budgeted attention
  management is not superior to free notification on BOTH task
  throughput and satisfaction, the budget demotes to an optional
  constraint, off by default." (vol. 2 table 3-4, translated)
- **N5 (priors)**: calibration-performance rank correlation with 95% CI
  upper bound below 0.1 across three products (E5a), or a surprise
  ledger whose breaker never fires and calendar is never referenced
  while engineering cost exceeds 5% (E5b) — either retires N5 from the
  axiom table. (vol. 5 §8.1, translated)

### 3. The experiment registry as acceptance list (the vol. 6 §9.2 handoff)

| Leg | Registered boundary (vol. 6) | Slice carrier |
|---|---|---|
| E1a (field) | ≥90% single-intent share, per-domain CI lower bound ≥85%, 3 domains, ≥4 weeks | not run on the slice; the intent record's delegation/PROV fields *are* the logging schema |
| E1b (controlled) | attribution / interruption-recovery / audit task family; within-subject, ≥12/arm, d=0.2 | `slice_app` vs `classic_arm` over the shared `scenario.h`; all three task shapes present (PROV attribution walk, state@t restore, range audit) |
| E2 experimental | remote-restore latency + error rate, query-vs-button | the undo surface: `ifl_undo_nth` sugar over slice redefinition vs `px_undo` stack — both arms carry both distances (near/far) in their transcripts |
| **E2 engineering** | 1000 mutable objects, ≥2000 changes, residency ≤ 3× working set, linear growth | `e2_engineering_leg.c` — **run at landing**; the series' first registered number |
| E3a/b (field) | tier shares; residual-value rates all <1% | all three tiers exercised and countable (`ifl_stats`); tier dial user-visible (S14) |
| E3c | semilattice design acceptance, n≥8 implementers, median ≥4 on 5-point | every registered lattice (NUM-max, SET-union, and the user-channel types) has its `sq`/`le` design in reviewable code + this ADR |
| E4 (controlled) | throughput AND satisfaction both-worse to demote | budgeted vs free configurations are ledger parameters (finite `initiative_budget` vs unlimited); both buildable, breaker and debits observable |
| E5a (field) | Brier-scored calibration vs performance, \|ρ\| CI upper <0.1 | not run on the slice; the surprise ledger + prediction hooks are the instrumentation points |
| E5b (deployment) | 8 weeks; breaker triggers ≥1 or calendar referenced ≥5%; cost ≤5% | surprise-debit records, breaker state, and receipts are the measurement points, all queryable |

Seeds (vol. 6 ch. 5): S21, T3, T6, T2 and the other field legs run on
existing products, not on the slice; T5's three-arm design reuses this
slice's `classic_arm` pattern (arm 3 = direct manipulation as the honest
classical optimum). S24's artifact-consistency leg needs the generative
loop — see CAVEATS.

### 4. Registered design decisions (operationalizations)

These interpret the volumes where they left freedom; each is binding on
the implementation commit, and each may only be revised by a successor
ADR under the stricter-only rule.

**(a) The scenario.** TRIP-17: one shared trip-plan interface with four
regions — `budget` (bounded int, join = max, tier 2), `stops` (ordered
inserts, no lattice, tier 1 default), `tags` (grow-only set, join =
union, tier 2), `title` (single line, tier 3). One person (H), agent A
delegated by H ("book the trip", sub-budget 40), agent B sub-delegated
by A ("verify prices", sub-budget 15) — delegation chain length 2, the
vol. 6 §6.1 minimum. The script (~50 ops) contains: a concurrent
same-region edit pair that merges under tier 2; a concurrent
`stops` pair that hard-forks under tier 1 and is later stitched by an
explicit merge-intent; a tier-3 concurrent pair arbitrated
last-writer-wins; a sub-budget overdraft that trips B's breaker and
propagates receipts up the chain; a surprise-class debit (unexpected
price change); one tier-dial switch (S14); one black-box window
configuration and export (S3); state@t queries at three addresses; one
undo by slice redefinition; and one branch from an earlier address.

**(b) Payload openness (B2).** The core ships an *open* payload-type
registry: a type is `(value_size, sq?, le?)`; `sq` absent means no
join exists and the type is rigid. Built-ins NUM (u32, max) and SET
(u64 tag union) register through the same channel as user types; the
scenario registers its remaining types (SEQ insert, STR line) through
the user channel to exercise the extension path. Rigid types are
first-class — "no lattice" is an answer, not an error (vol. 5 §3.3).
The slice's inline payload discipline is 8 bytes; larger user values
are out of the slice's registered scope.

**(c) Tier semantics (N3).** Tier is per-region, runtime, and every
tier state — including the default at region creation — is set by a
logged, sourced, undoable tier intent (constraint D1: no silent
defaults). Tier 1 (fork): both versions live as per-region version
chains; a merge-intent (itself a full adjudication-flow intent) stitches
them with a declared policy. Tier 2 (tolerant merge): the registered
`sq` applies, PROV records dual derivation, and the *system* is debited
a background-rate initiative item for executing the merge (vol. 5 §4.2
step 4). Tier 3 (strict convergence): an arbiter imposes the registered
total order (t, then sid, then iid) and the loser is *excluded from the
fold by the arbitration record*. Interpretation note, registered here:
vol. 5 §6.1's "only one of the two versions exists in the log" is
implemented as fold-level exclusion — the loser stays in the append-only
submission log and the black box (deleting it would weaken I6's
provenance completeness; retention is the stricter reading), while no
projection ever folds it.

**(d) Undo is a query (N2).** There is no undo primitive. The API
exposes causal slices and `ifl_state_at`; `ifl_undo_nth` is app-level
sugar that (1) computes the rewound slice, (2) emits a CURSOR intent
carrying who moved "now" and why (I6: every visible change has a
source), and (3) re-projects. CURSOR intents address the view; the fold
skips them. Dual address (S9, vol. 5 §5.3) is carried at type level:
`ifl_project_history` (strict causal expansion) and `ifl_project_set`
(semantic grouping, order-free) are two projection families over one
log. Branching = selecting a different successor set; new intents after
a rewind extend the rewound point while the abandoned branch stays
addressable.

**(e) The budget algebra (N4).** One ledger per source; debit classes
INITIATIVE and SURPRISE share one currency. The registered slice pricing
table (S16 spectrum, monotone by construction and by test):
background-merge 1, ambient-notice 2, foreground-proposal 4,
modal-interrupt 8. Debits check balance before posting; overdraft trips
the breaker *instead of* the action (failure and budget breach are the
same class of event, S1's algebra). Delegation opens a sub-ledger whose
ceiling comes from the delegator's remaining balance; sub-ledger
overdraft produces a receipt — origin, reason (PROV activity ref),
itemized cost, rollback address (a causal slice), checkpoint — and
propagates up the chain as a receipt, never as a silent failure. A
passive system is the all-zero budget configuration (I2), not a mode.

**(f) The E2 engineering leg, operationalized.** Registered load: 1000
mutable objects, each a 40-byte row (24-byte label, u32 value, u16 seq,
u16 flags, u32 last-intent ref); mutations are NUM and SET payloads,
mixed 85% causal / 15% concurrent pairs, from 3 sources plus the system
source. History residency is measured by instrumented byte accounting
(the sum of all bytes retained solely because of N2: intent records at
48 bytes, the iid index, fork/merge/arbitration records, PROV links) —
not by RSS, so the number is deterministic and recomputable. Working
set is the live state: 1000 rows plus the region table. Checkpoints at
n = 500, 1000, 2000, 4000 changes.

  The boundary sentence "residency ≤ 3× working set" admits two
  readings and this ADR binds the fair one. *Reading A* (ratio ≤3 at
  every n) is a rigged execution: residency grows linearly in changes
  while the working set is fixed, so any full-retention log fails past
  n ≈ 3·(working/rec) — the axiom would die by arithmetic, not by
  measurement, and vol. 6's own Photoshop precedent (20 full snapshots
  ≈ 20× state, affordable in 1998) shows 3× was never meant as an
  at-all-n bound. *Reading B* (bound evaluated at the registered load
  floor, n = 2000, with linearity guarding the curve shape beyond) is
  registered here: **ratio ≤ 3.0 at n = 2000; and across all
  checkpoints, a linear fit with R² ≥ 0.99 and per-checkpoint
  incremental bytes-per-change within ±15% of the session mean.** The
  rolling window (S3) is a deployment parameter and is explicitly
  *disabled* during measurement — flattening the curve to pass a
  linearity check would be boundary smuggling in the other direction.
  Raw bytes at every checkpoint are published with the result so any
  stricter reading can be recomputed by a reviewer.

**(g) Semantic-tree-first (vol. 7 handoff).** The slice's one interface
is the query surface over the semantic projection; the printed
transcript *is* the interface. Visual rendering is not merely absent —
it is registered as a same-rank future projection (with a11y and
generative), none privileged above the semantic tree. This also follows
from the rehabilitation volume's A7 verdict: nothing in the new ontology
mandates a raster pipeline.

**(h) S5 remains unloaded, by condition.** The slice's registered main
line (vol. 5 §11.3) carries S3 and S14 only. S5 ("example-as-spec" at
the consumable layer's creation interface) requires the generative
loop, which the main line does not include; loading it is therefore
deferred past this slice to the Phase-4 decision round, with its seat
and condition recorded here so the handoff cannot be silently dropped.

### 5. Predictions (registered before implementation)

Commitment: this section is committed before any `slice/` source file
exists on the branch. Each is falsifiable by the slice's own output;
adjudication is recorded in `## Validation` and in
`measurements/e2-engineering-leg/RESULT.md` at the measurement commit.

- **C1 — four-column closure.** This ADR passes
  `scripts/check_doc_sections.sh --check --four-column`. *Falsified if:*
  the gate exits non-zero on the branch head.
- **C2 — calibration green.** The calibration layer (I1, I2, I3, I5,
  I6, I7 suites; the rebound signal-1 static gate; the differential
  test) all pass at the implementation commit. *Falsified if:* any
  invariant test fails or the static gate finds a special-case branch —
  which would be cluster-rebound evidence, the heaviest possible
  outcome, to be recorded as such, not patched around.
- **C3 — I1 short-circuit.** A single-source instantiation of the full
  scenario shows, via `ifl_stats`: concurrent pairs = 0, lattice
  checks = 0, tier adjudications = 0, system-executed merges = 0. The
  flow exits at step 1 everywhere. *Falsified if:* any counter is
  non-zero (the flow "thinks about" merging in a world with one
  source).
- **C4 — I2 silence.** With every source's initiative budget set to
  zero: zero unaccounted world changes, zero passive-mode branches (the
  static gate), and every agent initiative resolves to a receipt. The
  user's own intents still apply. *Falsified if:* any unbudgeted change
  or any `passive` flag exists anywhere in the core.
- **C5 — I3 degeneration.** With every region switched to tier 3 via
  tier intents (runtime configuration, not a compile flag): zero fork
  nodes, zero merge-intents, all concurrent pairs resolved in-window,
  and the run is behaviorally indistinguishable from a classic
  single-truth collaborator. *Falsified if:* any fork residue appears.
- **C6 — I4 semilattice laws.** 1,000,000 random concurrent groups per
  round over every registered lattice type: associativity,
  commutativity, idempotence hold; any counterexample shrinks to a
  minimal form; a synthetic non-associative `sq` offered to the
  registry is *rejected* (the checker must bite). *Falsified if:* any
  counterexample survives — which pre-triggers E3c (vol. 6 table 4-1).
- **C7 — I5 undo-as-query.** For every undo path in the scripted
  scenario, the sugar path and the pure-projection rewrite
  (slice + `ifl_state_at` + re-project, no sugar) produce
  byte-identical transcripts, world states, and log side-effects.
  *Falsified if:* any byte differs.
- **C8 — E2 engineering leg.** At n = 2000: residency ≤ 3.0× working
  set (predicted interval 2.2–2.9× under the registered shapes);
  across checkpoints 500–4000: R² ≥ 0.99 and incremental cost within
  ±15% of mean. *Falsified if:* either boundary breaks — in which case
  the honest consequence is recorded: N2's engineering leg has fired
  and the demotion recommendation (axiom → scenario option, vol. 2
  table 3-2) goes to the maintainer with the data.
- **C9 — control-arm cost, measured not judged.** The classical control
  arm, encoding the same script honestly-optimally on the frozen body,
  pays measurable machinery the treatment arm does not: a mode machine
  consulted per handler entry, a hand-maintained attribution side-list
  (≥1 record per world change), snapshot copies for time-addressed
  queries, and either value loss or manual version bookkeeping at the
  fork beat. The arms print the counts; the calibration suite locks
  them. (This is ADR-0023's P5 role, promoted to a standing
  comparison.)

### 6. Wiring

`slice/Makefile` (self-contained, `CFLAGS_EXTRA` passthrough, targets:
`all`, `apps`, `test`, `rebound-check`, `clean`); root `CMakeLists.txt`
gains `planex_slice_lib` + the two arms + the three test/harness
executables + ctest entries (`slice_calibration`, `slice_property`,
`e2_engineering_leg` — the E2 harness is a *self-judging* test so the
registered boundaries re-verify on every clean checkout, per vol. 6
§2.4's recomputation rule); `ci.yml` runs the two suites + the E2
harness in all four build jobs; `scripts/check_slice_rebound.sh` runs
in the make-based jobs via `make -C slice rebound-check`. The analysis
script for E2 lives in `scripts/` per vol. 6 §2.4; raw data lives in
`measurements/`.

## Essence Check

### Q1. Which essence axis does this decision affect?

- [x] **Intent space** — the slice's whole subject: N sources of intent
  as peers (N1), delegation as a typed, budgeted, receipted chain.
- [x] **State space** — state as a projection of an append-only intent
  log with a time coordinate (N2), divergence as tiered structure (N3).
- [x] **Semantic interface** — the budget algebra (N4) prices the
  system→human direction; the semantic-tree-first projection discipline
  (vol. 7) fixes what the interface *is* when rendering is not
  privileged.
- [ ] None.

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions:** attribution, interruption recovery, and audit become
*queries* (fold + PROV walk) instead of three hand-built features; one
delegation record replaces a protocol of side-channels; undo, history
panel, and version tree collapse into one slice mechanism (I5's whole
content); the tier dial makes a previously invisible global default an
addressable, reversible decision.

**Increases:** every world change now carries a source, a clock, a
budget line — the per-intent record is larger than a bare event, and
the E2 engineering leg exists precisely to price this honestly; the
tier decision itself is new cognitive surface the classical model
hides inside handler structure.

**Net assessment:** the compression is conditional on measurements not
yet taken — that is the point of building the instrument.

### Q3. Is there a gap between the claim and the implementation?

- **Claim:** the axioms N1–N5 are *measurable* — every third-column
  clause reduces to a number someone else can recompute.
- **Implementation:** this slice delivers the carriers and the one
  fully-mechanical leg (E2 engineering). The human-subject legs (E1b,
  E2 experimental, E4) are registered with arms ready and unrun; the
  field legs (E1a, E3a/b, E5a/b) need deployments this repository
  cannot conjure.
- **Gap:** the gap is the series' honest ledger — zero human-measured
  numbers at landing, one machine-measured number (E2 engineering), and
  no claim that calibration-passing says anything about user
  performance. The rebound suite is the only claim made *now*: that the
  new ontology can express its own degenerations without special
  cases.

### Q4. What is the cost, and who can verify it?

- **Cost:** ~2.5k lines of C17 in a parallel tree, one gate script, CI
  minutes in four jobs, and the permanent obligation that the frozen
  control arm must not drift during any future measurement campaign.
- **Who can verify:** anyone. `make -C slice test` runs the calibration
  and property suites; `make -C slice rebound-check` runs the static
  gate and its synthetic-bite demo; `./build/e2_engineering_leg`
  regenerates the E2 numbers and re-judges the boundaries;
  `scripts/analyze_e2_residency.sh measurements/e2-engineering-leg/e2_residency.csv`
  recomputes the published ratios from raw data.

### Q5. What are the counterexamples?

- **A single-user, no-agent document editor.** N=1 forever. The slice's
  machinery is pure overhead there — which is exactly why I1 demands
  the flow short-circuit at step 1 and the rebound gate forbids
  special-casing the case instead.
- **A hard-realtime render loop.** 16ms frame budgets do not tolerate
  per-intent PROV bookkeeping on the hot path; nothing here claims
  they do. (The rehabilitation volume's rendering-economics theorem
  already retires the obligation.)
- **A generative/consumable interface (S24's home).** Needs the
  generative loop the slice deliberately does not carry (see Decision
  §4h); judging the consumable layer by this slice would be a category
  error.
- **Adversarial multi-tenancy.** Malicious sources, Byzantine agents,
  permission escalation — the slice's `trust`/`authority` fields are
  typed inputs, not a security model.

**Scope statement:** this decision covers the construction and
calibration of a headless, deterministic, single-process instrument
with four registered sources (person, two agents, system). It does not
cover distributed deployment, hostile-source hardening, or any
human-subject measurement.

## Consequences

### Positive

- The series gets its first machine-measured number (E2 engineering
  leg) under a pre-registered boundary, and the calibration layer makes
  the seven invariants CI-executable rather than citable.
- The frozen-control-arm fight is set up symmetrically: same task
  script, two ontologies, printed counts on both sides — ADR-0023's
  P5 methodology promoted to a standing instrument.
- The rebound gate gives the overthrow-versus-rebound question
  (vol. 5 §9.2) its first automated enforcement in either world: the
  failure mode where a "retired" concept sneaks back as a special case
  is now a shell script and a synthetic-bite demo, not a hope.

### Negative

- A second parallel ontology tree to keep building on every toolchain
  rev (MSVC `/WX`, `-Werror`, UBSan, CodeQL) — the v4 tax again, now
  with tests wired into CI, so it can actually fail releases.
- The E2 harness as a ctest makes the registered memory boundaries a
  CI obligation: a future "cheaper records" refactor that silently
  drops provenance to satisfy the ratio would need to defeat the I6
  suite too, but an honest density regression now reds the build
  rather than a report.
- Every future measurement campaign must re-freeze or explicitly
  re-baseline the control arm; `classic_arm` is dead weight in the tree
  between campaigns (by design — it is a preserved specimen).

### Neutral

- No shipping-API change, no abstraction count change, no doc-claim
  change outside this ADR, the changelog, and TESTING.md. The slice
  cannot affect any existing gate outcome (its files are outside every
  existing gate's scan scope except CodeQL's, which is
  reporting-tier).

## Alternatives Considered

### Alternative 1: Build the slice on the shipping API (extend px_*)

- What: add sources/clock/log semantics as an extension of the seven
  abstractions — `px_relation` with an actor field, undo generalized
  under `px_undo`.
- Why rejected: it smears two ontologies into one ABI. The axioms under
  test *include* the claim that identity-attributed multi-source flow
  is not a parameter of an event system but a different substrate
  (N1's regression clause forbids source-blind signatures in the core).
  Building the test inside the old core would bias the very question —
  and any special case added to make the old core host the new
  semantics would be the rebound signal the calibration suite exists to
  detect.

### Alternative 2: A standalone repository for the slice

- What: `planex-slice` elsewhere, referenced by URI.
- Why rejected: the control arm must be the frozen Planex body under
  the same language, discipline, and task set (vol. 6 §6.3); a
  separate repo either forks that body (unfreezable drift, no shared
  CI culture) or depends on it as a submodule (a new dependency class
  this zero-dependency repository has never had). The v4/ precedent —
  parallel ontology tree, separate library, same repository — was built
  for exactly this shape.

### Alternative 3: Defer the slice until the full rewrite (Phase 4)

- What: skip the instrument; rewrite Planex on the new ontology
  directly; measure on the product.
- Why rejected: vol. 6 §1.1's argument verbatim — an instrument built
  after the commitment cannot falsify it. A rewrite measured on itself
  is the textbook accomplice-shaped instrument; the sixty-year failure
  mode this whole program studies (unstated axioms that can never die)
  would be reproduced with newer vocabulary. The slice is cheap
  precisely so that the expensive decision can stay reversible.

### Alternative 4: Resume the v4 interpretant line instead

- What: grow the slice out of `v4/` (Interpretant/Perlocution/
  Breakdown), which already probes meaning-side semantics.
- Why rejected: v4 pressure-tests the *interpretation* seam
  (Peirce/Searle: what a sign does to a mind); the axioms under test
  here are the *confluence* seams — multi-source identity, time
  addressability, divergence tiers, attention budgeting. Different
  questions, different type systems. v4's own ADR-0012 findings (seam
  pressure-test results) are cited by the research series as input,
  not as substrate. Reusing its tree would couple two experiments that
  share only a build system.

## CAVEATS

- This ADR **measures nothing about users**. Calibration passing is a
  statement about the *instrument* (the ontology can express its
  degenerations without special cases; the invariants hold under
  fuzzing), not about human performance. Every human-subject claim
  remains unmeasured until the E-series runs with participants.
- The E2 engineering-leg operationalization (Decision §4f, Reading B)
  is this ADR's interpretation of vol. 6 §3.2's boundary sentence. It
  is registered openly, with raw data published, so a maintainer
  applying the stricter at-all-n reading can re-adjudicate from the
  same numbers without re-running anything.
- The slice is headless and deterministic by design. "One interface"
  is satisfied by the query surface over the semantic projection; no
  visual rendering claim is made or testable here (Decision §4g).
- S5 (example-as-spec) is *not* loaded; its enabling condition (the
  generative loop) is outside the registered main line (Decision §4h).
  This caveat is the paper trail so vol. 7's handoff is answered, not
  dropped.
- The pricing table (1/2/4/8) is a slice-local default standing in for
  E4's measured table; only its monotonicity is enforced (I7-adjacent
  test), never its values.
- A C6 pass is an existence proof for the *registered* lattices under
  the *tested* generators, not a proof that every future payload type
  admits a lattice — E3c's acceptance question is untouched by
  calibration.

## Known issues

- **Issue:** the slice implements time-addressability at per-region
  version-chain granularity with a global joint down-closure checker in
  tests, rather than materializing vol. 5 §5.1's global cut algebra as
  a first-class data structure.
- **Why accepted:** the instrument needs addresses, queries, rewind and
  branch to be *real* — which they are — while the full cut-lattice
  machinery is Phase-4 scale. The tests prove joint down-closure of
  every address the scenario produces, so the simplification cannot
  silently produce non-cuts.
- **Tracking:** if E2's experimental leg or the audit task family (E1b)
  needs global-cut addresses beyond region granularity, the upgrade is
  a successor ADR.
- **Mitigation:** none needed at instrument scale; recorded so the
  boundary of the claim is explicit.

## Refutation Criteria

- **Calibration falsifies this ADR** if C2–C7 fail: any invariant
  violation, any surviving semilattice counterexample, any byte of
  differential drift, or any rebound signal the static gate catches —
  each is recorded as cluster-rebound evidence against the
  corresponding axiom (N1↔I1, N4↔I2, N3↔I3, merge algebra↔I4, N2↔I5,
  PROV↔I6, N4↔I7), and this ADR's role flips from "instrument built"
  to "gap recorded".
- **The E2 engineering leg falsifies N2's axiom status** (not this
  ADR's craft) if C8 breaks: residency > 3.0× at n = 2000 or
  non-linear growth. The consequence is pre-committed: the demotion
  recommendation (N2 → scenario option) goes to the maintainer with
  the raw data, per vol. 2 table 3-2's two-exit OR.
- **The acceptance-list role falsifies the sequence** if any registered
  E-leg turns out to have no carrier on the slice (a row of the table
  in Decision §3 that cannot be executed when its protocol runs) —
  vol. 6 §9.1's "instrument without reading rules" condition.
- **Who notices, where:** CI (the wired suites and the self-judging E2
  harness), `make -C slice rebound-check`, any fresh-checkout
  recomputation of `measurements/`, and the maintainer's own re-run of
  the analysis script.

## Regression Clause

- **The control arm stays frozen.** `classic_arm` must continue to
  compile and run against the shipping library exactly as at
  main@8229080. A successor ADR may re-baseline the frozen point only
  by naming the new commit and re-registering every measurement taken
  against the old one; it may not quietly "improve" the control.
- **Calibration only tightens.** Once green, I1–I7, the rebound gate,
  and the differential test must not regress; relaxing any of them
  requires a successor ADR that cites the specific failure it is
  excusing and re-runs the falsifiability demonstration (the
  synthetic-bite precedent from ADR-0024's Validation).
- **Registered numbers are immutable.** `measurements/` data is
  append-only; corrections land as new runs with new seeds, never as
  edits to a published result.
- **Boundaries only tighten.** The E-series boundaries quoted in
  Decision §2 and §3 (90%/85%, d = 0.2, 3×, R² 0.99, ±15%, median 4,
  20 disturbances) follow vol. 6's stricter-only rule; any revision
  must be stricter and archived.
- **The instrument may not become a demo.** No visual-first projection,
  no unregistered metric, no "performance claim" may be added to the
  slice's outputs; the slice's success criterion remains "produces
  decisive numbers, whichever way they point" (vol. 6 §6.2).
- **Four-column inheritance.** Every successor ADR in this line
  (≥ 0026) carries its own Refutation Criteria and Regression Clause
  per [ADR-0024](ADR-0024-adr-four-column-discipline.md).

## Validation

Preserved at the implementation and measurement commits, following
ADR-0024's transcript precedent (actual gate output, not assertions
about it).

**At the implementation commit** (measured, not asserted):

```
$ bash scripts/check_doc_sections.sh --check --four-column
check_doc_sections: all 24 ADR file(s) have all mandatory sections
(four-column mode: ADR-0024 and later also checked for
## Refutation Criteria and ## Regression Clause).

$ make -C slice test          # calibration + property + arms + E2
  [TEST] c3_single_source_short_circuit               OK
  [TEST] c4_zero_budget_all_receipts                  OK
  [TEST] c5_all_strict_classical                      OK
  [TEST] c7_undo_sugar_is_projection                  OK
  [TEST] i6_prov_completeness                         OK
  [TEST] i7_ladder_monotone                           OK
  [TEST] c9_treatment_side_zero_machinery             OK
  [TEST] views_are_down_closed                        OK
8/8 calibration sections pass
  [TEST] c6_num_lattice_one_million   (NUM: 1000000 groups) OK
  [TEST] c6_set_lattice_one_million   (SET: 1000000 groups) OK
  [TEST] c6_registry_admits_the_builtins (clones admitted) OK
  [TEST] c6_registry_rejects_nonassociative (the checker bit) OK
  [TEST] c6_counterexample_shrinks (shrunk 49/1f/e3 -> 0/0/3) OK
  [TEST] c6_mixed_rounds              (mixed: 1000000 groups) OK
6/6 property sections pass

$ make -C slice rebound-check
rebound gate: clean (no kind dispatch, no passive mode, no
single-source case)
synthetic bite: all planted rebounds caught (3 patterns matched)
rebound gate: PASS
```

Root gates on the branch head: `make test` 39/39; orthogonality
19/20 (1 pre-existing skip: the transaction-API section, unchanged
from main); feedback 14/14; v05 12/12; v06 32/32; v07 21/21;
v08 31/31; `check-build-sync`, `gen_adr_index --check`, lifecycle,
links (79 files), orphans, terms, stale-abstraction and
limitations-freshness, essence admission 6/6 — all PASS.

**At the measurement commit** — the E2 harness checkpoint table as
published in `measurements/e2-engineering-leg/` (see RESULT.md
there for the full C1–C9 ledger):

```
checkpoint,n_changes,n_log,history_bytes,working_set_bytes,ratio
1,500,501,34048,50440,0.675020
2,1000,1001,58048,50440,1.150833
3,2000,2001,106048,50440,2.102458
4,4000,4001,202048,50440,4.005710

[C8a] residency at n=2000: 2.102 x working set (boundary <= 3.0)
C8a PASS
[C8b] linearity: R^2 = 1.000000 (boundary >= 0.99), slope = 48.0
[C8b] incremental cost: mean 48.0, window [40.8, 55.2]: within
C8b PASS
```

The measured 2.102x lands just under the predicted interval
[2.2, 2.9] — recorded as a deviation in RESULT.md, not smoothed:
the implemented working set (the world struct, region table at its
registered capacity) is slightly heavier than the prediction's
shape. Three measurement adjudications (records-not-capacity;
working-set-is-the-struct; incremental-baseline-at-load-complete)
are registered in the code where they were decided, each with the
rejected alternative's numbers alongside.

## HISTORY

- 2026-11-01: Proposed — registration commit. Predictions C1–C9
  registered before any `slice/` source file exists on this branch;
  the E-series acceptance mapping, the operationalizations (Decision
  §4a–h), and the E2 Reading-B adjudication rule fixed in the same
  commit. Implementation and measurement commits follow in that order;
  their outcomes will be recorded here and in `## Validation`.
- 2026-11-01: Implementation commit. The core (log/clocks/tiers/
  ledger/projections), TRIP-17 and both arms, the calibration and
  property suites, the rebound gate, and the build wiring land.
  C1, C2, C3, C4, C5, C6, C7, C9 verified at this commit (see
  Validation); the E2 leg runs at landing below. Design readings
  registered where they live: the living-knowledge rule, the REBORN
  exemption, fork-head time-travel, the 48-byte record, the
  hot-object concurrency beat (the first E2 run's 2-realized-pairs
  failure mode is on the record in the generator).
- 2026-11-01: Measurement commit. C8 lands: residency 2.102x at
  n=2000, R^2 = 1.000000, increments within +/-15%. The series'
  first registered, machine-measured number, published with raw
  data and a recompute script. All nine predictions PASS; the
  honest ledger (zero human-measured numbers) is unchanged.
- 2026-11-01: Validated — the enforcement is implemented and
  proven to bite (`make -C slice rebound-check`: the rebound gate
  and its synthetic-bite demonstration; the property registry's
  non-associative rejection); C1–C9 PASS, transcribed in
  `## Validation`. The E-series human legs remain ready-not-run;
  promotion to Accepted is a maintainer ruling.

## References

- Research series (repository-external, 卷内底本 for everything cited
  as "vol. N"): *范式冻结* (vol. 1), *立宪* (vol. 2, tables 3-1..3-4
  and ch. 6), *矿脉图* (vol. 3, S-seeds), *原点* (vol. 4, B-bills),
  *意图流：语义草案* (vol. 5, ch. 3–9, §11.3), *可证伪评测协议*
  (vol. 6, ch. 2–6, §9), *公理翻案书* (vol. 7, ch. 4.3, §8).
- In-repo precedents:
  [ADR-0023](../proposed/ADR-0023-semantic-drift-probe.md) (prediction-before-
  measurement at probe scale; the classic-control-arm pattern),
  [ADR-0024](ADR-0024-adr-four-column-discipline.md) (the four-column
  discipline this ADR is the first full-size instance of),
  [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md)
  (prototype protocol), [ADR-0012](../accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md)
  (the v4 parallel-tree precedent).
- Code (landing in the implementation commit): `slice/`,
  `scripts/check_slice_rebound.sh`,
  `scripts/analyze_e2_residency.sh`,
  `measurements/e2-engineering-leg/`.
- External, as cited by the volumes and load-bearing here: Popper
  1963 (demarcation — the refutation-column lineage); Nosek et al.
  2018 (preregistration); Claessen & Hughes 2000 (property-based
  testing with shrinking — C6's method); Shapiro et al. 2011 and
  Kuper 2014 (the semilattice conditions C6 tests); Fidge 1988 /
  Mattern 1989 (vector clocks — the concurrency decider); W3C
  PROV-DM 2013 (the provenance record shapes); Fowler 2005 (event
  sourcing — the projection's lineage); Dourish 2001, drawing on
  Heidegger's Zuhandenheit/Vorhandenheit, for the readiness/rupture
  spectrum the S16 pricing table grades.
