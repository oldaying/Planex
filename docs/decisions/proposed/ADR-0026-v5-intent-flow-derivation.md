# ADR-0026: The v5 derivation — the full rewrite begins as a parallel tree, gated

## Status of This Memo

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: 2026-11-01

## When to use this decision

This decision governs the `v5/` tree and everything under it: the
promoted intent-flow core (`if5_*`), the cut algebra (`if5_cut.c`), the
seated structures (the semantic tree, the surprise schedule), the v5
test suites, and the v5 rebound gate. It applies to every commit that
touches `v5/` until a successor ADR supersedes it. It also fixes the
terms under which the research series' Phase 4 (full rewrite) may
proceed at all: the tree's shape, its equivalence obligations to the
frozen instrument, and the promotion gate that separates "derivation"
from "shipping direction".

## When NOT to use this decision

This decision does not touch the shipping API (`include/planex/`,
`src/`), does not promote or demote any of the 7 canonical abstractions,
does not modify the slice instrument (`slice/`, ADR-0025's registered
surface), and does not modify any landed measurement
(`measurements/`). Nothing here flips a default, deprecates a symbol,
or moves a file outside `v5/` (build wiring excepted — see Decision
§6). Nothing here authorizes promotion of the intent-flow ontology to
a shipping direction: that is the maintainer's act, gated on the E-series
human legs exactly as registered in Decision §5. And nothing here runs
a human-subject experiment: the E-series legs that need participants
remain registered-not-run, and this ADR's predictions (the V-series)
are engineering predictions only — they say nothing about users, which
is the same boundary ADR-0025's CAVEATS drew for the C-series.

## Status

Proposed

## Context

The research series (seven volumes delivered 2026-10 through 2026-11)
fixed a five-step sequence for its final act: semantics draft →
evaluation protocol → rehabilitation book → vertical slice → **(gated)
full rewrite**. The first four are done. The gate question is therefore
now live, and this ADR is the independent decision ADR-0025 explicitly
reserved: "Nothing here authorizes Phase 4 (full rewrite); that is a
separate decision with its own ADR" (ADR-0025, When NOT to use).

**The gate's measured state.** The slice's registered predictions
C1–C9 all PASS (measurements/e2-engineering-leg/RESULT.md). The E2
engineering leg produced the series' first registered measured number:
history residency 2.102× the working set at n = 2000 (registered
boundary ≤ 3.0×), growth linear with R² = 1.000000 and a slope of
48.0 bytes per change (the registered record density), incremental
cost 48.000 at every checkpoint. The deviation below the registered
prediction interval [2.2, 2.9] is recorded in RESULT.md as measured,
with its cause (the working set's region table is heavier than the
prediction's shape assumed). The E-series human legs — E1a/E1b, the
E2 experimental leg, E3a/b/c, E4, E5a/b — are **registered and not
run**: they need human participants, which no agent session can
supply. Vol. 6 §8.3's upgrade ladder is explicit about what that
vacancy means: instrument verification (the calibration layer and the
E2 engineering leg, both landed) licenses feasibility judgments only;
axiom-level claims require the pilot tier (n ≥ 12) and multi-family
replication. So the honest ledger at decision time reads: engineering
gate PASSED with numbers, human gate VACANT by capability, not by
choice.

**The control-arm constraint.** Vol. 6 §6.3 registered the design
principle for every E-series controlled leg: the control arm is the
frozen Planex shipping ontology, and "对照臂实现完成后冻结版本号，
测量期间不得改动——防止对照在实验中途悄悄进化" — the control must
not evolve mid-experiment-window. The window is open (the legs are
registered, unrun). An in-place rewrite of `src/` would mutate the
control arm under a live registration. That is not a stylistic
preference; it is a protocol violation that would contaminate every
E-series outcome that follows.

**The repo's own precedent.** This repository already contains the
pattern that satisfies both constraints at once: `v4/` is a clean-room
essence-derivation tree that lives beside the shipping API, builds as
its own library (`planex_v4_lib`, never linked into `planex_lib`), and
verifies claims without touching shipping behavior (ADR-0012's
orthogonality pressure tests). The slice followed the same shape
(`planex_slice_lib`). A fifth derivation tree — `v5/` — is therefore
not a new invention but the third use of an established repo pattern,
with one difference: v4 derived the OLD axioms' essence, while v5
derives on the NEW axiom table (vols. 3/5) that the slice just
instrumented.

**The registered debts this decision can discharge.** ADR-0025's Known
issues section registered exactly one Phase-4-scale item: "the slice
implements time-addressability at per-region version-chain granularity
with a global joint down-closure checker in tests, rather than
materializing vol. 5 §5.1's global cut algebra as a first-class data
structure," with the tracking clause "if E2's experimental leg or the
audit task family (E1b) needs global-cut addresses beyond region
granularity, the upgrade is a successor ADR." Vol. 7 discharged the
rehabilitation court's constructive obligations on paper — S15
(semantic tree first class) + S17 (prediction-error scheduling) as the
replacement design object for the dead reproducibility claim, and S24's
two-layer seating (consumable projections over a durable intent flow) —
with vol. 7 ch. 5's refutation condition: S15 must prove implementable
WITHOUT borrowing reproduction primacy, or the seat goes back to
re-review. These are ontology-shape obligations; the slice was an
instrument and deliberately carried none of them (ADR-0025 Decision
§4g kept the projections same-rank). A rewrite decision that did not
carry them forward would drop the court's constructive branch on the
floor.

**What the evidence does NOT license.** Vol. 6 §6.2's sentence governs
the slice and, by extension, anything built on its verdicts: the
instrument's honor is to give the axioms a chance to die. The
C-series' all-PASS means the axioms survived the engineering axes; it
does not mean they helped anyone. A rewrite that claimed human benefit
on this evidence would commit the exact failure mode vol. 6 §8.3's
ladder exists to prevent (N=1 numbers promoted to axiom-level claims).
This ADR therefore claims engineering feasibility and nothing else, and
structures the rewrite so that the unrun human legs retain full
authority over the expensive endgame (promotion).

## Decision

### 1. The rewrite's shape: a fifth derivation tree, parallel and additive

The full rewrite proceeds as `v5/` — the fifth essence-derivation tree
(the v4/ precedent's naming discipline: `vN` without a dot is a
derivation iteration, `v0.N` is the product version; they have never
collided and do not start now). v5 is **additive**: it adds a tree, a
library target, test targets, and gate coverage. It does not modify
`include/planex/`, `src/`, `slice/`, or `measurements/`. The shipping
API stays frozen as the E-series control arm for as long as the
human-leg registrations are live (vol. 6 §6.3); the slice instrument
stays frozen as the registered carrier of those legs (vol. 6 §6.2).
`planex_v5_lib` is never linked into `planex_lib` — the same firewall
v4 and the slice established.

### 2. The gate adjudication: the derivation proceeds, the promotion waits

The engineering gate is adjudicated PASSED on the registered numbers
(C1–C9, E2 engineering leg, both cited in Context). The human gate is
adjudicated VACANT and is honestly carried as vacancy — not converted
into optimism, not converted into a demand for patience either: the
derivation work (which needs no human data) proceeds now, and the
promotion decision (which needs human data) is pre-committed to wait.
This split is the whole point of the parallel-tree shape: it lets the
program make real progress without spending a single unit of the
evidence the human legs will need to stay valid.

### 3. Stage R1 — this ADR's executable register

R1 promotes the slice's semantic core to library grade and seats the
court's obligations. Concretely:

**(a) Promotion, not re-implementation.** The ifl core
(`ifl_core.c`, `ifl_merge.c`, `ifl_budget.c`, 2407 lines) is promoted
to `v5/src/if5_{core,merge,budget}.c` with the `if5_` namespace,
carrying the seven implementation-era design fixes ADR-0025 registered
(living-knowledge rule, REBORN exemption, realloc reacquisition,
sub-ceiling/carved split, delegated_by scrub, fork-head
time-pointing, set-projection NUL discipline) and their code comments
intact. Provenance is recorded in the file headers: every if5 source
names its slice ancestor and the promotion commit. The registered
record density holds: `if5_intent` stays exactly 48 bytes (V8).

**(b) The global cut algebra, materialized (discharging ADR-0025's
registered Phase-4-scale known issue at R1 granularity).** Vol. 5
§5.1's causal slice becomes a first-class value: `if5_cut` = a log
prefix bound + pruned subtree roots (the undo algebra of §5.2) + the
cached living-knowledge frontier. The algebra: `if5_cut_at` (principal
cut), `if5_cut_prune` (the undo constructor — removing a causal
up-set from a down-closed set leaves a down-closed set, so the
constructor provably preserves the invariant), `if5_cut_join` (union)
and `if5_cut_meet` (intersection) on principal cuts, `if5_cut_le`
(the subset order, with the covering test registered in the source),
and `if5_cut_is_down_closed` — the structural verifier promoted from
the slice's test-private checker to a public constructor-checked
invariant. Cuts are **addressable**: `if5_cut_register` enters a cut
into the world's registry, and any region can be folded at any
registered address — the "global-cut addresses beyond region
granularity" that ADR-0025's tracking clause anticipated for E1b's
audit family. The per-source (non-prefix) cut — a knowledge address
that has seen source A's late work but not source B's — is
explicitly DEFERRED: the REBORN clock-collision ADR-0025 documented
makes per-source membership by clock comparison unsound, and
resolving it is R2-scale semantics, registered as this ADR's Known
issue #1.

**(c) S15 seated: the semantic tree is first class.** The semantic
projection stops being a text dump and becomes a structured, walkable
tree: `if5_tree_walk` visits every region in depth-first containment
order (region creation gains an optional parent; 0 = root, the
slice-compatible flat shape), each node carrying exactly what an
accessibility tree carries — name, folded value, semantic role
(payload family), tier, contribution count, PROV anchor. Vol. 4 S15's
statement is the seat's contract: semantic structure is the work,
vision is one projection, screen-reader users are first-class
testers. The no-borrowing clause (vol. 7 ch. 5) is enforced
statically: the v5 rebound gate bans visual-field vocabulary
(`rect`, `glyph`, `font`, `color`, `colour`, `pixel`, `rgba`) from
`v5/include` and `v5/src` — a semantic tree that needed a rectangle
to exist would be the reproduction primacy the court killed, sneaking
back in as a field name.

**(d) S17 seated: the surprise timetable, as a pure query (N5's
no-new-slots discipline).** Vol. 4 S17: the design object is not the
picture but the timetable of controlled prediction errors;
onboarding, diffs, changelogs and AI explanations are one object.
Vol. 5's N5 discipline (accountization, no new slots) forbids seating
it as a new log family. So `if5_schedule` is a **query over records
the core already logs**: surprise-class debits, breaker receipts,
tier dial moves, arbitration exclusions, concurrent merges — each
entry PROV-backed (its iid resolves to a real record) and carrying
the posted magnitude. The one record-format delta this requires is
registered openly: the slice's `debit` word carried the severity
rung only; if5 packs the debit CLASS into the same word's high bits.
No registered observation surface reads that word — the black-box
export, all three projections, the receipts, and the stats do not
print it (verified by the differential, V1) — so the delta is
invisible on the equivalence surface and is the minimal encoding that
makes "when was this user surprised" answerable from the black box
alone. E5b's cost/benefit question ("is the ledger worth its
bookkeeping") now has its measurement substrate on the v5 side.

**(e) S24 seated: the two-layer boundary as an invariant.** Vol. 7
ch. 8's seating — a consumable layer (projections, generated per
query and discarded) over a durable layer (the intent flow) — is
enforced, not asserted: the seats test snapshots the durable layer's
full byte image (log count, history bytes, working set, stats
struct, receipts, ledgers), runs every projection at every TRIP-17
checkpoint, and demands the image unchanged (V6). A projection that
mutated the durable layer would be a consumable leaking into the
durable register — the exact smuggled-state shape S24 exists to
prevent.

### 4. The differential gate — the rewrite may extend, never alter

The master gate of R1: **v5 must be observationally equivalent to the
frozen slice on the registered observation surface**, enforced by
`test_v5_differential.c` driving both cores in lockstep through (i)
the full TRIP-17 scenario and (ii) seeded randomized op streams (≥ 100
streams × ≥ 200 ops, plus an E2-shaped load: 1000 regions, 4000
changes, the registered hotspot shape) and comparing, after every op
and at every checkpoint: every region's folded state at "now" and at
sampled historical views, all three projections (byte-exact), the
black-box export text, the stats struct (byte-exact), the receipt
store (count and fields), the log count, the history bytes (exact),
and the working-set delta (v5 − slice == the registered constant;
the absolute working sets differ by design — the parent field and
the cut registry — and the constant is asserted, printed, and
recorded in Validation). At the E2 shape the differential
additionally re-verifies v5's own residency against the registered
C8 boundary (≤ 3.0× at n = 2000) directly, so the boundary is
re-measured on the rewrite rather than transferred by argument (V9).
Any mismatch on any compared surface is a semantics alteration and
fails the build. This is the anti-accomplice clause for the whole
rewrite: the instrument measured the slice's semantics; if the
rewrite changes them silently, the E-series verdicts stop applying to
the product and the program has rebuilt the sixty-year failure mode
with newer vocabulary.

### 5. The promotion gate — registered now, executed by the maintainer

v5 becomes a shipping direction (any change to `include/planex/` or
`src/` that adopts the intent-flow ontology) only when the E-series
human legs, run on the slice per their registrations, do not trigger
their demotion clauses. The clauses, from vol. 6 ch. 3 (registered
default boundaries; revisions may only tighten):

- **E1a**: agent workflows show single-intent absolute majority →
  N1's strong form and the killer-domain choice fall together (vol. 6
  §7.2's binding).
- **E2 experimental leg**: on the REMOTE recovery subset, query-style
  undo TOST-equivalent to or worse than the stack button → N2
  demotes to scenario option. (The engineering leg already passed at
  2.102×; the OR in vol. 2 table 3-2 gives both exits equal force.)
- **E3a + E3b both**: tier-3 share ≥ 90% across domains AND all
  three residual-value metrics < 1% → the three tiers collapse to
  one. Independently, **E3c**: implementer semilattice-cost
  acceptance median < 4 → the merge algebra demotes to
  implementation detail.
- **E4**: budgeted attention management worse than free
  notifications on BOTH primary metrics (throughput and
  satisfaction) beyond the equivalence margin → N4 demotes to an
  optional constraint.
- **E5a**: calibration-performance rank correlation |ρ| 95% CI
  upper bound < 0.1 with consistent direction across products → N5
  is a candidate for revocation. **E5b**: ledger bookkeeping cost
  exceeding governance benefit over the deployment window → N5
  demotes to the protocol's metric family.

Any of these firing does not automatically delete v5 — it closes the
promotion gate, stalls R2+, and hands the maintainer the raw data and
the demotion recommendation, exactly as ADR-0025's refutation
consequences did for the slice. The maintainer may also adjudicate a
promotion on evidence short of this list — that is ADR-0014
lifecycle authority, not this ADR's to grant or withhold.

### 6. Wiring

`v5/Makefile` (self-contained, `CFLAGS_EXTRA` passthrough, the
slice Makefile's shape); root `CMakeLists.txt` gains
`planex_v5_lib` + the five test targets as ctest entries (never
linked into `planex_lib`); `ci.yml`'s cmake jobs run the v5 suites,
the make jobs run `make -C v5 test` and `make -C v5 rebound-check`;
`scripts/check_slice_rebound.sh` grows a v5 section (the same four
forbidden patterns over `v5/src` + `v5/include`, plus the S15
visual-field ban, with its own synthetic bite). TESTING.md and the
changelog record the layer. The four-column gate
(`check_doc_sections.sh --four-column`) applies to this file as an
ADR numbered ≥ 0024.

### 7. Stage R2 and beyond — registered, not executed

R2 is the mechanism-layer re-derivation: the seven shipping
abstractions re-derived as projections and queries over the durable
core (Estimate as a fold, Relation as the cause-graph over the log,
Closure as a judgment, Perception as the projection family, px_loop's
budget as N4's ledger, intent compilation as the type registry's
routing, px_interaction as the adjudication surface) — each mapping
argued, differential-gated where a slice counterpart exists, and
carried by its own successor ADR. R2 does not start before R1 lands
green and the maintainer has had the opportunity to rule on this ADR.

## Essence Check

### Q1. Which essence axis does this decision affect?

The intent axis — the same one ADR-0017/0018/0023/0025 moved. This
decision does not add an abstraction; it changes the SUBSTRATE an
abstraction set is derived over (the axiom table the research series
installed), inside a tree the shipping API does not link.

### Q2. Does it compress or increase human cognitive bandwidth?

Unmeasured, and registered as such. The engineering signs (the
differential's byte-identical transcripts at equal or lower
registered cost) say the substrate is affordable; whether it HELPS
anyone is exactly what E1b/E2-experimental/E4 exist to measure, on
the slice, with participants. This ADR makes no human-benefit claim
and structurally cannot: its predictions (V1–V9) are all machine-
checkable.

### Q3. Is there a gap between the claim and the implementation?

The claim is "derivation, gated, equivalent" and the implementation
is three gates wide: the differential (V1) for equivalence, the
seats suite (V4–V6) for the court's obligations, the rebound gate
for the no-reborrowing clauses. The gap-closer is the same one the
slice used: every claim in this ADR maps to a test that fails the
build if the claim is false.

### Q4. What is the cost, and who can verify it?

Cost: ~5,600 lines of promoted-plus-new tree, CI minutes for five
suites, and the permanent discipline that v5 semantics may not drift
from the slice without a registered decision. Verification: CI on
every push, and vol. 6 §8.4's recomputation rule — any fresh
checkout re-runs the differential and re-derives every number in
Validation.

### Q5. What are the counterexamples?

The registered demotion clauses (Decision §5) are the counterexample
budget, and they are all still live. The nearest-term engineering
counterexamples: a differential mismatch (v5 altered semantics), a
residency breach on v5, an S15 that cannot stand without visual
fields, a rebound signal in v5/src. Each has a named test or gate
that fires on it.

## Consequences

### Positive

- The program's Phase 4 begins without spending optionality: the
  control arm stays frozen, the instrument stays frozen, the human
  legs keep their full registered force, and the derivation work
  (which was always going to be needed if the axioms survive) gets
  done in parallel instead of in series with the human calendar.
- ADR-0025's one registered Phase-4-scale debt (the global cut
  algebra) is discharged at R1 granularity, with the per-source
  extension honestly registered as the next known issue instead of
  silently faked.
- The rehabilitation court's constructive obligations (S15/S17/S24)
  get executable seats with refutable conditions — vol. 7 ch. 5's
  "re-review if unimplementable without reproduction primacy"
  clause becomes a static gate plus a totality test, not a hope.
- The repo gains a second differential discipline: check-examples
  already demands byte-identical demo transcripts; the rewrite now
  demands byte-identical semantics against its own instrument.

### Negative

- Two parallel semantic cores now exist and must be kept equivalent
  by CI force; every future slice-side fix (the E-series harnesses
  may legitimately need some) must be mirrored into v5 or the
  differential fails — which is the point, but it is real labor.
- The debit-word class packing (Decision 3d) is a record-format
  fork between the slice and v5; it is invisible on every registered
  surface, but anyone reading raw `ifl_intent` structs across both
  trees must know the encoding difference. It is documented here and
  in if5.h.
- The working-set delta (parent field + cut registry) is a permanent
  few-KB tax on v5 worlds vs slice worlds. Registered, asserted by
  the differential, and small against the 48 B/change history slope.

### Neutral

- v5's test suite re-runs the calibration battery under new names;
  the C-series verdicts are slice verdicts and stay cited as such —
  the V-series are v5's own and cite the C-series only as ancestors.
- The naming (v4 = old-axiom derivation, v5 = new-axiom derivation)
  encodes program history in directory names; a future v6 will need
  its own one-line registration of what it derives.

## Alternatives Considered

### Alternative 1: In-place rewrite of the shipping API now

- What: rewrite `src/`/`include/planex/` directly on the intent-flow
  ontology; version-bump past it.
- Why rejected: it mutates the E-series control arm under live
  registrations (vol. 6 §6.3), and it converts an engineering-gated
  decision into an ungated one — exactly the "instrument after the
  commitment" shape vol. 6 §1.1 exists to forbid. Also gratuitous:
  the human legs do not need the rewrite; they need the slice, which
  exists.

### Alternative 2: Wait for the human legs before writing any v5 code

- What: do nothing code-side until E1b/E2/E4 etc. have run and
  adjudicated.
- Why rejected: it serializes two things that do not depend on each
  other. The derivation's inputs (the slice's semantics, the court's
  obligations) are frozen and available; the human legs' outputs
  gate PROMOTION, not DERIVATION. Vol. 6 §6.2's own logic supports
  the split: the slice carries the legs, so a parallel tree cannot
  contaminate them. The maximally conservative reading would also
  have forbidden building the slice before the protocol volume —
  the series already adjudicated that ordering question, in favor
  of instruments-before-commitments, not paralysis-before-data.

### Alternative 3: Extend the slice instead of promoting a new tree

- What: grow `slice/` into the product; add the cut algebra, seats,
  and library API there.
- Why rejected: the slice is a registered instrument whose surface
  the E-series legs cite. Growing it into a product would (a) evolve
  the thing being measured, (b) blur "instrument" into "proposal" —
  the exact clause ADR-0025 Decision §4 drew ("instrument, not
  demo"), and (c) leave the court's obligations (S15/S17/S24)
  welded to a tree whose ADR explicitly disclaims them. Promotion
  with provenance keeps both trees honest: the slice stays what it
  was registered to be, v5 carries the lineage in its headers.

### Alternative 4: Resume the v4 interpretant line instead

- What: grow v5 out of `v4/` (Interpretant/Perlocution/Breakdown).
- Why rejected: ADR-0025 rejected the same move for the slice, and
  the reason is unchanged — v4 pressure-tests the interpretation
  seam; the axioms under derivation here are the confluence seams
  (identity, time, divergence, budget). Different type systems.
  v4's ADR-0012 findings remain cited input; its tree remains a
  sibling derivation, not a substrate.

## CAVEATS

- This ADR **measures nothing about users**, and neither do its
  predictions. V1–V9 are machine-checkable engineering claims. Every
  human-subject claim in the program remains exactly as unmeasured as
  it was before this ADR existed.
- The differential proves equivalence on the REGISTERED observation
  surface (Decision §4's list). It does not prove the two cores are
  identical programs, and they are not: the debit word, the region's
  parent field, the cut registry, and the three new modules are v5's
  by design. The surface list is the contract; anything outside it
  that starts mattering later must be added to the differential
  before it is relied upon.
- The promotion gate (Decision §5) is registered as this ADR's best
  rendering of the vol. 6 demotion clauses at decision time. If the
  E-series protocols are revised (vol. 6 §9.1's own revision rule:
  tighter only, and on the record), the gate's clause list must be
  re-derived from the revised protocol, not from this file.
- S15's static visual-field ban is a vocabulary gate, not a
  semantics proof. A determined smuggler could encode a rectangle in
  two uint32s named `a` and `b`. The ban catches the accidental and
  the lazy — the deliberate were never a gate's problem (vol. 5's
  rebound-signal framing makes the same admission about the slice's
  gate).
- The cut registry is capacity-bounded (registered: 64 named cuts)
  and does not persist across world free; named-address persistence
  is an R2 question (and touches the black-box format, which is
  itself a registered surface — changing it is a successor-ADR
  matter).
- Residency re-verification on v5 (V9) inherits the E2 shape's
  registered measurement mode (full log retention); it says nothing
  about windowed retention, which remains unmeasured everywhere.

## Known issues

- **Issue:** the per-source (non-prefix) cut — addressing "the world
  as source A has seen it" — is not expressible in R1's cut algebra;
  cuts are prefix bounds plus prunes.
- **Why accepted:** per-source membership by clock comparison is
  unsound under the REBORN collision ADR-0025 documented (a re-based
  record reuses its source's clock slot value, so "vc ≤ F" cannot
  distinguish "before F's frontier" from "instead of what F pruned").
  Sound per-source cuts need record-identity sets or a monotone
  re-basing scheme — genuine new semantics, requiring their own
  registration and their own differential surface.
- **Tracking:** R2's successor ADR, or the first E1b pilot that needs
  knowledge-state addresses; whichever comes first.

- **Issue:** the surprise schedule itemizes surprise DEBITS and
  breaker receipts, but the slice's log did not record debit class
  at all — v5's packed debit word is the fix, and the slice retains
  the under-determination (its E5b deployments must rely on receipts
  only).
- **Why accepted:** the slice is frozen; fixing it there would evolve
  the instrument. The gap is documented on both sides.
- **Tracking:** none — the slice's E5b protocol already registers
  receipts as its measurable; if an E5b run needs per-debit
  itemization on the slice, that is a protocol revision (vol. 6
  §9.1's rule) and re-registers through the normal channel.

## Refutation Criteria

- **The E-series human legs falsify (any clause of Decision §5
  firing)** → the promotion gate closes and R2+ stalls. The
  consequence is pre-committed: the demotion recommendation goes to
  the maintainer with the raw data (vol. 2 table 3-2's two-exit OR;
  ADR-0025's precedent for how a firing prediction is handled). v5/
  itself is not auto-deleted — its fate is the maintainer's
  adjudication, per the Regression Clause.
- **The differential finds ANY mismatch on the registered surface**
  (a semantics alteration the rewrite did not register) → this
  decision's central claim is false and the build is red until the
  mismatch is either fixed or carried as an explicitly registered,
  ADR-documented deviation. Silent drift is the one outcome this
  ADR treats as categorically void.
- **v5's residency at the E2 shape exceeds 3.0× at n = 2000 or
  grows non-linearly** → the same consequence as ADR-0025's C8: the
  N2 demotion recommendation leaves the holster, now carrying both
  trees' numbers.
- **S15 proves unimplementable without reproduction primacy**
  (operationalized: the semantic tree cannot carry TRIP-17's
  semantic content without a visual-first field, or the totality/
  determinism predictions fail for structural reasons) → vol. 7
  ch. 5's re-review clause fires; the seat returns to the court's
  docket and this ADR's Decision 3c is void until re-adjudicated.
- **A rebound signal appears in v5/src or v5/include** (kind
  dispatch, quiet-mode flag, single-source special case, visual
  field, or a kind predicate) → the extended rebound gate fails the
  build; the merge that introduces it does not land.
- **Who notices, where:** CI (the differential, the seats suite, the
  cut suite, the calibration battery, the rebound gate), any fresh
  checkout re-running them, and — for the human-leg clauses — the
  E-series runs themselves, whose adjudications land in the
  measurements/ tree this decision does not touch.

## Regression Clause

If this ADR is superseded or deprecated, the successor inherits:

1. **The observational-equivalence envelope.** Any tree derived from
   v5 that claims the slice's lineage must either pass the
   differential on the registered surface or carry each deviation
   as a numbered, ADR-registered semantic decision with its own
   refutation criteria. "We rewrote it, so it behaves differently
   now" is not an available defense while any E-series
   registration is live.
2. **The promotion gate.** No successor may promote the
   intent-flow ontology to a shipping direction on engineering
   evidence alone; the human-leg gate (or the maintainer's
   explicitly recorded adjudication that supersedes it, per
   ADR-0014's lifecycle authority) travels with the lineage.
3. **The seat disciplines.** S17's query-only shape (no new log
   families without an N5-compliant registration) and S15's
   no-visual-fields rule bind any successor that keeps the seats.
   Dropping a seat is allowed only by recording which court
   obligation it discharged and where that obligation goes instead.
4. **The rollback shape.** v5/ is additive by construction; rollback
   is removal of the tree plus reversion of the wiring (CMakeLists,
   ci.yml, TESTING.md, changelog entries) plus deprecation of this
   ADR. The shipping API, the slice, and the measurements require
   NO rollback action — they were never touched. A successor that
   cannot say the same has already violated this clause.

## Validation

*(To be filled with measured gate outputs when R1 lands — the
three-beat discipline: this ADR registers before the v5 sources
exist; the implementation lands; the outputs below are then
transcribed from real runs, never asserted. Placeholder sections
that survive to merge are a four-column violation.)*

- Differential (V1): pending.
- Cut algebra laws (V2): pending.
- Cut-view fold agreement (V3): pending.
- S15 seat (V4): pending.
- S17 seat (V5): pending.
- S24 purity (V6): pending.
- Calibration battery on if5 (V7): pending.
- Record density (V8): pending.
- Residency at the E2 shape on v5 (V9): pending.

## HISTORY

- 2026-11-01 — Registered as Proposed before any v5/ source existed.
  Inputs: ADR-0025's landed measurements (C1–C9 PASS; E2 engineering
  leg 2.102× / R² = 1.000000 / 48.0 B per change), the E-series
  human-leg vacancy, vol. 6 §6.3's control-arm freeze, ADR-0025's
  registered Phase-4-scale known issue, vol. 7's constructive
  obligations (S15/S17/S24).

## References

- `docs/decisions/proposed/ADR-0025-intent-flow-vertical-slice.md` —
  the instrument whose verdicts gate this decision; its Known issues
  section registered the Phase-4-scale cut-algebra debt this ADR
  discharges at R1 granularity.
- `docs/decisions/proposed/ADR-0024-*` — the four-column discipline
  this file obeys (sections `## Refutation Criteria` and
  `## Regression Clause`).
- `docs/decisions/accepted/ADR-0012-*` — the v4 derivation precedent
  (orthogonality pressure tests; the parallel-tree shape).
- `docs/decisions/accepted/ADR-0014-*` — the lifecycle authority the
  promotion gate defers to.
- Research series vol. 5 (意图流语义草案) §5.1–5.4 — the causal-slice
  definition, undo-as-query, dual address, black-box parameters the
  cut algebra materializes; §4.2 the adjudication flow; ch. 7 the
  budget algebra; §8.1 N5's overturn conditions.
- Research series vol. 6 (可证伪评测协议) §1.1 (instruments before
  commitments), §3.1–3.5 (the E-series registrations this ADR's
  promotion gate cites), §6.2–6.3 (the slice as carrier; the
  control-arm freeze), §8.3 (the upgrade ladder that bounds every
  claim here), §9.2 (the handoff this decision consumes).
- Research series vol. 7 (公理翻案书) ch. 3 (S15+S17 as the
  replacement design object), ch. 5 (the implementability
  refutation condition), ch. 8 (S24's two-layer seating).
- `measurements/e2-engineering-leg/RESULT.md` — the registered
  numbers cited in Context.
- `slice/` in full — the frozen instrument; the differential's
  reference arm.
