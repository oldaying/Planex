# ADR-0023: The semantic-drift probe — mid-flight reinterpretation as the B→C stress test

## Status

Proposed

Date: 2026-10-31

## Context

The [continuous-intent speculation](../../concepts/speculation/continuous-intent-speculation.md)
names the criterion for graduating from Option B (the landed process
abstraction, `px_interaction`, [ADR-0018](../accepted/ADR-0018-interaction-process-promotion.md))
to Option C (intent gradient): *"evidence shows that 'process' alone
isn't enough."* The speculation's five scenarios (destructive hold,
form readiness, fuzzy drop, progressive search, scrubbing speed) all
stress **strength** — how much intent — and none stresses
**reinterpretation**: the gesture's *meaning* changing while the
trajectory continues.

That second axis is untested in this repository, and it is the one
where the classical event model pays its clearest structural cost:

- In the callback model, semantics is bound at **dispatch time** —
  the handler *is* the semantics. A mid-stream change of meaning is
  expressed either by a mode flag consulted inside every handler
  (a state machine threaded through all of them) or by
  unregister/re-register (teardown). Myers' Interactors (1990) made
  the technique an object but bound it at start-interaction time;
  in Harel's statecharts the "do action" belongs to a *state*, so a
  semantic change is a transition and the old state's activity ends.
  Neither tradition has a word for "same trajectory, new meaning."
- In Planex, `px_interaction` separates the **trajectory**
  (semantic-free values: samples, measures) from the
  **interpretation** (the three bridges: the phase hook, the
  commit/cancel closure bindings, the phase estimate). That
  separation makes a prediction: **drift is a bridge operation, not
  a process death.** Reinterpreting a running process should be
  repointing the interpretation while the trajectory continues —
  no teardown, no anchor reset, no trajectory amputation.

If that prediction is wrong — if expressing drift forces a new API
seam, or a process kill-and-rebirth, or per-sample estimate writes —
then the 7-abstraction set has a real boundary here, and this is
exactly the evidence the B→C criterion (or an eighth-abstraction
ADR) requires. Either way the probe pays for itself: it converts an
untested claim into a measured one.

## Decision

Build **one probe, not one API**: `examples/semantic_drift_probe.c`
plus a `[K]` section in `tests/test_v06_interaction.c`. No framework
code changes; the probe compiles against the public header as it
ships. The probe implements one deterministic scenario twice, side
by side, in the same process:

**The scenario (designed, not borrowed from any specific product):**
a five-item list with a trash margin below it. Press on an item and
drag within the list — the gesture *is* a reorder. Cross into the
margin — the gesture *becomes* a trash drag. Release over the trash
region — the item is deleted. One continuous trajectory, two
semantics over time, two committed effects. Crossing the boundary
commits the list-side effect (the reorder to the slot held at
crossing — an app policy choice the probe makes visible: *where*
effects resolve is policy; the framework provides the seams). A
fifth segment exercises ESC after the drift: cancel must kill only
the unresolved remainder, leaving the committed crossing effect
undoable, not cancelled.

**Encoding A — bridge repointing (the literal claim):** at the
drift boundary the app calls `px_interaction_on_commit` on the
*live* process, rebinding the commit closure and payload; the
trajectory ring is never reset.

**Encoding B — hook compilation (the denotational reading):** the
phase hook compiles the outcome payload at COMMITTED time from the
trajectory plus an app-side semantic-mode field switched at the
boundary; the interaction's own bindings are never touched.

**The classical control:** the same script through the event-model
idiom — a mode enum consulted in every handler, an anchor variable
reset at the drift point, and a manual undo list — so the
differences are measured, not asserted.

### Predictions (registered before implementation)

These were written before the probe's first compile. Each is
falsifiable by the probe's own output:

- **P1 — API closure.** Both encodings compile against the public
  header; zero new framework symbols, zero core edits. *Falsified
  if:* expressing drift requires any addition to `interaction.c`,
  `planex.h`, or the other six abstractions.
- **P2 — trajectory continuity.** Across the drift point,
  `px_interaction_total` is monotone, the stored ring spans both
  segments, and the measures (path length, duration, displacement)
  treat the gesture as ONE trajectory. *Falsified if:* either
  encoding requires `px_interaction_reset` or free-and-recreate at
  the boundary.
- **P3 — undo granularity.** One gesture with two committed
  semantics produces TWO registry entries (`px_undo_count() == 2`),
  independently restorable in either order. *Falsified if:* the
  encoding must collapse them into one step or push them outside
  the undo registry.
- **P4 — THE INVARIANT survives the drift.** Zero estimate writes
  per sample, including across the boundary; live feedback during
  drift is derived at render time from the trajectory; the semantic
  world is touched at transitions only. *Falsified if:* expressing
  drift demands per-sample estimate writes or observer fan-out.
- **P5 — the classical control's cost (measured, not pass/fail).**
  The control pays a mode-flag consult in every handler, an anchor
  reset at drift (amputating the pre-drift samples), and hand-rolled
  undo ordering. The probe prints the counts.

### Verdict rule

- **P1–P4 all hold:** the B→C criterion does not trigger on the
  reinterpretation axis. Option C's remaining motivation narrows to
  strength/gradient, which this probe does not test. The ADR
  records the narrowing and stays a research record.
- **Any of P1–P4 fails:** the probe becomes boundary-EXPOSING
  evidence — the `hover_drag_4abs.c` role for a gap nobody had
  measured — and this ADR flips to recording the gap as input to
  the next abstraction decision.

### Measured outcome (filled after the probe's first run; the
predictions above were registered first)

- **P1 — PASS.** Both encodings compiled against the shipping
  header; the change adds an example, a test section, docs, and
  nothing else — zero framework edits.
- **P2 — PASS.** 37 samples, one trajectory; 21 pre-drift samples
  still queryable after the drift (`px_interaction_at`), path
  length 180px reading across the seam.
- **P3 — PASS.** `px_undo_count() == 2` after one gesture;
  `px_undo()` peels the trash, the second peels the reorder,
  original order restored. The ESC-after-drift segment: cancel
  leaves exactly the crossing-committed step (`px_undo_count() ==
  1`) — cancel is scoped to the unresolved remainder.
- **P4 — PASS.** 0 estimate writes during samples, drift included
  (the probe counts writes inside a sample window around every
  `px_interaction_sample` call); 3 writes total, all at
  transitions (2 at the crossing commit, 1 at release).
- **P5 — measured.** The classical control pays 37 mode consults
  (one per handler entry), one anchor reset that amputates all 21
  pre-drift samples (15 retained, 36px of the 180px path), and a
  hand-rolled undo stack. Encoding A pays 1 bridge rebind;
  encoding B pays 1 mode-field switch consulted once at the
  outcome.
- **Verdict:** the first branch fires — the B→C criterion does
  not trigger on the reinterpretation axis. Recorded, and scoped:
  strength/gradient remains untested.

## Essence Check

### Q1. Which essence axis does this decision affect?

- [x] **Intent space** — the user→machine direction: whether the
  intent carried by an in-flight process can be reinterpreted
  without killing the process. This is the axis the speculation
  document's Option C guards.
- [ ] State space — explicitly not: P4 guards the process/state
  boundary against exactly this conflation.
- [ ] Semantic interface — the bridges under test are the existing
  ones; no encoding/decoding surface changes.

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions:** if P1–P4 hold, "what the user is doing" survives
a change of "what it means" — the app author writes one rebind (or
one mode-field switch) instead of a mode machine threaded through
every handler; undo history preserves both readings of one gesture
without manual bookkeeping; post-hoc analysis (the trajectory ring)
can answer "what did the user actually do?" across the boundary.

**Increases:** the two-encoding probe itself is new reading surface
(an example, one ADR); the crossing-commit policy is a decision the
app must now make explicitly where the classical model hides it
inside handler structure.

**Net assessment:** Compresses, conditionally on the verdict — the
measurement is the point.

### Q3. Is there a gap between the claim and the implementation?

- **Claim:** "a continuous interaction process with identity,
  trajectory, and outcome" whose interpretation is repointable
  (this ADR's Context prediction).
- **Implementation:** the three bridges exist and are mutable at
  any time (`px_interaction_on_commit` overwrites its binding,
  copying the new payload; the hook is a field); nothing in
  `interaction.c` forbids or supports mid-flight rebinding today —
  it is simply unexercised.
- **Gap:** the gap IS the experiment. This ADR exists to convert
  "unexercised" into "measured."

### Q4. What is the cost, and who can verify it?

- **Cost:** one example (~400 LOC) and one test section; the
  cognitive cost of a second evidence demo in `examples/`. No
  framework cost — that is P1, enforced by the probe compiling
  against the unchanged header.
- **Who can verify:** anyone. `make check-examples` runs the probe
  against its `.expected` snapshot (byte-identical); `make test_v06`
  runs the `[K]` section; the predictions are printed next to the
  measurements in the probe's output, so a reader can check P1–P5
  against the run without reading the code.
- **Verification scenario:** delete the rebind call from encoding A
  and re-run — the probe must FAIL (the commit fires the reorder
  closure, not the trash closure), demonstrating the test bites.

### Q5. What are the counterexamples?

- **A click.** No trajectory, no drift; intent compilation's
  domain. The probe's machinery is ceremony for it.
- **Intent strength, not meaning.** A press that grows from 20% to
  100% commitment (the speculation's destructive-hold scenario)
  changes *how much*, not *what*. This probe says nothing about it;
  Option C's core motivation stands untested by construction.
- **Multi-pointer drift.** Two processes reinterpreting each other
  (bimanual gestures) need the arbitration layer NG-6 defers. The
  probe is single-pointer.
- **Recognizer-driven reinterpretation.** A gesture recognizer that
  re-reads the whole trajectory mid-flight (e.g. "this looks like a
  circle now") is a consumer of the ring, not a bridge operation;
  expressible, but out of scope.

**Scope statement:** this decision applies to single-process,
single-pointer, meaning-level reinterpretation of an in-flight
`px_interaction`. It does NOT apply to strength gradients,
multi-touch arbitration, or recognition.

## Consequences

### Positive

- The B→C criterion gains its first measurement on the
  reinterpretation axis, whichever way it lands.
- The mid-flight mutability of the bridges goes from incidental
  (nothing forbids it) to exercised (a demo, tests, and a printed
  verdict) — or the incidental freedom is exposed as insufficient,
  which is promotion-grade evidence.
- The classical-control encoding gives the docs an honest
  side-by-side for the drift scenario, in the
  `hover_drag_4abs`/`hover_drag_interaction` tradition.

### Negative

- One more example in the catalog that must stay build-synced and
  byte-stable (Gate 13, `check-examples`).
- The crossing-commit policy, once printed in a canonical demo,
  becomes a pattern readers may copy; if it is a bad policy, the
  demo teaches it. Mitigated by stating it AS policy in the output.

### Neutral

- No framework change in either verdict branch; the 7-abstraction
  count and every doc claim are untouched by this ADR alone.

## Alternatives Considered

### Alternative 1: Do nothing — the bridges are obviously enough

- What: rely on the reading that mutable bridges trivially imply
  drift is expressible.
- Why rejected: "obviously" is the word this repository's whole
  method exists to delete. The mutability is real, but the
  trajectory/undo/invariant interactions across a live rebind are
  exactly the kind of composition nobody has run. An unexercised
  freedom is a claim, not evidence.

### Alternative 2: Graduate to Option C now

- What: implement the intent gradient (strength field, phase hooks
  on Closure) and subsume reinterpretation under it.
- Why rejected: the speculation document's own criterion gates C on
  evidence that process-alone is insufficient — for strength. This
  probe is the honest intermediate step for the *adjacent* axis;
  skipping it would promote on unmeasured motivation, the exact
  move ADR-0016's protocol was built to prevent.

### Alternative 3: A framework `px_interaction_reinterpret` API

- What: add a first-class "change semantics" operation that fires a
  transition (e.g. `PX_INT_DRIFTED`) and republishes.
- Why rejected: it presumes the verdict. If the bridges suffice
  (P1–P4 hold), the API is dead weight and a 16th symbol; if they
  don't, the right shape of the fix is unknowable before the probe
  shows WHERE it breaks. Build the measurement first.

### Alternative 4: Fold the drift case into the classical control forever

- What: accept mode-flag machines as the answer for drift, as every
  mainstream toolkit does.
- Why rejected: that is the null hypothesis, not a decision. The
  probe runs it as the CONTROL — it stays in the comparison either
  way; rejecting it as *the* answer is what the experiment is for.

## CAVEATS

- This ADR does **not** promote, demote, or resize any abstraction.
  The count remains 7 canonical; the probe is evidence-gathering in
  the ADR-0016 tradition.
- This ADR does **not** test intent strength/gradient — Option C's
  central motivation. A P1–P4 pass narrows C's open territory; it
  does not close it.
- The scenario's crossing-commit is an app policy, not a framework
  recommendation. The probe prints it as a choice so the demo cannot
  silently canonize it.
- A P1–P4 pass is an existence proof for THESE encodings, not a
  proof that every drift-shaped interaction is bridge-expressible.
  The corpus's Category D verdicts are untouched.

## Known issues

- **Issue:** `px_interaction_on_commit` copies the payload at bind
  time, so encoding A's rebound payload cannot reflect
  post-drift trajectory state (e.g. where in the trash the release
  lands); the final effect must be compiled at outcome time — which
  is encoding B's shape.
- **Why accepted:** the copy-at-bind semantics is what makes commit
  payloads replayable values (intent-as-value, the ADR-0018
  denotational claim); weakening it to capture-at-commit would
  trade a stronger invariant for demo convenience.
- **Tracking:** none — this tension is informational for the
  promotion-grade decision, recorded by the probe itself.
- **Mitigation:** encoding B (hook compilation) is the sanctioned
  shape when the outcome depends on the post-drift trajectory; the
  probe demonstrates both and prints the difference.

## HISTORY

- 2026-10-31: Proposed — predictions P1–P5 registered before
  implementation; probe and `[K]` tests landing in the same change.
  Measured in the same change: P1–P4 PASS, P5 measured (see
  Decision → Measured outcome); verdict — no B→C trigger on the
  reinterpretation axis.

## References

- Code: `examples/semantic_drift_probe.c` (the probe),
  `tests/test_v06_interaction.c` section `[K]` (the lock)
- Evidence protocol precedent:
  [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md)
  (Option B's prototype record),
  [ADR-0018](../accepted/ADR-0018-interaction-process-promotion.md)
  (the promotion whose invariant this probe stresses)
- Criterion under test: [continuous-intent-speculation.md](../../concepts/speculation/continuous-intent-speculation.md)
  (B→C decision criteria; the five strength scenarios)
- Boundary-demo precedent: `examples/hover_drag_4abs.c` (the
  boundary-EXPOSING form this ADR's verdict rule may re-instantiate)
- External: Brad Myers, "A New Model for Handling Input" (ACM TOIS
  1990) — Interactors bind the technique at start time; David
  Harel, "Statecharts: A Visual Formalism for Complex Systems"
  (1987) — the do-action belongs to a state; Allen Newell's
  "theories of the middle range" framing, per this repo's
  usage — a probe is a middle-range instrument.
