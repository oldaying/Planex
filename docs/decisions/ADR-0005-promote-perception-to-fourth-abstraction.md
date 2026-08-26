# ADR-0005: Promote Perception to the fourth abstraction

## Status

Accepted

Date: 2026-08-25

Supersedes: [ADR-0001](ADR-0001-perception-currently-noop.md)

## Context

Planex's README and `docs/concepts/why-three-abstractions.md` claim that Planex is built on three abstractions: **Relation**, **Estimate**, **Closure**. This is the project's central thesis and the basis of its differentiation from React / SwiftUI / Flutter.

However, UI essence has **four** fundamental axes, not three:

1. Intent space (user → machine direction)
2. State space (machine-side state, with time + uncertainty)
3. State-state relationships (the network structure of state)
4. Semantic interface (the bidirectional encoding/decoding surface, including the machine → user direction)

These four axes are documented in [ui-essence-layers.md](../concepts/ui-essence-layers.md) Layer 2 (cognitive) and Layer 3 (semantic), drawing on:

- Norman's 7-stage model — stages 1-4 (execution side) + stages 5-7 (evaluation side)
- Hutchins/Hollan/Norman's two gulfs — execution gulf + evaluation gulf
- Winograd/Flores speech-act theory — request/promise/declare (intent direction) + interpretation (perception direction)

The first three Planex abstractions cover axes 1-3:

| Essence axis | Planex abstraction |
|---|---|
| Intent space (user → machine) | Closure |
| State space | Estimate |
| State-state relationships | Relation |
| **machine → user direction** | **(no abstraction — currently a no-op in Closure stage 5)** |

**This is a structural gap.** Planex's README claims "3 abstractions = UI essence", but UI essence has 4 axes. The 4th axis (machine → user) is currently:

- Listed as Closure stage 5 (`px_perception_fn` callback)
- Implemented as a no-op placeholder (see `src/closure.c:131` and [ADR-0001](ADR-0001-perception-currently-noop.md))
- Actually done through a separate `on_render` callback in `px_app_desc`, which has **no first-class abstraction status**

This ADR closes that gap by promoting Perception to the 4th first-class abstraction.

### Forces

1. **Planex's stated commitment**: The project's manifesto and ADR-0001 both record Planex as "essence-driven". A research-grade project cannot claim to be essence-driven while leaving an essence axis unabstracted.

2. **The honest-claim requirement**: [path-C-lineage.md](../concepts/path-C-lineage.md) commits Planex to "honest acknowledgment" — no over-claiming. The current "3 abstractions" claim over-claims because the 4th axis is unabstracted.

3. **Three (c)-route prototypes already validated**: `counter_denotative.c`, `calculator_denotative.c`, and `counter_interactive.c` prove that pure-function render with hit regions works in real interactive windows. The (b) route builds on this foundation.

4. **Existing implementation friction**: Planex's current `on_render` callback is structurally the same as React's render callback — it is the very thing Planex is supposed to be different from. The (b) route eliminates this inconsistency.

### Constraints

- C17 implementation (per [ADR-0004](ADR-0004-use-c-not-rust-zig-cpp.md))
- Zero external dependencies
- Existing 25 demos must remain functional (migration path required)
- Backward compatibility is NOT required (Planex is pre-v1.0, see [NG-4](../concepts/non-goals.md))

## Decision

**Promote Perception to the fourth first-class abstraction. Restructure Closure from 7 stages to 5 stages.**

### New structure

| Essence axis | Abstraction | Coverage |
|---|---|---|
| Intent space (user → machine) | **Closure** (5 stages: Goal → Intent → Action → Execution → Evaluation) | Execution side of Norman's 7-stage model |
| State space | **Estimate** (value + time + confidence) | Unchanged |
| State-state relationships | **Relation** (queryable graph) | Unchanged |
| Semantic interface (machine → user) | **Perception** (first-class, multiple per Estimate set) | Evaluation side of Norman's 7-stage model |

### Closure API change

**Before (7 stages)**:
```c
px_closure* px_closure_new(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_perception_fn perception,    // ← removed
    px_eval_fn       evaluation,
    void*            user);
```

**After (5 stages)**:
```c
px_closure* px_closure_new(
    const char*      goal,
    px_intent_kind   intent_kind,
    px_action_fn     action,
    px_eval_fn       evaluation,
    void*            user);
```

The `px_perception_fn` parameter is removed from Closure. Norman's stages 5 (Perception), 6 (Interpretation) move to the new Perception abstraction. Stage 7 (Evaluation) remains in Closure as machine-side auto-evaluation.

Stage 6 (Interpretation) is the user's cognitive act and is not modeled in code — same as before, listed only for Norman-completeness.

### New Perception API

```c
typedef struct px_perception px_perception;

/* A perception function: takes a set of Estimates as input,
 * returns a denotation (pixel buffer, a11y tree, log string, etc.).
 * Pure function: same inputs → same output, no side effects. */
typedef void* (*px_perceive_fn)(px_estimate** inputs, int n_inputs, void* user);

px_perception* px_perception_new(
    const char*     name,
    px_perceive_fn   fn,
    px_estimate**    inputs,     /* which Estimates this perception depends on */
    int              n_inputs,
    void*            user);

void px_perception_free(px_perception* p);

/* Runtime query: which perceptions depend on the given Estimate?
 * Used by app loop to know which perceptions to re-evaluate
 * when an Estimate changes. */
px_perception** px_perceptions_for_estimate(px_estimate* est, int* out_count);
```

### Migration of `on_render` callback

`px_app_desc.on_render` callback is **removed**. Rendering now happens via:

```c
// User code creates perceptions
px_estimate* inputs[] = { count_estimate };
px_perception* screen_render = px_perception_new(
    "counter_pixels",
    render_to_pixels,        // pure function
    inputs, 1,
    &app);

// (Optionally) additional perceptions for other denotations
px_perception* a11y_render = px_perception_new(
    "counter_a11y",
    render_to_a11y_text,
    inputs, 1,
    &app);

// App loop automatically:
//   - Tracks which Estimates changed each frame
//   - Finds dependent perceptions
//   - Calls their pure functions
//   - Blits results to appropriate outputs (window/screen/a11y/...)
px_app_run(&desc);
```

### Manifesto change

README's tagline changes from:

> "Plane + X — what if a UI library's core abstractions were Relation + Estimate + Closure, not Component + State + Event?"

to:

> "Plane + X — what if a UI library's core abstractions were Relation + Estimate + Closure + Perception, directly mapping UI essence's four axes?"

`docs/concepts/why-three-abstractions.md` is renamed to `docs/concepts/why-four-abstractions.md` and rewritten to reflect the four-axis essence mapping.

## Essence Check

### Q1. Which essence axis does this decision affect?

✅ Semantic interface (the bidirectional encoding/decoding surface, specifically the machine → user direction)

This decision directly addresses an unabstracted essence axis. Layer 3 of [ui-essence-layers.md](../concepts/ui-essence-layers.md) is the layer being completed.

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions (cognitive bandwidth reduced):**

- **README claim becomes true**: "4 abstractions = 4 essence axes" — no hidden gap. Users reading the README can trust it.
- **Symmetric structure**: user→machine (Closure) and machine→user (Perception) are first-class peers. No "Perception is a sub-stage of Closure" cognitive burden.
- **Multiple denotations coexist naturally**: screen / a11y / headless / GPU are all Perception instances, not after-thoughts.
- **Render is unit-testable**: pure function `px_perceive_fn` can be tested without app loop (already validated by counter_denotative 4 unit tests, calculator_denotative 4 unit tests, counter_interactive 2 unit tests).
- **Static reasoning enabled**: given an Estimate set, the resulting pixels are statically computable — no callback indirection.

**Increases (cognitive bandwidth added):**

- **One more abstraction to learn**: users now encounter 4 (Relation, Estimate, Closure, Perception) instead of 3.
- **Closure's 7-stage story changes**: was "Norman's complete 7 stages in Closure", now "Norman's stages 1-4 + 7 in Closure, stages 5-6 in Perception". Users who learned the 7-stage story need to relearn.
- **Hit region pattern**: interactive demos now require `px_hit_region` as part of Perception output (per `counter_interactive.c` pattern). Users must learn this pattern.

**Net assessment:** Long-term compression (no gap, symmetric structure, testability). Short-term increase (one new concept, relearned Closure story). Net positive for research-grade use.

### Q3. Is there a gap between the claim and the implementation?

**Claim**: 4 abstractions (Relation + Estimate + Closure + Perception), each directly mapping an essence axis.

**Implementation**: 4 abstractions, each with a real API and runtime behavior. Perception is no longer a no-op — it has `px_perception_new`, dependency tracking via Relation graph, and is invoked by the app loop.

**Gap**: None. **Claim = Implementation** — the strongest possible state per Conal Elliott's denotative design.

This decision closes the gap recorded in [ADR-0001](ADR-0001-perception-currently-noop.md). ADR-0001 is superseded.

### Q4. What is the cost, and who can verify it?

**Cost — direct engineering**:

| Component | Estimated effort |
|---|---|
| New `src/perception.c` (~200 lines: struct, registry, query) | 1-2 days |
| Rewrite `src/app.c` to drive perceptions instead of `on_render` callback | 2-3 days |
| Update `include/planex/planex.h`: remove `px_perception_fn` from Closure, add new Perception API | 0.5 day |
| Migrate 25 demos: change `px_closure_new` calls (remove perception arg) | 12-25 hours total |
| Migrate 25 demos: rewrite render logic as pure `px_perceive_fn` | 12-25 hours total |
| Rewrite `README.md` tagline + manifesto | 0.5 day |
| Rename and rewrite `why-three-abstractions.md` → `why-four-abstractions.md` | 1 day |
| Update `glossary.md`, `architecture.md`, `examples/README.md`, `roadmap-matrix.md`, `limitations.md` | 1 day |
| Update ADR-0001 status to "Superseded by ADR-0005" | 0.5 hour |
| **Total** | **~2-3 weeks of full-time work** |

**Cost — non-engineering**:

- Existing 25 demos' API breaks (acceptable per [NG-4](../concepts/non-goals.md) backward-compatibility pre-v1.0)
- Manifesto's "3" branding changes to "4" — communicates a public stance shift

**Verification**:

- **Cost verifier**: the maintainer (immediate, during implementation)
- **Learning curve verifier**: any new user who tries to write a Planex app from scratch — they will encounter 4 abstractions and the hit-region pattern. If they cannot proceed, the cognitive cost is too high.
- **Performance verifier**: running `counter_interactive` (already validated at 1273 frames / 60fps in pure-function mode). (b) route's performance is the same as (c) prototype — already measured.

**Verification scenario**: after migration, run the existing 25 demos + new pure-function tests. All should pass. If any demo cannot be migrated cleanly, that's a counterexample (see Q5).

### Q5. What are the counterexamples?

**Counterexample 1: External-data-driven UI**

Video players: rendering comes from a video decoder (external frame stream), not from Estimates. A Perception function `render_to_pixels(inputs)` cannot express this — the input is not in the Estimate graph, it's an external stream.

**Scope statement**: Planex does not target video playback UIs. This is recorded as a non-goal (see [NG-13](../concepts/non-goals.md) — to be added).

**Counterexample 2: Embedded WebView**

A WebView's rendering is opaque — the HTML/CSS rendering happens inside the WebView engine. It cannot be expressed as a Perception function of Planex Estimates.

**Scope statement**: Planex does not support embedding opaque renderers (WebView, native canvas). If a use case requires this, the user must compose Planex with the opaque renderer at the window level, not at the Perception level.

**Counterexample 3: Hover / pressed / transient interaction states**

Mouse hover, button press transient state, drag preview — these are multi-frame interaction processes, not single-state Estimates. The current Perception API takes Estimates as input, which doesn't naturally express "the hover trajectory of the last 50ms".

**Scope statement**: This is recorded as [Limitations L11](../concepts/limitations.md) — multi-frame interaction processes are not abstracted, by any UI library including Planex. See [continuous-intent-speculation.md](../concepts/continuous-intent-speculation.md) for the future-research marker. (b) route does not address this; it's orthogonal.

**Counterexample 4: GPU acceleration in the future**

When Planex adds a GPU backend ( Vulkan / Metal / D3D12 / WebGPU), pure-function Perception will face the "per-frame texture upload" cost (~1-2ms per frame at 1080p). This is the (c) route's known GPU cost, inherited by (b).

**Scope statement**: GPU backend is [NG-7](../concepts/non-goals.md) for now. When it becomes a real goal, the cost will be addressed via texture pooling or render-cmd abstraction layered on top of Perception — both compatible with (b)'s structure.

**Scope summary**: (b) route applies to **state-driven UIs** (counter, calculator, form, todo — Planex's current scope). It does NOT apply to **external-data-driven UIs** (video, WebView, embedded canvas) or **multi-frame interaction processes** (hover, drag preview). These are documented limitations, not (b)'s failures.

## Consequences

### Positive

- **README claim becomes true**: "4 abstractions" maps 1:1 to UI essence's 4 axes. No over-claiming.
- **Symmetric structure**: Closure and Perception are first-class peers. user→machine and machine→user are equally abstracted.
- **Multiple denotations coexist**: screen / a11y / headless / future GPU — all are Perception instances. No special-casing.
- **Render is unit-testable**: pure `px_perceive_fn` takes Estimates, returns denotation. Already validated by 3 prototypes with 10 unit tests passing.
- **Hit regions as Perception output**: interactive dispatch is decoupled from rendering. Validated by `counter_interactive.c` (8 clicks, 1273 frames, 60fps).
- **Anti-pattern tests become possible**: "why React's onClick can't have multiple denotations" becomes a strong argument.
- **ADRs 0001 closed**: Perception gap explicitly resolved.
- **Layer 4 (behavioral) seed has a home**: `PX_REL_AFFORDS` can eventually be expressed as a Perception subtype. Future Layer 4 work fits naturally.

### Negative

- **API breaks for all 25 demos**: each demo's `px_closure_new` call changes signature. Migration is mechanical but tedious (~12-25 hours).
- **Render logic rewrite for all 25 demos**: each demo's `on_render(fb, user)` callback becomes `render_to_pixels(inputs, n, user)` pure function. Migration requires understanding (~12-25 hours).
- **Manifesto rewrite**: README's "3 abstractions" branding must change to "4". This is a public stance shift — communicates that Planex reconsidered its essence model.
- **Closure's 7-stage story is broken**: users who learned "Closure implements Norman's 7 stages" need to relearn "Closure implements 5 stages, Perception implements the other 2". The 7-stage cognitive scaffold is gone.
- **One more abstraction to learn**: users encounter Relation, Estimate, Closure, Perception (4) instead of 3.

### Neutral

- **Planex is now explicitly a Layer 1-3 implementation with Layer 4 seed** (per [ui-essence-layers.md](../concepts/ui-essence-layers.md)). No change to which layers are implemented, just sharper alignment within Layer 2-3.

## Alternatives Considered

### Alternative 1: (a) Implement Stage 5 as a no-op filler

- **What**: Fill in `closure.c:131` with snapshot collection, keep Perception as Closure's sub-stage.
- **Why rejected**: Hides the architectural question. Perception remains a second-class citizen. If rendering needs independent evolution (GPU, a11y, headless test snapshots), it has to fight its way out of the Closure namespace. This is the same compromise Planex's current `on_render` callback makes — it doesn't address the structural asymmetry.

### Alternative 2: (c) Absorb Perception into Estimate (Conal's denotative path)

- **What**: Make rendering a pure function `Estimate → Pixel`, remove `on_render` callback, but keep "3 abstractions" claim.
- **Why rejected**: Philosophically elegant but conflicts with Planex's "essence-driven" stance. The (c) route says "state = presentation", which is a philosophical commitment (Conal/Haskell position) that not everyone accepts. By staying at "3 abstractions" while extending Estimate's role, the claim says one thing (3) but the implementation does another (Estimate absorbs Perception's job). This is a different kind of gap than (a) — it's a *semantic* gap, not a *completeness* gap. (b) is more honest: 4 axes of essence = 4 abstractions.
- **However**: the (c) route's pure-function implementation technique is **preserved** in (b). Perception's `px_perceive_fn` is a pure function. (b) keeps (c)'s engineering elegance while being more philosophically honest.

### Alternative 3: (b)+(c) merged — 4 abstractions AND pure-function Estimate semantics

- **What**: Promote Perception to 4th abstraction (b), AND redefine Estimate to include its denotation (c).
- **Why rejected**: Redundant. (b) already provides 4 abstractions with pure-function Perception. Adding (c)'s "Estimate has a denotation" doesn't add anything that isn't already expressed by "Perception takes Estimate as input and returns pixels". (c)'s philosophical move is only necessary when Perception is NOT a separate abstraction — once it is, (c) becomes unnecessary.

### Alternative 4: (f) Perception as graph node, not abstraction

- **What**: Add `px_perception` struct + `PX_REL_PERCEIVED_BY` relation kind, but don't promote to first-class abstraction. Keep "3 abstractions" claim.
- **Why rejected**: This is engineering compromise. It admits Perception needs first-class behavior (multiple instances, queryable, lifecycle) but refuses to give it first-class status in the manifesto. It's the "PX_REL_AFFORDS pattern" extended to Perception — useful, but doesn't satisfy the "essence-driven" stance. If Planex is essence-driven, Perception gets the abstraction status. If Planex is engineering-driven, (f) would be fine. The choice depends on Planex's identity.

### Alternative 5: (g) Dual mode — support both callback and pure-function

- **What**: Keep `on_render` callback for existing demos, add `px_perception_new` for new pure-function demos.
- **Why rejected**: Doesn't make a decision. Defers the question while letting both code paths accumulate. Research-grade projects that defer decisions tend to die with both paths half-implemented (see [path-C-lineage.md](../concepts/path-C-lineage.md) Failure mode 5 — confusing research with production).

### Alternative 6: (h) Defer decision to v1.0

- **What**: Leave Perception as no-op placeholder, decide later when there's more usage data.
- **Why rejected**: This continues the over-claim currently in the README. "Honest acknowledgment" (per path-C-lineage.md) requires either committing to a decision or explicitly marking the gap as unresolved — both are valid. Deferring silently is neither. If Planex chooses to defer, it must change the README to say "2.5 abstractions" or similar — which is more disruptive than just making the decision.

## References

- Code (current state):
  - `src/closure.c:131` — the no-op placeholder that this ADR replaces
  - `include/planex/planex.h:246` — Perception listed as Stage 5
  - `include/planex/planex.h:274` — `px_perception_fn` typedef (to be removed from Closure, used by new Perception API)

- Validating prototypes (already in repo):
  - `examples/counter_denotative.c` — single-state pure-function render (4 unit tests pass)
  - `examples/calculator_denotative.c` — multi-state pure-function render (4 unit tests pass, 3 calculation scenarios pass)
  - `examples/counter_interactive.c` — real Win32 window with hit regions (2 unit tests pass, 8 clicks / 1273 frames at 60fps validated on Windows)

- Related docs:
  - [ui-essence-layers.md](../concepts/ui-essence-layers.md) — Layer 2-3 (cognitive + semantic), this decision completes these layers
  - [path-C-lineage.md](../concepts/path-C-lineage.md) — honest acknowledgment of Planex's position
  - [why-three-abstractions.md](../concepts/why-three-abstractions.md) — current manifesto, will be renamed to `why-four-abstractions.md`
  - [roadmap-matrix.md](../concepts/roadmap-matrix.md) — Perception row currently entirely red
  - [limitations.md L1](../concepts/limitations.md) — current Perception gap, will be marked resolved
  - [continuous-intent-speculation.md](../concepts/continuous-intent-speculation.md) — orthogonal to this ADR; (b) doesn't address Layer 5

- Related ADRs:
  - [ADR-0001](ADR-0001-perception-currently-noop.md) — **superseded by this ADR**
  - [ADR-0002](ADR-0002-relation-necessity-pending-undo.md) — orthogonal; (b) doesn't address Relation necessity
  - [ADR-0004](ADR-0004-use-c-not-rust-zig-cpp.md) — C17 constraint is preserved

- External:
  - Don Norman, *The Design of Everyday Things* (1988) — 7-stage model
  - Hutchins, Hollan, Norman, *Direct Manipulation Interfaces* (1985) — two gulfs
  - Winograd, Flores, *Understanding Computers and Cognition* (1986) — speech-act theory
  - Conal Elliott, *Denotational Design with Type Class Morphisms* — pure-function semantics
  - Planex path-C-lineage.md — historical context of why this is the right moment to make this decision

## Implementation plan

This ADR is accepted but not yet implemented. Implementation proceeds in two phases:

### Phase 1: API migration (target: v0.2)

- Add `px_perception` struct and API to `include/planex/planex.h`
- Remove `px_perception_fn` parameter from `px_closure_new`
- Update all 25 demos to new `px_closure_new` signature (mechanical change)
- Update README tagline and manifesto
- Mark ADR-0001 as superseded
- **At end of Phase 1**: claim becomes true ("4 abstractions"), but `px_perception_new` is internally a stub — it registers but doesn't yet drive rendering. `on_render` callback still works for compatibility.

### Phase 2: Implementation migration (target: v0.3)

- Implement `src/perception.c` (~200 lines)
- Rewrite `src/app.c` to drive perceptions instead of `on_render`
- Remove `on_render` callback entirely
- Migrate all 25 demos' render logic to pure `px_perceive_fn` functions
- Add hit regions as Perception output (per `counter_interactive.c` pattern)
- **At end of Phase 2**: implementation matches claim. 4 abstractions, each fully functional.

### Counterexample validation

During Phase 2, the following demos will stress-test the (b) route:

- `text_input_ime.c` — IME composition state, tests whether Perception can handle transient multi-frame state (related to L11)
- `modal.c` — async promise/declare lifecycle, tests whether Perception can render during async operations
- `form.c` — derived estimates, tests whether Perception correctly tracks dependency changes

If any of these cannot be migrated cleanly, the failure is recorded as a new Limitation entry, and the scope statement in Q5 is updated.

---

**This ADR closes the longest-standing gap in Planex**: the Perception no-op recorded in ADR-0001 since 2026-08-24. By promoting Perception to the 4th first-class abstraction, Planex's claim and implementation finally match — the strongest possible state for a research-grade, essence-driven project.
