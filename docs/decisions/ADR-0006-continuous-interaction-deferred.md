# ADR-0006: Continuous interaction abstraction deferred to v1.0+

## Status

Accepted

Date: 2026-08-26

## Context

The [UI Pattern Coverage Matrix](../concepts/ui-pattern-coverage.md) systematically checked 68 common UI patterns against Planex's 4 abstractions. The results:

| Category | ✅ Clean | ⚠️ Forced | ❌ Cannot | Total |
|---|---|---|---|---|
| A: Discrete state | 12 | 0 | 0 | 12 |
| B: Animation & time | 6 | 0 | 0 | 6 |
| C: Undo & history | 1 | 2 | 2 | 5 |
| **D: Continuous/transient interaction** | **0** | **11** | **4** | **15** |
| E: Layout & spatial | 2 | 4 | 0 | 6 |
| F: Async & external data | 4 | 4 | 0 | 8 |
| G: Multi-window | 2 | 2 | 1 | 5 |
| H: Accessibility | 3 | 3 | 0 | 6 |
| I: Extension | 1 | 3 | 1 | 5 |

**Category D has 0/15 clean patterns.** Every continuous/transient interaction (hover, drag, gesture, scroll, pinch) is either forced into Estimate (semantically wrong) or cannot be expressed at all.

### Root cause analysis

Continuous interactions share these properties:

- **Have time dimension** (like Estimate) — they unfold over multiple frames
- **Can be cancelled** (like Closure) — user can abort a drag mid-way
- **Are NOT persistent state** — hover ends, drag commits, gesture completes
- **Are NOT discrete intent** — mouse movement is not a speech act

They are a **5th kind of thing** — continuous processes that don't fit any of the 4 existing abstractions cleanly.

This was first observed in [continuous-intent-speculation.md](../concepts/continuous-intent-speculation.md) as a hypothesis. The pattern coverage matrix confirms it with 15 data points.

### Forces

1. **Path C failure modes**: Adding a 5th abstraction now triggers at least 3 of the 7 documented failure modes (simultaneous change, theoretical ambition without proof, no incremental adoption). See [path-C-lineage.md](../concepts/path-C-lineage.md).

2. **4 abstractions just stabilized**: Matrix just turned all green (v0.3). API just migrated (v0.2). 78 tests just passing. Adding a 5th abstraction now means a new matrix row, all red, on an unstable foundation.

3. **Insufficient evidence**: The 0/15 finding is from paper analysis. No real demo has been written to measure how painful the Estimate hack actually is. Some patterns might be tolerable with hacks; others might be impossible. We don't know which.

4. **Philosophical uncertainty**: "Intent is continuous" is contested (see [alternative-perspectives.md](../concepts/alternative-perspectives.md)). Winograd/Flores treat intent as discrete speech acts. Card/Moran/Newell GOMS treats interaction as discrete stages. Only modern motor-planning research suggests continuity. The philosophical foundation for a 5th abstraction is not yet solid.

## Decision

**Defer the continuous interaction abstraction to v1.0+. Do NOT implement it in v0.x.**

Instead:

1. **Document the boundary** — [Limitations L12](../concepts/limitations.md) records 0/15 clean patterns
2. **Write a boundary-exposing demo** — `hover_drag_4abs.c` implements hover + drag using Estimate hacks, with inline comments marking each hack. The demo's experience will be the evidence for future decisions.
3. **Update continuous-intent-speculation.md** — upgrade from "speculation" to "confirmed by pattern analysis"
4. **Write ADR-0007 only if the demo proves the hack is intolerable** — if `hover_drag_4abs.c` shows that Estimate hacks are workable, no 5th abstraction is needed. If it shows they're intolerable, ADR-0007 will propose the 5th abstraction with concrete API design based on real hack experience.

## Essence Check

### Q1. Which essence axis does this decision affect?

**None directly.** This is a "do not add" decision, not a "change" decision. The 4 existing abstractions remain unchanged. The 5th axis (continuous interaction) is acknowledged but not implemented.

### Q2. Does it compress or increase human cognitive bandwidth?

- **Compresses**: by documenting the boundary, users know Planex's scope clearly. No false expectation that hover/drag are first-class.
- **Increases**: users who need hover/drag must use Estimate hacks — more cognitive load per pattern.
- **Net**: compresses (honest scope > false expectation).

### Q3. Is there a gap between the claim and the implementation?

- **Claim**: Planex has 4 abstractions covering UI essence's 4 axes.
- **Implementation**: 4 abstractions implemented. Continuous interaction is documented as out of scope (L12).
- **Gap**: None. The claim is "4 abstractions" — it doesn't claim "covers all UI patterns." L12 explicitly documents the boundary.

### Q4. What is the cost, and who can verify it?

- **Cost**: hover/drag/gesture patterns require Estimate hacks. Each hack is ~20-30 extra lines per pattern.
- **Verifier**: anyone writing a Planex app with hover/drag will encounter the hack.
- **Verification scenario**: `hover_drag_4abs.c` demo — if the author finds the hack tolerable, the cost is acceptable. If not, ADR-0007.

### Q5. What are the counterexamples?

- **If a user only needs discrete state UI** (counter, form, todo): no impact. L12 doesn't affect them.
- **If a user needs hover/drag**: must use hacks. L12 documents this.
- **If a user needs gesture/touch**: ❌ cannot express. L12 documents this. Planex doesn't target touch UI (see [NG-6](../concepts/non-goals.md)).

**Scope statement**: This decision applies to v0.x. v1.0+ may revisit based on evidence from `hover_drag_4abs.c` and user feedback.

## Consequences

### Positive

- 4 abstractions remain stable — no API breakage
- Matrix stays all green — no new red row
- Honest documentation of boundary — users know what to expect
- Evidence-gathering approach (demo before abstraction) follows Path C lessons

### Negative

- 15/68 UI patterns are forced or impossible — significant coverage gap
- Users who need hover/drag must use hacks — painful
- Planex cannot target gesture-heavy UIs (touch, creative tools) until this is resolved

### Neutral

- The 5th abstraction question remains open — neither accepted nor rejected. Evidence will decide.

## Alternatives Considered

### Alternative 1: Implement 5th abstraction now

- **What**: Add `px_interaction` or `px_flow` abstraction for continuous processes
- **Why rejected**: Path C failure modes (3+ triggered). 4 abstractions just stabilized. Insufficient evidence (paper analysis only). Philosophical foundation uncertain. See Context above.

### Alternative 2: Absorb into Estimate (Conal-style)

- **What**: Extend Estimate to handle continuous input streams (e.g., mouse position as `Behavior<Position>`)
- **Why rejected**: Semantically wrong. Estimate is "state with time + uncertainty." Mouse position is not state — it's transient input. Conflating them would make Estimate's semantics unclear, breaking the anti-pattern test for Estimate.

### Alternative 3: Absorb into Closure

- **What**: Add a `PX_INTENT_CONTINUOUS` kind or "process Closure" that spans multiple frames
- **Why rejected**: Closure is "intent → action → evaluation" — a discrete unit. Making it span frames would break the speech-act model (ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS are discrete acts, not continuous processes).

### Alternative 4: Ignore the boundary

- **What**: Don't document, don't write demo, pretend it doesn't exist
- **Why rejected**: Violates Planex's "honest acknowledgment" commitment (see [path-C-lineage.md](../concepts/path-C-lineage.md)). Research-grade projects must document their boundaries.

## References

- [UI Pattern Coverage Matrix](../concepts/ui-pattern-coverage.md) — the 68-pattern analysis that revealed 0/15 in Category D
- [Limitations L12](../concepts/limitations.md) — the limitation entry
- [Continuous Intent Speculation](../concepts/continuous-intent-speculation.md) — the original observation (now confirmed)
- [Path C Lineage](../concepts/path-C-lineage.md) — failure modes that inform this decision
- [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md) — the 4th abstraction decision (this ADR defers the 5th)
- `examples/hover_drag_4abs.c` — the boundary-exposing demo (to be written)
