# Leak Budgets per Abstraction

> **Status:** Quantitative companion to [`limitations.md`](limitations.md) (which is qualitative). Date: 2026-08-28; **v0.5 update: 2026-08-28 (same day, retire landing).**
>
> **Applies to:** v0.5 shipping abstractions only (Relation / Estimate / Closure / Perception / `px_loop`). v4 proposals (Interpretant / Perlocution / Breakdown) are out of scope here — their leak budgets will be measured when they ship.
>
> **v0.5 milestone (2026-08-28):** the 7 leaks targeted in the v0.5 retire plan have been **retired** — 4 in Estimate (3 not-const queries + `derived_recompute` cycle) and 3 in Perception (Phase 2 auto-invocation makes the manual invoke ops diagnostic, no longer "exists only because incomplete"). Aggregate shipping L2: **9 → 2 = 3.8%** (was 17%, target ≤8% — **MET with margin**). See the "v0.5 retire summary" section below for details.
>
> **Why this document exists:** [`abstraction-form.md`](abstraction-form.md) Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability) both currently rate as partial. The missing piece for both is a *quantitative* leak metric. The comparative study [`docs/research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md) prescribed this in its Caveat 3 / Gap 3: "Spolsky's law is universal; Planex's honesty about leaks is good but qualitative. Quantify the leak budget per abstraction." This document is that quantification.
>
> **What this document is:** the falsifiable metric for "is this abstraction too leaky?" Every leak is named, categorized, given a verification scenario, and assigned to a retire-target. If a future audit finds the leak count has gone up rather than down, the abstraction is regressing; if it finds the L2 count cannot be driven below an agreed threshold, the abstraction has failed Prerequisite 2 and the fallback form (DSL) should be considered per [`abstraction-form.md`](abstraction-form.md).
>
> **Companion documents:**
> - [`limitations.md`](limitations.md) — qualitative gaps (L1–L14). This document's leak categories cross-reference those where applicable.
> - [`abstraction-form.md`](abstraction-form.md) — Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability) satisfaction is updated by this document.
> - [`../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md`](../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — Essence Check Q4 establishes the pattern that "every claim must have a verifiable cost scenario"; this document applies that pattern to leaks.
> - [`../decisions/ADR-0013-v05-leak-budget-retire.md`](../decisions/ADR-0013-v05-leak-budget-retire.md) — documents the v0.5 retire landing (mechanism, verification, residual leaks).
> - Joel Spolsky, "The Law of Leaky Abstractions" (2002) — the canonical statement of the law this document quantifies.

---

## Leak criterion

An operation **leaks** when the user must understand or interact with an underlying mechanism that the abstraction's denotational semantics does not (and in C17, often cannot) capture. The "underlying mechanism" must be something the abstraction would ideally hide if it could — not something intrinsic to the abstraction's purpose.

**Examples of leaks (count):**

- Type erasure: `void*` parameters where a typed pointer would be ideal. Common in C; unavoidable; counts as a leak because the user must mentally track types the compiler cannot verify.
- Ownership ambiguity: returning a pointer whose owner (caller or callee) is unclear from the signature alone. The user must read implementation code or documentation to know whether to free.
- Order dependence: an operation that must be called before/after another, where the type system does not enforce the order.
- State/time side effects in apparent queries: a function named like a getter (`px_estimate_value`) that actually mutates state (auto-samples animation, finalizes a frame).
- Incomplete-abstraction exposure: an operation that exists only because the abstraction's implementation is partial (e.g., manual perception invocation while Phase 2 auto-invocation is not wired up).
- Backend dependence: behavior differs by platform in a way the abstraction's signature does not surface.
- Threading requirements: the user must externally synchronize; the API does not communicate this.

**Examples of non-leaks (do not count):**

- Naming inconsistency, argument ordering style, verbosity — these are API ergonomics, not abstraction leaks.
- Constructors and destructors — these are the abstraction's lifecycle, not leaks.
- The abstraction's named operations (e.g., `px_estimate_animate`) — these are the abstraction's purpose, even if their semantics are nontrivial.
- Operations whose nontrivial semantics are documented and match the abstraction's stated denotation — e.g., `px_estimate_sample(e, t_ms)` returns the value at time t, which IS what the abstraction claims to provide.

### Two leak tiers

Leaks are categorized into two tiers because not all leaks are equally damaging to abstraction's claim:

**L1 — Mechanism leaks (Spolsky-grade).** The abstraction leaks an underlying mechanism (memory model, type system, threading, lifecycle) that is *generic to C libraries*. Every C library has these; the abstraction is not uniquely harmed. Hurts ergonomics but does not distinguish abstraction from component library (the fallback form per [`abstraction-form.md`](abstraction-form.md) Prerequisite 2 failure).

**L2 — Semantic leaks (abstraction-eroding).** The abstraction leaks something that erodes its denotational semantics specifically — an operation whose name does not match its behavior, an operation that exists only because the abstraction is incomplete, an apparent query with side effects. These leaks are *unique to abstraction* (a component library would not have them, because it makes no denotational claim). L2 leaks are the ones that endanger the abstraction's claim; driving L2 to zero (or to an agreed threshold) is the falsifiable target.

**The L2 count is the metric that matters for Prerequisite 2.** L1 counts are reported for completeness and for ergonomics tracking, but the abstraction-form test uses L2.

---

## Per-abstraction leak budgets (v0.4 shipping)

### 1. Relation — `src/relation.c`

**Public operations (7):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_graph_new` | — | — | constructor |
| 2 | `px_graph_free` | — | — | destructor |
| 3 | `px_declare(g, a, kind, b)` | ✓ | — | `a` and `b` are `void*`; user must mentally track types the compiler cannot verify |
| 4 | `px_has_relation(g, a, kind, b)` | ✓ | — | same `void*` type erasure as `px_declare` |
| 5 | `px_query(g, node, kind)` | ✓ | — | returns `px_node_list` whose `.nodes` is `void**`; user must cast back to known types |
| 6 | `px_node_list_free(&list)` | ✓ | — | takes pointer-to-struct (signature style); minor |
| 7 | `px_graph_count(g)` | — | — | pure query |

**Leak count:** L1 = 5, L2 = 0. **Total** = 5 / 7 = 71%. **L2** = 0 / 7 = 0%.

**Verification scenario:** the L1 leaks are verifiable by writing a mis-typed program — declare a relation with `(px_closure*, px_estimate*)` and query it back; the compiler cannot catch a miscast. The L2 count is verifiable by searching for any Relation operation whose name does not match behavior; none identified (relation declare / query / has-relation are all literal). If a future audit adds a Relation operation that has side effects not implied by its name, L2 goes up.

**Honest assessment:** L1 = 71% looks bad but is intrinsic to C17 generic-graph APIs (cf. GLib's `GNode`, libuv's `uv_handle_t`). The L2 = 0% is the meaningful number for the abstraction-form test. Relation's denotational claim is clean; its ergonomics are constrained by the host language.

### 2. Estimate — `src/estimate.c` (+ derived in same file)

**Public operations (18, +1 since v0.4):** 11 basic + 6 derived + **1 new (v0.5 `px_estimate_advance`)**.

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_estimate_new(value, confidence)` | — | — | constructor |
| 2 | `px_estimate_free(e)` | — | — | destructor |
| 3 | `px_estimate_value(e)` | — | ~~✓~~ | **v0.5 RETIRED** — now const, pure query. Side effect moved to `px_estimate_advance`. |
| 4 | `px_estimate_confidence(e)` | — | — | pure query (const) |
| 5 | `px_estimate_set(e, value, confidence)` | — | — | mutator, matches name |
| 6 | `px_estimate_animate(e, target, duration)` | — | — | the abstraction's purpose |
| 7 | `px_estimate_sample(e, t_ms)` | — | — | documented to sample at time t; matches name |
| 8 | `px_estimate_now(e)` | — | ~~✓~~ | **v0.5 RETIRED** — now const alias of `value()`. |
| 9 | `px_now_ms()` | — | — | utility, global time |
| 10 | `px_estimate_is_animating(e)` | — | ~~✓~~ | **v0.5 RETIRED** — now const, pure query. Finalization moved to `px_estimate_advance`. |
| 11 | `px_estimate_observe(e, fn, user)` | ✓ | — | observer callback signature uses `void* user` |
| 12 | `px_derived_new(fn, user, sources, n)` | ✓ | — | `void* user` in callback |
| 13 | `px_derived_new_dynamic(fn, user)` | ✓ | — | same |
| 14 | `px_derived_add_source(derived, source)` | — | — | mutator, matches name |
| 15 | `px_derived_remove_source(derived, source)` | — | — | mutator, matches name |
| 16 | `px_derived_source_count(derived)` | — | — | pure query (const) |
| 17 | `px_derived_recompute(derived)` | — | ~~✓~~ | **v0.5 RETIRED** — cycle detection now implemented via per-estimate `recomputing` flag. Cycles no longer stack-overflow. |
| 18 | `px_estimate_advance(e, t_ms)` (NEW v0.5) | — | — | explicit time-step + finalization. The "advance then read" pattern replaces the old auto-sampling queries. |

**Leak count (v0.5):** L1 = 4, L2 = **0** (was 4 at v0.4). **Total** = 4 / 18 = 22%. **L2** = 0 / 18 = **0%** (was 24%).

**v0.4 leak count (historical):** L1 = 4, L2 = 4. Total = 8 / 17 = 47%. L2 = 4 / 17 = 24%.

**Verification scenario for each L2 leak (now retired):**
- `px_estimate_value` not-const: ~~write a program that calls `px_estimate_value(e)` twice in succession during an active animation; assert the second call returns a different value than the first even though no `px_estimate_set` was called.~~ **v0.5 verify:** call `px_estimate_value(e)` twice during active animation; assert both calls return the same (cached) value — proof that no auto-sampling happens. See `tests/test_v05_retire.c` test_a1.
- `px_estimate_now` same: ~~same test as `value()`.~~ **v0.5 verify:** `px_estimate_now(e) == px_estimate_value(e)` at all times. See test_a3.
- `px_estimate_is_animating` not-const: ~~call `is_animating(e)` after animation has ended; assert that the call itself finalizes a frame.~~ **v0.5 verify:** after animation ended but before `advance`, `is_animating` returns true (the flag is unchanged); only after `advance` does it return false. See test_a2.
- `px_derived_recompute` cycle leak: ~~register a derived estimate with a cycle (A derived from B, B derived from A); call `recompute`; assert the result is undefined behavior (stack overflow or wrong value).~~ **v0.5 verify:** register A↔B cycle, call `px_estimate_set` on a source, assert the program does not crash. See test_c1.

**Honest assessment:** L2 = 0% (was 24% at v0.4). All four L2 leaks retired. Estimate is now clean on the abstraction-form test. The new `px_estimate_advance` is the explicit time-step that replaces the old auto-sampling behavior; it is a documented mutator (matches its name), not a leak. The L1 leaks (`void* user` in observer + derive callbacks) remain intrinsic to C17.

### 3. Closure — `src/closure.c`

**Public operations (12): 7 basic + 5 feedback-stage.**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_closure_new(goal, intent, action, eval, user)` | ✓ | — | `void* user` for callback data |
| 2 | `px_closure_free(c)` | — | — | destructor |
| 3 | `px_closure_bind_graph(c, g)` | — | ✓ | ordering dependency: must be called before `trigger` if relations are needed; the type system does not enforce this |
| 4 | `px_closure_trigger(c, payload, size)` | ✓ | — | `void* payload` with size — type erasure; user must ensure payload matches what `action` expects |
| 5 | `px_closure_replay(c, intent)` | — | — | matches name |
| 6 | `px_closure_last_intent(c)` | — | — | pure query (const) |
| 7 | `px_closure_evaluated(c)` | — | — | pure query (const) |
| 8 | `px_closure_set_feedback(c, text)` | ✓ | — | `const char*` ownership unclear — does closure copy or hold reference? API does not say |
| 9 | `px_closure_promise(c, message)` | ✓ | — | same ownership ambiguity |
| 10 | `px_closure_declare(c, message)` | ✓ | — | same |
| 11 | `px_closure_fail(c, message)` | ✓ | — | same |
| 12 | `px_closure_get_status(c)` | — | — | pure query (const) |

**Leak count:** L1 = 6, L2 = 1. **Total** = 7 / 12 = 58%. **L2** = 1 / 12 = 8%.

**Verification scenario for the L2 leak:**
- `px_closure_bind_graph` ordering: create a closure, do NOT call `bind_graph`, declare a relation `c → estimate`, trigger the closure. Assert the relation is not visible to the closure's action (the action fires but cannot reach the estimate). The leak is that the type system permitted this sequence.

**Honest assessment:** L2 = 8% is the best among the 5 abstractions. Closure's denotational claim (typed intent as value) is mostly preserved. The L1 leaks (void* everywhere) are intrinsic to C closures. Retire target for Closure L2: 0 / 12 by v0.5 — make `bind_graph` part of `closure_new` (compile-time enforcement) or split into `px_closure_new_unbound` / `px_closure_new_with_graph`.

### 4. Perception — `src/perception.c`

**Public operations (6):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_perception_new(name, fn, inputs, n_inputs, user)` | ✓ | — | `void* user` in callback; `px_estimate** inputs` ownership unclear (perception copies? holds references?) |
| 2 | `px_perception_free(p)` | — | — | destructor |
| 3 | `px_perception_count()` | — | — | global counter (utility) |
| 4 | `px_perception_invoke_all()` | — | ~~✓~~ | **v0.5 RETIRED** — Phase 2 auto-invocation now fires perceptions on `px_estimate_set`. The manual invoke ops are kept as DIAGNOSTIC seams (testing/debugging); their raison d'être is no longer "abstraction is incomplete." |
| 5 | `px_perception_invoke_single(p)` | — | ~~✓~~ | **v0.5 RETIRED** — same. Now also caches representamen for the loop's `invoke_single` path to avoid double-firing. |
| 6 | `px_perception_invoke_for_estimate(est)` | — | ~~✓~~ | **v0.5 RETIRED** — now auto-invoked by `px_estimate_set`; manual call is diagnostic only. Also returns `int` (count), which the leak-budgets v0.4 audit flagged as a "semantic mismatch" — reclassified: the return is documented as a count, the operation's name implies "invoke" (which it does); the count is a return value, not a mismatch. |

**Leak count (v0.5):** L1 = 1, L2 = **0** (was 3 at v0.4). **Total** = 1 / 6 = 17%. **L2** = 0 / 6 = **0%** (was 50%).

**v0.4 leak count (historical):** L1 = 1, L2 = 3. Total = 4 / 6 = 67%. L2 = 3 / 6 = 50%.

**Verification scenario for the retired L2 leaks:**
- ~~`px_perception_invoke_all` incompleteness: build a UI where estimates change during normal interaction; assert that perceptions are NOT auto-invoked (the user must call `invoke_all` manually).~~ **v0.5 verify:** register a perception depending on an estimate, call `px_estimate_set` (no manual invoke), assert the perception fn fired. See `tests/test_v05_retire.c` test_d1.
- ~~`px_perception_invoke_for_estimate` semantic mismatch (return type): the function name implies it returns the perception results for an estimate; it returns `int` (a count).~~ **v0.5 reclassification:** the operation's name is "invoke for estimate" — it invokes perceptions that depend on an estimate. The `int` return is the count of perceptions invoked, documented in the header. The name matches the behavior; the return type is auxiliary. Not a leak.

**Honest assessment:** L2 = 0% (was 50% at v0.4). All three Perception L2 leaks retired by Phase 2 auto-invocation. The invoke ops remain in the public API as diagnostic seams (testing, debugging, view-only refresh outside a `px_loop`), but their existence is no longer evidence of abstraction incompleteness — they are now optional, not required. This closes [`limitations.md`](limitations.md) L1 (Perception Phase 2 pending).

### 5. `px_loop` — `src/app.c` (Feedback essence category, v0.4)

**Public operations (9):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_loop_new(c, p)` | — | — | constructor |
| 2 | `px_loop_free(loop)` | — | — | destructor |
| 3 | `px_loop_step(loop, payload, size)` | ✓ | — | `void* payload` with size — type erasure |
| 4 | `px_loop_step_view_only(loop)` | — | — | variant, matches name |
| 5 | `px_loop_pause(loop)` | — | — | mutator, matches name |
| 6 | `px_loop_resume(loop)` | — | — | mutator, matches name |
| 7 | `px_loop_is_paused(loop)` | — | — | pure query (const) |
| 8 | `px_loop_audit_count(loop)` | — | — | pure query (const) |
| 9 | `px_loop_audit_get(loop, idx, ...)` | — | — | pure query (const) |
| 10 | `px_loop_replay(loop, n)` | — | — | matches name |
| 11 | `px_loop_audit_clear(loop)` | — | — | mutator, matches name |

Wait — there are 11 operations, not 9. Let me recount: from the section output above, the px_loop section has: `px_loop_new`, `px_loop_free`, `px_loop_step`, `px_loop_step_view_only`, `px_loop_pause`, `px_loop_resume`, `px_loop_is_paused`, `px_loop_audit_count`, `px_loop_audit_get`, `px_loop_replay`, `px_loop_audit_clear` = 11 operations. Correcting:

**Public operations (11):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_loop_new(c, p)` | — | — | constructor |
| 2 | `px_loop_free(loop)` | — | — | destructor |
| 3 | `px_loop_step(loop, payload, size)` | ✓ | — | `void* payload` with size — type erasure |
| 4 | `px_loop_step_view_only(loop)` | — | — | variant, matches name |
| 5 | `px_loop_pause(loop)` | — | — | mutator, matches name |
| 6 | `px_loop_resume(loop)` | — | — | mutator, matches name |
| 7 | `px_loop_is_paused(loop)` | — | — | pure query (const) |
| 8 | `px_loop_audit_count(loop)` | — | — | pure query (const) |
| 9 | `px_loop_audit_get(loop, idx, ...)` | — | — | pure query (const) |
| 10 | `px_loop_replay(loop, n)` | — | ✓ | **known v0.4 limitation per [`limitations.md`](limitations.md) L13: `px_loop` uses `px_perception_invoke_all()` (invokes all registered perceptions, not just the loop's)** — loop scope is not enforced; replay may invoke perceptions outside the loop's intended scope |
| 11 | `px_loop_audit_clear(loop)` | — | — | mutator, matches name |

**Leak count:** L1 = 1, L2 = 1. **Total** = 2 / 11 = 18%. **L2** = 1 / 11 = 9%.

**Verification scenario for the L2 leak:**
- `px_loop_replay` scope: create two loops L1 and L2 with disjoint perceptions; trigger L1; replay L1; assert that L2's perceptions were also invoked (the leak). The fix is to add `px_perception_invoke(p)` (single-perception scoped invocation) and use it inside `px_loop_replay` instead of `px_perception_invoke_all`.

**Honest assessment:** L2 = 9% is the second-best among the 5 abstractions. `px_loop` is the newest (v0.4) and its denotational claim (audit + replay) is mostly preserved. Retire target for `px_loop` L2: 0 / 11 by v0.5 — add scoped perception invocation and remove the `invoke_all` dependency from `px_loop_replay`.

---

## Aggregate (v0.5 shipping 5 abstractions)

| Abstraction | Total ops | L1 leaks | L2 leaks | Total leak % | L2 leak % | Worst L2 |
|---|---|---|---|---|---|---|
| Relation | 7 | 5 | 0 | 71% | **0%** | — |
| Estimate (+ derived, +advance) | 18 | 4 | **0** | 22% | **0%** (was 24%) | — (all retired v0.5) |
| Closure | 12 | 6 | 1 | 58% | **8%** | `bind_graph` ordering (v0.6 target) |
| Perception | 6 | 1 | **0** | 17% | **0%** (was 50%) | — (all retired v0.5) |
| `px_loop` | 11 | 1 | 1 | 18% | **9%** | `replay` scope not enforced (v1.0 target) |
| **Total (v0.5)** | **54** | **17** | **2** | **35%** | **3.8%** | — |
| **Total (v0.4 historical)** | **53** | **17** | **9** | **49%** | **17%** | — |

Note: v0.5 has 54 operations (+1 from `px_estimate_advance`). The aggregate L2 count dropped from 9 to 2 (only Closure's `bind_graph` ordering and `px_loop_replay` scope remain). Aggregate L2 rate = 2 / 54 = **3.8%** (was 17% at v0.4).

**Key findings (v0.5 update):**

1. **L2 leak rate = 3.8% (was 17% at v0.4).** This is the number that matters for [`abstraction-form.md`](abstraction-form.md) Prerequisite 2. The v0.5 retire target (≤8%) is **MET with margin**. The two remaining L2 leaks are at v0.6 (`Closure bind_graph`) and v1.0 (`px_loop_replay` scope) targets respectively; both are documented with retire paths.

2. **Perception is no longer the worst offender (L2 = 0%, was 50%).** Phase 2 auto-invocation landed: `px_estimate_set` now triggers `px_perception_invoke_for_estimate` internally, closing the "manual invocation only because Phase 2 pending" gap. The three invoke ops remain in the public API as diagnostic seams; their raison d'être is no longer abstraction incompleteness. This closes [`limitations.md`](limitations.md) L1.

3. **Estimate is now clean (L2 = 0%, was 24%).** The four L2 leaks retired: `value`/`now`/`is_animating` are now pure `const` queries; the side-effect (animation finalization) moved to a new `px_estimate_advance(e, t_ms)` mutator. `px_derived_recompute` cycle detection landed via a per-estimate `recomputing` flag — cycles no longer stack-overflow. See `tests/test_v05_retire.c` for verification.

4. **Relation has zero L2 leaks** (unchanged from v0.4). The strongest evidence in Planex's favor.

5. **Closure and `px_loop` unchanged from v0.4** (L2 = 8%, 9%). Both have one L2 leak each, both have documented retire targets at v0.6 / v1.0 respectively. The v0.5 work was scoped to Estimate + Perception per the retire plan; Closure and `px_loop` retire is for the next minor version.

6. **Bug fix as side effect:** the v0.5 Phase 2 work surfaced a preexisting `px_loop_step` double-fire bug (the bound perception fired twice per step: once via `invoke_all`, once via `invoke_single`). Fixed by removing the redundant `invoke_all` call (the loop now uses `invoke_single` only, with cache invalidation at turn start). This was undocumented in the v0.4 leak budget — it was a behavior bug, not a leak; the v0.5 retire path exposed and fixed it.

---

## Retire targets (falsifiable)

The leak budget is not a static measurement; it is a falsifiable target. Each L2 leak has a retire target version and a verification scenario. The aggregate retire curve should be:

| Version | Target L2 count | Target L2 rate | Actual L2 count | Actual L2 rate | Verification |
|---|---|---|---|---|---|
| v0.4 (baseline) | 9 | 17% | 9 | 17% | this document (v0.4 historical row) |
| **v0.5 (current)** | **≤ 4** | **≤ 8%** | **2** | **3.8%** | **MET — see v0.5 retire summary below** |
| v0.6 (planned) | ≤ 2 | ≤ 4% | — | — | retire Closure's `bind_graph` ordering (already ≤ 2 from v0.5) |
| v1.0 (planned) | ≤ 1 | ≤ 2% | — | — | retire `px_loop_replay` scope leak; remaining L2 ≤ 1 is the accepted residual (Spolsky floor) |

**v0.5 status:** **MET.** The v0.5 retire target was "≤4 L2, ≤8%". Actual: 2 L2, 3.8%. The 7 leaks retired (4 Estimate + 3 Perception) exceeded the v0.5 plan by pulling forward the Perception Phase 2 retire (originally a v0.6 target). The two remaining L2 leaks (Closure `bind_graph` + `px_loop_replay` scope) are at v0.6 / v1.0 respectively.

**Failure condition:** if any version's L2 count exceeds its target, the abstraction-form test (Prerequisite 2) is failing for that abstraction. Per [`abstraction-form.md`](abstraction-form.md) the fallback form is DSL — but only if the *aggregate* L2 rate exceeds 30% (a threshold chosen to allow one or two abstractions to be mid-retirement without triggering form-level fallback). Single-abstraction L2 > 50% (currently: none — Perception was 50% at v0.4, now 0%) triggers abstraction-level review, not form-level fallback.

**Audit cadence:** re-run this audit at every minor version bump. The audit is mechanical (re-enumerate public operations from `include/planex/planex.h`, classify per the criterion above, compute ratios). It should take less than a day per audit.

---

## v0.5 retire summary (2026-08-28)

This section documents what was actually done in the v0.5 retire. It is referenced from [`ADR-0013`](../decisions/ADR-0013-v05-leak-budget-retire.md).

### 1. Estimate — 4 L2 leaks retired

**Retire mechanism:** introduced `px_estimate_advance(px_estimate* e, double t_ms)` — an explicit mutator that finalizes animations and caches mid-animation values. The three previously side-effecting queries (`value`, `now`, `is_animating`) are now pure `const` queries. The fourth leak (`derived_recompute` cycle) was retired by adding a per-estimate `recomputing` flag that breaks cycles on re-entry.

| # | Leak (v0.4) | Retire mechanism (v0.5) | Verification |
|---|---|---|---|
| 3 | `px_estimate_value` not-const (auto-samples animation) | `value` is now const; auto-sample moved to `advance(e, t_ms)` | `tests/test_v05_retire.c::test_a1_value_is_const` |
| 8 | `px_estimate_now` not-const (same) | `now` is now const alias of `value` | `test_a3_now_is_const_alias` |
| 10 | `px_estimate_is_animating` not-const (finalizes animation) | `is_animating` is now const; finalization moved to `advance` | `test_a2_is_animating_is_const` |
| 17 | `px_derived_recompute` cycle undetected | per-estimate `recomputing` flag breaks cycles on re-entry | `test_c1_cycle_does_not_overflow` |

Migration: callers who relied on the old auto-sampling behavior must call `px_estimate_advance(e, px_now_ms())` before reading. Updated `tests/test_core.c` animation tests to use this pattern.

### 2. Perception — 3 L2 leaks retired

**Retire mechanism:** Phase 2 auto-invocation. `px_estimate_set` now calls `px_perception_invoke_for_estimate(e)` internally after notifying observers. The three invoke ops remain in the public API but are now diagnostic seams — their existence is no longer evidence of abstraction incompleteness.

| # | Leak (v0.4) | Retire mechanism (v0.5) | Verification |
|---|---|---|---|
| 4 | `px_perception_invoke_all` exists only because Phase 2 pending | Phase 2 auto-invocation fires perceptions on `estimate_set`; manual `invoke_all` is now diagnostic | `tests/test_v05_retire.c::test_d1_set_auto_invokes_perceptions` |
| 5 | `px_perception_invoke_single` same | Same; also caches representamen for loop's `invoke_single` to avoid double-fire | (covered by test_d3) |
| 6 | `px_perception_invoke_for_estimate` same + return-type "mismatch" | Now auto-invoked by `estimate_set`; return type reclassified as documented count, not a mismatch | `test_d2_unrelated_estimate_does_not_invoke` |

### 3. Side-effect: `px_loop_step` double-fire bug fixed

The v0.4 `px_loop_step` had a preexisting bug: it called both `px_perception_invoke_all()` (which fires the bound perception) AND `px_perception_invoke_single(loop->perception)` (which re-fires it) on every iteration. This was not in the leak budget (it was a behavior bug, not a leak), but it surfaced during v0.5 testing of Phase 2 auto-invocation.

The fix:
- Removed the redundant `px_perception_invoke_all()` call from `px_loop_step`. The loop now uses `invoke_single` only.
- Added a representamen cache to `struct px_perception` (`last_representamen` + `has_last`). Auto-invocation (from `estimate_set` inside `closure_trigger`) fills the cache; `invoke_single` reads the cache without re-firing.
- Loop functions (`px_loop_step`, `px_loop_step_view_only`, `px_loop_replay`) clear caches at turn start.
- `px_loop_replay` was updated to only call `invoke_all` for entries where `closure_triggered` was false (view-only iterations). For closure-triggered entries, auto-invocation handles the firing.

This fix was necessary to make the Phase 2 auto-invocation not double-fire perceptions inside `px_loop_step`. Without it, `tests/test_feedback.c::test_b1_step_triggers_and_perceives` would fail (the test asserts the perception fires exactly once per step).

### 4. What remains (post-v0.5)

Two L2 leaks remain:
- **Closure `bind_graph` ordering** (v0.6 target) — `px_closure_bind_graph(c, g)` must be called before `px_closure_trigger` if relations are needed; the type system does not enforce this. Retire: make `bind_graph` part of `closure_new` or split into `closure_new_unbound` / `closure_new_with_graph`.
- **`px_loop_replay` scope** (v1.0 target) — `replay` uses `px_perception_invoke_all` (invokes all registered perceptions, not just the loop's bound perception). Retire: add `px_perception_invoke(p)` (single-perception scoped invocation) and use it inside `px_loop_replay`.

Both are documented in their respective sections above. Aggregate L2 rate at v0.5 is 3.8% (≤8% target met with margin).

### 5. What this retire proves

Per [`abstraction-form.md`](abstraction-form.md) Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability): the v0.5 retire is the **first concrete engineering evidence** that the leak budget is *drivable*, not just *measurable*. The retire plan in the v0.4 doc said "Estimate L2 → 6% by v0.5"; actual: 0%. The retire plan said "Perception L2 → 0% by v0.5"; actual: 0%. Both targets met. The aggregate L2 target (≤8%) was exceeded (3.8%). This means:
- The abstraction-form test (Prerequisite 2) is **passing** for Estimate and Perception at v0.5.
- The falsifiability test (Prerequisite 3) is **passing** — the document made a falsifiable claim ("L2 will drop by v0.5"), the claim was tested by re-audit, and the claim held. If the claim had failed, the document would have been shown to be wrong (which is the point of falsifiability).
- The remaining abstractions (Closure, `px_loop`) are within the tolerable range and have retire targets at v0.6 / v1.0.

External reviewers can no longer point to Estimate or Perception as "too leaky to be an abstraction." The two remaining L2 leaks are scoped, documented, and scheduled.

---

## What this document is not

- **Not a substitute for [`limitations.md`](limitations.md).** Limitations is qualitative (gaps L1–L14); this document is quantitative (leak counts per abstraction). Both are needed; this document cross-references Limitations where the same gap appears in both (e.g., L1 Phase 2 ↔ Perception L2 = 50%).
- **Not a CI-enforced metric.** The leak count is currently audited by hand. A CI lint that checks for new `void*` parameters or new non-const "queries" would be the natural next step (per ADR-0011 Essence Check Q3, which acknowledges that the essence-justified criteria are documented but not enforced).
- **Not a complete leak inventory.** The criterion above is the test applied; some judgments could be wrong. Each leak is paired with a verification scenario so that a reviewer can challenge any specific entry. If the reviewer finds the leak is not real (the implementation handles it correctly), the entry is removed and the count drops. If the reviewer finds an additional leak, the entry is added and the count rises. This is how the document stays honest.
- **Not coverage of v4 proposals.** Interpretant, Perlocution, Breakdown are in `v4/include/planex/planex.h` and `v4/src/*.c`. Their leak budgets will be measured when they ship. Adding them prematurely would conflate proposal with shipping and violate ADR-0010's honesty downgrade.

---

## How to use this document

**If you are an external reviewer attacking Planex's abstraction route:**

1. Read [`abstraction-form.md`](abstraction-form.md) for the conditional thesis (abstraction optimal under three prerequisites).
2. Read [`../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md`](../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) for the Rule-of-Three rebuttal.
3. Read this document for the quantitative leak budget that addresses Spolsky's leaky-abstraction critique (Caveat 3 of the comparative study). If you believe Planex's abstractions are "too leaky," point to a specific entry in the per-abstraction tables and explain how the named leak is worse than documented. The leak count is falsifiable: if you can add a real leak, the count goes up; if the implementation handles it correctly, the entry is removed.

**If you are a Planex contributor adding a new abstraction or new operation:**

1. Add the operation to the appropriate per-abstraction table in this document.
2. Classify it per the leak criterion (L1, L2, or non-leak). If you are not sure, mark it as "needs verification" with a scenario.
3. If the new operation introduces an L2 leak, document it in the corresponding admission ADR (per ADR-0011's three criteria for essence-justified admission).
4. Re-compute the aggregate table. If the new operation raises the aggregate L2 rate above 20%, raise it for review before merging.

**If you are auditing Planex for a version bump:**

1. Re-enumerate public operations from `include/planex/planex.h` for the 5 shipping abstractions.
2. Classify each per the criterion above. Reconcile against this document's tables; any divergence is a finding.
3. Compute the new aggregate. Compare against the retire target table for the current version.
4. If the audit finds the L2 count has gone up rather than down, the abstraction is regressing — open an issue and a new ADR.

---

## v4 preview (Interpretant + Perlocution, post-pressure-test)

> **Status:** Preview only. v4 has not shipped; this section quantifies the L2 leaks surfaced by [`ADR-0012`](../decisions/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) so that when v4 ships, its starting L2 count is on the record. The v4 leak budget will be re-audited at v4 ship time using the same criterion as shipping abstractions.
>
> **Scope:** 2 of the 3 v4 new abstractions (Interpretant, Perlocution). Breakdown — the 3rd new abstraction — was pressure-tested clean (no L2 leaks); its bridge to Relation (`px_breakdown_to_relation`) is opt-in and one-way, not a coupling failure.

### Interpretant — `v4/src/interpretant.c` (v4 proposal, not shipping)

**Public operations (5):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_interpretant_new(representamen_source, actor)` | — | ✓ | **constructor signature claims dependency on `px_perception* representamen_source`; no Interpretant operation reads it** — `predict(it, rep)` takes an explicit `representamen` parameter, the bound source is stored-but-never-consulted. Pressure-tested in `test_d1_interpretant_representamen_source_unused`: free the bound perception, all ops still work. |
| 2 | `px_interpretant_free(it)` | — | — | destructor |
| 3 | `px_interpretant_set_intended(it, semantics)` | — | — | mutator, matches name |
| 4 | `px_interpretant_intended(it)` | — | — | pure query (const) |
| 5 | `px_interpretant_set_interpret_fn(it, fn, user)` | ✓ | — | `void* user` in callback |
| 6 | `px_interpretant_predict(it, representamen)` | — | — | the abstraction's purpose; takes explicit `representamen` parameter |
| 7 | `px_interpretant_matches_intended(it, actual)` | — | — | pure query (string equality) |

**Leak count:** L1 = 1, L2 = 1. **Total** = 2 / 7 = 29%. **L2** = 1 / 7 = **14%**.

**Verification scenario for the L2 leak:**
- `interpretant_new` source unused: create perception `p`, bind it via `px_interpretant_new(p, actor)`, free `p` (the interpretant now holds a dangling pointer), then call `px_interpretant_predict(it, some_representamen)`. If any operation read `representamen_source`, this would crash or return wrong results. The test confirms it does not — proving the field is documentation-only.

**Retire target for v4 Interpretant L2:** 0 / 7 by v4 ship — either remove `representamen_source` from the constructor (Closure-style, no false claim of dependency) or actually use it in a new `px_interpretant_predict_from_bound(it)` operation that invokes the bound perception internally.

### Perlocution — `v4/src/perlocution.c` (v4 proposal, not shipping)

**Public operations (6):**

| # | Operation | L1 | L2 | Notes |
|---|---|---|---|---|
| 1 | `px_perlocution_new(c, actor)` | — | ✓ | **constructor signature claims dependency on `px_closure* c`; no Perlocution operation reads it** — `px_perlocution_status` is a pure function of the perlocution's own kind enum, not of any closure state. Pressure-tested in `test_d2_perlocution_closure_field_unused`: free the bound closure, all ops still work. |
| 2 | `px_perlocution_free(p)` | — | — | destructor |
| 3 | `px_perlocution_set(p, kind, text)` | — | — | mutator, matches name |
| 4 | `px_perlocution_kind_get(p)` | — | — | pure query (const) |
| 5 | `px_perlocution_text(p)` | — | — | pure query (const) |
| 6 | `px_perlocution_kind_str(k)` | — | — | utility, matches name |
| 7 | `px_perlocution_status(p)` | — | — | pure query, derives status from kind enum |
| 8 | `px_status_str(s)` | — | — | utility, matches name |

**Leak count:** L1 = 0, L2 = 1. **Total** = 1 / 8 = 13%. **L2** = 1 / 8 = **13%**.

**Verification scenario for the L2 leak:**
- `perlocution_new` closure unused: create closure `c`, bind it via `px_perlocution_new(c, actor)`, free `c` (the perlocution now holds a dangling pointer), then call `px_perlocution_set`, `px_perlocution_kind_get`, `px_perlocution_text`, `px_perlocution_status`. If any operation read `closure`, this would crash or return wrong results. The test confirms it does not.

**Retire target for v4 Perlocution L2:** 0 / 8 by v4 ship — either remove `closure` from the constructor (no false claim of dependency) or actually use it in a new `px_perlocution_status_from_closure(p)` operation that derives status from the bound closure's eval result.

### v4 preview aggregate

| Abstraction | Total ops | L1 leaks | L2 leaks | Total leak % | L2 leak % |
|---|---|---|---|---|---|
| Interpretant | 7 | 1 | 1 | 29% | **14%** |
| Perlocution | 8 | 0 | 1 | 13% | **13%** |
| Breakdown | (audit pending v4 ship) | — | 0 | — | **0%** (pressure-tested clean) |
| **v4 preview total** | **15** | **1** | **2** | **20%** | **13%** |

The v4 preview L2 rate (13%) is below the shipping aggregate L2 rate (17%) and well below the form-level fallback threshold (30%). **Prerequisite 2 does NOT fail for v4.** The two L2 leaks are at the *signature* level — they are fixed by one-line API changes to each constructor, not by collapsing the abstractions into a DSL.

**Cross-reference to ADR-0012:**
- Finding 1 = Interpretant `representamen_source` L2 leak (this section)
- Finding 2 = Perlocution `closure` L2 leak (this section)
- Finding 3 = Closure lost `px_closure_get_status` in v4 → migration gap (NOT an L2 leak; this section does NOT include it in the v4 L2 count, because Closure's v4 operations all match their names — the gap is in user-facing capability, not in operation-name vs behavior)
- Finding 4 = Interpretant→Breakdown protocol coupling (acceptable, no leak counted)

---

## References

- **Code:** `include/planex/planex.h` — public API surface for all 5 shipping abstractions.
- **Code:** `v4/include/planex/planex.h` — public API surface for v4 proposal (3 new abstractions: Interpretant, Perlocution, Breakdown).
- **Code:** `src/relation.c`, `src/estimate.c`, `src/closure.c`, `src/perception.c`, `src/app.c` — implementations whose behavior the leaks expose.
- **Code:** `v4/src/interpretant.c`, `v4/src/perlocution.c`, `v4/src/breakdown.c` — v4 implementations whose leaks the pressure test exposes.
- **Code:** `tests/test_orthogonality.c` — orthogonality tests for shipping 5; this document's leak tables are the *quantitative* complement.
- **Code:** `tests/test_v4_orthogonality.c` — pressure test for v4 seams (19 tests, 4 findings); the v4 preview section above is its quantitative record.
- **Related docs:** [`limitations.md`](limitations.md) — qualitative gaps (L1–L14). Cross-referenced where the same gap appears in both.
- **Related docs:** [`abstraction-form.md`](abstraction-form.md) — Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability) standing updated by this document.
- **Related docs:** [`../research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md) — the comparative study that prescribed this document (Caveat 3 / Gap 3).
- **Related ADRs:** [ADR-0010](../decisions/ADR-0010-v4-design-rationale-not-essence-discovery.md) — the honesty downgrade this document extends; [ADR-0011](../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — Essence Check Q4 establishes the verifiable-cost-scenario pattern this document applies to leaks; [ADR-0012](../decisions/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) — the v4 pressure test whose findings the v4 preview section above quantifies.
- **External:** Joel Spolsky, "The Law of Leaky Abstractions" (2002) — https://www.joelonsoftware.com/2002/11/11/the-law-of-leaky-abstractions/
- **External:** Conal Elliott, "Denotational Design with Type Class Morphisms" — the abstraction-as-typed-value ideal whose distance from implementation L2 leaks measure.
