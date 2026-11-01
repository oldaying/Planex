# ADR-0027: Stage R2 — the mechanism layer re-derived: seven abstractions as projections, queries, and judgments over the durable core

## Status of This Memo

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: 2026-11-01

## When to use this decision

This decision governs stage R2 of the v5 derivation: the mechanism-layer
re-derivation that ADR-0026 §7 registered. It applies to every commit
that adds or touches the v5 mechanism modules (`if5_estimate.c`,
`if5_cause.c`, `if5_judge.c`, `if5_projection.c`, `if5_process.c`,
`if5_route.c`), the knowledge-cut extension of the cut algebra, the
containment-as-content change to the semantic tree, and the v5 public
header's mechanism section. It fixes the seven mapping arguments (one per
shipping abstraction), the two Known-issue discharges ADR-0026 tracked to
this ADR, the M-series predictions that gate the landing, and the
differential obligations that keep R1's byte-exact surfaces byte-exact.

## When NOT to use this decision

This decision does not touch the shipping API (`include/planex/`,
`src/`), does not modify the slice instrument (`slice/`,
`measurements/`), and does not modify any R1 surface the differential
already compares byte-for-byte (the three projections, the black-box
export, the record word). It does not promote the intent-flow ontology to
a shipping direction — the promotion gate is ADR-0026 §5's, unchanged.
It does not run any human-subject experiment; the E-series legs stay
registered-not-run, and the M-series below are engineering predictions
only. It does not build a visual/pixel projection: S15's contract
(semantic structure is the work, vision is one projection) plus the v5
rebound gate's visual-field vocabulary ban together mean the projection
family registered here is textual and structural — a rendering layer is
a presentation question, registered for R3, not smuggled in here.

## Status

Proposed

## Context

Stage R1 landed green (main `10d1781`): the promoted core, the cut
algebra, three seats (S15/S17/S24), and the differential — V1–V9 all
PASS, CI 13/13 on a fresh clone. ADR-0026 §7 then registered, without
executing, what R2 is:

> R2 is the mechanism-layer re-derivation: the seven shipping
> abstractions re-derived as projections and queries over the durable
> core (Estimate as a fold, Relation as the cause-graph over the log,
> Closure as a judgment, Perception as the projection family, px_loop's
> budget as N4's ledger, intent compilation as the type registry's
> routing, px_interaction as the adjudication surface) — each mapping
> argued, differential-gated where a slice counterpart exists, and
> carried by its own successor ADR. R2 does not start before R1 lands
> green and the maintainer has had the opportunity to rule on this ADR.

R1 has landed green. This ADR is that successor.

**Two tracked Known issues come due with R2.** ADR-0026's Known issues
section registered: (1) S15 containment is working-set metadata in R1,
not logged intent — containment changes are not themselves undoable, and
the tracking clause names "R2's successor ADR, with the mechanism-layer
re-derivation where the semantic tree's structure becomes first-class
content"; (2) the per-source (non-prefix) cut is not expressible in R1's
cut algebra — clock comparison is unsound under the REBORN collision,
and sound per-source cuts "need record-identity sets or a monotone
re-basing scheme", tracked to "R2's successor ADR, or the first E1b
pilot that needs knowledge-state addresses; whichever comes first".
A third, smaller item sits in CAVEATS: named-address persistence for the
cut registry "is an R2 question (and touches the black-box format,
which is itself a registered surface — changing it is a successor-ADR
matter)". This ADR discharges the first two and deliberately does NOT
touch the third (see Alternatives).

**What the seven abstractions actually are, on the shipping side.**
Read from the code, not from memory of the docs:

- **Estimate** (`src/estimate.c`, 737 lines): a value with confidence
  and time — a mutable cell with an observer linked list, an animation
  trajectory (from → to over duration, ease-out), and Stage-3 derived
  estimates that recompute when a source fires (`derived_recompute`,
  with cycle detection and propagation-depth accounting). Time is
  encoded inside the cell (the trajectory), not addressed from outside.
- **Relation** (`src/relation.c`, 218 lines): a linked list of typed
  edges between opaque pointers — nine kinds (BESIDE, DEPENDS_ON,
  TRIGGERS, VARIES_WITH, AFFORDS, CONTAINS, WITHDRAWS_FOR, PRESENTS_FOR,
  INTERPRETS_AS), optionally actor-scoped. Edges are ASSERTED by
  application code; nothing ties an edge to evidence.
- **Closure** (`src/closure.c`, 440 lines): the five execution-side
  stages (Goal, Intent, Action, Execution, Evaluation). Intent is a
  typed value, not a callback — but Action still IS a callback
  (`px_action_fn`): the runtime invokes caller code to mutate state,
  and undo works by replaying recorded intents.
- **Perception** (`src/perception.c`, 460 lines): a pure function from
  estimates to a denotation, kept in a process-global linked list, with
  `px_perceptions_for_estimate` discovery and `px_perception_invoke_all`
  fan-out.
- **px_loop's budget** (`src/feedback.c`, 512 lines): `budget_ms` on
  the loop; each audit entry records wall-clock duration and whether it
  exceeded the budget; overrun counting and a warn-once notice. The
  budget is observed AFTER the fact — nothing is ever refused.
- **Intent compilation** (`src/hit.c`, the `px_afford_compile` line):
  the routing-side compile step that turns a physical hit into the full
  semantic payload — resolution by position, payload assembly, then
  closure trigger.
- **px_interaction** (`src/interaction.c`, 401 lines): the continuous
  process `begin(ev) → sample(ev)* → commit(ev) | cancel(reason)` with
  the invariant that samples never notify observers and never
  auto-invoke perceptions — the semantic world is touched only at phase
  transitions.

**The mechanism-layer question.** On the shipping side these are seven
independent state families: seven storage shapes, three registries
(global perception list, process-global interaction list, graph edge
list), two observer protocols, one callback-execution path. The intent-
flow axioms (vols. 3/5, instrumented by the slice, promoted by R1)
say the mechanism layer should instead be READS over one durable
structure: state is a fold at an address (N2), causality is the log's
own PROV, adjudication replaces callback execution, budget refusal
replaces after-the-fact counting (N4), and the open payload-type
registry (B2) is the routing table. R1 built and measured that durable
core. R2 is the step that proves the claim it was built for: that the
seven abstractions DERIVE from it — each either already expressible
as a query (and this ADR supplies the query), or honestly not
expressible (and this ADR says so, in the mapping table, rather than
pretending).

## Decision

### 1. The shape: a mechanism layer inside v5, additive, read-mostly

R2 adds a mechanism layer to `v5/`: six new modules
(`if5_estimate.c`, `if5_cause.c`, `if5_judge.c`, `if5_projection.c`,
`if5_process.c`, `if5_route.c`), a knowledge-cut extension of
`if5_cut.c`, and the containment-as-content change to `if5_tree.c` and
the core. The layer is **read-mostly**: five of the six modules are
pure queries over the durable core (S24's purity invariant extends to
them — test-enforced by durable-image memcmp, the same instrument
`test_v5_seats` uses). The two write paths are the process module's
phase transitions (which submit through the ordinary `if5_submit`
channel — no new log family for the samples, ever) and the route
module's compile step (which is sugar over `if5_submit`; M6 asserts
byte-equality with the direct path). The shipping API, the slice, and
the R1 differential surfaces are untouched; `planex_v5_lib` still never
links into `planex_lib`.

### 2. The seven mappings, argued

Each mapping below states: the shipping concept (as read from the
code above), the v5 construction, what is GAINED, and what is
HONESTLY NOT CARRIED. The "not carried" column is load-bearing: a
mapping that pretended completeness would be the silent-alteration
failure mode this tree exists to prevent, run in reverse.

**(a) Estimate as a fold (`if5_estimate.c`).** The shipping Estimate
is a mutable cell whose value, confidence, and trajectory live in the
cell. The v5 construction: an estimate is a QUERY at an address —
`if5_estimate_at(w, rid, view)` folds the region's contributing
records at the view and returns `{value, contributions, evidence,
last_iid}`, where `evidence` is an ORDINAL, not a probability:
3 = single chain, fully applied; 2 = concurrent work present and
stitched (merged); 1 = fork unresolved (two versions live at this
view); 0 = no contribution. Confidence-as-claimed-number was always a
caller-supplied prior; the fold's honest equivalent is this evidence
state, which the log can actually support. The trajectory maps to
`if5_estimate_trace`: the fold evaluated along a view sequence —
time becomes an addressable dimension instead of a field inside the
cell, and the shipping animation (from → to over duration) is the
two-sample special case. The derived estimates (Stage 3's
`px_derived_new`) need NO v5 counterpart construct: a region's fold
already depends only on its contributing records, so dependency
tracking is structural, not an add-on; `if5_estimate_sources` makes
the dependency set explicit (the contributor enumeration). NOT
CARRIED: caller-chosen confidence priors, and animation easing curves
(presentation, R3).

**(b) Relation as the cause-graph over the log (`if5_cause.c`).**
The shipping Relation is an asserted edge in a list. The v5
construction: relations the log can EVIDENCE become queries, and the
mapping table is explicit — DEPENDS_ON is the PROV edge
(`prov_a`/`prov_b` of a record: this change depended on those);
TRIGGERS is the causal-successor query (`if5_cause_effects`: which
records' clocks know this one); VARIES_WITH is the concurrency
enumeration (`if5_cause_concurrents`: records on the same region
whose clocks are incomparable); CONTAINS is the containment the tree
folds (Decision §3). NOT CARRIED — deliberately: BESIDE, AFFORDS,
WITHDRAWS_FOR, PRESENTS_FOR, INTERPRETS_AS have no causal semantics;
they are layout and presentation assertions, and carrying them into
the durable core would either log bookkeeping (N5 violation) or sneak
visual vocabulary past the S15 ban. They stay shipping-side metadata
forever, and this ADR says so on the record.

**(c) Closure as a judgment (`if5_judge.c`).** The shipping Closure
runs: goal → intent value → action callback → execution → evaluation.
The v5 construction: the closure's five stages become a READ of the
adjudicated record — `if5_judgment_of(w, iid, view)` returns the
goal's address (region name, payload family — the S15 node's own
semantic description), the intent (the record's value word), the
adjudication (verdict: APPLIED / MERGED / FORKED / ARBIT_LOST /
EXCLUDED, read from the flags the adjudicator already writes), the
submitter's observation point (`observed_cut`: what the submitter knew
when asking), and the evaluation (the fold at the view, against which
the goal's achievement is checkable). The deepest change is Action:
the shipping Action is a callback with arbitrary mutation power; the
v5 action is the TYPE-registered semilattice join (B2's `sq`/`le`) —
mutation is typed, not arbitrary, which is exactly vol. 5's "intent is
a value, not a function" taken to its consequence. NOT CARRIED:
arbitrary action callbacks (by design, not by omission — this is the
axiom table's own content decision), and goal STRINGS (the goal is
the address, not prose).

**(d) Perception as the projection family (`if5_projection.c`).**
The shipping Perception is a pure function in a global list, with
discovery and invoke-all. The v5 construction: a WORLD-scoped
projection registry (not process-global — the world is the unit),
`if5_projection_register` entering named projections through the same
open-channel discipline B2 established for payload types, with the
built-ins (semantic, set, history — the differential's frozen trio —
plus the tree and the schedule, text-rendered) registered through the
SAME channel at world creation (equality of standing, the registered
rule). `if5_projection_invoke` by name; `if5_projection_count`/`Name`
for discovery. The three frozen projections are WRAPPED, not modified:
M4 asserts the registry path is byte-identical to the direct call.
NOT CARRIED: pixel denotations (S15's contract keeps vision out of
the core; a renderer is a client of the tree, R3's question), and
estimate-subscription wiring (the fold is pull-based; push/pull is a
loop-policy question, see (e)).

**(e) px_loop's budget as N4's ledger (no new module — an argument
and a test).** The shipping budget is observed after the fact: the
loop records that an iteration exceeded `budget_ms`, counts overruns,
warns once; nothing is refused. The v5 mapping: the budget moves from
the loop object to the LEDGER (N4) and from counting to REFUSAL —
every agent-side initiative debits, an exhausted budget trips the
breaker and the submission is refused with a receipt. The audit
question px_loop's overrun counter answered ("was the feedback
timely?") is answered by the receipt stream instead: the breaker
receipt IS the overrun record, itemized (initiative vs surprise
units), PROV-anchored to the refusing moment, carrying the
rollback address. The loop itself — the closed cycle of intent →
adjudication → projection → next intent — needs no object in v5
because it needs no state: it is the application's own loop over
`if5_submit` and `if5_projection_invoke`, with the world as the
single coordination point. M7 pins the mapping's behavioral edge
(exhaustion refuses exactly once, receipts anchor, others unaffected).
NOT CARRIED: wall-clock timing (the ledger counts units, not
milliseconds — binding a unit to a millisecond is a deployment
calibration, an E-series protocol question, not a core question).

**(f) Intent compilation as the type registry's routing
(`if5_route.c`).** The shipping compile step resolves a physical hit
into a semantic payload and triggers. The v5 construction: compilation
is ROUTING THROUGH THE OPEN REGISTRY — `if5_route_register(ptype, fn)`
attaches a compile function (text → value word + submit options) to a
registered payload type; `if5_compile(w, src, rid, ptype, text, opts)`
routes, parses, and submits through the ordinary channel. Built-in
routes for NUM (decimal), SET (hex bit-set), STR (8-byte line) are
registered through the same public channel as user routes (B2's
equality-of-standing rule, again). The shipping step's spatial
resolution (which region did the hit land in) is a PRESENTATION
question — in v5 the caller names the region; hit-testing on visual
geometry cannot exist in a core with no visual fields. M6 asserts the
sugar property: the compiled record is byte-equal to the direct
submission of the same value. NOT CARRIED: geometric hit-testing
(client-side, forever, by S15's contract).

**(g) px_interaction as the adjudication surface (`if5_process.c`).**
The shipping interaction is a process with identity, trajectory, and
outcome, samples fanning out to nothing. The v5 construction:
`if5_process_begin(w, src, rid, ptype)` opens the process IN THE LOG
(a PROCESS record through the ordinary channel — identity is an iid,
so the process opening is itself adjudicated, undoable, and provable);
`if5_process_sample(p, value)` is a pure working-set update — zero
submissions, zero debits, zero log growth (the px invariant, kept and
test-enforced against the durable image); `if5_process_commit(p, v,
opts)` submits the outcome through the ordinary channel — the outcome
record's vector clock makes it a causal successor of the opening, so
the process's causal structure is structural, not asserted;
`if5_process_cancel(p)` closes the slot AND undoes the opening
(the cut): a cancelled process leaves the causal history the way an
undone change does, which is a STRONGER cancellation than the shipping
`cancel(reason)` (a callback with no history semantics). The outcome
record's process is recoverable by query: the most recent unexcluded
PROCESS record of the same source causally before it. NOT CARRIED:
persistent sample trajectories (samples are working-set, by the same
accountization that keeps region names working-set — registered as
Known issue #1 below), and the cancel reason string (the v5 cancel is
a cut; attaching a reason word would be a new record field — a
successor-ADR question, registered).

### 3. Known issue #1 discharged: containment becomes content

The tree's containment folds, and containment CHANGES become logged
intents. A new payload family `IF5_T_REPARENT` (value word:
`{child rid, new parent}`) is carried through the ordinary submit
channel; it is a structural record (excluded from the value fold, like
REGION and TIER), so region VALUES are untouched (M8 asserts value
invariance under reparenting). The tree walk at a view computes each
region's parent as the LAST APPLIED, view-live reparent record for
that region, falling back to the creation-time parent (which stays
working-set seed metadata, exactly like the region's name — seeds are
not content, changes are). The working-set `parent` field becomes a
cache of the live fold, the same way `region.cache` caches the folded
value. Consequences, each registered: reparent is UNDOABLE (the
ordinary view-exclusion path applies — an undone structural move
restores the prior tree, M8); reparent is PROV-carrying like every
record; the walk order stays deterministic (declaration order within
folded parent). The ptype enum gains `IF5_T_REPARENT = 12` and the
user-extension start moves to 13 — a v5-public-header change made
once, here, in a tree with no external users, rather than later when
it would be a compat break; the surface-list contract (ADR-0026
CAVEATS) is amended by exactly this paragraph. Regression duty: in a
world with NO reparent records the folded parent IS the working-set
parent, so the walk is byte-identical in shape to R1's — M8 asserts it
against an independent in-test reimplementation of the R1 order.

### 4. Known issue #2 discharged: the per-source knowledge cut

The per-source cut is constructed the sound way ADR-0026 named:
record identity, not clock comparison. `if5_kcut_of_source(w, sid,
prefix)` addresses the set "the causal down-closure of source S's
records up to `prefix`" — the world as S has made it matter. Membership
is a REACHABILITY test, not a stored list: `if5_kcut_contains(w, k,
iid)` is true iff `iid` can reach (along PROV edges, forward in
causality) some record of S at or below `prefix`. No clock comparison
anywhere, so the REBORN collision cannot bite; the represented set is
down-closed BY CONSTRUCTION (a record's causal predecessors are on
every path to it, hence also reach S's records), and `if5_kcut_is_down_closed`
verifies it independently (recursive predecessor check) for the test
battery. The cut carries the frontier (the join of member vcs) and
folds: `if5_kcut_state_at` evaluates a region over exactly the member
records. Scope honesty, registered: this is the DEPENDENCY knowledge
("what S's work stands on"), not the OBSERVATIONAL knowledge ("what S
happened to see pass by") — S's `observed_cut` glimpses that the
closure does not include are deliberately out of scope, because
"seen but never used" has no causal consequence and faking it would
re-import the unsound clock comparison through the back door. When
the world has a single source, the kcut degenerates to the prefix cut
and its fold equals `if5_now`'s (M9 asserts the degeneration); with
many sources it is strictly finer than any prefix cut, which is the
point. Storage: O(1) per cut (sid + prefix + frontier) — the identity
set is implicit in the log, never materialized, so no capacity cliff.

### 5. The differential discipline: V1's surface stays frozen; the M-series is the new gate

R1's differential compares the slice and v5 byte-for-byte on the
registered surface (the three projections, the black-box export, the
record word) over TRIP-17, 100 seeded random streams × 200 ops, and
the E2 shape. R2 does not touch that surface: the three frozen
projections are wrapped by the registry but produce identical bytes
(M4 re-asserts on the registry path), the record word changes by zero
bits (REPARENT and PROCESS are new ptype VALUES, not new fields; the
48-byte density holds — V8's assert stays live), and the R1 random
streams never submit the new families so the historical bytes are
unchanged. The mechanism layer gets its OWN gate, the M-series
(pre-registered below), whose shape follows the series' discipline:
every M is a machine-checkable engineering statement, checked against
an INDEPENDENT implementation in the test (a second way to compute
the same thing), never against itself.

### 6. Wiring

`v5/Makefile` grows the six modules and two test binaries
(`test_v5_mech`, `test_v5_structure`); the root `CMakeLists.txt` grows
the sources into `planex_v5_lib` and the two suites as ctest targets
(never linked into `planex_lib`); `ci.yml`'s make jobs run them via
`make -C v5 test`, the cmake jobs via ctest. The rebound gate's v5
section extends to the new modules with the same four forbidden
patterns plus the S15 visual-field ban, and gains one mechanism-layer
ban of its own with a synthetic bite: `if5_process_sample`'s body may
not call `if5_submit`, `if5__append`, or `if5__post_debit` (samples
stay pure — the px invariant, now statically enforced). TESTING.md and
the changelog record the layer. The four-column gate applies (this
is ADR-0027 ≥ 0024).

### 7. R3 and beyond — registered, not executed

R3, when it is decided, is the presentation-and-persistence stage:
the client-side rendering of the semantic tree (the S15 "vision is
one projection" clause, executed as a CLIENT), sample-trajectory
persistence if a measured need ever appears, the named-address
persistence the cut registry defers, and the preparation work for
the E-series human legs that only the maintainer can commission.
Nothing in this ADR starts R3.

## Essence Check

### Q1. Which essence axis does this decision affect?

The intent axis, extended to the mechanism layer: R1 changed the
SUBSTRATE the axioms sit on; R2 removes the seven parallel state
families and derives their semantics from one durable structure. The
compression is structural: one log, one fold, one adjudicator, one
registry — where the shipping tree has seven storages and three
registries.

### Q2. Does it compress or increase human cognitive bandwidth?

Unmeasured, and registered as such — the same boundary ADR-0025
(CAVEATS) and ADR-0026 (Essence Q2) drew. The M-series says the
derivation is FAITHFUL and AFFORDABLE; whether it HELPS anyone is
what E1b/E2-experimental/E4 exist to measure, on the instrument,
with participants. What can be said on engineering evidence alone:
the mechanism layer's concepts are fewer than the shipping layer's
(one fold family vs. cells-plus-observers-plus-trajectories;
one PROV graph vs. an asserted edge list; one ledger vs. overrun
counters), and every mechanism query is replayable at any address —
but concept-count is not human bandwidth, and this ADR does not
claim it is.

### Q3. Is there a gap between the claim and the implementation?

The gap this ADR admits and structures: the mappings' NOT-CARRIED
columns are the gap, stated per abstraction (confidence priors,
easing curves, five relation kinds, action callbacks, pixel
denotations, wall-clock budgets, geometric hit-testing, sample
persistence, cancel reasons). Each is either presentation (R3,
client-side) or an axiom-level content decision the court already
made. Nothing in the NOT-CARRIED columns is silently dropped: the
table above is the contract.

### Q4. What is the cost, and who can verify it?

~3k lines of v5 additions, two test suites (~1.3k lines), CI minutes
(two more v5 binaries, same job count). Verification is public and
mechanical: `make -C v5 test`, `make -C v5 rebound-check`, ctest,
ASAN/UBSan, the -Wconversion family on the new sources, and the
M-series assertions themselves — every one runnable from a fresh
clone, none requiring trust in this document.

### Q5. What are the counterexamples?

The honest ones, pre-registered: (1) an application that genuinely
needs caller-supplied confidence priors (Bayesian UIs) cannot derive
them from the evidence ordinal — it must carry them client-side;
(2) an application that needs asserted-by-convention relations
(BESIDE as a layout hint) must keep a side table — the core will not
store what the log cannot evidence; (3) an application that needs
millisecond budget enforcement must calibrate units to time at the
deployment site — the ledger refuses units, not milliseconds. Each
counterexample marks a real boundary of the axiom table, not a
failure of this derivation — and each is exactly the kind of boundary
the E-series legs are registered to probe.

## Consequences

### Positive

- The seven abstractions stop being seven things to keep consistent;
  consistency becomes the fold's job, and the fold is already
  differential-gated.
- The mechanism layer is replayable and addressable for free: any
  mechanism query runs at any view (the tree, the schedule, the
  estimates, the judgments, the cause-graph — all take views).
- Cancellation gains causal semantics (the cut), which the shipping
  interaction never had.
- The two ADR-0026 Known issues land with tests pinning them shut.
- The projection registry makes the S15/S17 seats extensible the way
  B2 made payloads extensible — one discipline, applied twice.

### Negative

- Two new payload families (REPARENT, PROCESS) grow the enum and the
  structural-record set; the user-extension start moves 12 → 13
  (once, now, in a tree with no external users).
- The mechanism layer adds ~3k lines to review before R3 can start.
- Confidence becomes an ordinal, which is LESS expressive than the
  shipping float — by design, but the cost is real for Bayesian
  clients.
- The per-source kcut is O(log) per membership test in this
  implementation (reachability walk), not O(1); fine at registered
  scales (n = 2000), registered as a cost, not hidden.

### Neutral

- The ptype enum change is a v5-public-header amendment carried by
  this ADR's own text (Decision §3) — the surface-list contract
  self-amends exactly once, on the record.
- px_loop has no v5 object at all; applications own their loops.
  This is a shape difference, not a removal: the loop's audit duties
  moved to the receipt stream, its budget to the ledger.

## Alternatives Considered

### Alternative 1: Re-derive the mechanism layer as v5 copies of the shipping APIs

Copy `px_estimate`/`px_relation`/... into `v5/` with `if5_` prefixes,
backed by the core. Rejected: that is PORTING, not deriving — it
would preserve the seven state families and add an eighth thing (the
core) under them, the opposite of the claim being tested. The value
of R2 is precisely that the abstractions come out as READS; a copy
would make the differential trivially pass and the derivation
vacuous.

### Alternative 2: Skip the mechanism layer; go straight to R3 (presentation)

Argument for: the E-series legs run on the SLICE, not on v5, so v5's
mechanism layer is not on any measured critical path. Rejected: R1
without R2 is a core with no clients — the derivation's central
claim ("the seven abstractions derive from the durable core") would
stand untested forever, and ADR-0026 §7 registered R2 as the next
decision precisely because the claim is the point. Deferring it
indefinitely would be deciding by fatigue, which the series'
discipline forbids.

### Alternative 3: Argue the mappings on paper, land no code

A mapping table in an ADR, no modules. Rejected for the series'
registered reason: vol. 6 §6.2's instrument-honor sentence — the
axioms get a chance to die. An unimplemented mapping cannot fail;
paper derivations are how the reproducibility claim died unnoticed
(vol. 4's own autopsy). The M-series exists so the mappings can fail.

### Alternative 4: Also discharge the cut-registry persistence item

ADR-0026 CAVEATS tracks named-address persistence here too, but it
touches the black-box format — a REGISTERED SURFACE. Rejected for
this ADR: changing a registered surface mid-tree without a
differential-driven need would spend the differential's authority on
a convenience. The kcut (Decision §4) gives E1b's audit family the
knowledge addresses it actually needs, at O(1) storage, without
touching the export format. Persistence stays tracked, unspent.

## CAVEATS

- The M-series is engineering-falsifiable ONLY. Nothing here is a
  human-subject claim; the promotion gate (ADR-0026 §5) is untouched
  and remains the maintainer's act.
- The mapping table (Decision §2) is the contract. A later change
  that quietly widens a NOT-CARRIED column into the core (e.g.,
  logging BESIDE edges as records) is a N5 violation and a
  silent-alteration of this decision — the rebound gate's pattern
  list is not exhaustive against it; the ADR text is the guard.
- The evidence ordinal is not a probability and must not be read as
  one. Any consumer treating IF5_CONF_* as Bayesian confidence is
  misusing the API; the header says so.
- The kcut's membership test is a reachability walk over the log —
  its cost grows with log length. At the registered scales this is
  noise (measured in M9's runtime); at 100× scale it would want an
  index, which would be a successor-ADR performance change.
- if5_process slots are working-set; a process open at world free is
  simply gone (the OPENING record stays in the log — the record is
  durable, the slot is not). Known issue #1 below tracks the sample
  trajectory's non-persistence.

## Known issues

- **Issue:** the process sample trajectory is working-set state and
  does not survive world free; only the begin and commit records are
  durable.
- **Why accepted:** persisting samples would be a new log family for
  non-semantic data — the exact N5 violation the accountization
  discipline forbids, and the px_interaction invariant (samples fan
  out to nothing) is the shipping tree's own recognition of the same
  thing. If a measured need appears (e.g., an E5 leg wanting
  trajectory analysis), it re-registers through vol. 6 §9.1's
  protocol-revision channel first.
- **Tracking:** R3's successor ADR, or a protocol revision that
  creates the need; whichever comes first.

- **Issue:** the cancel path carries no reason word — v5's cancel is
  a cut, and the record word has no free field for prose.
- **Why accepted:** the 48-byte density is registered (V8) and the
  differential's authority depends on it; a reason word is a new
  field, which is a record-format change requiring its own
  registration and surface-list amendment. The need is hypothetical
  until an E-series leg asks for cancellation analytics.
- **Tracking:** none — re-registers through a successor ADR if a
  measured need appears.

- **Issue:** the kcut expresses dependency knowledge, not
  observational knowledge (Decision §4's scope honesty); "what S saw
  but never used" is not addressable.
- **Why accepted:** the observational reading requires either clock
  comparison (unsound under REBORN, ADR-0025's documented collision)
  or per-record observation sets (a record-format change). The
  dependency reading is the one with causal consequences and is
  sound with zero format change.
- **Tracking:** the first E1b pilot that needs observational
  addresses; it re-registers with its own format proposal.

## Refutation Criteria

- **Any M-series prediction fails on the landed code** → this
  decision's central claim (the seven abstractions derive from the
  durable core, faithfully) is falsified in the specific mapping the
  M covers; the build is red until the mapping is fixed or the
  failure is carried as an explicitly registered deviation in this
  file, on the record, with its cause.
- **The R1 differential surface drifts by one byte** (the three
  projections, the black-box export, the record word, over TRIP-17 +
  the 100 streams + the E2 shape) → the registry wrap, the ptype
  additions, or the tree's folded containment altered what R1
  registered; this is the silent-alteration failure the whole tree
  exists to prevent, and it voids the stage until fixed.
- **A mechanism query mutates the durable layer** (S24 purity
  violated by any of the five query modules) → the layer's
  read-mostly contract is broken; red until repaired.
- **The rebound gate's new sample-purity ban fails to bite its
  synthetic violation** → the gate is decorative exactly where this
  ADR relies on it; red until the gate's bite is restored.

## Regression Clause

If this decision is later superseded or reversed, the v5 tree does
not auto-delete (the ADR-0026 precedent): the mechanism modules may
be removed tree-locally, the ptype enum amendment must be re-derived
by whatever replaces the mechanism layer (removing REPARENT/PROCESS
values changes the user-extension start again — a breaking change
requiring its own registration), and the R1 differential must be
re-run green on whatever remains. The shipping API, the slice, and
the measurements are not affected by any outcome here — they were
never touched.

## Validation

*(To be filled with measured gate outputs when R2 lands — the
three-beat discipline: this ADR registers before the mechanism-layer
sources exist; the implementation lands; the outputs below are then
transcribed from real runs, never asserted. Placeholder sections
that survive to merge are a four-column violation.)*

- Estimate fold agreement + evidence ordinals (M1): pending.
- Cause-graph exactness + reversal + concurrency + reachability (M2): pending.
- Judgment totality + verdict consistency (M3): pending.
- Projection registry byte-equality with the frozen trio (M4): pending.
- Process discipline: log growth, sample purity, causal succession,
  cancel-as-cut (M5): pending.
- Routing equivalence with direct submission (M6): pending.
- Budget mapping: refusal exactness + receipt anchoring (M7): pending.
- Reparent: fold semantics, undo, R1-order regression (M8): pending.
- Knowledge cut: down-closure, degeneration, frontier (M9): pending.
- R1 differential re-run (V1 surface, byte-exact): pending.

## HISTORY

- 2026-11-01 — Registered as Proposed before any R2 mechanism source
  existed. Inputs: ADR-0026 §7's registration and its two tracked
  Known issues; R1's landed state (main `10d1781`, V1–V9 PASS, CI
  13/13 fresh-clone); the shipping-side code survey of the seven
  abstractions (file/line citations in Context); vols. 3/5 (axiom
  table), vol. 6 §6.2/§6.3 (instrument honor, control-arm freeze),
  vol. 7 (S15/S17/S24 constructive obligations).

## References

- `docs/decisions/proposed/ADR-0026-v5-intent-flow-derivation.md` —
  the decision this stage executes under; §7 registered R2, the
  Known issues section tracked both discharges to this file.
- `docs/decisions/proposed/ADR-0025-intent-flow-vertical-slice.md` —
  the instrument; its design-fix register (REBORN exemption,
  living-knowledge rule) is the semantics the kcut must not violate.
- `docs/decisions/proposed/ADR-0024-*` — the four-column discipline
  this file obeys.
- Vol. 5 (`意图流语义草案`) ch. 3.3 (B2 open channel), ch. 5 (N2
  time-addressability, §5.1 cut algebra), ch. 6 (N3 tiers), ch. 8
  (N5 accountization) — the axiom provisions the mappings derive
  from.
- Vol. 4 (`矿脉图`) seeds S15/S17/S24 — the seats the projection
  family and tree must keep honoring.
- `src/estimate.c`, `src/relation.c`, `src/closure.c`,
  `src/perception.c`, `src/feedback.c`, `src/hit.c`,
  `src/interaction.c` — the shipping mechanisms being re-derived,
  cited as surveyed (line counts in Context).
