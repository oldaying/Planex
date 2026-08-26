# ADR-0008: Feedback as 5th essence category (v0.4)

## Status

Accepted

Date: 2026-08-27

Implements: the gap acknowledged in [ADR-0007](ADR-0007-essence-derivation-v2-revision.md).

## Context

Per [essence-derivation-v2.md](../concepts/essence-derivation-v2.md) and ADR-0007, Feedback / closed-loop coupling was identified as a 5th essence category — distinct from State / Communication / Presentation / Relational ontology. ADR-0007 acknowledged the gap but did not close it. This ADR closes it.

Feedback is the closed loop of:
1. user intent → action (Closure)
2. action → state change (Estimate)
3. state change → perception (Perception)
4. perception → next intent (user)

Before v0.4, this loop was **implicit** — application code manually called `px_perception_invoke_for_estimate()` or `px_perception_invoke_all()` after `px_closure_trigger()`. The loop existed structurally but was not first-class.

### Forces

1. **v2's finding stands**: Feedback is essence, not feature. Cross-tradition convergence (history, HCI, modern architecture, phenomenology, math) confirmed this.

2. **Planex's essence-driven commitment**: An essence-driven project cannot leave an identified essence category as implicit engineering. If Feedback is essence, it must be first-class.

3. **Closing the gap tests v2**: If Feedback cannot be designed as first-class without breaking the 4 existing abstractions, v2 was wrong. Designing it is the test.

4. **Closing the gap reveals limits**: The design process exposed that Planex's perception API is global (no `px_perception_invoke(p)` to invoke a single perception). This is documented as a v0.4 limitation; future API refinement can address it.

### Constraints

- The 4 existing abstractions (Estimate, Closure, Perception, Relation) remain as-is.
- C17 implementation (per ADR-0004).
- Zero external dependencies.
- Backward compatibility is NOT required (Planex is pre-v1.0).

## Decision

### D1. Add `px_loop` as the Feedback abstraction

A `px_loop` binds a Closure (intent side) to a Perception (view side) into a first-class closed loop. It does NOT replace the manual `px_closure_trigger + px_perception_invoke_all` pattern — that still works. `px_loop` is for callers who want audit, interruption, replay, or breakdown detection.

### D2. API surface

```c
typedef struct px_loop px_loop;

px_loop* px_loop_new(px_closure* c, px_perception* p);
void     px_loop_free(px_loop* loop);

/* One full iteration: trigger closure, invoke perception. */
int     px_loop_step(px_loop* loop, void* payload, size_t size);

/* View-only refresh: invoke perception without triggering closure. */
int     px_loop_step_view_only(px_loop* loop);

/* Pause/resume the loop. */
void    px_loop_pause(px_loop* loop);
void    px_loop_resume(px_loop* loop);
bool    px_loop_is_paused(const px_loop* loop);

/* Audit log: each iteration records (triggered? perceived? timestamp). */
typedef struct {
    bool   closure_triggered;
    bool   perception_invoked;
    double timestamp_ms;
} px_loop_audit_entry;

int     px_loop_audit_count(const px_loop* loop);
int     px_loop_audit_get(const px_loop* loop, px_loop_audit_entry* out, int max);
int     px_loop_replay(px_loop* loop, int n);  /* replay last n iterations */
void    px_loop_audit_clear(px_loop* loop);
```

### D3. Implementation notes

- `px_loop` does NOT own closure or perception. Caller frees them.
- Audit log is a fixed-size ring buffer (1024 entries). When full, oldest entries are overwritten. Production may want growable storage.
- `timestamp_ms` is `px_now_ms()` at iteration start.
- `px_loop_replay` re-triggers the closure using its `last_intent` payload. For closures whose action depends on payload, this replays with the same payload bytes. For stateless actions (like inc), replay re-runs the action with whatever state currently exists.
- The loop currently uses `px_perception_invoke_all()` to invoke perception — broader than strictly needed (invokes all perceptions, not just the loop's). This is a v0.4 limitation; a future `px_perception_invoke(p)` API would let the loop target its specific perception.

### D4. What this enables (per essence-driven design)

Planex v0.3 could not do these. v0.4 can:

1. **Audit the loop**: `px_loop_audit_get` answers "which perception fired after which trigger?" — impossible in v0.3.
2. **Interrupt the loop**: `px_loop_pause` lets you batch closure triggers without re-rendering between each — impossible in v0.3.
3. **Replay the loop**: `px_loop_replay` re-runs recorded iterations for testing/debugging — impossible in v0.3.
4. **Detect breakdown**: a perception that fails to fire leaves a `perception_invoked=false` audit entry — loop stalled is now visible, not silent.

These capabilities are essence-driven: they correspond to the philosophy/HCI traditions' treatment of feedback as primitive (CSP's trace, statechart's transition, Heidegger's breakdown, Norman's gulf of evaluation).

## Essence Check

### Q1. Which essence axis does this decision affect?

**Feedback** — the 5th essence category identified by v2. This ADR closes the gap acknowledged in ADR-0007.

### Q2. Does it compress or increase human cognitive bandwidth?

**Compresses**: callers who want audit/interrupt/replay/breakdown no longer have to hand-roll it. The loop's first-class status makes the closed loop visible in the API, not hidden in application code.

**Increases**: callers now have one more concept (`px_loop`) to learn. This is the cost of closing an essence gap — the new concept corresponds to a real essence category, not a feature.

### Q3. Is there a gap between the claim and the implementation?

**Before this ADR**: claim said "4 of 5 essence categories implemented, Feedback partial". Implementation matched (Feedback was implicit). The gap was acknowledged but not closed.

**After this ADR**: claim says "5 of 5 essence categories implemented". Implementation matches — `px_loop` is first-class. No gap.

### Q4. What is the cost, and who can verify it?

- **Cost**: `px_loop` adds a new struct + 11 functions. Implementation is ~200 lines of C. Memory: 1024-entry audit buffer per loop (16KB per loop on 64-bit systems).
- **Verifier**: any caller who wants audit/interrupt/replay/breakdown. Test suite `tests/test_feedback.c` (13 tests, all pass) verifies the API works as designed.
- **Verification scenario**:
  - Batch updates: caller triggers 100 closures in a tight loop with `px_loop_pause`, then resumes for 1 perception invocation (1 render instead of 100).
  - Test replay: caller records a trigger→perceive sequence, resets state, replays — final state matches original.

### Q5. What are the counterexamples?

- **Counterexample 1**: Callers who don't want audit/interrupt/replay can ignore `px_loop` entirely. The existing `px_closure_trigger + px_perception_invoke_all` pattern still works. `px_loop` is opt-in.

- **Counterexample 2**: `px_loop` currently uses `px_perception_invoke_all()` (global) instead of `px_perception_invoke(p)` (specific). This means a loop will invoke other registered perceptions, not just its own. Future v0.5+ should add `px_perception_invoke(p)` to scope this.

- **Counterexample 3**: Replay uses `last_intent` payload, which is overwritten on each trigger. If a caller wants to replay 5 different payloads, they need to capture each before triggering. This is documented in the API; future API refinement could add explicit payload capture.

### Scope Statement

This ADR applies to Planex's Feedback essence category. The `px_loop` abstraction is the implementation of this category. It does **not** apply to:
- The 4 existing abstractions (unchanged)
- The deferred essence candidates (Embodiment, Situatedness, Affordance-as-relation, Breakdown) — these remain deferred per ADR-0007
- Future API refinements like `px_perception_invoke(p)` — those are separate decisions

## Consequences

### Positive

- **5 of 5 essence categories implemented**. Planex's essence claim is now complete (within Layer 1-3 scope).
- **Audit, interrupt, replay, breakdown detection** are now first-class — these were the capabilities ADR-0007 said Planex lacked.
- **v2 validated**. The fact that Feedback could be designed as first-class without breaking the 4 existing abstractions supports v2's analysis.
- **Test coverage**: 13 new tests in `tests/test_feedback.c` validate the API works.

### Negative

- **One more concept to learn**: `px_loop` is a 5th abstraction (or a 5th essence instantiation). Contributors must understand when to use it vs the manual pattern.
- **`px_perception_invoke_all()` over-perceives**: a loop currently invokes all registered perceptions, not just its own. This is a v0.4 limitation.
- **Replay uses last_intent**: replay re-triggers with the closure's last payload. Callers wanting multi-payload replay must capture payloads manually.
- **Audit buffer is fixed-size**: 1024 entries, oldest overwritten. Production may want growable storage.

### Neutral

- **Backward compatible**: existing code using `px_closure_trigger + px_perception_invoke_all` still works. `px_loop` is opt-in.
- **No breaking API changes**: 4 existing abstractions unchanged.

## Alternatives Considered

### A1. Don't close the gap — leave Feedback implicit

Rejected. ADR-0007 explicitly identified Feedback as essence. Leaving it implicit violates Planex's essence-driven commitment. The honest-claim requirement forces closing it.

### A2. Make Feedback implicit but add audit hooks

Rejected. Audit without a first-class loop concept means callers still hand-roll the loop and just attach audit. This doesn't make the loop first-class — it just observes an implicit loop. Essence-driven design requires the loop itself to be the abstraction.

### A3. Wait for use cases before designing

Rejected. v2 already identified Feedback as essence via cross-tradition convergence. The design pressure is sufficient. Waiting would leave the gap open indefinitely.

### A4. Use `px_perception_invoke(p)` instead of `invoke_all`

Not rejected — deferred to v0.5+. The v0.4 API uses `invoke_all` because Planex's perception API doesn't have `invoke(p)` yet. Adding it would require changes to perception.c and the perception registry. v0.4 keeps the scope minimal; v0.5+ can refine.

## References

- [ADR-0007](ADR-0007-essence-derivation-v2-revision.md) — acknowledged the Feedback gap; this ADR closes it
- [essence-derivation-v2.md](../concepts/essence-derivation-v2.md) — the derivation that identified Feedback as essence
- [why-four-abstractions.md](../concepts/why-four-abstractions.md) — canonical manifesto, updated for v0.4
- [limitations.md](../concepts/limitations.md) — L13 (Feedback gap) marked as resolved
- [tests/test_feedback.c](../../tests/test_feedback.c) — 13 tests validating the Feedback API
- [src/feedback.c](../../src/feedback.c) — implementation
- [research/reports/00-summary.md](../../../research/reports/00-summary.md) — research reports that grounded v2

## See also

- [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md) — Perception's promotion (precedent for adding an abstraction)
- [ADR-0006](ADR-0006-continuous-interaction-deferred.md) — another deferred essence ADR
