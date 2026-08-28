# ADR-0001: Perception is currently a no-op placeholder

## Status

**Superseded by [ADR-0005](../accepted/ADR-0005-promote-perception-to-fourth-abstraction.md)**

Accepted: 2026-08-24
Superseded: 2026-08-25

> This ADR is retained for historical record. The gap it documented (Perception as no-op in Closure stage 5) is resolved by ADR-0005, which promotes Perception to the fourth first-class abstraction. Do not act on this ADR — read [ADR-0005](../accepted/ADR-0005-promote-perception-to-fourth-abstraction.md) instead.

Date: 2026-08-24

## Context

The README and `docs/concepts/why-three-abstractions.md` claim that Planex is built on three abstractions: **Relation**, **Estimate**, **Closure**. This is the project's central thesis and the basis of its differentiation from React / SwiftUI / Flutter.

However, UI has two fundamental directions:

1. **user → machine** (encoding intent into state changes)
2. **machine → user** (decoding state into perceivable form)

Closure (with its 7-stage Norman loop) covers direction (1) — it includes a `perception` function pointer at Stage 5. But the actual implementation in `src/closure.c` contains this comment at line 131:

```c
/* Stage 5: Perception (collect visible state — currently a no-op
 * placeholder; Stage 1+ will store a snapshot for diffing). */
if (c->perception) {
    c->perception(c->user);
}
```

The placeholder exists but is empty. Rendering is instead done through a separate `on_render` callback in `px_app_desc`, which is **not** modeled as a first-class abstraction — it's an implementation detail of the app loop.

This creates a tension:

- The README claims "3 abstractions", and one of those (Closure) **claims** to cover perception via its Stage 5
- But Stage 5 doesn't actually do anything
- The actual rendering happens through a callback that has no first-class status
- Therefore the "machine → user" direction of UI is not abstracted; it's delegated to an implementation detail

This is a real architectural decision that has been deferred, not resolved. Without an ADR, future maintainers (or the original maintainer after a memory-erasing six months) might:

- Try to "complete" Stage 5 in a way that doesn't fit the larger design
- Promote Perception to a 4th abstraction without considering the consequences
- Forget that the gap exists and treat the README's "3 abstractions" as complete

## Decision

**Accept that Perception is currently an unresolved design question, and explicitly record the three candidate paths forward.**

The three candidates are:

### Path (a): Implement Perception as a proper Stage 5
Fill in the no-op with snapshot collection and state diffing. Keep Perception inside Closure as a sub-step.

- **Pro:** Minimal API change. The "3 abstractions" claim stays true.
- **Con:** Perception is then a second-class citizen — it lives only as a stage of Closure, not as its own thing. If rendering needs independent evolution (e.g. GPU backends, headless test rendering, accessibility trees), it has to fight its way out of the Closure namespace.

### Path (b): Promote Perception to a 4th abstraction
Rename the project's core claim from "3 abstractions" to "4 abstractions" and treat Perception as an equal to Relation / Estimate / Closure.

- **Pro:** Honest about UI's two-directional nature. Perception gets its own API surface, its own lifecycle, its own anti-pattern tests.
- **Con:** Breaks the "3" branding. Forces a rewrite of `why-three-abstractions.md`. Means the manifesto is currently wrong, not just incomplete.

### Path (c): Absorb Perception into Estimate (Conal's denotative path)
Following Conal Elliott's denotative design — rendering is a pure function `Estimate → Pixel`, not a callback. The `on_render` callback in `px_app_desc` is removed; instead, the runtime observes the active Estimate graph and renders accordingly.

- **Pro:** The cleanest theoretically. Eliminates the rendering-as-callback hack. Makes the "machine → user" direction a property of Estimate, not a separate abstraction.
- **Con:** Most invasive. Requires re-architecting `px_app_run` and the framebuffer layer. Forces every demo to be rewritten.

**Until a path is chosen, the README's "3 abstractions" claim is technically false — it is currently 2.5 abstractions plus an empty slot.**

## Consequences

### Positive
- The gap is now explicit. Anyone reading the ADR understands that Perception is unresolved, not forgotten.
- Future work on Perception has a framework to choose between (a), (b), (c).
- The matrix in `docs/concepts/state/roadmap-matrix.md` row for Perception is consistent with this ADR.

### Negative
- The README's tagline ("three abstractions: Relation + Estimate + Closure") is currently an aspiration, not a description. We accept this tension rather than silently dropping the claim.
- Choosing path (b) or (c) will require rewriting the manifesto. This is deferred, not avoided.

### Neutral
- This ADR does not commit to (a), (b), or (c). It only commits to the diagnosis.

## Alternatives Considered

### Alternative 1: Silently implement Stage 5 without an ADR
- **What:** Just write the snapshot/diffing code into `closure.c:131`, never mention it again.
- **Why rejected:** Hides the architectural question. Future contributors won't know that Perception's role is contested. The README's "3 abstractions" claim becomes a silent lie rather than an explicit debt.

### Alternative 2: Delete Perception from Closure entirely
- **What:** Remove Stage 5 from the API. Rendering is purely `on_render` callback, not abstracted.
- **Why rejected:** Loses the connection to Norman's 7-stage model, which is a stated theoretical foundation. Would require rewriting every doc that references the 7-stage loop. Also forecloses path (a) and (b) above prematurely.

### Alternative 3: Rename README's tagline to "2.5 abstractions" immediately
- **What:** Be brutally honest today.
- **Why rejected:** Confusing to readers. "2.5" is not a real number of abstractions. Better to keep "3" as aspiration and use this ADR + the matrix to expose the gap to anyone who reads deeper.

## CAVEATS

This ADR is retained for historical record only. It does NOT cover:

- The v4 essence-rederivation proposal (Interpretant / Perlocution / Breakdown) — see ADR-0010 and `essence-derivation-v4-clean.md` for that thread.
- The question of whether Perception as the 4th abstraction closes the essence-coverage gap — ADR-0007 (v2 essence revision) and ADR-0008 (Feedback as 5th abstraction) both extend/correct this framing.
- The operational implementation of Perception Phase 2 — see `leak-budgets.md` for the L1/L2 audit mechanism that tracks the retire curve.

Do not act on this ADR. Read [ADR-0005](../accepted/ADR-0005-promote-perception-to-fourth-abstraction.md) instead.

## Known issues

> This ADR is **Superseded**; the "known issues" recorded below are the
> ones that were accepted *at acceptance time* (2026-08-24) and motivated
> the eventual supersession. They are retained for historical audit
> rather than as current operating constraints.

- **Issue**: The "3 abstractions" tagline in the README continued to
  over-claim while this ADR was active (2026-08-24 → 2026-08-25).
  During that ~24-hour window, anyone reading the README + ADR-0001
  together received a self-contradictory signal: README said "3",
  ADR-0001 said "actually 2.5 plus an empty slot".
- **Why accepted**: the over-claim window was bounded (one day); the
  ADR itself was the corrective action, so the contradiction was
  already public and on the record. Repairing the README before the
  ADR landed would have created the inverse problem (README says
  "2.5" with no ADR explaining why).
- **Tracking**: closed by [ADR-0005](../accepted/ADR-0005-promote-perception-to-fourth-abstraction.md)
  on 2026-08-25, which promoted Perception to the 4th first-class
  abstraction and made the (now revised) "4 abstractions" claim true.
- **Mitigation (no longer applicable)**: while this ADR was active,
  callers reading the README's "3 abstractions" claim were directed
  to consult `docs/concepts/state/roadmap-matrix.md` row for
  Perception, which was entirely red — the matrix was the honest
  counter-signal against the README's tagline.

- **Issue**: Path (c) (Conal Elliott's denotative route) was recorded
  as a viable candidate but never prototyped during this ADR's
  active window. The decision to defer prototyping until
  evidence-of-need meant the cleanest theoretical path was unevidenced.
- **Why accepted**: the v0.1 codebase was 48 hours old when this ADR
  was accepted; prototyping all three paths (a/b/c) before choosing
  would have stalled the project. The ADR's role was to *name* the
  paths, not to evaluate them.
- **Tracking**: the (c) route's pure-function technique was
  subsequently validated by `counter_denotative.c`,
  `calculator_denotative.c`, `counter_interactive.c` (10 unit tests
  pass). ADR-0005's (b) route absorbed (c)'s pure-function
  implementation while keeping the philosophically-honest 4th-abstraction
  framing. Path (c) is therefore preserved as engineering technique,
  superseded as philosophical stance.
- **Mitigation (historical)**: callers wanting pure-function render
  could use the `on_render(fb, user)` callback pattern — which is
  structurally the (c) route without the philosophical commitment.

## HISTORY

- 2026-08-24: Proposed
- 2026-08-24: Accepted
- 2026-08-25: Superseded by ADR-0005 (Promote Perception to fourth abstraction)

## References

- Code: `src/closure.c:131` — the no-op placeholder
- Code: `include/planex/planex.h:246` — Perception listed as Stage 5
- Related docs: `docs/concepts/state/roadmap-matrix.md` — Perception row is entirely red
- External: Don Norman, *The Design of Everyday Things* — 7-stage interaction model
- External: Conal Elliott, "Denotational Design with Type Class Morphisms" — the (c) path
- Related ADRs: ADR-0002 (Relation necessity pending undo) — similar pattern of "claimed but unproven"
