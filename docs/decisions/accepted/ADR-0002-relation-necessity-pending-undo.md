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

- The matrix in `docs/concepts/state/roadmap-matrix.md` row for Relation, Proof-of-concept column remains 🔴 until undo-via-graph is implemented.
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

## CAVEATS

This ADR records a *diagnosis* (Relation's necessity is unproven) and *names a test* (undo-via-graph) — it does NOT:

- Prove Relation is necessary. The undo-via-graph existence proof is a *forward-looking* criterion, not a *retroactive* validation. Until the proof lands, the claim is on credit.
- Prove Relation is unnecessary. The flip side — if Solid.js's reactive primitives can also do undo-via-graph, Relation would be demoted. Neither direction is closed; both remain open.
- Dictate the implementation path. Whether undo-via-graph is implemented as a snapshot-only-on-triggered-closure optimization, a global graph query API, or a per-Closure subgraph walk is a separate engineering decision.
- Close the broader "is Planex's abstraction set correct?" question. This ADR covers only Relation. Estimate and Closure have separate existence proofs (animation time; 7-stage loop) that this ADR does not touch.
- Address the v4 essence-rederivation proposal (Interpretant / Perlocution / Breakdown) — those are tracked in `essence-derivation-v4-clean.md` and ADR-0010 and are orthogonal to Relation's necessity.

The decision here is narrowly scoped: acknowledge the gap, name the test. All downstream consequences (whether to demote Relation, when to implement undo, what API shape) are out of scope.

## Known issues

- ~~**Issue**: Relation's necessity remains unproven as of v0.5. The undo-via-graph test named in this ADR has not been implemented; demos still use Relation as declarative sugar over what could be Solid-style signals.~~ **RESOLVED in v0.5.** `examples/undo_via_graph.c` (7 tests, CI runs it on Linux + Windows) is the undo-via-graph proof named in this ADR. The example's header explicitly states *"This closes ADR-0002: Relation's necessity is proven."* See the `## Resolution` section below.
- ~~**Issue**: Without undo-via-graph, Planex's anti-pattern claim against Solid.js ("Solid tracks dependencies per-effect; Planex tracks them as a globally queryable graph") is asserted but not demonstrated. Skeptics can fairly ask "show me the code where Solid can't do X".~~ **RESOLVED in v0.5** (same example). `examples/undo_via_graph.c` includes a Solid.js comparison narrative in its stdout output (captured in `examples/undo_via_graph.expected`); the demonstration is no longer missing.

## Resolution

**Resolved on 2026-08-28** by the landing of [`examples/undo_via_graph.c`](../../../examples/undo_via_graph.c) — a 7-test example that:

1. Binds a `Closure` to a `Relation` graph via `px_closure_bind_graph(inc, graph)`.
2. Enables undo via `px_undo_set_enabled(true)`.
3. Triggers `Closure` multiple times — each trigger auto-snapshots only the `Estimate`s reachable via `TRIGGERS` from that `Closure`.
4. Asserts via 7 tests that unrelated `Estimate`s stay untouched (the final test verifies `unrelated` stays at 999 across all operations).
5. Includes a Solid.js comparison narrative in stdout (captured in `examples/undo_via_graph.expected` and asserted by Gate 10 `make check-examples`).

CI runs the example on both `linux-cmake` (`./build/undo_via_graph`) and `windows` (`.\build\Release\undo_via_graph.exe`); a regression that removes the proof breaks the build.

This ADR remains `Accepted` (not `Superseded`) because the diagnosis it recorded ("Relation's necessity is unproven") was correct at the time, the named test (undo-via-graph) was the correct test, and the resolution (the test landed and proved the claim) is the project working as designed. The ADR is the historical record of the diagnosis + the named test + the eventual resolution.

## HISTORY

- 2026-08-24: Proposed
- 2026-08-24: Accepted
- 2026-08-27: Open question (undo-via-graph implementation) confirmed still-open at v0.4 cycle close; no supersession, no deprecation
- 2026-08-28: Confirmed still-open at v0.5 leak-budget retire (ADR-0013); explicitly deferred to v1.0 cycle
- **2026-08-28: RESOLVED.** `examples/undo_via_graph.c` (7 tests, CI runs on Linux + Windows) is the undo-via-graph proof this ADR named. Relation's necessity is proven. See `## Resolution` above.

## References

- Code: `src/relation.c` — current Relation implementation
- Code: `include/planex/planex.h` — Relation API surface
- Related docs: `docs/concepts/state/roadmap-matrix.md` — Relation row, Proof column
- External: Solid.js (https://www.solidjs.com) — the strongest alternative to Relation
- External: Christopher Alexander, "A City is Not a Tree" (1965) — theoretical foundation for Relation
- External: Sketchpad (Sutherland 1963) — first constraint-graph UI
- Related ADRs: ADR-0001 (Perception no-op) — same pattern of "claimed but unproven"
