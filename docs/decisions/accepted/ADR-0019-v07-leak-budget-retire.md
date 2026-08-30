# ADR-0019: v0.7 leak-budget retire — Closure constructor split takes aggregate L2 to zero

## Status

Accepted

Date: 2026-08-30

Completes the retire cycle [ADR-0013](ADR-0013-v05-leak-budget-retire.md) began: the Closure `bind_graph` ordering leak — the single remaining aggregate L2 since v0.6 — is retired by constructor shape, and the aggregate L2 rate reaches **0%** for the first time. This ADR also records the two v0.7 retires that rode Lines 2–3 (budget-by-default retiring the "feedback without time constraints" doc-only axiom; the Estimate schema retiring the contract half of the `void*` L1), which needed no ADR of their own but belong to the same cycle's ledger.

## Context

The v0.4 leak audit found 9 aggregate L2 leaks (17%). v0.5 retired 7 of them (ADR-0013: 4 Estimate + 3 Perception). v0.6 retired the px_loop scope leak and made the Closure `bind_graph` omission *loud* (one-time stderr warning) rather than retired — the honest intermediate state. The leak-budget retire curve: 17% → 3.8% → 1.7% → 1.0% → **0%**.

The remaining leak's shape: `px_closure_bind_graph(c, g)` must be called before any `px_closure_trigger(c, ...)` for undo to record, and the C type system cannot enforce that ordering. A caller who triggers first and binds later gets silently uneffective undo. v0.6 made that loud; loud is a mitigation, not a retirement. The v0.7 roadmap Line 5 named the retire: "constructor shape that makes the dependency impossible to forget."

The evidence that the constructor shape is the right fix: the v0.5 audit already named it ("make `bind_graph` part of `closure_new` (compile-time enforcement) or split into `px_closure_new_unbound` / `px_closure_new_with_graph`" — leak-budgets.md §3, v0.4 text), and every example that used the two-call form carried the same three-line ceremony (create, declare edges, bind) in the same order — the duplication of ceremony was itself a signal that the API had the wrong shape.

## Decision

1. **`px_closure_new_with_graph(goal, intent_kind, action, evaluation, user, graph)`** — the graph arrives with the closure, before any trigger can race it. The ordering mistake is unwritable: there is no moment in the program where a closure exists, undo is enabled, and the graph binding is still pending a future call. Passing NULL is the explicit no-undo form (same semantics as plain `px_closure_new`).

2. **`px_closure_bind_graph` is deprecated** (registry entry, deprecation-registry.md): still callable through the deprecation window — the two-call form remains *correct* when used in the right order, and `tests/test_orthogonality.c`'s unbound-closure cases exercise it deliberately. Replacement: the split constructor. Removal at a named version boundary (v1.0 candidate), per the registry process.

3. **The v0.6 one-time warning stays**: undo enabled + no graph bound (a plain `px_closure_new` under enabled undo) still warns once. The constructor split removes the *accidental* unbound case; the warning now guards only the *deliberate* one being misused.

4. **The aggregate L2 budget flips to 0/99 = 0%** and the leak-budget gate holds it there: any new L2 leak must either be retired in the same version or explicitly re-budgeted in this document's successor.

### What else rode this cycle (no separate ADR needed)

- **Line 2 (budget as contract):** the default 16ms frame budget retires the doc-only form of Axiom A4 — "feedback has no time constraints" is no longer true of any `px_loop`. Overruns are loud (warn-once; strict abort under `-DPX_DEBUG_BUDGET`).
- **Line 3 (Estimate schema):** the `void*` L1 entries' *contract half* retires — values are describable (kind + name + print + equal), tests assert kind-aware, a11y names values through the schema. The *pointer half* (`void* user` in callbacks) is permanent documented host cost.
- Both were leak-budget/doc-sync changes, not abstraction admissions; the changelog and leak-budgets.md v0.7 summary are their record, following the v0.6 precedent.

## Essence Check

> This decision retires the last L2 leak on Closure — the abstraction whose denotational claim (typed intent as value) the ordering leak was eroding. Q1–Q5 per the ADR-0011 bar's spirit.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface
- [x] **Intent space, execution side** — Closure's undo-record pipeline. The fix makes the *act's* history complete by construction rather than by call discipline.
- [ ] State space / Presentation / Feedback

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses: the three-call ceremony (new → declare → bind) becomes two (new-with-graph → declare), and — the real compression — one *ordering rule to remember* is deleted. The API now has no "must call X before Y" anywhere on the undo path. Cost: one more constructor variant in the header (13 Closure ops total).

### Q3. Is there a gap between the claim and the implementation?

No gap for the leak itself: `test_v07.c` e1 pins graph-at-birth (undo records with zero bind calls), e2 pins that the deprecated form still works when used correctly. The honest residue: `px_closure_new` (no graph) still exists and is correct for no-undo closures — the warning guards its misuse under enabled undo, unchanged from v0.6.

### Q4. What is the cost, and who can verify it?

- **Cost:** one additional constructor; a deprecation window to carry (bind_graph stays until removal); every caller of the old form sees a deprecation note in the header and the registry.
- **Who can verify:** `make test_v07` (section E) pins both the new constructor and the deprecated form's continued correctness; `make check-examples` runs the migrated examples (undo_via_graph, palette_afford, integration_4abs, counter_perception_window — all now flag-bearers for the split form); the leak-budget gate holds aggregate L2 at 0.

### Q5. What are the counterexamples?

- **Closures that genuinely never undo.** Plain `px_closure_new` remains the right form; forcing a graph argument would be ceremony. The split is an *option*, not a mandate — the same opt-in posture as `intent_graph`.
- **Late-bound graphs** (graph created after closures): the two-call form remains the only way to express this, which is exactly why it is deprecated rather than removed. If a real consumer needs late binding as a first-class pattern, that consumer's ADR can un-deprecate or replace it — no such consumer exists today.

## Consequences

### Positive

- **Aggregate L2 = 0%.** The falsifiability contract's quantitative half is fully green for the first time; from here, any new L2 is a regression the budget gate catches.
- The undo path has zero ordering rules — the class of "forgot to bind" bugs is deleted from the API's grammar.
- The examples now teach the safe form by default.

### Negative

- Two constructors coexist through the deprecation window (new / new_with_graph); a reader must learn why.
- `bind_graph`'s eventual removal (v1.0 candidate) is a breaking change that UPGRADING.md must carry.

### Neutral

- `px_closure_new` with NULL-graph semantics is identical to before; the v0.6 warning behavior is unchanged.

## Alternatives Considered

### Alternative 1: Keep bind_graph, silence the warning after N fires

Rejected: a warning that gives up is not loudness, it is noise. The leak's shape (un-enforceable ordering) can only be fixed by shape.

### Alternative 2: Make the graph a required argument of plain px_closure_new

Rejected: breaks every no-undo caller for zero semantic gain — closures without undo are legitimate (the counterexamples above). The split constructor adds the guarantee without breaking anyone.

### Alternative 3: Runtime hard error on unbound trigger under enabled undo

Rejected: converts a correctness footgun into a crash footgun. The constructor split removes the *accident*; the remaining unbound cases are deliberate and deserve a warning, not an abort.

## CAVEATS

- This ADR does **not** promise removal of `px_closure_bind_graph` in v0.7 — deprecation is the window; removal needs its own version-boundary decision.
- This ADR does **not** claim Closure is leak-free in the L1 sense (the `void*` payload/user host tax remains, documented in leak-budgets.md).
- Aggregate L2 = 0% is a *snapshot* claim: the budget gate's job is to make any regression loud, not to promise stasis.

## Known issues

- `test_orthogonality.c` still uses the two-call form in its bound cases (incidental to its per-abstraction anti-pattern design; migrating it is cosmetic churn with no evidence value — the deprecated path is deliberately pinned by `test_v07.c` e2 instead).
- The deprecation has no compile-time marker (no `__attribute__((deprecated))`): Planex builds warning-clean with `-Wall -Wextra -Wpedantic -Werror` on three compilers, and the attribute's portability across them (MSVC needs `__declspec(deprecated)`) adds gating ceremony; the header doc + registry are the deprecation record. Revisit if a portable pattern is wanted.

## HISTORY

- 2026-08-30: Accepted — constructor split landed; examples migrated; registry entry added; aggregate L2 0/99 = 0%.
- 2026-08-29: v0.6 made the omission loud (one-time warning) — the intermediate state this ADR completes.
- 2026-08-28: v0.5 audit (ADR-0013) named the constructor split as the retire path.

## References

- Code: [`src/closure.c`](../../../src/closure.c) (`px_closure_new_with_graph`, deprecated `px_closure_bind_graph`), [`include/planex/planex.h`](../../../include/planex/planex.h)
- Evidence: [`tests/test_v07.c`](../../../tests/test_v07.c) section E; migrated examples: `undo_via_graph.c`, `palette_afford.c`, `integration_4abs.c`, `counter_perception_window.c`
- ADRs: [ADR-0013](ADR-0013-v05-leak-budget-retire.md) (the retire precedent this completes), [ADR-0002](ADR-0002-relation-necessity-pending-undo.md) (undo-via-graph, the mechanism the ordering leak eroded)
- Registry: [deprecation-registry.md](../../reference/deprecation-registry.md) (`px_closure_bind_graph` entry)
- Budget: [leak-budgets.md](../../concepts/canonical/leak-budgets.md) (v0.7 summary — aggregate 0%)
- Roadmap: [v0.7-roadmap.md](../../concepts/history/v0.7-roadmap.md) Line 5
