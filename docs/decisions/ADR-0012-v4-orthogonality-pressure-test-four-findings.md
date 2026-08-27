# ADR-0012: v4 Orthogonality Pressure Test — Four Findings (Two L2 Leaks + One Migration Gap + One Protocol Coupling)

> **Status:** Accepted (2026-08-28)
>
> **Closes:** `abstraction-form.md` Prerequisite 2 honesty-table row "3/3 v4 untested"
>
> **Opens:** Retire targets for v4 L2 leaks (when v4 ships) + migration-cycle proposal (ADR-0013 candidate)

---

## Context

`docs/concepts/abstraction-form.md` Prerequisite 2 (orthogonal separability) honesty-table row states:

> 5/5 shipping pass; **3/3 v4 untested**; leak budget: 9 L2 / 53 ops (17%) overall

The "3/3 v4 untested" referred to the three NEW first-class abstractions introduced by essence-derivation-v4-clean.md:

| Pair | v0.4 sibling | v4 new | Why this seam was at risk |
|------|--------------|--------|---------------------------|
| 1 | Perception (essence #2) | Interpretant (essence #3) | v3 bolted Interpretant onto Perception as a sub-API; v4 promoted it to first-class. The split could have left the abstraction still implicitly depending on Perception internally — invisible without a pressure test. |
| 2 | Closure (essence #4) | Perlocution (essence #5) | v3 bolted Perlocution onto Closure as a sub-API; v4 promoted it to first-class AND removed Closure's `px_closure_get_status` / `promise` / `declare` / `fail` (moved to Perlocution). The split could have left Closure observably incomplete without Perlocution — invisible without a pressure test. |
| 3 | Relation (essence #6) | Breakdown (essence #8) | `px_breakdown_to_relation(b, g, node)` is an explicit bridge function. The question was whether this is a one-way bridge (Breakdown → Relation, opt-in) or a hidden bidirectional coupling (Relation queries reach back into Breakdown's per-actor storage). |

Spolsky's "law of leaky abstractions" is universal — every non-trivial abstraction leaks. The question this ADR answers: **do the v4 proposal seams leak enough to fail Prerequisite 2, or do the leaks stay within the v0.4 leak budget's pattern (L2 ≤ 17% aggregate)?** Before this ADR, the question was unanswered.

A pressure test was needed because the v4 sources (`v4/include/planex/planex.h` + `v4/src/*.c`) compile and pass smoke tests, but **no test verified orthogonality**. The existing `tests/test_orthogonality.c` covers v0.4 shipping 5; `tests/test_v3_prototype.c` covers v3 Path B (where Interpretant/Perlocution were sub-APIs of Perception/Closure — *not* first-class). v4's first-class status was untested.

## Decision

**Pressure-test the three v4 seams via a new test suite, `tests/test_v4_orthogonality.c` (19 tests, six categories), and record the four findings this ADR documents.**

The pressure test compiles v4 sources as a separate static library (`planex_v4_lib`), NOT linked into the shipping `planex_lib`. v4 remains a clean-room verification artifact — ABI breaks from shipping Planex are intentional and do not affect users until v4 ships.

### Test categories

- **A. Removal** — remove ONE v4 abstraction, verify others still work
- **B. Swap** — replace a binding with NULL/freed, observe what breaks
- **C. Composition** — any subset usable independently
- **D. Boundary** — the actual pressure tests; this is where the four findings surfaced
- **E. Essence Claim** — verify v4 essence claims hold

### The four findings

All 19 tests pass. The four findings are **NOT test failures** — they are **L2 leaks / migration gaps / protocol couplings** that the tests were designed to surface. The tests pass because the abstractions behave as documented; the findings document the gap between "as documented" and "as the denotational ideal would claim".

#### Finding 1 — Interpretant.representamen_source field unused by any operation (L2 leak)

`px_interpretant_new(px_perception* representamen_source, px_actor* actor)` stores the perception pointer in the struct. But **no Interpretant operation reads it**:

- `px_interpretant_set_intended` — writes to `it->intended`, no Perception reference
- `px_interpretant_intended` — reads `it->intended`, no Perception reference
- `px_interpretant_set_interpret_fn` — writes `it->fn` and `it->fn_user`, no Perception reference
- `px_interpretant_predict(it, representamen)` — calls `it->fn(representamen, it->actor, it->fn_user)`; the representamen is passed explicitly by the caller, NOT read from `it->representamen_source`
- `px_interpretant_matches_intended(it, actual)` — string equality between `it->intended` and `actual`; no Perception reference

Test `test_d1_interpretant_representamen_source_unused` confirms: create a perception, bind it to an interpretant, FREE the perception (the interpretant now holds a dangling pointer), then verify all Interpretant operations still work — proving the field is never consulted.

This is an **L2 leak** by the criterion in `docs/concepts/leak-budgets.md`: "An operation leaks when the user must understand or interact with an underlying mechanism that the abstraction's denotational semantics does not capture." Here, the *constructor signature* leaks a claim of dependency that the operations don't honor. The user is told (via the type signature) "this interpretant is bound to this perception" — but the binding is documentation-only, not enforced or used.

**Why this matters:** A user might assume the Interpretant will use its bound Perception internally — e.g., calling `px_interpretant_predict(it, NULL)` expecting the abstraction to invoke its bound perception. They will get NULL (predict calls fn with NULL representamen). The semantics are surprising relative to the constructor's type signature.

#### Finding 2 — Perlocution.closure field unused by any operation (L2 leak)

`px_perlocution_new(px_closure* c, px_actor* actor)` stores the closure pointer in the struct. But **no Perlocution operation reads it**:

- `px_perlocution_set(p, kind, text)` — writes to `p->kind` and `p->text`, no Closure reference
- `px_perlocution_kind_get` — reads `p->kind`, no Closure reference
- `px_perlocution_text` — reads `p->text`, no Closure reference
- `px_perlocution_status` — pure function of `p->kind`, no Closure reference

Test `test_d2_perlocution_closure_field_unused` confirms: create a closure, bind it to a perlocution, FREE the closure (the perlocution now holds a dangling pointer), then verify all Perlocution operations still work — proving the field is never consulted.

This is the **same L2 pattern as Finding 1**, transposed to the Perlocution/Closure seam. The constructor claims a dependency on Closure that operations don't honor.

**Why this matters:** A user might assume `px_perlocution_status(p)` queries the bound closure's state — e.g., returning FAILED if the closure's eval returned false. It does not. Status is a pure function of the perlocution's own kind enum. The user must call `px_perlocution_set(...)` to update the status; the abstraction does not derive it from the closure.

#### Finding 3 — Closure lost px_closure_get_status in v4 (migration gap, NOT an L2 leak)

v0.4 Closure had `px_closure_get_status(c)` returning `IDLE / RUNNING / DONE / FAILED`. v4 REMOVED this; operational status is now derived from Perlocution via `px_perlocution_status(per)`.

This is **NOT an L2 leak** — Closure's v4 operations all match their names (`trigger` triggers, `replay` replays, `last_intent` returns the last intent, etc.). There is no operation whose name implies behavior it does not perform.

This is a **migration gap** — the kind ADR-0011 Q3 self-flags ("criteria documented but not enforced; migration cycle not exercised"). A v0.4 user moving to v4 cannot query closure status without instantiating a Perlocution. The closure's observable behavior is reduced: in v0.4 you could ask "did this closure succeed?" from the closure alone; in v4 you must wire up a Perlocution object.

**Why this matters:** This is the kind of regression that ADR-0011 Q3 was warning about. The essence-redistribution is *correct* — "operational status is observable by the actor, hence perlocutionary, not illocutionary" is sound semantically. But the migration cost is real, and Planex has no documented porting recipe or temporary bridge API. This finding flags ADR-0013 as a candidate ("Migration cycle for v0.4→v4 essence redistribution").

#### Finding 4 — Interpretant→Breakdown is protocol coupling, NOT code coupling (acceptable)

The essence derivation claims: "Interpretant mismatch → Breakdown candidate" (see `v4/include/planex/planex.h` Interpretant section header comment). But **no Interpretant operation calls into Breakdown internally**. The user must wire the protocol:

```c
void* predicted = px_interpretant_predict(it, representamen);
if (!px_interpretant_matches_intended(it, predicted)) {
    px_breakdown_record(actor, PX_BD_INTERPRETANT_MISMATCH, "actor misread", NULL);
}
```

Test `test_d4_interpretant_breakdown_protocol_coupling` verifies:
1. When `matches_intended` returns false, **no breakdown is recorded** by the Interpretant abstraction (it doesn't reference Breakdown).
2. The user-side wiring (the if-statement above) does produce a breakdown.
3. The two abstractions are independent in code — Interpretant's `.c` file does not include Breakdown's header (the include is via `planex.h`, which includes everything, but no Interpretant function calls any `px_breakdown_*` function).

This is **acceptable per `abstraction-form.md` Prerequisite 2**: orthogonality is about *code* coupling (one abstraction's operations referencing another's state). Protocol coupling — the *user* wiring compositions — is the point of orthogonal abstractions. If the recipe is documented (e.g., in a tutorial), this is not a Prerequisite 2 failure.

**Why this matters:** This finding is included to *forestall* a future misreading. A reviewer might see "Interpretant mismatch → Breakdown" in the essence derivation and conclude the two abstractions are coupled. They are not — the arrow is in the *recipe*, not in the *code*. The test proves this by demonstrating that `matches_intended == false` does NOT record a breakdown on its own.

### Two implementation findings (not abstraction-level)

These are not abstraction-level findings; they are v4 verification-scale implementation issues that surfaced during test writing:

1. **Per-actor breakdown storage needs `px_breakdown_reset()` test API.** `v4/src/breakdown.c` uses a global `g_actor_breakdowns[32]` table keyed by `px_actor*` pointer. When actor memory is freed and the allocator reuses it for a new actor, the dangling pointer in the table collides with the new actor — `px_breakdown_count(new_actor)` returns the old actor's count. This is documented in `breakdown.c` as "verification-scale only" storage, but the test suite needed `px_breakdown_reset()` (added to v4 header + `breakdown.c`) to isolate tests. When v4 ships, this storage must be replaced with a per-actor hash or per-actor linked list owned by the actor struct.

2. **`px_breakdown_get(actor, idx)` returns records in LIFO (most-recent-first) order.** Records are prepended to a per-actor linked list, so `idx=0` is the most recent, not the oldest. The v4 header does NOT document this. A reader expecting chronological order would be surprised. Fix: document the order in `planex.h` Breakdown section, or change to FIFO (or to a documented "two-sided queue"). Trivial fix; included in v0.5 if v4 ships.

## Essence Check

Per `docs/decisions/README.md` "ADR format (mandatory)", all abstraction-affecting decisions require an Essence Check. This ADR is unusual: it does not *make* an abstraction-affecting decision; it *audits* an abstraction-affecting decision already made (ADR-0010 v4 essence derivation). The Essence Check answers are framed accordingly.

### Q1: Which essence demand does this serve?

**Demand #3 (Representamen)** and **Demand #5 (Perlocution)** and **Demand #8 (Breakdown)**.

The pressure test verifies whether v4's three new first-class abstractions — Interpretant (#3), Perlocution (#5), Breakdown (#8) — satisfy the orthogonal-separability clause of `abstraction-form.md` Prerequisite 2. Before this audit, those demands' implementation status was "designed but not pressure-tested." After this audit, two of the three demands (Interpretant, Perlocution) have known L2 leaks; the third (Breakdown) is clean.

### Q2: Can this be traced to a specific tradition (author + paper + date) that defines the essence demand?

Yes, three traditions:

- **Peirce 1860s (triadic semiotics)**: representamen → object → interpretant. Planex's Interpretant abstraction is the third leg of Peirce's triad; the pressure test verifies that this leg is separable from the first (Perception/Representamen) without implicit coupling.
- **Searle 1969/1975 (speech act theory)**: illocution (Closure) vs perlocution (Perlocution). Planex's Perlocution abstraction is the *effect on the actor's mental state*; the pressure test verifies that this is separable from the illocution (Closure) without implicit coupling.
- **Heidegger 1927 / Winograd-Flores 1986 / Dourish 2001 / Suchman 1987** (breakdown-recovery, embodiment, situatedness): Breakdown is the moment the boundary becomes visible to the actor. Planex's Breakdown abstraction records semantic breakdowns; the pressure test verifies that this is separable from Relation (the relational ontology) — the bridge is opt-in, not required.

### Q3: Is the abstraction orthogonal to the other essences, or does it overlap?

**Pressure-tested answer: orthogonal in CODE, with two documented L2 leaks at the constructor-signature level, and one migration gap at the user-facing level.**

Specifically:
- Interpretant ⊥ Perception: orthogonality HOLDS in code (no Interpretant operation reads Perception). The L2 leak is at the constructor-signature level — `px_interpretant_new` accepts a Perception pointer that is never read.
- Perlocution ⊥ Closure: orthogonality HOLDS in code (no Perlocution operation reads Closure). The L2 leak is at the constructor-signature level — `px_perlocution_new` accepts a Closure pointer that is never read.
- Breakdown ⊥ Relation: orthogonality HOLDS in code AND at the signature level. The bridge (`px_breakdown_to_relation`) is one-way and opt-in.
- Closure ↔ Perlocution are orthogonal in code but NOT in use — Closure's lost `px_closure_get_status` requires Perlocution to observe operational status. This is a migration gap, not a Prerequisite 2 failure.

**Q3 self-acknowledged gap:** The findings are documented in this ADR; the *enforcement* (a CI lint that flags new v4 abstractions whose constructor accepts a pointer that no operation reads) is NOT yet in place. ADR-0011 Q3 already acknowledges this gap for the essence-justified three criteria; the same gap applies here. A future CI lint should:
1. For each new abstraction's constructor parameters, verify that at least one operation reads each parameter (directly or via a setter/getter that uses it).
2. If a parameter is accepted but never read, flag it as an L2 leak candidate requiring either removal or active use.

### Q4: For each claim made about the abstraction, can you name a verifiable cost (what happens if the claim is false)?

The four findings ARE the verifiable costs:

- **Claim "Interpretant is bound to a perception":** verifiable cost = if false, `px_interpretant_predict(it, NULL)` would crash or return wrong value (because the abstraction would try to invoke the bound perception). Pressure test confirms the claim IS false (predict works without the perception), so the cost is paid: the constructor signature is misleading. Finding 1.
- **Claim "Perlocution is bound to a closure":** verifiable cost = if false, `px_perlocution_status(p)` would not depend on the closure. Pressure test confirms the claim IS false (status is a pure function of kind), so the cost is paid: the constructor signature is misleading. Finding 2.
- **Claim "Closure observable behavior includes status":** verifiable cost = if false, a v0.4 program querying `px_closure_get_status` would not compile against v4. Pressure test confirms the claim IS false (no status query exists in v4 Closure API), so the cost is paid: migration requires Perlocution instantiation. Finding 3.
- **Claim "Interpretant mismatch → Breakdown candidate":** verifiable cost = if the abstractions are code-coupled, calling `px_interpretant_matches_intended(it, actual)` with a mismatch should auto-record a breakdown. Pressure test confirms the abstractions are NOT code-coupled — the user must wire the protocol. The claim is a recipe, not a runtime behavior. Finding 4.

### Q5: If this abstraction is wrong, what is the fallback form?

Per `abstraction-form.md` Prerequisite 2 failure cascade:

> Orthogonality breaks → tangled layer cake → fallback form is DSL.

**Pressure-test verdict: Prerequisite 2 does NOT fail for v4.** The 3/3 v4 seams all pass code-orthogonality (no abstraction's operations reference another's state). The two L2 leaks are at the *signature* level, not the *code-coupling* level — they are fixed by removing the unused parameter (one-line change to each constructor), not by collapsing the abstractions into a DSL.

If, despite this, the leaks compound (e.g., future v4 operations are added that DO read the stored pointers, creating implicit coupling), the fallback is:

- For Interpretant/Perception: collapse back to v3 Path B's "Perception with intended_interpretant sub-API" form. This is the form Planex rejected in v4 — but it remains a known-working fallback if first-class separation proves unworkable at scale.
- For Perlocution/Closure: collapse back to v3 Path B's "Closure with perlocution sub-API" form. Same rationale.
- For Breakdown/Relation: there is no fallback — Breakdown without Relation is just a per-actor event log, which is the form Planex uses internally before any `to_relation` bridge is called. The bridge exists because Relation queries are useful for breakdowns; removing the bridge does not collapse Breakdown into Relation.

## Consequences

### Positive

1. **The v4 proposal's riskiest claim is now empirically grounded.** Before this audit, `abstraction-form.md`'s honesty-table row "3/3 v4 untested" was the most pointed critique a reviewer could make. After this audit, that row can be updated to "3/3 v4 pressure-tested: code-orthogonal; 2 L2 leaks at signature level + 1 migration gap; all bounded by retire targets." The risk of external reviewers pointing at v4 as "prematurely ossified" is materially reduced.

2. **The leak-budget framework now extends naturally to v4.** Findings 1 and 2 are textbook L2 leaks by `leak-budgets.md`'s definition — they fit the existing criterion without modification. The retire-target machinery (v0.5/v0.6/v1.0 retire curve) applies directly: v4's L2 count starts at 2 (across Interpretant's 5 ops + Perlocution's 6 ops = 11 ops, giving a v4 L2 rate of 2/11 = 18%), and retire targets can be set when v4 ships.

3. **The migration gap is surfaced before v4 ships.** Finding 3 (Closure lost status) was knowable from reading the v4 header, but the pressure test makes it concrete and documented. This is the kind of regression that, if discovered by a v0.4 user *after* v4 ships, becomes a "Planex broke my code" GitHub issue. Discovered now, it becomes ADR-0013's responsibility — a migration cycle with a temporary status accessor or a documented porting recipe.

### Negative

1. **The v4 proposal is no longer "clean."** Before this audit, v4's pitch was "essence derivation v4 clean-room." After this audit, v4 has two L2 leaks and a migration gap. The word "clean" in `essence-derivation-v4-clean.md` should be qualified — perhaps rename to `essence-derivation-v4-pressure-tested.md` in a future revision. The naming issue is cosmetic but the semantic shift is real: v4 is *audited*, not *clean*.

2. **A new test-only API (`px_breakdown_reset`) is now in the v4 header.** This pollutes the abstraction's public surface for the sake of test isolation. When v4 ships, the storage must be redesigned so `px_breakdown_reset` is removed from the public API (per Finding 5 implementation note).

3. **The two L2 leaks require fixing before v4 ships.** Each fix is a one-line API change (remove the unused parameter from the constructor, or actually use the parameter in a new operation). But every API change to v4 delays shipping. The fixes are small but the principle (no L2 leaks in shipping v4) must be enforced, which means v4 ships with a tighter API than the v4-clean.md derivation specified.

### Neutral

1. **`tests/test_v4_orthogonality.c` is now a permanent test artifact.** It must be maintained alongside v4. If v4's API changes, the test must change. The test currently exercises 19 scenarios; future v4 operations require new test cases. This is normal maintenance overhead, not a special burden.

2. **The four findings create documentation work.** Each finding must be cross-linked from `abstraction-form.md` (Prerequisite 2 row), `leak-budgets.md` (v4 preview section), `essence-derivation-v4-clean.md` (postscript), and `limitations.md` (if any of the gaps are limitation-grade). This cross-linking is the cost of the audit; without it, the findings would be local to the ADR and lose force.

## Alternatives Considered

### Alternative A: Do not pressure-test v4; ship it as-is and react to GitHub issues

**Rejected.** This is the opposite of ADR-0010's "honesty downgrade" posture. v4 was already framed as "design rationale, not essence discovery" precisely because its claims were unverified. Pressure-testing is the verification step. Skipping it would have left the honesty-downgrade framing as marketing, not substance.

### Alternative B: Pressure-test by code-reading only (no test suite)

**Rejected.** Code-reading would have surfaced Finding 1 and Finding 2 (the unused fields are visible in `v4/src/interpretant.c` and `v4/src/perlocution.c`). But code-reading would NOT have surfaced Finding 3 (the migration gap requires running a v0.4-style program against v4 to discover that no status query exists) or Finding 4 (protocol coupling requires running the recipe to confirm no auto-record happens). A test suite is the falsifiable artifact; code-reading is the assertion.

### Alternative C: Pressure-test by integrating v4 into the shipping Planex (replacing v0.4)

**Rejected.** v4's ABI breaks are intentional and unresolved. Replacing v0.4 with v4 would break every shipping example, every shipping test, every shipping demo. The pressure test needed to verify v4 *as a proposal* without committing to it. The `planex_v4_lib` separate-library approach gives this isolation.

### Alternative D: After finding L2 leaks, mark v4 as "rejected" and stay on v0.4

**Rejected.** The two L2 leaks are bounded and retire-target-able; they do not indicate Prerequisite 2 failure. The migration gap is real but is ADR-0013's territory, not a reason to abandon v4. The three v4 abstractions (Interpretant, Perlocution, Breakdown) are essence-correct; the leaks are implementation defects, not essence errors. Rejecting v4 would be the "premature ossification" failure mode that `abstraction-form.md` Prerequisite 1 warns against.

## References

- **Code:** `tests/test_v4_orthogonality.c` — the 19-test pressure suite, four findings as test cases D1-D4
- **Code:** `v4/include/planex/planex.h` — v4 API surface (Interpretant section, Perlocution section, Breakdown section)
- **Code:** `v4/src/interpretant.c`, `v4/src/perlocution.c`, `v4/src/breakdown.c`, `v4/src/loop.c` — implementations whose leaks the test exposes
- **Code:** `v4/src/breakdown.c` `px_breakdown_reset()` — test-only API added for test isolation (implementation note 1)
- **Build:** `CMakeLists.txt` — `planex_v4_lib` static library + `test_v4_orthogonality` executable + `v4_orthogonality_tests` test target
- **Related docs:** [`docs/concepts/abstraction-form.md`](../concepts/abstraction-form.md) — Prerequisite 2 honesty-table row updated by this ADR
- **Related docs:** [`docs/concepts/leak-budgets.md`](../concepts/leak-budgets.md) — v4 preview section added by this ADR
- **Related docs:** [`docs/concepts/essence-derivation-v4-clean.md`](../concepts/essence-derivation-v4-clean.md) — postscript added by this ADR
- **Related ADRs:** [ADR-0010](ADR-0010-v4-design-rationale-not-essence-discovery.md) — the v4 framing this ADR audits
- **Related ADRs:** [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — Q3 self-acknowledged gap (criteria documented but not enforced) that Finding 3 + the implementation findings extend
- **Proposed (this ADR opens):** ADR-0013 — Migration cycle for v0.4→v4 essence redistribution (specifically addressing Finding 3)
- **External:** Joel Spolsky, "The Law of Leaky Abstractions" (2002) — the canonical statement that this ADR's L2 findings quantify for v4
- **External:** Conal Elliott, "Denotational Design with Type Class Morphisms" — the abstraction-as-typed-value ideal whose distance from v4 implementation L2 leaks measure
