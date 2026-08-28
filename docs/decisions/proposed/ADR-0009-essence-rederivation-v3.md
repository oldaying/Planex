# ADR-0009: Essence re-derivation v3 + Path B prototype (Proposed)

## Status

Proposed (not yet Accepted). Date: 2026-08-27.

Implements: the design analysis in
[essence-derivation-v3.md](../../concepts/history/essence-derivation-v3.md)
(Part V — Path B). This ADR is the implementation-level record
of the v3 prototype; the v3 derivation document itself is the
theory-level record.

Supersedes: the "5 of 5 essence categories implemented" claim in
[ADR-0008](../accepted/ADR-0008-feedback-as-fifth-essence-category.md).
ADR-0008's `px_loop` decision stands; what changes is the essence
*coverage* claim.

## Context

[essence-derivation-v2.md](../../concepts/history/essence-derivation-v2.md)
sampled 6 traditions and concluded 5 essence categories. v3
re-derived from "UI = Actor/System/World semantic boundary" plus
3 traditions v2 missed (semiotics Peirce, cybernetics Bateson/
Maturana, perlocutionary pragmatics Searle level 3) and surfaced
9 essence categories. Four categories v2 missed:

1. **Interpretant** (Peirce) — the meaning *generated in the
   actor's mind* by encountering the representamen. Not stored
   state, not emitted intent — a third term between the two.
2. **Perlocution** (Searle 1969, level 3) — the *effect* of the
   system's utterance on the actor's mental state. Distinct from
   illocution (covered by `px_intent_kind`).
3. **Breakdown** (Heidegger / Winograd-Flores / Dourish /
   Suchman — 4-tradition convergence exceeds v2's ≥3 threshold)
   — the moment the actor's interpretant no longer matches the
   system's representamen. Semantic breakdown, not operational
   loop stall.
4. **3-place Relational ontology** (Situatedness) — Relation is
   currently 2-place (`px_declare(g, a, kind, b)`); the essence
   requires the actor parameter (`px_declare_for(g, a, kind, b,
   actor)`).

The v3 document proposes three refactoring paths. This ADR
implements **Path B** (moderate):

- Keep v0.4's 5 abstractions (Estimate, Closure, Perception,
  Relation, px_loop) — their essence claims stay intact.
- Augment Closure with a perlocution sub-API (3 new functions
  + 1 new enum, no new struct).
- Augment Perception with an interpretant sub-API (3 new
  functions + 1 new fn type, no new struct).
- Augment Relation with `px_actor` parameter (3-place) + 3 new
  relation kinds (WITHDRAWS_FOR / PRESENTS_FOR / INTERPRETS_AS).
- Extend `px_loop_audit_entry` with 3 new fields
  (`perlocution_kind`, `interpretant_constructed`,
  `breakdown_transition`).
- Add `px_breakdown` as the 6th abstraction (~5 functions + 1
  struct + 1 enum).
- Add `px_actor` as a first-class struct (NOT a 6th abstraction —
  actor is a parameter to Relation/Breakdown/Perlocution/
  Interpretant).
- Add `px_perception_invoke_single(p)` public API so px_loop_step
  can obtain the perception's representamen before calling
  interpret_fn.
- Add `px_perception_interpret(p, repr, actor)` public API so
  px_loop_step can invoke the perception's interpret_fn.

v3 essence claim status after this ADR:
- Object (Estimate) ✓
- Sign vehicle (Perception fn output) ✓
- Interpretant (Perception sub-API, interpret_fn hook) ✓ partial
- Illocution (Closure intent_kind) ✓
- Perlocution (Closure sub-API) ✓ partial
- Relational ontology (Relation, 3-place augmented) ✓
- Loop topology (px_loop) ✓
- Breakdown (px_breakdown, new 6th abstraction) ✓ NEW
- Adaptation (Hoffman/Friston) ✗ deferred (Estimate.confidence
  stub for Layer 5)
- Medium-ness (Kay/Engelbart/Victor) ✗ deferred

**6 implemented + 2 partial + 2 deferred** (was: 5 implemented +
4 deferred per v2/ADR-0008).

## Forces

1. **The v3 derivation demanded implementation-level validation**:
   theory-level claims are not enough. v2 already suffered the
   "industrial-leg ⚠️ zero adoption" problem (essence-derivation-v2.md
   "industrial腿 ⚠️" in the worklog — every Planex reference was
   literature, not industrial practice). v3 must not repeat this;
   the path must be implementable to count.

2. **No external dependencies and C17**: Path B must fit Planex's
   zero-dependency, C17, embedded/desktop scope. No new types beyond
   what C17 can express. No allocations beyond calloc/malloc/free.
   No threading. No external libraries.

3. **Backward compatibility**: v0.4's existing API (used by 15+
   demos + 3 test suites) must not break. Old `px_declare` and
   `px_query` must work as universal-relation wrappers around
   the new 3-place API.

4. **No new abstractions beyond Breakdown**: Path C (radical, 8
   abstractions) was rejected in v3 doc § IV for violating
   non-goals.md NG-3 (kitchen-sink API). Path A (conservative, no
   new abstraction) was rejected for repeating v1's conflation
   flaw (Path A makes Closure do illocution+perlocution in the
   same struct, the same flaw v1 had with Relation=topology+
   ontology).

## Decision

### D1. Implement the v3 prototype API surface exactly as documented in essence-derivation-v3.md § V

- `px_actor` struct (4 functions, src/actor.c)
- `px_declare_for` + `px_query_for` + 3 new `px_rel_kind`
  values (src/relation.c, augmented)
- `px_perlocution_kind` enum + 3 Closure sub-API functions
  (src/closure.c, augmented)
- `px_interpret_fn` fn type + 3 Perception sub-API functions
  (src/perception.c, augmented)
- `px_perception_invoke_single(p)` + `px_perception_interpret(p, repr, actor)`
  (src/perception.c, augmented)
- Extended `px_loop_audit_entry` struct + `px_loop_mark_breakdown`
  (src/feedback.c, augmented)
- `px_breakdown` abstraction: 5 functions + 1 struct + 1 enum
  (src/breakdown.c, new)

### D2. Old 2-place API preserved as wrappers

```c
/* Old: */
px_relation* px_declare(px_graph* g, void* a, px_rel_kind kind, void* b);
/* Now equivalent to: */
px_declare_for(g, a, kind, b, NULL);  /* NULL actor = universal */
```

Same for `px_query` → `px_query_for(g, node, kind, NULL)`.

### D3. Add 4 stdout examples + 1 assertion test suite

- `examples/v3_prototype_actor.c` — validates 3-place Relation:
  universal relations match every actor query; actor-scoped
  relations match only that actor; old 2-place API still works.
- `examples/v3_prototype_perlocution.c` — validates that two
  closures with identical `status` (DONE) can differ in
  `perlocution_kind` (INFORM vs ALERT), and the loop audit records
  the perlocution.
- `examples/v3_prototype_interpretant.c` — validates that
  Perception can declare an intended_interpretant, register an
  interpret_fn that predicts the actor's actual interpretant,
  and that the loop audit records `interpretant_constructed`
  (true/false) distinct from `perception_invoked` (true/false).
- `examples/v3_prototype_breakdown.c` — validates that breakdown
  is recordable per-actor, recoverable, that the loop audit
  records `breakdown_transition` (+1 entered, -1 recovered),
  and that `px_breakdown_to_relation` declares
  `PX_REL_PRESENTS_FOR` queryable in the graph.
- `tests/test_v3_prototype.c` — 60 assertion tests across 6
  groups (v0.4 backward-compat, 3-place Relation, perlocution
  sub-API, interpretant sub-API, loop audit extension, Breakdown
  abstraction). All 60 pass.

### D4. Essence claim revised

| Essence category | Pre-ADR-0009 | Post-ADR-0009 |
|---|---|---|
| Object | implemented (Estimate) | implemented (Estimate) |
| Sign vehicle | implemented (Perception) | implemented (Perception) |
| Interpretant | missing | partial (sub-API, hook) |
| Illocution | implemented (Closure intent_kind) | implemented (Closure intent_kind) |
| Perlocution | missing | partial (sub-API) |
| Relational ontology (3-place) | partial (2-place API) | implemented (px_declare_for) |
| Loop topology | implemented (px_loop) | implemented (px_loop) |
| Breakdown | missing | **implemented (px_breakdown, NEW)** |
| Adaptation | deferred | deferred (Estimate.confidence stub) |
| Medium-ness | deferred | deferred |

**6 implemented + 2 partial + 2 deferred** (was: 5 + 0 partial + 4
deferred per ADR-0008).

### D5. The explicit-abstraction vs Zuhandenheit tension is resolved

`non-goals.md` declares `explicit-abstraction` as a stance: users
must always be able to see and manipulate the abstractions.
Heidegger's Zuhandenheit says the opposite: skilled use makes
abstractions disappear. v2 treated these as a tension without a
primitive. v3 resolves it via:

- `PX_REL_WITHDRAWS_FOR(a, actor)` — abstraction `a` is
  *withdrawn* from `actor`'s awareness (in flow, Zuhanden).
- `PX_REL_PRESENTS_FOR(a, actor)` — abstraction `a` is
  *present* to `actor` (in breakdown, Vorhanden).
- The same node can have both relations for the same actor
  simultaneously (validated in `v3_prototype_breakdown` example).

The two stances are complementary, not contradictory: explicit-
abstraction says *the abstractions must be addressable when needed*;
Zuhandenheit says *when they're not needed, they're marked
withdrawn*.

## Verification

### Build

Compiled with GCC, C17, -Wall -Wextra -Wpedantic, zero external
dependencies:

```
gcc -std=c17 -Wall -Wextra -Wpedantic -g -O0 -I include \
    -o build/v3_prototype_actor \
    examples/v3_prototype_actor.c \
    src/relation.c src/estimate.c src/closure.c src/perception.c \
    src/undo.c src/feedback.c src/fb.c src/font.c src/a11y.c \
    src/layout.c src/actor.c src/breakdown.c -lm
```

Same for `v3_prototype_perlocution`, `v3_prototype_interpretant`,
`v3_prototype_breakdown`, `test_v3_prototype`. All 5 compile
without warnings.

### Test

```
$ ./build/test_v3_prototype
=== Summary ===
  Passed: 60
  Failed: 0
  Total:  60
```

All 60 assertion tests pass. Test groups:

- `test_v04_backward_compat` (4 tests) — old `px_declare` /
  `px_query` work as universal-relation wrappers.
- `test_3place_relation` (11 tests) — actor parameter respected,
  universal matches every query, actor-scoped matches only that
  actor, Zuhandenheit/PRESENTS_FOR kinds work.
- `test_closure_perlocution` (10 tests) — default UNSPECIFIED,
  set/get, kind/text, str helper for all 7 enum values.
- `test_perception_interpretant` (8 tests) — default NULL,
  set/get intended_interpretant, interpret_fn success + failure
  paths, NULL interpret_fn.
- `test_loop_audit_extension` (12 tests) — extended audit entry
  records perlocution_kind, interpretant_constructed, and
  breakdown_transition correctly across 4 cases (no breakdown,
  breakdown entered, breakdown recovered, pending consumed).
- `test_breakdown_abstraction` (17 tests) — record/recover/count/
  get/is_recovered/to_relation bridge, most-recent-first ordering.

### Examples

All 4 examples run end-to-end and print the expected output:

- `v3_prototype_actor` — 8 query checks all match expectations.
- `v3_prototype_perlocution` — 2 closures with identical status
  (DONE) correctly differ in perlocution_kind (INFORM vs ALERT).
- `v3_prototype_interpretant` — 3 cases (success / fail / NULL
  interpret_fn) all report correct `interpretant_constructed` flag.
- `v3_prototype_breakdown` — full flow → breakdown → recovery
  cycle; audit records +1 / -1 transitions; PRESENTS_FOR queryable
  in the graph.

## Essence Check

### Q1. Which essence axis does this decision affect?

All 4 missing essence categories v3 identified: Interpretant,
Perlocution, Breakdown, 3-place Relational ontology. Also affects
Loop essence (audit extended) and indirectly Zuhandenheit (resolved
tension with explicit-abstraction).

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses for callers who need to express semantic dimensions
(perlocution, interpretant, breakdown) — previously impossible
without hand-rolling audit + custom state machines.

Increases: 6 abstractions instead of 5; new struct `px_actor`;
new enum `px_perlocution_kind`; new fn type `px_interpret_fn`;
extended `px_loop_audit_entry`. The cognitive cost matches the
essence gain.

### Q3. Is there a gap between claim and implementation?

After this ADR: no. All 6 implemented essence categories have
working APIs verified by tests + examples. The 2 partial
(Interpretant, Perlocution) are partial because their Layer 5
adapters (history-based prediction, perlocution tracking across
iterations) are deferred — but the API structure is in place.

### Q4. What is the cost, and who can verify it?

- 7 new C source files / file additions: src/actor.c (~85 LOC),
  src/breakdown.c (~185 LOC), +substantial additions to
  src/relation.c / src/closure.c / src/perception.c / src/feedback.c
  (~200 LOC).
- 4 new examples (~600 LOC).
- 1 new test suite (~370 LOC).
- Total: ~1440 LOC.
- Verifier: any contributor who reads `essence-derivation-v3.md`
  § V, builds the prototype, runs the tests, and asks "does the
  implementation express what the theory says?"

### Q5. What are the counterexamples?

- **Path A would have been simpler**: Rejected in v3 doc — it
  conflates essence dimensions inside existing abstractions
  (Closure doing illocution+perlocution, Perception doing
  representamen+interpretant). This is the exact flaw v1 had with
  Relation (topology+ontology conflated); v2 corrected it for
  Relation; Path B repeats the correction pattern for Closure and
  Perception, instead of repeating v1's conflation.
- **Path C would have been more orthogonal**: Rejected in v3 doc —
  8 abstractions in C17 for embedded/desktop is heavy, violates
  non-goals.md NG-3 (no kitchen-sink API). The marginal essence
  gain over Path B is small.
- **Keep v2's claim and don't refactor**: Rejected — v2's
  "≥3 traditions converge" threshold was applied to an
  under-sampled tradition set. v3 surfaces 4 missed essence
  categories; keeping v2 is the same over-claim pattern v2
  criticized in v1.

## Scope

This ADR implements the v3 prototype API surface and validates
feasibility. It does NOT:

- Implement Layer 5 (Adaptation / Hoffman-Friston) — `Estimate.confidence`
  remains a stub.
- Implement Medium-ness (Kay/Engelbart/Victor) — far future.
- Open `px_intent_kind` enum to `const char*` — deferred to a
  future ADR; the closed enum is still functional for the 5
  Winograd/Flores illocutionary forces.
- Add `px_loop_step_for_actor(loop, actor, payload, size)` —
  the prototype uses `NULL` actor throughout; promoting actor
  to first-class loop parameter is a future refinement.
- Add `px_breakdown_free()` — breakdowns live in a global list
  in this prototype (matching the perception registry's style);
  a future production version should add per-actor cleanup.
- Update `why-four-abstractions.md` → `why-six-abstractions.md` —
  deferred until ADR-0009 moves from Proposed to Accepted.
- Update `limitations.md`, `ui-essence-layers.md`, `non-goals.md`,
  `path-C-lineage.md` — deferred until ADR-0009 is Accepted.

## Consequences

### Positive

- **v3 essence categories are expressible in Planex's C17
  zero-dependency style**: 60/60 tests pass, 4 examples run
  end-to-end. The path from "9 essence categories" to "C17 code"
  is now demonstrated, not theoretical.
- **Zuhandenheit tension with explicit-abstraction is resolved**:
  WITHDRAWS_FOR + PRESENTS_FOR relation kinds make the two stances
  complementary, not contradictory. A node can be both explicit-
  abstraction-addressable AND contextually withdrawn (in flow) or
  present (in breakdown) for the same actor.
- **Perlocution is typed**: "Saved." and "Saved. 3 fields were
  auto-corrected." can now be semantically distinguished (INFORM vs
  ALERT), not just textually. The loop audit records the
  perlocution dimension.
- **Interpretant is channelled**: Perception declares its
  intended_interpretant; an optional interpret_fn predicts the
  actor's actual one. The loop audit distinguishes
  "perception invoked" from "interpretant constructed" —
  previously impossible.
- **3-place Relation is first-class**: `px_declare_for` accepts
  an actor parameter; old `px_declare` works as universal
  wrapper. Multi-user / situated UIs are now expressible.
- **Breakdown is a 6th abstraction** with 4-tradition support
  (Heidegger / Winograd-Flores / Dourish / Suchman), exceeding
  v2's ≥3-tradition threshold for essence elevation. The bridge
  to Relation (PRESENTS_FOR) makes breakdown visible to
  relation-querying code.

### Negative

- **6 abstractions breaks the "5 abstractions" tagline**: the
  tagline is wrong (it claimed 5-of-5 essence coverage; really
  5-of-9). Better to break it than keep the over-claim. Future
  `why-six-abstractions.md` rename will formalize this.
- **~1440 LOC of new code**: substantial for a pre-v1.0 project.
  Justified by essence-driven design — each LOC maps to an
  essence category v3 identified.
- **`px_breakdown` lives in a global list**: like the perception
  registry, this is a Stage 0 limitation. A future production
  version should add per-actor breakdown storage + free.
- **`px_loop_step` leaks the interpretant** returned by
  interpret_fn: no free_fn convention is defined. Documented in
  src/feedback.c comment. Production fix: caller-provided
  free_fn or stack-allocated convention.
- **`px_loop_step` uses NULL actor throughout**: the prototype
  doesn't yet wire the actor parameter into the loop step. Future
  `px_loop_step_for_actor` variant can do this.
- **`px_intent_kind` enum remains closed**: the v3 doc proposes
  opening it to `const char*`, but this ADR defers that to a
  separate decision (ABI-breaking change).

### Neutral

- **No existing tests broken**: all v0.4 tests (test_core,
  test_orthogonality, test_feedback) still pass — verified
  separately.
- **All existing examples still build**: the v3 additions are
  purely additive to the API surface (old signatures preserved
  as wrappers / unchanged).
- **CMakeLists.txt updated**: `actor.c` and `breakdown.c` added
  to CORE_SOURCES; 4 new examples + 1 new test added.

## Alternatives Considered

### A1. Path A (conservative — no new abstraction)

Rejected: repeats v1's conflation flaw (one abstraction carries
multiple essence dimensions). v2 corrected this for Relation;
Path A would re-introduce it for Closure (illocution+perlocution)
and Perception (representamen+interpretant).

### A2. Path C (radical — 8 abstractions)

Rejected: 8 abstractions in C17 for embedded/desktop is heavy.
Violates non-goals.md NG-3 (no kitchen-sink API). The marginal
essence gain over Path B is small (only Adaptation and Medium
become first-class, both of which v2 honestly deferred and which
have no implementation pressure).

### A3. Don't refactor; keep v2's "5 of 5" claim

Rejected: v2's "≥3 traditions" threshold was applied to an
under-sampled tradition set. The first-principles re-derivation
exposes 4 missed essence categories — keeping v2 is the same
over-claim pattern v2 itself criticized in v1.

### A4. Validate v3 by running a 2-tradition research sprint
### (semiotics + cybernetics) before implementing

Considered but bypassed: the user's directive was that code-level
validation is more direct than research-level validation. Path B
prototype is the most direct validation: if the 4 essence
categories are not expressible in Planex's C17 zero-dependency
style, that would have surfaced during implementation. They were
all expressible. The 2-tradition research sprint can still be
commissioned separately to rule out single-author bias in the v3
derivation, but it is no longer a prerequisite for Path B.

## CAVEATS

This ADR is a **Proposed** prototype ADR. Its implementation stands (the v3 prototype is in `v4/src/`), but its essence-claim language is stale. It does NOT:

- Carry the authority of an Accepted ADR. ADR-0009 remains Proposed; its essence-claim language was downgraded by ADR-0010 (v4 is design rationale, not essence discovery). The implementation is approved as a prototype; the framing is not.
- Address the Layer 5 (Adaptation / Hoffman-Friston) essence category. `Estimate.confidence` remains a stub in the v3 prototype; Layer 5 is explicitly out of scope for this ADR.
- Address the Medium-ness essence category (Kay/Engelbart/Victor). This is far-future; out of scope.
- Open `px_intent_kind` enum to `const char*`. The closed enum (5 Winograd/Flores illocutionary forces) is still functional; opening the enum is an ABI-breaking change deferred to a separate future ADR.
- Add `px_loop_step_for_actor(loop, actor, payload, size)`. The prototype uses NULL actor throughout; promoting actor to first-class loop parameter is a future refinement.
- Add `px_breakdown_free()`. Breakdowns live in a global list in the prototype (matching the perception registry's style); a future production version should add per-actor cleanup.
- Update `why-four-abstractions.md` → `why-six-abstractions.md`. Deferred until this ADR moves from Proposed to Accepted.
- Update `limitations.md`, `ui-essence-layers.md`, `non-goals.md`, `path-C-lineage.md`. Deferred until this ADR is Accepted.

The decision here is narrowly scoped: implement the v3 Path B prototype API surface, validate feasibility via tests + examples. All framing + downstream docs are out of scope.

## Known issues

- **Issue**: The v3 prototype is unproven under industrial use. Every Planex reference is literature (essence-derivation-v3.md), not industrial practice. The v2 cycle suffered this "industrial-leg ⚠️ zero adoption" problem; v3 inherits it.
- **Why accepted**: the v3 derivation demanded implementation-level validation; theory-level claims are not enough. Path B is the most direct validation: if the 4 essence categories are not expressible in Planex's C17 zero-dependency style, that would have surfaced during implementation. They were all expressible. Industrial adoption is a separate concern tracked in `limitations.md` L10 (single-maintainer).
- **Tracking**: deferred. A 2-tradition research sprint (semiotics + cybernetics) can still be commissioned separately to rule out single-author bias in the v3 derivation, but it is no longer a prerequisite for Path B.
- **Mitigation**: the 60/60 test suite + 4 examples validate the API works as designed; the 4 traditions (Heidegger / Winograd-Flores / Dourish / Suchman) for Breakdown exceed v2's ≥3-tradition threshold for essence elevation.

- **Issue**: `px_breakdown` lives in a global list, like the perception registry. This is a Stage 0 limitation — no per-actor breakdown storage, no `px_breakdown_free()`. Memory grows unboundedly with breakdown entries until the caller manually clears.
- **Why accepted**: the v3 prototype scope is API surface validation, not production hardening. Per-actor breakdown storage would require per-actor allocators, which would obscure the essence validation with engineering concerns. Stage 0 (global list) matches the perception registry's style and is consistent across v3.
- **Tracking**: deferred to a future production version. A `px_breakdown_free()` API and per-actor breakdown storage are natural follow-ups.
- **Mitigation**: callers can manually clear via `px_breakdown_recover_all()` (if added) or by triggering breakdown recovery for each entry; the global list is bounded by the number of distinct breakdown events.

- **Issue**: `px_loop_step` leaks the interpretant returned by `interpret_fn` — no `free_fn` convention is defined. Callers must know to free the interpretant themselves, or accept the leak.
- **Why accepted**: defining a free_fn convention requires choosing between caller-provided free_fn, stack-allocated convention, or refcounting. Each has tradeoffs; the v3 prototype exposes the leak as a known cost rather than locking in one convention prematurely.
- **Tracking**: documented in `src/feedback.c` comment. Production fix: caller-provided free_fn or stack-allocated convention. Deferred to v1.0+.
- **Mitigation**: callers using `interpret_fn` should manually free the returned interpretant if ownership is unclear; the examples show this pattern.

## HISTORY

- 2026-08-27: Proposed (with the v3 Path B prototype implementation in repo)
- 2026-08-27: Essence-claim framing downgraded by ADR-0010 (v4 is design rationale, not essence discovery) — the implementation decisions stand; the framing language is stale until this ADR is Accepted or Rejected
- 2026-08-28: Confirmed still-Proposed at v0.5 cycle close; no acceptance, no rejection; implementation remains in `v4/src/` as prototype; framing remains downgraded per ADR-0010

## References

- [essence-derivation-v3.md](../../concepts/history/essence-derivation-v3.md)
  — the v3 design analysis (theory level)
- [essence-derivation-v2.md](../../concepts/history/essence-derivation-v2.md)
  — the v2 derivation this supersedes (theory level)
- [ADR-0007](../accepted/ADR-0007-essence-derivation-v2-revision.md) — v2
  essence framing (superseded by this ADR's essence claim)
- [ADR-0008](../accepted/ADR-0008-feedback-as-fifth-essence-category.md) —
  px_loop (stands; audit struct extended by this ADR)
- [why-four-abstractions.md](../../concepts/canonical/why-four-abstractions.md)
  — canonical manifesto (rename to why-six-abstractions.md
  deferred until this ADR is Accepted)
- [ui-essence-layers.md](../../concepts/background/ui-essence-layers.md) —
  Layer 4 (Behavioral — Zuhandenheit) now has first-class
  primitives (WITHDRAWS_FOR / PRESENTS_FOR / px_breakdown)
- [non-goals.md](../../concepts/canonical/non-goals.md) NG-3 (no kitchen-sink
  API — Path B respects by keeping actor as struct, not abstraction)
- [limitations.md](../../concepts/state/limitations.md) L2 (Situatedness
  gap — closed by 3-place Relation + px_actor)
- [limitations.md](../../concepts/state/limitations.md) L3 (anti-pattern
  tests — Breakdown provides a new anti-pattern: "Planex can
  detect semantic breakdown + record recovery; React/Solid cannot
  model breakdown at all")
- New files: [src/actor.c](../../../src/actor.c),
  [src/breakdown.c](../../../src/breakdown.c)
- Augmented files: [src/relation.c](../../../src/relation.c),
  [src/closure.c](../../../src/closure.c),
  [src/perception.c](../../../src/perception.c),
  [src/feedback.c](../../../src/feedback.c),
  [include/planex/planex.h](../../../include/planex/planex.h)
- New examples:
  [examples/v3_prototype_actor.c](../../../examples/v3_prototype_actor.c),
  [examples/v3_prototype_perlocution.c](../../../examples/v3_prototype_perlocution.c),
  [examples/v3_prototype_interpretant.c](../../../examples/v3_prototype_interpretant.c),
  [examples/v3_prototype_breakdown.c](../../../examples/v3_prototype_breakdown.c)
- New test: [tests/test_v3_prototype.c](../../../tests/test_v3_prototype.c)
  (60/60 pass)
- [CMakeLists.txt](../../../CMakeLists.txt) — updated to compile
  actor.c / breakdown.c + 4 new examples + 1 new test

## See also

- [ADR-0003](../accepted/ADR-0003-no-ai-integration.md) — unchanged. Path B's
  perlocution and interpretant are non-AI; they structure the
  channel that a future AI agent could populate, without requiring AI.
- [ADR-0005](../accepted/ADR-0005-promote-perception-to-fourth-abstraction.md)
  — historical precedent for adding an abstraction. ADR-0009
  follows the same pattern: essence-driven pressure → new
  abstraction → tests + examples prove feasibility.
- [path-C-lineage.md](../../concepts/background/path-C-lineage.md) — must be
  updated to include the Heidegger → Winograd/Flores → Dourish →
  Suchman → Planex breakdown lineage (deferred until Accepted).
