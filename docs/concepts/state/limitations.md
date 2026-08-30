# Limitations and Known Gaps

> **Applies to**: v0.6. What Planex claims vs. what Planex actually delivers. This document exists to keep the README's "5 abstractions" tagline honest by surfacing the gaps that the tagline hides.

Research-grade projects gain credibility by being explicit about their limitations. seL4 lists which security properties are proven and which aren't. Lean lists which axioms are consistent and which are open. Planex must do the same.

This document is the **canonical source of truth** for "what's not done yet." If a gap is not listed here, it's either (a) actually done or (b) a bug — file an issue.

---

## L1: Perception — Phase 1 done, Phase 2 done as of v0.5

**Severity:** ~~Medium — Phase 1 closed the API gap; Phase 2 will close the runtime gap~~ **Closed in v0.5.**

**Status:** **RESOLVED (Phase 1 + Phase 2).** As of v0.5, `px_estimate_set` auto-invokes dependent perceptions via `px_perception_invoke_for_estimate(e)` internally. The three manual invoke ops (`invoke_all`, `invoke_single`, `invoke_for_estimate`) remain in the public API as **diagnostic seams** — their raison d'être is no longer abstraction incompleteness; they exist for testing, debugging, and view-only refresh outside a `px_loop`.

### History (superseded)

Originally (v0.1.0), Planex claimed "4 abstractions" but Perception was a no-op placeholder in `Closure`'s stage 5. This was a structural gap documented in [ADR-0001](../../decisions/superseded/ADR-0001-perception-currently-noop.md) (now superseded). <!-- stale-allow: historical claim about v0.1.0 state; the "4 abstractions" tagline was real at that version -->

### Resolution (ADR-0005, v0.2)

[ADR-0005](../../decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md) promoted Perception to the 4th first-class abstraction:

- New `px_perception` struct + API (`px_perception_new`, `px_perception_free`, `px_perception_count`)
- `Closure` restructured from 7 stages to 5 stages (execution side only)
- `px_closure_new` signature changed: removed `perception` parameter
- 25 legacy 3-abstraction-era demos removed; new `counter_4abs.c` and `multi_perception.c` demos added

### Phase 2 resolution (ADR-0013, v0.5)

[ADR-0013](../../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) landed Phase 2 auto-invocation:

1. `px_estimate_set` calls `px_perception_invoke_for_estimate(e)` after `notify(e)`.
2. Perceptions fire automatically when their source estimates change.
3. The three manual invoke ops remain as diagnostic seams (testing, debugging, view-only refresh outside a `px_loop`).
4. A representamen cache (`last_representamen` + `has_last` fields on `struct px_perception`) avoids the double-fire bug between auto-invocation and `px_loop_step`'s `invoke_single`.

**Quantitative confirmation (closed):** this gap was also quantified in [`leak-budgets.md`](../canonical/leak-budgets.md) — Perception's L2 (semantic-leak) rate was **50%** at v0.4 (3 of 6 public operations existed only as workarounds for the missing Phase 2 auto-invocation). **As of v0.5, Perception L2 = 0%** (the 3 leaks retired). The single biggest leak-budget win available without introducing new abstractions has been realized.

**Status in roadmap matrix:** Perception row — Theory ✅, Proof ✅ (Phase 2 landed), Engineering ✅, Docs ✅, Anti-pattern 🔴 (the `antipattern_perception.c` demo still uses manual invocation; should be updated to show the auto-invocation path). <!-- fresh-allow: 🔴 here refers to demo content staleness (P2.4), not non-existence; antipattern_perception.c exists and CI runs it -->

---

## L2: Relation's necessity — RESOLVED in v0.5 (undo-via-graph implemented)

**Severity:** ~~Medium — affects the abstraction's credibility~~ **Closed in v0.5.** Relation is no longer "on credit"; the undo-via-graph existence proof named in [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md) has been implemented and run in CI.

**Status:** **RESOLVED.** The undo-via-graph proof is in [`examples/undo_via_graph.c`](../../../examples/undo_via_graph.c), a 7-test example that:

1. Binds a `Closure` to a `Relation` graph via `px_closure_bind_graph(inc, graph)`.
2. Enables undo via `px_undo_set_enabled(true)`.
3. Triggers `Closure` multiple times — each trigger auto-snapshots only the `Estimate`s reachable via `TRIGGERS` from that `Closure`.
4. Calls `px_undo()` and asserts that only the affected `Estimate`s are restored; unrelated `Estimate`s are untouched (the [final test](../../../examples/undo_via_graph.c) verifies `unrelated` stays at 999 across all 7 operations).

The file's header explicitly states: *"This closes ADR-0002: Relation's necessity is proven."* CI runs this example on both `linux-cmake` (`./build/undo_via_graph`) and `windows` (`.\build\Release\undo_via_graph.exe`); a regression that removes the proof breaks the build.

### Why this closes the necessity claim

Solid.js tracks dependencies per-effect — each `createEffect` knows its sources, but there is no global query "which effects depend on this signal?". To do undo-via-graph in Solid, you must either (a) snapshot everything (Redux-style, expensive), or (b) maintain a separate dependency index (= rebuilding Relation). Planex's `Relation` is a globally queryable graph — undo-via-graph is a natural consequence, not syntactic sugar.

### Status in roadmap matrix

[roadmap-matrix.md](roadmap-matrix.md) row for Relation, Proof-of-concept column is **✅** (was 🔴 through v0.4). The matrix was updated when `undo_via_graph.c` landed; this limitations entry is now closed to match.

### History (superseded)

Originally (v0.1.0 – v0.4), Planex honestly acknowledged that Relation's necessity was unproven — the abstraction's existence was on credit pending the undo-via-graph test. [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md) recorded this gap explicitly and named undo-via-graph as the test. The implementation landed as `examples/undo_via_graph.c` (7 tests), CI runs it across both Linux and Windows build matrices, and the test header explicitly closes ADR-0002. The ADR's `## Known issues` section has been updated to reflect the resolution (the gap is no longer open); the ADR itself remains `Accepted` as the historical record of the diagnosis + the named test + the eventual resolution.

---

## L3: Anti-pattern tests — RESOLVED in v0.4 (antipattern_*.c landed)

**Severity:** ~~Medium — affects defensibility against skeptics~~ **Closed in v0.4.** All shipping abstractions now have concrete anti-pattern demonstrations that show a capability Planex has and React / Solid / plain callbacks cannot cleanly express.

**Status:** **RESOLVED.** Three anti-pattern examples are committed in [`examples/`](../../../examples/):

| Abstraction | Example | Anti-patterns demonstrated | CI job |
|---|---|---|---|
| Estimate | [`examples/antipattern_estimate.c`](../../../examples/antipattern_estimate.c) | 3 — time-sampled animation; `Behavior = Time → Value`; confidence-first-class | `linux-cmake` runs `./build/antipattern_estimate` |
| Closure | [`examples/antipattern_closure.c`](../../../examples/antipattern_closure.c) | 5 — 5-stage loop with auto-evaluation; typed intent as value; promise/declare/fail | `linux-cmake` runs `./build/antipattern_closure` |
| Perception | [`examples/antipattern_perception.c`](../../../examples/antipattern_perception.c) | 4 — multi-denotation of same state; pure-function rendering; independent per-channel lifecycle | `linux-cmake` runs `./build/antipattern_perception` |

Each example's `*.expected` snapshot (committed under `examples/`) is asserted by Gate 10 (`make check-examples`); a regression that changes an anti-pattern example's output breaks CI.

### Why this closes the defensibility claim

Before these examples landed, Planex's anti-pattern claim against React / Solid was asserted but not demonstrated — a skeptic could fairly ask "show me the code where Solid can't do X". The three `antipattern_*.c` files are that code. Each one constructs a Planex implementation, then explicitly narrates (in the example's stdout, captured in the `.expected` file) what React / Solid / plain callbacks would need to do to replicate it, and why the replication is worse (more boilerplate, less testable, or fundamentally breaks the abstraction's claim).

### Status in roadmap matrix

[roadmap-matrix.md](roadmap-matrix.md) Anti-pattern test column is **✅** for all four abstractions in the v0.3 matrix (Relation, Estimate, Closure, Perception — the matrix pre-dates px_loop's ADR-0008 promotion). This was previously 🔴 for all four through v0.2; the matrix was updated when the anti-pattern examples landed. <!-- fresh-allow: "previously 🔴" is past-tense historical narrative, not a current claim; current state is ✅ --> <!-- stale-allow: "four abstractions" refers to the v0.3 matrix's row count (4); the v0.5 shipping set is 5 abstractions (px_loop added by ADR-0008); roadmap-matrix.md is itself v0.3-snapshot -->

### Caveat (P2.4 — separate open gap, not L3)

[`examples/antipattern_perception.c`](../../../examples/antipattern_perception.c) currently uses manual invocation (`px_perception_invoke_all()` + `px_perception_invoke_for_estimate(e)`) to demonstrate the multi-denotation pattern. After Phase 2 auto-invocation landed (ADR-0013, v0.5), this demo should be updated to show the auto-invocation path. This is tracked as **P2.4** in [doc-organization.md](../../doc-organization.md) Part IX deferral table; it is not a re-opening of L3 — the demo exists, runs in CI, and demonstrates the anti-pattern claim. The content refresh is a polish task, not a gap-closure.

---

## L4: Undo / redo — partially RESOLVED in v0.5 (undo-via-graph landed; redo not yet)

**Severity:** ~~Low for users, High for the abstraction claim~~ **Partially closed in v0.5.** `px_undo()` and the undo recording API exist, are tested in CI via `examples/undo_via_graph.c`, and prove the Intent-as-value → undo-via-graph pipeline. `px_replay()` and `px_redo()` remain unimplemented.

**Status:** **PARTIALLY RESOLVED.** The public undo API landed in [`include/planex/planex.h`](../../../include/planex/planex.h) lines 542–568 + [`src/undo.c`](../../../src/undo.c) (174 LOC):

| API | Purpose | CI verification |
|---|---|---|
| `px_undo_record(g, c)` | Snapshots affected `Estimate`s before a `Closure` action | `examples/undo_via_graph.c` test 1 |
| `px_undo()` | Restores the last snapshot | test 2 + test 3 (chain) |
| `px_undo_count()` | Returns remaining undo steps | tests 4 + 6 |
| `px_undo_clear()` | Clears undo history | (utility; not directly tested) |
| `px_undo_set_enabled(bool)` | Toggle undo recording at runtime | test 6 |
| `px_undo_is_enabled()` | Query current toggle state | (utility) |

The undo path is the *scoped* undo-via-graph design (only `Estimate`s reachable via `TRIGGERS` from the triggering `Closure` are snapshotted); see L2 above for why this design simultaneously proves `Relation`'s necessity.

### What is still open

- **`px_replay()` is not yet implemented.** Closure's Intent-as-value design enables replay (the intent stream can be re-fed into the closure), but no public replay API is shipped. The `Intent-as-value` claim is therefore about the *enabling property* (undo, which is now landed), not about *full round-trip* (replay, which remains open).
- **`px_redo()` is not yet implemented.** The forward-equivalent of `px_undo()`; needs an undo-stack-walk in the opposite direction. Trivially implementable on top of the existing `px_undo_record` snapshot infrastructure.

### Implication

Users looking for "undo out of the box" get it (via `px_undo_record` + `px_undo` + `px_undo_set_enabled`). Users looking for full "undo/redo round-trip" or "intent stream replay" still need to wait for the follow-up `px_replay` / `px_redo` API — tracked in [doc-organization.md](../../doc-organization.md) Part IX deferral table.

### Relationship to L2

L2 (Relation necessity) is closed by undo-via-graph. L4 (undo/redo system) is partially closed by the same implementation. The two limitations are linked by design: undo-via-graph simultaneously proves Relation's necessity and ships the undo half of the Intent-as-value → undo/redo pipeline. A future `px_replay` / `px_redo` follow-up closes the second half of L4 without re-touching L2.

---

## L5: macOS backend is code-complete but untested

**Severity:** Low — affects portability

`src/cocoa.c` exists and compiles, but has not been tested on actual macOS hardware. There may be runtime issues that only surface on real hardware.

**Workaround:** Linux (X11) and Windows (Win32 GDI) are the tested backends. macOS users should consider this backend experimental.

**Status:** See `PLATFORMS.md`.

---

## L6: GPU rendering not yet available

**Severity:** Low — affects performance ceiling

Current rendering is software rasterization (CPU framebuffer, blitted to window via XShm / BitBlt / NSBitmapImageRep). This is fast enough for typical UI (60fps at 1080p with reasonable widget counts) but does not scale to:
- 4K+ resolutions with complex scenes
- High-frequency animation (120Hz+)
- 3D / perspective transforms
- Shader effects

A GPU backend (Vulkan / Metal / D3D12 / WebGPU) is on the roadmap but explicitly deferred until abstraction questions (L1, L2) are resolved. See [NG-7 in non-goals](../canonical/non-goals.md).

---

## L7: No formal semantics for the abstractions

**Severity:** Low for users, High for the project's research credibility

Each abstraction has a theoretical foundation cited in `docs/concepts/canonical/why-four-abstractions.md`:

- Estimate ← Conal Elliott's FRP (`Behavior = Time → a`)
- Closure ← Don Norman's 7-stage model + Winograd/Flores speech-act theory
- Relation ← Sketchpad constraint graph + Alexander's semilattice

But these are citations, not formalizations. Planex does not provide:

- A denotational semantics for Estimate (e.g. a mathematical function `Estimate α = Time → α × Confidence`)
- A formal definition of Intent kinds (ASSERT / REQUEST / PROMISE / DECLARE / EXPRESS) as a typed algebra
- A graph-theoretic axiomatization of Relation (e.g. which relation kinds are transitive, reflexive, etc.)

**Implication:** Planex is currently "informed by theory" but not "formalized". For a research-grade project aiming at the standard set by seL4 and Lean, this is a gap.

**Decision:** This is acknowledged but not prioritized in the current roadmap. Formalization is a v1.0 concern, not a v0.x concern.

---

## L8: Memory safety relies on manual review

**Severity:** Medium — affects safety-critical use cases

Planex is written in C17 (see [ADR-0004](../../decisions/accepted/ADR-0004-use-c-not-rust-zig-cpp.md)). C provides no:
- Bounds checking
- Use-after-free protection
- Data race detection
- Ownership tracking

This means:
- `px_estimate_free(estimate)` followed by `px_estimate_value(estimate)` is undefined behavior, not a caught error.
- Concurrent access to Estimates from multiple threads without synchronization is a data race.
- Buffer overflows in user-supplied `action` / `perception` / `evaluation` callbacks are not detected.

**Implication:** Planex is not currently suitable for safety-critical systems (medical, automotive, avionics) without external static analysis (Coverity, CodeQL) and rigorous code review.

**Mitigation:** The unit tests in `tests/test_core.c` cover happy paths. Fuzzing and static analysis are planned but not yet integrated into CI.

---

## L9: Accessibility is logging-only

**Severity:** Medium — affects users with disabilities

`include/planex/a11y.h` and `src/a11y.c` provide an accessibility API: roles, states, announcements, focus management. But the current implementation only logs to stdout — there is **no bridge to actual screen readers** (AT-SPI on Linux, UIAutomation on Windows, NSAccessibility on macOS).

**Implication:** Planex UIs are not currently usable by visually impaired users relying on screen readers.

**Status:** See `PLATFORMS.md` — accessibility is marked "logging-only" for all platforms.

---

## L10: Single-maintainer bus factor

**Severity:** High — affects long-term sustainability

As of v0.1.0, Planex is a single-maintainer project. The author holds the full design context in his head; the ADRs and matrix attempt to externalize that, but no second maintainer exists who could continue the project if the original author stops.

This is acknowledged but not currently a priority. The pre-1.0 research phase is not the right time to recruit co-maintainers.

---

## L11: Multi-frame interaction processes are not abstracted

**Severity:** Medium — affects testability and debuggability of hover/drag/focus interactions

All mainstream UI libraries — including Planex — model intent as **discrete events**: click, key press, submit, hover enter, hover leave. There is no abstraction for the **continuous gradient of intent** that real users experience (approach → potential → decided → executing → retracting).

Concrete consequences:

- **Hover/drag bugs are hard to test.** There is no way to assert "at frame N during the hover-in transition, the button color should be X". Only the discrete end states (hover/not-hover) are expressible.
- **Bugs in hover/drag leak downstream.** Because UI layer cannot catch them, they surface as user-reported issues, not as test failures.
- **Confirmation dialogs are a hack for missing intent gradient.** They exist because the UI cannot express "the user's intent is at 70% — they want to do this but aren't fully committed". So the system forces a discrete yes/no.
- **"Undo" buttons are a hack for missing retraction phase.** They exist because once intent is committed (discrete), the only way to roll back is a separate action — not a natural decay of intent strength.

This is not unique to Planex. It is shared by React, Vue, SwiftUI, Flutter, Qt, GTK, Dear ImGui — all descend from the DOM/ event model inherited from 1995 web specs.

Planex's Closure uses five discrete Intent kinds (`PX_INTENT_ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS`), inherited from Winograd/Flores speech-act theory. This is the same simplification — intent is treated as a discrete act, not a continuous process.

**Implication:** Hover/drag/focus interactions in Planex apps cannot be cleanly unit-tested. Bugs in these interactions will be reported by users, not caught by tests.

**Status:** See [continuous-intent-speculation.md](../speculation/continuous-intent-speculation.md) for the future-research marker. Planex does not currently commit to fixing this — it is documented as a known shared blind spot of all UI libraries.

**Decision criteria for future action:**
- Move from "document" to "implement partial" if multiple users report hover/drag test bugs as a pain point.
- Move from "partial" to "full continuous intent" only if there is evidence that "process" alone isn't enough — users need to express intent strength, not just record interactions.

---

## L12: Continuous interaction processes — prototype landed in v0.6 (canonical promotion pending)

**Severity:** ~~High — affects 15/68 common UI patterns~~ **Partially closed in v0.6 (prototype).** The [ADR-0006](../../decisions/accepted/ADR-0006-continuous-interaction-deferred.md) evidence protocol completed: `hover_drag_4abs.c` measured the Estimate hack ("INTOLERABLE for complex gesture/touch UIs"), and [ADR-0016 (proposed)](../../decisions/proposed/ADR-0016-interaction-prototype-option-b.md) landed the Option-B prototype: `px_interaction` (process: begin → sample* → commit|cancel, inert hot path, transitions-only bridges to Closure/Estimate) + `px_region`/`px_afford_at` (intent compilation as an AFFORDS graph query) + `PX_EV_WHEEL` on all three real backends. Category D is NOT re-scored here — that is the promotion ADR's job; the canonical abstraction count remains 5.

Per `ui-pattern-coverage.md` Category D: **0/15 continuous/transient interaction patterns are cleanly expressible** with the current 5 abstractions (the 4 original + `px_loop` from v0.4; `px_loop` adds closed-loop coupling but not continuous interaction primitives — see ADR-0006 for the deferral).

Affected patterns include:
- Hover highlight (forced into Estimate — semantically wrong)
- Drag preview / drag-drop reorder (forced — drag is multi-frame process, not state)
- Mouse cursor position (forced — 60fps updates to Estimate = expensive + wrong)
- Swipe gesture / pinch-to-zoom / knob rotation (❌ cannot express)
- Scroll position (forced — high-frequency transient)
- Tooltip with delay (forced — two transient dimensions)
- Autocomplete suggestions (forced — async list + temporary selection)

**Root cause:** These patterns involve *continuous processes* — they have time dimension (like Estimate), can be cancelled (like Closure), but are NOT persistent state and NOT discrete intent. Forcing them into Estimate is semantically wrong — Estimate is "state with time + uncertainty", not "high-frequency transient input stream."

This validates `continuous-intent-speculation.md`: intent is modeled as discrete events, but real interaction is continuous.

**Status:** **PARTIALLY RESOLVED (prototype).** `px_interaction` exists in [`src/interaction.c`](../../../src/interaction.c) with 27 CI assertions ([`tests/test_v06_interaction.c`](../../../tests/test_v06_interaction.c)), including the inertness invariant (samples fire no observers and no perceptions). The boundary-closing demo [`examples/hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c) implements the same list-reorder scenario as `hover_drag_4abs.c` with 5 of 7 HACKs retired and 2 estimate-writes total (vs O(events)). What remains open: multi-touch/pointer routing (NG-6 stands), the intent gradient (Option C, speculative), Category D re-scoring, and canonical promotion (separate ADR passing the ADR-0011 admission bar).

**Decision:** See [ADR-0006](../../decisions/accepted/ADR-0006-continuous-interaction-deferred.md) for the v0.x deferral and [ADR-0016](../../decisions/proposed/ADR-0016-interaction-prototype-option-b.md) for the v0.6 prototype.

**Status in roadmap matrix:** Not tracked (would be a 6th row if promoted).

---

## L13: Feedback / Closed-loop coupling — RESOLVED in v0.4 (ADR-0008)

**Severity:** Resolved — was a gap in v0.3, closed by `px_loop` in v0.4

**History**: Per [ADR-0007](../../decisions/accepted/ADR-0007-essence-derivation-v2-revision.md) and [essence-derivation-v2.md](../history/essence-derivation-v2.md), v0.3 surfaced Feedback / closed-loop coupling as a 5th essence category — distinct from State / Communication / Presentation / Relational ontology. v0.3 implemented this loop only **implicitly** via Closure+Perception (application code manually called `px_perception_invoke_for_estimate()` after `px_closure_trigger()`).

**Resolution (v0.4, ADR-0008)**: New `px_loop` abstraction makes the loop first-class. Planex can now:
- audit which perception fired after which trigger
- interrupt the loop (pause/resume for batch updates, modal blocking)
- replay trigger→perception sequences (testing, debugging)
- detect breakdown (perception failed to fire leaves `perception_invoked=false` in audit)

13 tests in `tests/test_feedback.c` validate the API.

**Known v0.4 limitation**: `px_loop` uses `px_perception_invoke_all()` (invokes all registered perceptions, not just the loop's). Future v0.5+ should add `px_perception_invoke(p)` to scope this.

**Status in roadmap matrix:** Feedback row — Theory ✅, Proof ✅, Engineering ✅, Docs ✅.

---

## L14: Deferred essence candidates — acknowledged but not implemented

**Severity:** Low for current users; High for essence-claim honesty

Per [ADR-0007](../../decisions/accepted/ADR-0007-essence-derivation-v2-revision.md) and [essence-derivation-v2.md](../history/essence-derivation-v2.md), the 6-tradition literature survey surfaced **4 essence categories** that Planex does NOT implement. These are not engineering conveniences — they are documented essence claims from philosophy and HCI traditions:

### D1. Embodiment (Dourish, 2001)

UI's essence includes the **embodiment relation** — meaning emerges through engaged interaction, not pre-encoded by designers. UI is not an independent artefact but an extension of embodied action.

- **Why deferred**: requires giving up "UI as independent artefact" premise, which is foundational to Planex's current API (Estimates, Closures, Perceptions all exist independently of actor)
- **Source**: research/reports/05-phenomenology.md §5 (Dourish)

### D2. Situatedness (Suchman, 1987)

Action is situated — plans are post-hoc rationalizations, not action generators. UI cannot preset user flows.

- **Why deferred**: contradicts use-case-driven design, which Planex's Closure (typed intent) implicitly assumes
- **Source**: research/reports/05-phenomenology.md §3 (Suchman)

### D3. Affordance-as-relation (Gibson, original 1979)

Affordance is a relation between world and actor, not a property. UI's essence includes "what actions the actor can take" — which requires actor presence in the API.

- **Why deferred**: Planex's Relation is between things (Estimates, Closures); adding actor-to-thing relations would be a major API revision
- **Source**: research/reports/05-phenomenology.md §7 (Gibson)

### D4. Breakdown (Heidegger-Winograd/Flores, 1986)

UI's essence includes the moment of breakdown — when the tool becomes present-at-hand. This is when the user notices the UI itself (an error, a confusion, a surprise).

- **Why deferred**: requires a notion of "normal flow" vs "interrupted flow" that Planex's current abstractions don't express
- **Source**: research/reports/05-phenomenology.md §1 (Heidegger), §2 (Winograd/Flores)

### Why these are deferred, not dismissed

Per essence-driven principle (ADR-0007): **a deferred essence candidate must remain acknowledged**. If Planex silently drops them, it commits the same over-claim that v2 corrected. Future revisions should pick these up when use cases demand, not when completeness anxiety demands.

**Decision:** ADR-0007 acknowledges these as deferred essence. No implementation is planned until use-case pressure demands.

---

## How to read this document

If you're evaluating Planex for a use case:

- **Embedded device with simple UI:** L6, L8 may matter. L1 (Phase 2) was resolved in v0.5.
- **Desktop tool / research prototype:** Most limitations are tolerable.
- **Production system needing long-term support:** L10 is a hard blocker.
- **Safety-critical system:** L8 is a hard blocker.
- **Accessibility-required application:** L9 is a hard blocker.
- **Performance-critical (high-DPI, 4K+):** L6 may matter.
- **Heavy hover/drag/focus interaction:** L11 means you'll have to write integration tests manually; UI-layer unit tests for these interactions are not possible yet.
- **Need real-time perception-driven rendering:** L1 Phase 2 landed in v0.5 — perceptions now auto-invoke when their source estimates change. No manual invocation needed in application code (manual invoke ops remain as diagnostic seams for testing/debugging).

This document is the canonical answer to "is Planex ready for X?" questions. If a limitation is not listed here, please file an issue.
