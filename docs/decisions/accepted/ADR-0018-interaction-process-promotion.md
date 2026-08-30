# ADR-0018: Promote px_interaction (interaction process) to the 7th canonical abstraction

## Status

Accepted

Date: 2026-08-30

Promotes the v0.6 interaction prototype ([ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md), Option B) to canonical status, per the [v0.7 roadmap](../../concepts/history/v0.7-roadmap.md) Cross-cutting A — the promotion decision that rides Line 1's real-application evidence. Together with [ADR-0017](ADR-0017-intent-compilation-promotion.md) (the noun world), this completes the v0.7 re-score of [UI Pattern Corpus](../../reference/ui-pattern-corpus.md) Category D: four more flips to ✅ (P26, P27, P28, P36) and two downgrades from ❌ to ⚠️ (P29, P37).

## Context

ADR-0006 deferred continuous interaction with an evidence protocol: build the boundary-exposing demo, measure the pain, let the next ADR decide. `hover_drag_4abs.c` measured it (7 HACKs, "INTOLERABLE for complex gesture/touch UIs"); ADR-0016 landed the Option-B prototype in v0.6 with the promotion gate explicitly deferred: real-application evidence, Category D re-scoring, and its own ADR against the ADR-0011 admission bar.

All three gates now have evidence:

1. **Real application.** [`examples/palette_afford.c`](../../../examples/palette_afford.c) §5: the brightness slider is a drag process — 40 trajectory samples at zero estimate writes (live preview is *derived* per frame from the trajectory; only the commit writes the estimate, exactly once). This is the pattern the old demo paid in per-event estimate churn.
2. **The boundary-closing demo.** [`examples/hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c) (v0.6): hover as pure region query, drag as one process object with an inert hot path, cancel as a first-class outcome, tap-vs-drag distinguished by *measure* not mode flag, swipe derivable from velocity + displacement. 5 of 7 HACKs retired; the whole 130-event session costs 2 estimate writes.
3. **The test suites.** [`tests/test_v06_interaction.c`](../../../tests/test_v06_interaction.c) (27 tests: lifecycle, trajectory, bridges, THE INVARIANT, gesture derivation) and [`tests/test_v07.c`](../../../tests/test_v07.c) A (routing-side integration).

The design invariant that survived contact with the real app — the one thing this promotion is *really* about — is **sample laziness**: samples are inert (no observer fan-out, no perception auto-invoke); the process touches the semantic world only at transitions (BEGAN / MOVED / ENDED / COMMITTED / CANCELLED). This is what makes "process" not be "state": between transitions there is nothing for the rest of the system to react to.

## Decision

`px_interaction` is promoted from prototype to the **7th canonical abstraction**. Concretely:

- **Constitutive question:** *"What is the user doing over time?"* — the time-bounded half of intent. Intent compilation (ADR-0017) owns the instantaneous decode (*what does this input denote?*); the interaction process owns the trajectory between the first sample and the outcome. Neither subsumes the other: a click is decode-without-trajectory; a drag is trajectory-without-preknown-decode. Keeping them separate is what makes both questions crisp.
- **Canonical API surface:** `px_interaction_new/free/sample/commit/cancel`, the phase machine (`PX_INT_*`), pure trajectory measures (`position/velocity/displacement/duration/path_length/last/total`), and the three transitions-only bridges (`on_phase` hook, commit/cancel Closure triggers, `publish_phase` to Estimate). All v0.6 signatures unchanged — promotion adds status, not API.
- **THE INVARIANT is normative:** sample-laziness is now a canonical contract, not a prototype property. Any future change that makes samples fire observers, invoke perceptions, or write estimates is a **breaking change to this abstraction** and requires an ADR that explicitly re-litigates it. `test_v06_interaction.c` section D is the enforcement.
- **`publish_phase` remains the ONLY sanctioned seam to Estimate** — transitions only, never per-sample. The seam is what keeps process and state from re-conflating (the failure mode the corpus's Category D verdicts were recording).
- **Gestures stay derivable, not recognized.** Swipe/tap/drag discrimination is by measure (velocity/displacement thresholds in consumer code). A recognizer stack is future work and explicitly not promised (NG-6 unchanged: no touch input).
- **Corpus amendment (this ADR's amendment, per the closing rule):**
  - P26 Pressed button visual: ⚠️→✅ — press-release is the BEGAN→COMMITTED/ENDED arc; `publish_phase` feeds the estimate at transitions only; perception renders from it ([`hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c), mechanism proven in test c4).
  - P27 Drag preview (ghost image): ⚠️→✅ — the preview is *derived* per frame from the trajectory (pure read of `px_interaction_last`); zero writes while dragging.
  - P28 Drag-drop reorder: ⚠️→✅ — literally implemented: trajectory + commit closure + undo through the graph (demo §2).
  - P36 Color picker (drag slider): ⚠️→✅ — live preview derived from trajectory, one committed estimate write ([`palette_afford.c`](../../../examples/palette_afford.c) §5; co-grounded with ADR-0017 — the region compiles the press, the process owns the drag).
  - P29 Swipe gesture: ❌→⚠️ — the gesture is *derivable* from measures (test f1), but the pattern names touch input, which is NG-6. Downgraded from impossible to forced, honestly.
  - P37 Knob / rotary control: ❌→⚠️ — rotary is the same class as the slider (drag process + app-side angle math); derivable but undemonstrated, so forced rather than clean.

### Essence Check (admission bar per ADR-0011)

1. **Tradition traceability.** CSP (Hoare, *Communicating Sequential Processes*, 1985): a process is prefix + choice — `begin → P'`, and terminate as `commit ⊕ cancel`. The phase machine is that shape. Statecharts (Harel, 1987): the transitions-only discipline (states BEGAN/ACTIVE, events MOVE, actions at transitions) is statechart orthodoxy. Elliott's FRP (*Fran*, 1997) supplies the contrast that sharpens the claim: Behavior covers continuous *values*, not bounded *processes with outcomes* — `px_interaction` is not Behavior; it is a bounded process whose product is a trajectory + an outcome. (Citations carried from ADR-0016, which landed them with the prototype.)
2. **Constitutive orthogonality.** "What is the user doing over time?" is answered by no other abstraction. Estimate owns state (a trajectory is *not* an estimate — the v0.4-era hack the corpus penalized); Closure owns completed acts (a mid-drag process has not completed); px_loop owns control-yield cadence (frame timing, not user behavior); intent compilation owns the instantaneous decode. Precondition test: remove px_interaction and hover/drag collapse back into per-event estimate writes — the measured, scored, INTOLERABLE state.
3. **Denotational semantics.** The products are values: the trajectory ring (queryable via pure measures), the outcome phases (published to Estimate at transitions only), and the commit payload (a copied value — e.g. `px_reorder_intent` in the demo — replayable through the closure pipeline with undo). The measures are pure functions of the trajectory: same samples, same answers, at any later time — which is what makes "what did the user do?" answerable *after the fact*, the property per-event callbacks never had.

## Essence Check

> This decision adds a canonical abstraction — the maximal form of an essence-affecting change. The Q1–Q5 checks below apply the ADR-0011 admission bar.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface
- [x] **Intent space** — the user→machine direction, time-bounded half. ADR-0017 (same release) owns the instantaneous decode; this ADR owns the process between first sample and outcome. Together they close the execution gulf's translation segment from both sides.
- [ ] State space — explicitly *not*: the invariant exists to keep process out of state.
- [ ] Presentation space
- [ ] Feedback space — `publish_phase` *feeds* feedback at transitions; the loop's cadence is untouched.

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses, and the compression is the measured story of the two demos: `hover_drag_4abs.c` (pre-abstraction) paid 6 transient Estimates, 2 estimate writes per mouse move, and observer fan-out per event; `hover_drag_interaction.c` pays one process object, zero writes per sample, and hover computed at render time. The conceptual compression is larger than the mechanical one: hover, drag, tap, swipe, and cancel become *one* thing (a bounded process with measures and an outcome) instead of five ad-hoc event patterns. Cancel-as-first-class-outcome deletes a whole class of boolean-flag bookkeeping ("is this drag still valid?").

### Q3. Is there a gap between the claim and the implementation?

Yes, recorded:

- **One process at a time per object.** Multi-touch (simultaneous trajectories) needs multiple interaction objects with no cross-process arbitration; there is no `px_interaction_group`. NG-6 territory.
- **Phases are terminal.** A committed or cancelled process cannot restart; the app re-arms (the demo frees and re-creates). A reset API would be a small addition, deliberately not taken in v0.7 (no consumer evidence yet).
- **No recognizer.** Swipe/tap discrimination is threshold code in the consumer. Derivable is not recognized; the corpus verdicts for P29/P37 record exactly this distance.
- **Time source is caller-supplied.** Samples carry timestamps; the process never reads a clock. Deterministic (good for tests), but consumers must supply sane times for velocity measures to mean anything.

### Q4. What is the cost, and who can verify it?

- **Cost:** per-sample O(1) ring append (bounded memory, default 64 samples configurable at creation); per-transition hook + optional closure trigger + optional estimate write. The hot path is the cheapest code in the framework by design. API surface: 15 symbols.
- **Who can verify:** anyone. `make test_v06` (27 tests including section D, THE INVARIANT) and `make test_v07` run backend-free; `make check-examples` runs both demos end-to-end against snapshots; `make check-completeness` enforces the amended corpus distribution this ADR contributes to.

### Q5. What are the counterexamples?

- **A plain counter click.** Discrete, instantaneous, no trajectory — intent compilation's domain (ADR-0017), not this one. Wrapping a click in a process object is ceremony without information; `palette_afford.c` compiles clicks directly and uses the process only for the slider drag. The boundary runs through the app, and the app shows it.
- **Ambient streams that never commit.** Mouse-position tracking with no outcome is a sample stream, not a process; keeping it as a plain struct field (the demo's `pointer`) is *more* honest than an interaction object that never ends. The abstraction claims bounded processes with outcomes, not "a class for events".
- **Animation.** A 60-frame animation is a *value over time* (Behavior/estimate-animated territory, `animation_demo.c`), not a user process. Time-progression alone does not make something an interaction.

### Conditions

Per the v0.7 roadmap's conditions-ledger practice:

| Condition | State at promotion | Notes |
|---|---|---|
| Input-event richness of the event layer | partial — MOVE/DOWN/UP/WHEEL; no touch (NG-6) | trajectories make gestures derivable; the recognizer stack is future work |
| A consumer that needs outcomes, not samples | met — two demos + one real app commit through closures | sample laziness is the invariant this consumer set depends on |
| CSP/statechart lineage available for citation | met (carried from ADR-0016) | process algebra is stable mathematics; no condition risk |

Proof obligations going forward: multi-process arbitration (if/when touch lands), the drag-begin affordance seam (joint with ADR-0017, recorded in both).

## Consequences

### Positive

- Category D — the corpus's worst category since v0.5 — moves from 0/15 clean to 7/15 clean (with ADR-0017). L11's "multi-frame interaction processes are not abstracted" and L12's "prototype landed, promotion pending" both close.
- Hover/drag/cancel/tap/swipe become one concept with measures and outcomes — the semantic vocabulary the v0.4 demos lacked and hacked around.
- The invariant (inert samples, transitions-only seams) is now normative with CI enforcement — the process/state conflation that the corpus penalized cannot silently return.
- Undo composes with interaction outcomes for free (commit payload rides the closure pipeline; the demo reorders undoably).

### Negative

- Abstraction count 5→7 in one release (with ADR-0017) — the full doc-sync surface re-syncs in one commit (README tagline, leak budgets, roadmap matrix, stale-count gate, limitations). Large and mechanical, with drift risk if split.
- The phase machine is a new concept new users must learn to use the abstraction at all; the API is small but not free.
- Terminal phases force re-arm cycles in loops that drag repeatedly (the demo re-creates its process object). Slightly awkward, deliberately unsmoothed until a consumer motivates a reset API.

### Neutral

- ADR-0016 remains `proposed/` — its decision (land as prototype, defer promotion) was executed as written; this ADR completes its protocol rather than superseding it. ADR-0016's Essence Check language about "the 5 canonical abstractions" describes the v0.6 state and stays frozen.
- The interaction↔afford seam (drag-begin does not afford-route) is the recorded boundary between this ADR and ADR-0017; both Known-issues sections carry it.

## Alternatives Considered

### Alternative 1: Absorb into Estimate (Conal-style Behavior / continuous values)

Rejected (carried from ADR-0016, now with promotion-grade evidence): the trajectory is not state — 60fps estimate writes were the measured, scored failure. Behavior covers continuous *values*; a drag is a bounded process with an outcome.

### Alternative 2: Absorb into Closure (PX_INTENT_CONTINUOUS)

Rejected (carried): a 60-frame drag is not a locution; closure owns completed acts. Making closure own pre-completion process would re-conflate the exact distinction the corpus's D category penalized.

### Alternative 3: Full continuous-intent (Option C now)

Rejected (carried): intent gradients/confidence decay (Layer 5 speculation) have no consumer evidence; the seeds (`px_estimate_predict`/`surprise`) stay seeds. Promoting the bounded process now does not commit to the gradient story.

### Alternative 4: Keep prototype, defer promotion to v0.8

Rejected as dishonest for this release: the corpus re-score rides the promotions (a clean verdict requires canonical machinery), and the evidence gates ADR-0016 itself set — real app, re-score, admission ADR — are all met now. Deferring would mean either deferring the re-score (Line 1's success criteria unmet) or re-scoring on prototype grounding (Alternative 4 of ADR-0017's rejection, dishonest).

## CAVEATS

- This ADR does **not** claim touch, multi-touch, or gesture *recognition*. NG-6 stands. Gestures are derivable from measures; a recognizer stack is a separate future decision.
- This ADR does **not** change any v0.6 signature. Promotion is a status change plus normative force for the invariant; the API is frozen as-landed.
- This ADR does **not** promote the Friston seeds (`px_estimate_predict`/`surprise`); they remain v0.6 seeds pending consumer-driven promotion paths (roadmap Not-in-v0.7).
- This ADR does **not** claim the interaction object is the right home for *all* temporal input (see counterexamples: ambient streams and animations are deliberately outside).

## Known issues

- **No multi-process arbitration.** Two simultaneous drags are two objects with no group semantics. Fine for mouse; blocking for touch. NG-6.
- **Drag-begin does not afford-route** (the seam, joint with ADR-0017): an app whose regions drag begins on still wires begin via the raw fallback path. A future `px_afford` variant that resolves a *process* would be a new ADR; both ADRs record the obligation.
- **test_orthogonality.c does not yet enumerate the 6th/7th abstractions.** Section D of `test_v06_interaction.c` enforces the orthogonality *signature* (inert samples); folding into the per-abstraction anti-pattern structure is follow-up work, same note as ADR-0017.
- **Terminal-phase re-arm cycles.** Deliberate; a reset API awaits consumer evidence.

## HISTORY

- 2026-08-30: Accepted — promotion + corpus amendment (P26, P27, P28, P36 flip to ✅; P29, P37 downgrade ❌→⚠️) + conditions ledger row migrated into this ADR. Evidence: real app (palette_afford §5), boundary-closing demo (hover_drag_interaction), 27 + 7 tests.
- 2026-08-29: Prototype landed in v0.6 (ADR-0016, proposed) — Option B: process + invariant, promotion gate deferred.
- 2026-08-27: ADR-0006 deferral with evidence protocol; `hover_drag_4abs.c` measured the Estimate hack (7 HACKs, INTOLERABLE).

## References

- Code: [`src/interaction.c`](../../../src/interaction.c), [`include/planex/planex.h`](../../../include/planex/planex.h) (px_interaction API, PX_INT_* phases, px_int_sample)
- Evidence: [`examples/hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c), [`examples/palette_afford.c`](../../../examples/palette_afford.c) §5, [`tests/test_v06_interaction.c`](../../../tests/test_v06_interaction.c) (A–F), [`tests/test_v07.c`](../../../tests/test_v07.c) A
- ADRs: [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) (admission bar), [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md) (prototype decision, protocol completed here), [ADR-0017](ADR-0017-intent-compilation-promotion.md) (companion promotion — the noun world), [ADR-0006](ADR-0006-continuous-interaction-deferred.md) (the deferral this closes), [ADR-0008](ADR-0008-feedback-as-fifth-essence-category.md) (the 5th-abstraction precedent)
- Corpus: [ui-pattern-corpus.md](../../reference/ui-pattern-corpus.md) (amended Category D; closing rule followed)
- Prior art: Hoare, *Communicating Sequential Processes* (1985); Harel, "Statecharts: A Visual Formalism for Complex Systems" (1987); Elliott & Hudak, "Functional Reactive Animation" (ICFP 1997)
- Roadmap: [v0.7-roadmap.md](../../concepts/history/v0.7-roadmap.md) Cross-cutting A
