# ADR-0002: Relation's necessity is not yet proven

## Status

Accepted

Date: 2026-08-24

## Context

Planex's central claim is that UI should be built on three abstractions: **Relation**, **Estimate**, **Closure** — replacing React's Component + State + Event.

For each of the three, the project must answer: **why is this abstraction necessary, not just elegant?**

- Estimate has an existence proof: time-sampled animation. Without Estimate's `Behavior = Time → Value`, you fall back to `setTimeout + requestAnimationFrame` boilerplate.
- Closure has an existence proof: the 7-stage loop with auto-evaluation. Without Closure, `onClick={fn}` buries intent and offers no failure semantics.

**Relation currently has no comparable existence proof.** The demos (`counter`, `slider`, `radio`, `form`, `tabs`, `wizard`) all use Relation to declare dependencies — but Solid.js's Signal + dependency graph does the same thing. To a skeptic, Relation may look like syntactic sugar over Solid's reactive primitives.

This is a problem. If Relation is not necessary, the "3 abstractions" claim weakens to "2 abstractions + a convenience layer".

The strongest candidate for Relation's existence proof is **undo-via-graph**: when a Closure triggers, snapshot only the Estimates reachable from that Closure's Relation subgraph. This is impossible in Solid because Solid tracks dependencies per-effect, not as a globally queryable graph. If Planex can do this and Solid cannot, Relation is proven necessary.

Until that proof exists, this ADR records the gap.

## Decision

**Accept that Relation's necessity is currently unproven, and designate undo-via-graph as the test that would prove it.**

Specifically:

- The matrix in `docs/concepts/roadmap-matrix.md` row for Relation, Proof-of-concept column remains 🔴 until undo-via-graph is implemented.
- The matrix's Anti-pattern test column for Relation remains 🔴 until someone writes a concrete demonstration that Solid's Signal cannot do minimal-snapshot undo.
- README does **not** currently claim Relation is necessary; it claims Relation is part of the abstraction set. This is honest.

## Consequences

### Positive
- The burden of proof is explicit. Future contributors know what needs to be done to make Relation's claim true.
- A clear test (undo-via-graph) is named, not a vague "show it's useful".
- If Solid can also do undo-via-graph, we'll know early and can pivot (e.g. demote Relation to a convenience layer rather than a core abstraction).

### Negative
- Until the proof exists, the "3 abstractions" claim is partially on credit.
- The roadmap explicitly defers new widgets and docs in favor of this proof, which may slow community growth.

### Neutral
- This ADR doesn't dictate *how* to implement undo-via-graph — only that it's the test.

## Alternatives Considered

### Alternative 1: Declare Relation necessary by fiat
- **What:** Skip the proof. Argue from theory (Alexander's semilattice, Sketchpad constraint graph) that Relation must be necessary.
- **Why rejected:** Theory motivates, doesn't prove. seL4's formal proofs exist precisely because "it should work in theory" is not enough for a research-grade project. Planex must hold the same standard.

### Alternative 2: Demote Relation to a convenience layer now
- **What:** Admit Relation is just sugar over Solid-style signals. Drop it from the core abstraction set. Reduce Planex to "2 abstractions" (Estimate + Closure).
- **Why rejected:** Premature. The undo-via-graph test hasn't been done. Demoting before testing would throw away a possibly-necessary abstraction based on suspicion.

### Alternative 3: Pick a different existence proof
- **What:** Instead of undo-via-graph, prove Relation's necessity via something else (e.g. multi-window state synchronization, or hot reload).
- **Why rejected:** undo-via-graph is the cleanest because it directly uses Relation's defining feature (globally queryable graph). Other tests are weaker because they could be done without Relation's full power.

## References

- Code: `src/relation.c` — current Relation implementation
- Code: `include/planex/planex.h` — Relation API surface
- Related docs: `docs/concepts/roadmap-matrix.md` — Relation row, Proof column
- External: Solid.js (https://www.solidjs.com) — the strongest alternative to Relation
- External: Christopher Alexander, "A City is Not a Tree" (1965) — theoretical foundation for Relation
- External: Sketchpad (Sutherland 1963) — first constraint-graph UI
- Related ADRs: ADR-0001 (Perception no-op) — same pattern of "claimed but unproven"
