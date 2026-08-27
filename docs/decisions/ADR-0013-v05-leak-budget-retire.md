# ADR-0013: v0.5 Leak Budget Retire

**Status:** Accepted — 2026-08-28
**Decider:** Planex author
**Tags:** v0.5, leak-budget, abstraction-form, retire

## Context

[`docs/concepts/leak-budgets.md`](../concepts/leak-budgets.md) (commit `76303dc`, v0.4) prescribed a falsifiable retire curve for the L2 (semantic) leak count per abstraction:

| Version | Target L2 count | Target L2 rate |
|---|---|---|
| v0.4 (baseline) | 9 | 17% |
| v0.5 (planned) | ≤ 4 | ≤ 8% |
| v0.6 | ≤ 2 | ≤ 4% |
| v1.0 | ≤ 1 | ≤ 2% |

The v0.5 target was scoped (per the v0.4 doc) to retire Estimate's three not-const leaks + `derived_recompute` cycle. The v0.4 doc also prescribed Perception's three Phase-2-leak operations for retirement — originally targeted at v0.6.

During this v0.5 cycle, the author chose to **pull the Perception retire forward** (do at v0.5 what was originally planned for v0.6). The rationale: Perception L2 = 50% was the worst single-abstraction rate and was exactly at the abstraction-level review threshold. Leaving it for v0.6 would have meant Perception stayed at the "review trigger" boundary for an entire minor version.

This ADR documents what was actually done — the engineering record of the v0.5 retire landing.

## Decision

**Retire 7 L2 leaks in v0.5** (4 Estimate + 3 Perception), exceeding the v0.5 plan of 4 (Estimate only).

### 1. Estimate — 4 leaks retired

**Mechanism:** Introduced `px_estimate_advance(px_estimate* e, double t_ms)` — an explicit mutator for animation time-step + finalization. The three previously side-effecting queries become pure `const` queries:

| Operation | v0.4 signature | v0.5 signature |
|---|---|---|
| `px_estimate_value` | `double px_estimate_value(px_estimate* e)` (auto-samples) | `double px_estimate_value(const px_estimate* e)` (pure query) |
| `px_estimate_now` | `double px_estimate_now(px_estimate* e)` (auto-samples) | `double px_estimate_now(const px_estimate* e)` (alias of `value`) |
| `px_estimate_is_animating` | `bool px_estimate_is_animating(px_estimate* e)` (finalizes) | `bool px_estimate_is_animating(const px_estimate* e)` (pure query) |

For the `derived_recompute` cycle leak: added a `bool recomputing` field to `struct px_estimate`. `px_derived_recompute` checks the flag on entry; if already true, returns early (cycle broken). Otherwise sets it, runs the derive fn, calls `px_estimate_set` (which fires observers that may re-enter — that's correct, the flag prevents infinite recursion), clears the flag on exit.

**Migration cost:** Callers who relied on the old auto-sampling behavior must prepend `px_estimate_advance(e, px_now_ms())` before reading. Test files (`tests/test_core.c`) and examples updated. No public API removal — old signatures changed but are clearly marked. Pure ABI break for the 3 const-corrected signatures; acceptable because v0.5 is pre-1.0 and the leak-budgets doc explicitly prescribed this retire.

### 2. Perception — 3 leaks retired

**Mechanism:** Phase 2 auto-invocation. `px_estimate_set` now calls `px_perception_invoke_for_estimate(e)` internally after `notify(e)`. The three invoke ops (`invoke_all`, `invoke_single`, `invoke_for_estimate`) remain in the public API but are now **diagnostic seams** — testing, debugging, view-only refresh outside a `px_loop`. Their raison d'être changed from "abstraction is incomplete" to "diagnostic tooling."

The "return-type semantic mismatch" flagged in v0.4 for `px_perception_invoke_for_estimate` (returns `int` count, not perception results) was **reclassified as not a leak**: the operation's name is "invoke for estimate" — it invokes perceptions. The `int` return is a documented count, not a behavior-name mismatch.

### 3. Side-effect: `px_loop_step` double-fire bug fixed

Surfaced during v0.5 Phase 2 testing: the v0.4 `px_loop_step` called both `px_perception_invoke_all()` (fires bound perception) AND `px_perception_invoke_single(loop->perception)` (re-fires it) on every iteration. `tests/test_feedback.c::test_b1_step_triggers_and_perceives` actually asserted `g_perception_count == 1`, which was FAILING at HEAD before v0.5 — a preexisting test failure that the leak-budgets v0.4 doc didn't flag (it was a behavior bug, not a leak).

Fix:
- Removed redundant `px_perception_invoke_all()` call from `px_loop_step`. Loop now uses `invoke_single` only.
- Added `void* last_representamen; bool has_last;` cache to `struct px_perception`. Auto-invocation fills cache; `invoke_single` returns cached without re-firing.
- Loop functions clear caches at turn start (`px__perception_clear_cache` for bound perception in `step`, `px__perception_clear_all_caches` for all in `view_only` and `replay` iterations).
- `px_loop_replay` only calls `invoke_all` for entries where `closure_triggered` was false (view-only iterations).

This fix was necessary for Phase 2 to not break the loop's audit semantics.

## Alternatives considered

### A. Keep auto-invocation outside `px_loop_step` (disable flag)

**Idea:** Add a `g_in_loop_step` flag; `px_estimate_set` skips auto-invocation while flag is true; loop_step sets the flag.

**Rejected because:** This breaks multi-perception setups. If the loop's bound perception is `p1` and another perception `p2` depends on the same estimate, the loop_step would not auto-invoke `p2`. The user would silently miss `p2`'s update. The cache approach handles all perceptions uniformly.

### B. Remove the three invoke ops entirely (make internal)

**Idea:** Move `px_perception_invoke_all/single/for_estimate` to a private header; remove from public `planex.h`.

**Rejected because:** Tests (`tests/test_orthogonality.c` — 9 call sites, `examples/perception_phase2.c`) and the loop's own logic (`px_loop_step` calls `invoke_single`) need them. Removing them would either break the tests or require moving them to an internal-only header, complicating the public API surface. Keeping them as "diagnostic" is a cleaner contract: the leak (operation exists only because abstraction incomplete) is retired, but the operations remain for legitimate diagnostic use.

### C. Don't pull Perception retire forward to v0.5

**Idea:** Do only Estimate (4 leaks) in v0.5, leave Perception (3 leaks) for v0.6 per the original plan.

**Rejected because:** Perception L2 = 50% was at the abstraction-level review threshold (L2 > 50% triggers review). Leaving it at the boundary for one minor version risks external criticism. Also: the work is mostly in `px_estimate_set` (adding auto-invocation); the marginal cost of also retiring Perception's 3 leaks is small (just docs + header comments). Pulling forward gets us to 3.8% aggregate, well below the 8% target — gives a comfortable margin for v0.6 / v1.0.

## Consequences

### Positive

- Aggregate L2 rate: 17% → 3.8% (target was ≤8% — **MET with margin**).
- Estimate L2: 24% → 0% (target was 6%).
- Perception L2: 50% → 0% (was originally a v0.6 target).
- `limitations.md` L1 (Perception Phase 2 pending) — closed.
- `abstraction-form.md` Prerequisite 2 + Prerequisite 3 — first concrete engineering evidence that the leak budget is *drivable*, not just *measurable*.
- `px_loop_step` double-fire bug (preexisting, test_b1 was failing on HEAD) — fixed.
- New `px_estimate_advance` operation is documented as a mutator (matches its name) — not a leak.

### Negative

- ABI break: three signatures changed (`px_estimate_value/now/is_animating` now take `const px_estimate*`). Callers passing non-const pointers are fine; callers relying on side effects must update.
- Migration burden: callers who relied on auto-sampling must prepend `px_estimate_advance(e, px_now_ms())`. Test files updated; external callers (none at this stage — pre-1.0) would need migration.
- The new `px_estimate_advance` is one more operation to learn. Mitigated by clear doc + the const-queries contract being more uniform than the old mixed auto-sample/manual contract.
- Representamen cache leak: the cache in `struct px_perception` holds a non-owning reference to the representamen. When overwritten, the previous value is leaked (we don't know how to free it — perception fns are heterogeneous). This is a known v0.5 limitation; a future version can add a `free_fn` to `px_perception_new`.
- `px_loop_replay` no longer calls `invoke_all` for closure_triggered entries. If the user has multiple perceptions depending on different estimates that the closure changes, only the perceptions whose sources were actually mutated will fire (via auto-invocation). This is correct behavior, but it's a subtle semantic shift from v0.4 (which over-fired everything via `invoke_all`).

### Neutral

- The `px_loop_replay` scope leak (v0.4's only `px_loop` L2 leak) is unchanged. It remains a v1.0 target.

## Verification

The retire is verified by [`tests/test_v05_retire.c`](../../tests/test_v05_retire.c) (new file, 12 tests, all passing):

- `test_a1_value_is_const` — verifies `px_estimate_value` is a pure query during animation.
- `test_a2_is_animating_is_const` — verifies `px_estimate_is_animating` doesn't finalize as a side effect.
- `test_a3_now_is_const_alias` — verifies `px_estimate_now == px_estimate_value` at all times.
- `test_b1_advance_finalizes` — verifies `px_estimate_advance` finalizes on completion.
- `test_b2_advance_fires_observers_on_finalize` — verifies observers fire on finalize, not on mid-animation cache.
- `test_b3_advance_mid_animation_caches_value` — verifies mid-animation cache.
- `test_b4_advance_noop_when_not_animating` — verifies no-op when not animating.
- `test_c1_cycle_does_not_overflow` — verifies A↔B cycle doesn't crash.
- `test_c2_dag_still_works` — verifies legitimate DAGs still update correctly.
- `test_d1_set_auto_invokes_perceptions` — verifies Phase 2 auto-invocation.
- `test_d2_unrelated_estimate_does_not_invoke` — verifies auto-invocation is scoped.
- `test_d3_invoke_ops_still_diagnostic` — verifies the 3 invoke ops still work as diagnostic.

Plus: all preexisting test suites still pass:
- `tests/test_core.c` — 33/33 (animation tests updated for new contract).
- `tests/test_orthogonality.c` — 19/20 (1 skipped: known transaction gap, unrelated).
- `tests/test_feedback.c` — 13/13 (the preexisting test_b1 double-fire failure is now fixed).

Plus: all 11 examples build and run without behavior regression.

## References

- [`docs/concepts/leak-budgets.md`](../concepts/leak-budgets.md) — the document this ADR implements.
- [`docs/concepts/abstraction-form.md`](../concepts/abstraction-form.md) — Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability) that this retire verifies.
- [`docs/concepts/limitations.md`](../concepts/limitations.md) — L1 (Perception Phase 2 pending), closed by this retire.
- [`ADR-0011`](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — Essence Check Q4 establishes the verifiable-cost-scenario pattern; this retire is the engineering fulfillment of that pattern for the leak budget.
- Joel Spolsky, "The Law of Leaky Abstractions" (2002).
