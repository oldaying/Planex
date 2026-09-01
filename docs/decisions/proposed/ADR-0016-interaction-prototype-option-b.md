# ADR-0016: Interaction prototype — continuous interaction process (Option B)

## Status

Proposed

Date: 2026-08-29

## Context

ADR-0006 deferred the continuous-interaction abstraction to v1.0+ and
mandated an evidence-gathering protocol: write a boundary-exposing demo
(`hover_drag_4abs.c`), measure the pain, decide from evidence. The demo
landed and its printed verdict reads:

> "Hover: WORKABLE but wasteful ... Drag: WORKABLE but semantically
> wrong ... Gesture: NOT POSSIBLE ... The hacks are tolerable for
> simple hover/drag. They would be INTOLERABLE for complex
> gesture/touch UIs."

That is the A→B trigger from
[continuous-intent-speculation.md](../../concepts/speculation/continuous-intent-speculation.md)'s
decision criteria: the "process" gap is real and pain-causing. The
pattern matrix agrees — [ui-pattern-coverage.md](../../concepts/state/ui-pattern-coverage.md)
Category D stands at 0/15 clean (12 forced, 3 impossible), the single
largest gap in the 68-pattern corpus.

Three audits (two external, one internal) converge on the same root
cause: the five canonical abstractions model discrete intents
(Closure) and persistent state (Estimate) but have no word for *an
ongoing process with an outcome*. The audits additionally flagged the
adjacent gap — intent compilation: `on_click(ev.x, ev.y)` raw-coordinate
dispatch outsources the physical→semantic translation to every
application (Norman's execution-gulf "translation segment").

This ADR proposes **Option B** from the speculation document: a
prototype that covers "process" (not yet "intent gradient"), plus a
data-level affordance query for intent compilation — both landing as
**prototype code**, exactly like the v3 prototype section
(ADR-0009) did for Interpretant/Perlocution/Breakdown. The 5 canonical
abstractions are untouched.

## Decision

Implement, as a **v0.6 prototype** (not canonical):

1. **`px_interaction`** (`src/interaction.c`) — a continuous
   interaction process: `begin → sample* → commit | cancel`. A bounded
   ring retains the trajectory; pure queries derive duration,
   displacement, path length, velocity. Three bridges connect it to the
   canonical abstractions:
   - `px_interaction_on_phase` — the process→intent compilation hook
     (fires at begin/commit/cancel only, never per sample);
   - `px_interaction_on_commit/on_cancel` — the process resolves to a
     Closure trigger (discrete speech act) at outcome time;
   - `px_interaction_publish_phase` — phase published to an Estimate at
     transitions only; this is the ONLY sanctioned
     interaction→Estimate seam.

   **The design invariant**: samples never notify observers and never
   auto-invoke perceptions. The 60Hz hot path is inert — this is what
   makes a process NOT a state, and it is test-locked
   (test_v06_interaction.c section D).

2. **`px_region` + `px_afford_at`** (`src/hit.c`) — intent compilation
   as a graph query. A region is pure geometry data living as the `a`
   node of a `PX_REL_AFFORDS` edge; `px_afford_at(g, x, y)` answers
   "which affordance contains this point?" No new abstraction — it is
   a reader over the existing Relation graph.

3. **`PX_EV_WHEEL`** (`include/planex/window.h`) — the missing scroll
   event kind, wired through the X11 (Button4/5), Win32
   (WM_MOUSEWHEEL), and Cocoa (scrollWheel) backends, plus
   `on_wheel` in the app descriptor. Scroll patterns (corpus #34, #38)
   move from "cannot express input" to expressible.

4. **Evidence demo** — `examples/hover_drag_interaction.c` implements
   the same list-reorder scenario as `hover_drag_4abs.c` and prints
   the side-by-side metrics (130 mouse events → 2 estimate writes;
   hook fired 2× for 62 drag events; cancel first-class; gesture
   derivable from measures).

Prototype status is encoded in the header (a "v0.6 PROTOTYPE" section,
mirroring the "v3 PROTOTYPE" precedent) and in this ADR's lifecycle:
**Proposed**, not Accepted. Promotion to a canonical 6th abstraction
requires its own ADR with the full admission bar (ADR-0011), evidence
from real applications, and a ui-pattern-coverage.md re-scoring of
Category D.

## Essence Check

### Q1. Which essence axis does this decision affect?

The **feedback/interaction axis** at its continuous edge: A4's
"instantly visible, incremental" (Shneiderman's direct manipulation)
presupposes a process unit that the discrete Closure cannot express.
Traditions sampled for the abstraction's justification:
**Garnet Interactors** (Brad Myers 1990 — the interaction-technique-as-object
lineage), **CSP** (Hoare — a process is prefix + choice:
`begin → P'`, terminate as commit ⊕ cancel), **statecharts** (Harel —
the "do action" of ongoing activity that pure transition models lack),
**FRP** (Elliott — Behavior covers continuous *values*, not bounded
processes with *outcomes*), and **direct manipulation** (Shneiderman
1983 — "continuous representation, incremental actions": the process IS
the unit). The region/affordance half leans on **Gibson**'s affordance
via the existing PX_REL_AFFORDS vocabulary — hit-testing is an
affordance query, which is why it needs no new abstraction.

### Q2. Does it compress or increase human cognitive bandwidth?

For continuous-interaction apps: **compresses**. The old encoding
spent ~20-30 lines of HACK per pattern and paid observer fan-out on
every mouse move; the new encoding is one process object + one hook,
with the hot path inert. For apps that never leave discrete intent:
**zero cost** — the prototype is opt-in and the canonical five are
unchanged. Net: compresses.

### Q3. Is there a gap between the claim and the implementation?

The claim is precisely scoped: "a prototype exists, evidence-gathering
continues". It does NOT claim Category D is solved (12/15 patterns
still need re-scoring against the prototype), does NOT claim touch or
multi-pointer support (still NG-6), and does NOT promote the
abstraction count (docs must keep saying 5 canonical + 1 prototype).
`hover_drag_interaction.c` + `test_v06_interaction.c` are the
implementation-side evidence, and both ship in this change.

### Q4. What is the cost, and who can verify it?

Cost: ~600 LOC of prototype (interaction.c 300 + hit.c 150 + tests
~400 + example ~350), plus the cognitive cost of a 6th concept in the
header. Verifier: anyone can run `make test_v06` (27 assertions) and
`./build/hover_drag_interaction` and compare against
`hover_drag_4abs.c`'s printed metrics. Falsifiable: if real apps find
the bridges insufficient (e.g. they need per-sample perception),
Section D's invariant tests will be the thing to break first.

### Q5. What are the counterexamples?

- An app with only buttons/forms (discrete intents): the prototype is
  dead weight — but it costs nothing unless linked in (it is, today,
  always linked; see Known issues).
- A scrubbing UI needing sub-frame perception of the *sample stream*:
  the invariant forbids per-sample perception auto-invoke; such an app
  reads the sample in its render function instead (as
  hover_drag_interaction.c does) — workable, but the tension is real
  and recorded.
- Multi-touch (two concurrent processes on one surface): expressible
  as two px_interaction objects, but no pointer-routing layer exists
  yet to feed them correctly.

## Consequences

### Positive

- Category D's "process" half has a first-class expression path; the
  5 canonical abstractions are untouched (prototype protocol).
- The intent-compilation gap (raw coordinates → closures) closes with
  a Relation-native query instead of a new abstraction.
- Scroll becomes an input event on all three real backends.
- The ADR-0006 evidence protocol completes: boundary-exposing demo →
  prototype → measurable comparison → future promotion decision.

### Negative

- The header grows a second prototype section; readers must now
  distinguish canonical (5) from prototype (2 concepts) — a
  documentation burden paid in exchange for evidence.
- interaction.c/hit.c are always compiled into the core library even
  for apps that never use them (~450 LOC of dead weight until the
  promotion decision; a build flag can carve them out later).
- The per-sample-inertness invariant creates a real tension with
  "perceive the stream every frame" apps: they must read samples
  inside their perception functions via user data, which is less
  uniform than Estimate inputs.
- The void* graph means px_afford_at cannot type-check that an AFFORDS
  target is really a closure — a reversed declaration returns the
  region itself (documented; inherent L1 of the Relation design).

### Neutral

- ui-pattern-coverage.md is NOT re-scored in this change (it is a
  v0.4 snapshot file, exempt from freshness gates); re-scoring
  Category D against the prototype is the promotion ADR's job.

## Alternatives Considered

### Alternative 1: Absorb into Estimate (Conal-style Behavior)

Extend Estimate with a "stream mode" (mouse position as
`Behavior<Position>`, sampled at read time). Rejected in ADR-0006
(same reasoning holds): Estimate's semantics are "state with time +
uncertainty"; conflating a transient stream with persistent state is
the exact semantic erasure the anti-pattern tests guard against.
The prototype keeps the boundary explicit: the ONLY seam is
publish-at-transitions.

### Alternative 2: Absorb into Closure (PX_INTENT_CONTINUOUS)

Add a continuous intent kind or a "process Closure" spanning frames.
Rejected in ADR-0006: Closure models discrete speech acts
(Austin/Searle/Winograd-Flores); a 60-frame drag is not a locution.
The prototype instead makes the process RESOLVE to a Closure at
outcome time, preserving the speech-act model at the seam where it is
actually correct (the commit).

### Alternative 3: Full continuous-intent (Option C now)

Implement the intent-gradient abstraction (strength 0→1, preparation/
decision/execution/retraction phases) immediately. Rejected: the
speculation document's own decision criteria require evidence that
"process" alone is insufficient before graduating to "gradient";
Option C also breaks the existing Closure API. This ADR is the B
step; C remains a future decision gated on real usage.

### Alternative 4: Do nothing until v1.0

Keep the ADR-0006 deferral unchanged. Rejected because the deferral's
own evidence protocol has completed and returned a positive verdict
("INTOLERABLE for complex gesture/touch UIs"); sitting on it would be
process for its own sake. The prototype path lets evidence accumulate
without destabilizing the canonical set.

## CAVEATS

This ADR does NOT:

- Promote anything to a canonical abstraction. The count remains 5 +
  prototype. Promotion requires a separate ADR passing ADR-0011's
  admission gate (tradition, orthogonality, denotational semantics)
  plus a re-scored Category D.
- Claim touch or gesture *input* support. PX_EV still has no touch
  events; NG-6 (no mobile target) stands. The prototype makes
  gestures *derivable from trajectories* — a recognizer stack is
  future work.
- Address the "intent gradient" (Option C). Strength-as-value,
  preparation/retraction windows remain speculative.
- Change any canonical API signature, test, or doc claim. The 33/13/12
  existing test counts are untouched; test_v06 is purely additive.
- Solve multi-pointer routing. Two fingers = two interactions is
  expressible, but no pointer-routing layer feeds them yet.

## Known issues

- **Issue**: px_afford_at cannot verify that an AFFORDS edge target is
  a closure (void* type erasure, documented L1 of Relation). A
  reversed declaration `(closure AFFORDS region)` makes the query
  return the region.
- **Why accepted**: inherent to the graph's void* design; documented
  at the API; the alternative (type tags in the graph) is an L2-level
  redesign deferred to the promotion ADR.
- **Mitigation**: declare AFFORDS as (region, closure) — the examples
  establish the convention.

- **Issue**: interaction.c/hit.c compile into every link of libplanex
  even when unused.
- **Why accepted**: prototype-stage simplicity; the core is small
  (~450 LOC).
- **Mitigation**: if the promotion ADR rejects the abstraction, the
  files move to examples/; if it accepts, they become canonical
  anyway. A compile-time carve-out flag can be added on demand.

- **Issue**: per-sample perception is forbidden by design; apps that
  need frame-accurate stream rendering must read samples inside their
  perception functions via user pointers.
- **Why accepted**: the invariant is the whole point — it is what
  kills the 60Hz observer fan-out. The read-in-perception pattern is
  demonstrated in hover_drag_interaction.c.
- **Tracking**: if real apps hit this wall repeatedly, that is
  evidence for the promotion ADR to reconsider the invariant.

## HISTORY

- 2026-08-29: Proposed (prototype implemented: interaction.c, hit.c,
  test_v06_interaction.c 27/27, hover_drag_interaction.c demo, PX_EV_WHEEL
  on all backends)
- 2026-08-30: The promotion gates this ADR deliberately deferred were
  closed by [ADR-0017](../accepted/ADR-0017-intent-compilation-promotion.md)
  (intent compilation, the 6th) and
  [ADR-0018](../accepted/ADR-0018-interaction-process-promotion.md)
  (px_interaction, the 7th). This ADR stays proposed as the historical
  record of the prototype protocol (ADR-0022's reference note; the
  ADR-0009 precedent).

## References

- [ADR-0006](../accepted/ADR-0006-continuous-interaction-deferred.md) — the deferral
  this ADR executes the evidence protocol of
- [Continuous Intent Speculation](../../concepts/speculation/continuous-intent-speculation.md) —
  Option B definition + decision criteria
- [UI Pattern Coverage](../../concepts/state/ui-pattern-coverage.md) —
  Category D 0/15 (v0.4 snapshot)
- [ADR-0009](../proposed/ADR-0009-essence-rederivation-v3.md) — the
  v3 prototype precedent this follows
- [ADR-0011](../accepted/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) —
  the admission bar any promotion must pass
- `examples/hover_drag_4abs.c` — the boundary-exposing demo (evidence in)
- `examples/hover_drag_interaction.c` — the boundary-closing demo
  (evidence out)
- `tests/test_v06_interaction.c` — 27 assertions incl. the
  inertness invariant (section D)
- External: Brad Myers, "A New Model for Handling Input" (ACM TOIS
  1990, Garnet Interactors); Hoare, *Communicating Sequential
  Processes* (1985); Harel, "Statecharts: A Visual Formalism" (1987);
  Shneiderman, "Direct Manipulation" (IEEE Computer 16(8), 1983);
  Elliott & Hudak, "Functional Reactive Animation" (ICFP 1997)
