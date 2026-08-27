# Why Planex's Abstractions

> **Status:** Canonical manifesto, revised for v0.4.
>
> **Applies to**: v0.4. The "4 abstractions" tagline was set in v0.2; the manifesto was rewritten in v0.3 to reflect v2 essence-derivation revisions; v0.4 added `px_loop` as 5th abstraction (Feedback essence category, ADR-0008). Future revisions pending ADR-0009 (Proposed) and ADR-0010 (Accepted, framing downgrade).
>
> Per [essence-derivation-v2.md](essence-derivation-v2.md), [ADR-0007](../decisions/ADR-0007-essence-derivation-v2-revision.md), and [ADR-0008](../decisions/ADR-0008-feedback-as-fifth-essence-category.md): Planex implements **5 of 5 essence categories** (Feedback was added in v0.4 as `px_loop`), with 4 additional essence candidates (Embodiment, Situatedness, Affordance-as-relation, Breakdown) deferred as acknowledged.
>
> This document replaces the earlier "4 abstractions = 4 essence axes" framing. v0.4 closes the gap acknowledged in v0.3's essence-derivation v2.

---

## The thesis, revised

Planex is built on **abstractions chosen by essence, not by inspiration**. v0.4 implements all 5 essence categories identified by the 6-tradition literature survey. Several more essence categories (from philosophy) are deferred as acknowledged.

```
┌──────────────────────────────────────────────────────────────────┐
│  UI essence (per 6-tradition literature survey)                   │
├──────────────────────────────────────────────────────────────────┤
│  1. State                          → Estimate          ✅ impl   │
│  2. Communication (human→machine)  → Closure           ✅ impl   │
│  3. Presentation (machine→human)    → Perception        ✅ impl   │
│  4. Relational ontology             → Relation          ✅ impl   │
│  5. Feedback / closed-loop coupling → px_loop          ✅ v0.4   │
│                                                                   │
│  Deferred essence candidates (acknowledged, not implemented):    │
│  - Embodiment (Dourish)                                           │
│  - Situatedness (Suchman)                                         │
│  - Affordance-as-relation (Gibson)                                │
│  - Breakdown (Heidegger-Winograd/Flores)                          │
└──────────────────────────────────────────────────────────────────┘
```

This is the **strongest possible essence-driven claim**: Planex implements every essence category that the literature survey surfaced within Layers 1-3 of UI essence. Deferred categories are acknowledged, not silently dropped.

---

## The 5 implemented essence categories

### 1. Estimate — State

State with time and uncertainty. Not a discrete snapshot.

**Why not "state" (like React's `useState`)?**

Mainstream UI's `useState(42)` is a discrete snapshot — a value at a single moment. But real state has two extra dimensions:

- **Time**: animation is state that varies continuously. `useState(42)` cannot express "this value is moving from 0 to 100 over 500ms" without bolt-on `useEffect + setTimeout`.
- **Uncertainty**: sensor readings, network responses, predictions — all carry confidence. `useState(42)` cannot express "this value is 42 with 80% confidence".

Estimate subsumes both. Formally, it's inspired by:
- Conal Elliott's FRP — `Behavior = Time → α`
- Karl Friston's predictive coding — state is a prediction with confidence

**Status**: ✅ Implemented. The `Estimate` API (`px_estimate_new`, `px_estimate_set`, `px_estimate_animate`, `px_estimate_confidence`) covers time + uncertainty.

### 2. Closure — Communication (human→machine intent)

A 5-stage interaction unit, covering Don Norman's stages 1-4 + 7 (execution side).

**Why not "event" (like React's `onClick`)?**

Mainstream UI's `onClick={fn}` buries intent inside a function pointer. You cannot:
- Serialize it (for undo/redo, replay)
- Audit it (which intent was triggered when?)
- Evaluate it (did the action succeed?)
- Drive it externally (serialized intent stream)

Closure makes intent a **typed value** (one of `ASSERT` / `REQUEST` / `PROMISE` / `DECLARE` / `EXPRESS` — Winograd/Flores speech acts), not a callback. The 5 stages:

```
1. Goal          (human-readable description)
2. Intent        (typed value — speech act)
3. Action        (function that mutates Estimates)
4. Execution     (runtime invokes action)
5. Evaluation    (function that checks if goal achieved)
```

**Per ADR-0005**: Closure was 7 stages (Norman's complete model). Now it's 5 stages — stages 5 (Perception) and 6 (Interpretation) moved to the new Perception abstraction.

**Status**: ✅ Implemented. The `Closure` API (`px_closure_new`, `px_closure_trigger`, `px_closure_evaluated`, `px_closure_replay`, etc.) covers all 5 stages + replay.

### 3. Perception — Presentation (machine→human denotation)

A pure function denoting state. Multiple perceptions can coexist for the same Estimates.

**Why not "render" (like React's render function)?**

Mainstream UI has **one render path**. To produce multiple denotations (screen pixels + a11y tree + log + test snapshot), you'd have to special-case each inside a single render function, or maintain parallel trees (DOM + a11y tree).

Planex's Perception makes multiple denotations first-class:

```c
/* Same Estimates, four different Perception functions: */
px_perception* visual = px_perception_new("visual", render_pixels, ...);
px_perception* a11y    = px_perception_new("a11y",    render_a11y,    ...);
px_perception* json    = px_perception_new("json",    render_json,    ...);
px_perception* log     = px_perception_new("log",     render_log,     ...);
```

Each is a pure function. They are independent — removing one doesn't affect others.

This is inspired by:
- Conal Elliott's denotative design — pure-function denotation
- Don Norman's 7-stage model — stages 5-6 (evaluation side)

**Status**: ✅ Implemented (Phase 2). The `Perception` API (`px_perception_new`, `px_perception_free`, `px_perception_invoke_all`, `px_perception_invoke_for_estimate`, `px_perceptions_for_estimate`) covers registration, query, and invocation.

### 4. Relation — Relational ontology (UI defined by its relations)

A queryable graph, not a tree.

**v2 correction**: Earlier, Relation was framed as "structural" under a "UI is a network" premise. v2 found this was wrong — relational ontology is essence, not structural. UI cannot be defined without actor + situation (per Heidegger, Gibson, Dourish, Hutchins, Alexander). Planex's `Relation` is one engineering instantiation of this essence category; others (constraint systems, process networks, semilattices) are valid but not chosen.

**Why not a component tree (like React's element tree)?**

A tree can only express containment (`A contains B`). But UI has many other relationships:

- **Dependency** — A's value depends on B (`all_valid` depends on every field's validity)
- **Causality** — clicking A triggers B (button triggers counter change)
- **Spatial** — A is beside B (layout)
- **Affordance** — A affords action B (button affords clicking)

Trees force all of these into parent-child, losing semantic precision. Relations make each kind first-class.

This is inspired by:
- Sketchpad (Sutherland 1963) — constraint graph
- Christopher Alexander's "A City is Not a Tree" (1965) — semilattice vs tree
- Hutchins's Distributed Cognition — UI as relational system
- π-calculus / CSP — channels (relations) as primitive

**Status**: ✅ Implemented. The `Relation` API (`px_declare`, `px_query`, `PX_REL_TRIGGERS`, `PX_REL_DEPENDS_ON`, `PX_REL_BESIDE`, etc.) covers all four relation kinds. Used for undo-via-graph (`px_closure_bind_graph`).

### 5. px_loop — Feedback / closed-loop coupling (NEW in v0.4)

A first-class closed loop binding Closure (intent side) to Perception (view side).

**Why not "the render loop" (like every framework's main loop)?**

Mainstream UI frameworks have an implicit render loop: `event → state change → render → next event`. The loop exists structurally but is not first-class — you cannot audit it, interrupt it, replay it, or detect when it stalls.

Planex v0.4's `px_loop` makes the loop first-class:

```c
px_loop* loop = px_loop_new(closure, perception);

/* Run one iteration: trigger closure, then invoke perception. */
px_loop_step(loop, &payload, sizeof(payload));

/* Pause for batch updates — no re-render between triggers. */
px_loop_pause(loop);
for (int i = 0; i < 100; i++) px_closure_trigger(closure, ...);
px_loop_resume(loop);
px_loop_step_view_only(loop);  /* single render after 100 triggers */

/* Audit: which perception fired after which trigger? */
px_loop_audit_entry entries[10];
int n = px_loop_audit_get(loop, entries, 10);

/* Replay last 5 iterations for testing/debugging. */
px_loop_replay(loop, 5);
```

**Why this is essence, not feature**:
- **Heidegger**: breakdown is the moment feedback fails — essence revealed by its absence
- **Suchman**: feedback is what makes action situated
- **Math (CSP/statechart)**: loop/transition is primitive
- **Norman**: "to be in charge, the user must be informed" (feedback closes the gulf of evaluation)

Without Feedback as first-class, Planex could not:
- audit which perception fired after which trigger
- interrupt the loop (batch updates, modal blocking)
- replay trigger→perception sequences (testing, debugging)
- detect breakdown (perception failed to fire, loop stalled)

**Status**: ✅ Implemented in v0.4 (per ADR-0008). The `px_loop` API (`px_loop_new`, `px_loop_step`, `px_loop_step_view_only`, `px_loop_pause/resume`, `px_loop_audit_*`, `px_loop_replay`) covers lifecycle, audit, interruption, and replay. 13 tests in `tests/test_feedback.c` validate the API.

**Limitation**: `px_loop` currently uses `px_perception_invoke_all()` (invokes all registered perceptions, not just the loop's). Future v0.5+ should add `px_perception_invoke(p)` to scope this.

---

## The deferred essence candidates

These are essence categories (per the philosophy and HCI traditions surveyed in v2), not engineering conveniences. Planex **acknowledges them as essence** but explicitly defers implementation.

### D1. Embodiment (Dourish)

UI's essence includes the **embodiment relation** — meaning emerges through engaged interaction, not pre-encoded by designers. UI is not an independent artefact but an extension of embodied action.

- **Why deferred**: requires giving up "UI as independent artefact" premise, which is foundational to Planex's current API (Estimates, Closures, Perceptions all exist independently of actor)
- **Acknowledged in**: [limitations.md](limitations.md) L14

### D2. Situatedness (Suchman)

Action is situated — plans are post-hoc rationalizations, not action generators. UI cannot preset user flows.

- **Why deferred**: contradicts use-case-driven design, which Planex's Closure (typed intent) implicitly assumes
- **Acknowledged in**: [limitations.md](limitations.md) L14

### D3. Affordance-as-relation (Gibson, original)

Affordance is a relation between world and actor, not a property. UI's essence includes "what actions the actor can take" — which requires actor presence in the API.

- **Why deferred**: Planex's Relation is between things (Estimates, Closures); adding actor-to-thing relations would be a major API revision
- **Acknowledged in**: [limitations.md](limitations.md) L14

### D4. Breakdown (Heidegger-Winograd/Flores)

UI's essence includes the moment of breakdown — when the tool becomes present-at-hand. This is when the user notices the UI itself (an error, a confusion, a surprise).

- **Why deferred**: requires a notion of "normal flow" vs "interrupted flow" that Planex's current abstractions don't express
- **Acknowledged in**: [limitations.md](limitations.md) L14

### Why these are deferred, not dismissed

Per essence-driven principle: **a deferred essence candidate must remain acknowledged**. If Planex silently drops them, it commits the same over-claim that v2 corrected. Future revisions should pick these up when use cases demand, not when completeness anxiety demands.

---

## What this replaces

Before v2, Planex claimed "4 abstractions = 4 essence axes". This was an over-claim that:
- Treated Planex's `Relation` (graph data structure) as if it were the only essence instantiation of relational ontology
- Did not acknowledge Feedback as a separate essence category (it was implicit in Closure+Perception)
- Did not acknowledge the deferred essence candidates at all

v2 (essence-derivation-v2.md) is the audit that surfaced these issues. ADR-0007 records the revision. ADR-0008 closes the Feedback gap by adding `px_loop` in v0.4.

The history:
1. v0.1.0 (2026-08-22): Planex released with "3 abstractions" claim. Perception was a no-op in Closure stage 5. ADR-0001 recorded the gap.
2. v0.2 (2026-08-25): ADR-0005 promoted Perception to 4th abstraction. Closure restructured 7→5 stages.
3. v0.3 (2026-08-26): 4 abstractions fully implemented. Essence derivation v1 attempted first-principles derivation; concluded "3 essence + 1 structural".
4. v0.3.1 (2026-08-27): Essence derivation v2 audited v1 against a 6-tradition literature survey. Found v1 was wrong about Relation (it is essence, not structural) and missed Feedback as a separate essence category. v2 also surfaced 4 deferred essence candidates from philosophy. ADR-0007 records the revision.
5. **v0.4 (2026-08-27)**: ADR-0008 closes the Feedback gap. `px_loop` becomes the 5th essence instantiation. Planex now implements 5 of 5 essence categories (within Layers 1-3 scope).

The current claim: **"Planex implements 5 of 5 essence categories within Layers 1-3. 4 additional essence categories (Embodiment, Situatedness, Affordance-as-relation, Breakdown) are acknowledged as essence but deferred."**

---

## What this is NOT

Planex is **not**:

- A general-purpose React replacement. Planex targets research-grade and embedded UI, not the broad market React serves.
- A "done" UI library. Pre-v1.0, API may break between minor versions.
- An AI-native UI runtime. Planex does not target AI-driven UI.
- A complete UI essence implementation. Per [ui-essence-layers.md](ui-essence-layers.md), Planex implements Layers 1-3 (physical + cognitive + semantic). Layers 4-6 (behavioral + evolutionary + medium) are out of scope. **Within Layers 1-3**, Planex now implements 5 of 5 essence categories (post-v0.4). The 4 deferred essence candidates (Embodiment, Situatedness, Affordance-as-relation, Breakdown) span Layers 2-4 and are not yet implemented.

Planex **is**:

- An experiment in whether Path C (constraint-graph + time-function + speech-act UI + closed-loop coupling) can be made practical, building on 60 years of prior attempts. See [path-C-lineage.md](path-C-lineage.md).
- An honest essence-driven project: the claim matches the implementation, including its gaps and deferrals.

---

## See also

- [essence-derivation-v2.md](essence-derivation-v2.md) — the audit that produced the v0.4 framing
- [ADR-0007](../decisions/ADR-0007-essence-derivation-v2-revision.md) — records the v2 revision
- [ADR-0008](../decisions/ADR-0008-feedback-as-fifth-essence-category.md) — records the Feedback design (v0.4)
- [ADR-0005](../decisions/ADR-0005-promote-perception-to-fourth-abstraction.md) — the decision that promoted Perception
- [ADR-0001](../decisions/ADR-0001-perception-currently-noop.md) — historical record of the original gap (superseded by ADR-0005)
- [UI Essence Layers](ui-essence-layers.md) — the 6-layer essence model; Planex implements layers 1-3
- [Alternative Perspectives](alternative-perspectives.md) — four academic schools; Planex adopts Cognitive + Mathematical/Linguistic
- [Path C Lineage](path-C-lineage.md) — 60-year history of constraint-graph UI attempts
- [Limitations](limitations.md) — including the deferred essence candidates
- [Roadmap Matrix](roadmap-matrix.md) — maturity tracking per abstraction
- [examples/counter_4abs.c](../../examples/counter_4abs.c) — canonical 4-abstraction hello world
- [examples/integration_4abs.c](../../examples/integration_4abs.c) — all 4 abstractions + all features in one demo
- [tests/test_feedback.c](../../tests/test_feedback.c) — 13 tests for the v0.4 Feedback API

---

## Bibliography

The 6-tradition survey that grounds this revision is archived in `research/reports/` (workspace, not in repo). Primary sources cited in v2:

- Alexander, Christopher. *A City is Not a Tree* (1965). https://www.patternlanguage.com/archive/cityisnotatree.html
- Conal Elliott. *Push-Pull Functional Reactive Programming* (2009). http://conal.net/papers/push-pull-frp
- Conal Elliott. *Denotational Design with Type Class Morphisms* (2012). http://conal.net/papers/type-class-morphisms
- Dourish, Paul. *Where the Action Is* (2001). https://www.dourish.com/embodied/
- Friston, Karl. "The free-energy principle: a unified brain theory?" *Nature Reviews Neuroscience* (2010). https://www.nature.com/articles/nrn2787
- Gibson, James J. *The Ecological Approach to Visual Perception* (1979). (affordance theory)
- Harel, David. *Statecharts: A Visual Formalism for Complex Systems* (1987).
- Hutchins, Edwin. *Cognition in the Wild* (1995). https://pages.ucsd.edu/~ehutchins/citw.html
- Norman, Don. *The Design of Everyday Things* (1988).
- Sutherland, Ivan. *Sketchpad* (1963). https://dspace.mit.edu/entities/publication/b5e8025c-c8b2-4843-84e0-76db824e07e6
- Suchman, Lucy. *Plans and Situated Actions* (1987).
- Winograd, Terry; Flores, Fernando. *Understanding Computers and Cognition* (1986).

---

## The thesis, revised

Planex is built on **abstractions chosen by essence, not by inspiration**. The current implementation covers 4 essence categories; one more is partially present; several are deferred.

The earlier framing ("4 abstractions = 4 essence axes") was **too strong**. It conflated essence categories with engineering instantiations, and it omitted categories the literature survey surfaced.

The honest framing, post [v2](essence-derivation-v2.md):

```
┌──────────────────────────────────────────────────────────────────┐
│  UI essence (per 6-tradition literature survey)                   │
├──────────────────────────────────────────────────────────────────┤
│  1. State                          → Estimate          ✅ impl   │
│  2. Communication (human→machine)  → Closure           ✅ impl   │
│  3. Presentation (machine→human)    → Perception        ✅ impl   │
│  4. Relational ontology             → Relation          ✅ impl   │
│  5. Feedback / closed-loop coupling → (partial)         ⚠️ gap    │
│                                                                   │
│  Deferred essence candidates (acknowledged, not implemented):    │
│  - Embodiment (Dourish)                                           │
│  - Situatedness (Suchman)                                         │
│  - Affordance-as-relation (Gibson)                                │
│  - Breakdown (Heidegger-Winograd/Flores)                          │
└──────────────────────────────────────────────────────────────────┘
```

This is **stronger and more honest** than the earlier "4 = 4" claim. It says what Planex has, what it partially has, and what it defers — all on the same essence footing.

---

## Why this matters

An essence-driven project's strongest commitment is: **the project's claim matches the project's implementation**.

If Planex claims "4 essence axes = 4 abstractions" but actually:
- misses Feedback as a first-class category (it's implicit in Closure+Perception), AND
- defers Embodiment/Situatedness/etc. without acknowledging them as essence,

then the claim is over-stated. This is what v2 found: a structural over-claim.

The fix is not to add 5 new abstractions. The fix is to **make the claim honest**, then evaluate which gaps to close.

---

## The 4 implemented essence categories

### 1. Estimate — State

State with time and uncertainty. Not a discrete snapshot.

**Why not "state" (like React's `useState`)?**

Mainstream UI's `useState(42)` is a discrete snapshot — a value at a single moment. But real state has two extra dimensions:

- **Time**: animation is state that varies continuously. `useState(42)` cannot express "this value is moving from 0 to 100 over 500ms" without bolt-on `useEffect + setTimeout`.
- **Uncertainty**: sensor readings, network responses, predictions — all carry confidence. `useState(42)` cannot express "this value is 42 with 80% confidence".

Estimate subsumes both. Formally, it's inspired by:
- Conal Elliott's FRP — `Behavior = Time → α`
- Karl Friston's predictive coding — state is a prediction with confidence

**Status**: ✅ Implemented. The `Estimate` API (`px_estimate_new`, `px_estimate_set`, `px_estimate_animate`, `px_estimate_confidence`) covers time + uncertainty.

### 2. Closure — Communication (human→machine intent)

A 5-stage interaction unit, covering Don Norman's stages 1-4 + 7 (execution side).

**Why not "event" (like React's `onClick`)?**

Mainstream UI's `onClick={fn}` buries intent inside a function pointer. You cannot:
- Serialize it (for undo/redo, replay)
- Audit it (which intent was triggered when?)
- Evaluate it (did the action succeed?)
- Drive it externally (serialized intent stream)

Closure makes intent a **typed value** (one of `ASSERT` / `REQUEST` / `PROMISE` / `DECLARE` / `EXPRESS` — Winograd/Flores speech acts), not a callback. The 5 stages:

```
1. Goal          (human-readable description)
2. Intent        (typed value — speech act)
3. Action        (function that mutates Estimates)
4. Execution     (runtime invokes action)
5. Evaluation    (function that checks if goal achieved)
```

**Per ADR-0005**: Closure was 7 stages (Norman's complete model). Now it's 5 stages — stages 5 (Perception) and 6 (Interpretation) moved to the new Perception abstraction.

**Status**: ✅ Implemented. The `Closure` API (`px_closure_new`, `px_closure_trigger`, `px_closure_evaluated`, `px_closure_replay`, etc.) covers all 5 stages + replay.

### 3. Perception — Presentation (machine→human denotation)

A pure function denoting state. Multiple perceptions can coexist for the same Estimates.

**Why not "render" (like React's render function)?**

Mainstream UI has **one render path**. To produce multiple denotations (screen pixels + a11y tree + log + test snapshot), you'd have to special-case each inside a single render function, or maintain parallel trees (DOM + a11y tree).

Planex's Perception makes multiple denotations first-class:

```c
/* Same Estimates, four different Perception functions: */
px_perception* visual = px_perception_new("visual", render_pixels, ...);
px_perception* a11y    = px_perception_new("a11y",    render_a11y,    ...);
px_perception* json    = px_perception_new("json",    render_json,    ...);
px_perception* log     = px_perception_new("log",     render_log,     ...);
```

Each is a pure function. They are independent — removing one doesn't affect others.

This is inspired by:
- Conal Elliott's denotative design — pure-function denotation
- Don Norman's 7-stage model — stages 5-6 (evaluation side)

**Status**: ✅ Implemented (Phase 2). The `Perception` API (`px_perception_new`, `px_perception_free`, `px_perception_invoke_all`, `px_perception_invoke_for_estimate`, `px_perceptions_for_estimate`) covers registration, query, and invocation.

### 4. Relation — Relational ontology (UI defined by its relations)

A queryable graph, not a tree.

**v2 correction**: Earlier, Relation was framed as "structural" under a "UI is a network" premise. v2 found this was wrong — relational ontology is essence, not structural. UI cannot be defined without actor + situation (per Heidegger, Gibson, Dourish, Hutchins, Alexander). Planex's `Relation` is one engineering instantiation of this essence category; others (constraint systems, process networks, semilattices) are valid but not chosen.

**Why not a component tree (like React's element tree)?**

A tree can only express containment (`A contains B`). But UI has many other relationships:

- **Dependency** — A's value depends on B (`all_valid` depends on every field's validity)
- **Causality** — clicking A triggers B (button triggers counter change)
- **Spatial** — A is beside B (layout)
- **Affordance** — A affords action B (button affords clicking)

Trees force all of these into parent-child, losing semantic precision. Relations make each kind first-class.

This is inspired by:
- Sketchpad (Sutherland 1963) — constraint graph
- Christopher Alexander's "A City is Not a Tree" (1965) — semilattice vs tree
- Hutchins's Distributed Cognition — UI as relational system
- π-calculus / CSP — channels (relations) as primitive

**Status**: ✅ Implemented. The `Relation` API (`px_declare`, `px_query`, `PX_REL_TRIGGERS`, `PX_REL_DEPENDS_ON`, `PX_REL_BESIDE`, etc.) covers all four relation kinds. Used for undo-via-graph (`px_closure_bind_graph`).

---

## The 1 partial essence category

### 5. Feedback / Closed-loop coupling — ⚠️ partial

**What it is**: The closed loop of (intent → action → state change → perception → next intent). Without this loop, UI is one-shot, not interactive.

**Cross-tradition convergence** (per v2):
- History: every era independently rediscovered feedback (Sketchpad's rubber-band line, Engelbart's real-time display, Apple HIG's "to be in charge, user must be informed")
- HCI: Norman's gulf of evaluation, KLM's system response R, Suchman's situatedness (feedback as the situating medium)
- Modern architecture: every framework requires it (state→render loop), but none make it first-class
- Phenomenology: breakdown (Heidegger-Winograd/Flores) is the moment feedback fails — essence revealed by its absence
- Math: CSP's trace, statechart's transition, FRP's causality — feedback primitive in formal models

**Current status in Planex**: ⚠️ **partial — implicit, not first-class**.

The loop exists structurally:
1. `px_closure_trigger(c, ...)` — user intent → action
2. Action mutates `px_estimate`
3. Estimate observers fire, derived estimates recompute
4. Application manually invokes `px_perception_invoke_for_estimate(e)` or `px_perception_invoke_all()`
5. User sees new state, forms next intent

But the loop itself is **not a first-class concept**. Planex cannot:
- Audit the loop (which perception fired after which trigger?)
- Interrupt the loop (pause perception until next intent — useful for batch updates)
- Replay the loop (re-run trigger→perception sequences for testing)
- Detect breakdown (perception failed to fire, intent loop stalled)

**Why this is a gap, not a deferment**: Planex claims to implement interactive UI essence. Interactive means feedback-coupled. If Feedback isn't first-class, the essence claim is partial.

**What needs to happen**: A `px_loop` (or similar) type that makes the closed loop explicit, observable, interruptible, replayable. Design TBD — see [essence-derivation-v2.md](essence-derivation-v2.md) Step 2. **Not yet implemented.**

---

## The deferred essence candidates

These are essence categories (per the philosophy and HCI traditions surveyed in v2), not engineering conveniences. Planex **acknowledges them as essence** but explicitly defers implementation.

### D1. Embodiment (Dourish)

UI's essence includes the **embodiment relation** — meaning emerges through engaged interaction, not pre-encoded by designers. UI is not an independent artefact but an extension of embodied action.

- **Why deferred**: requires giving up "UI as independent artefact" premise, which is foundational to Planex's current API (Estimates, Closures, Perceptions all exist independently of actor)
- **Acknowledged in**: [limitations.md](limitations.md)

### D2. Situatedness (Suchman)

Action is situated — plans are post-hoc rationalizations, not action generators. UI cannot preset user flows.

- **Why deferred**: contradicts use-case-driven design, which Planex's Closure (typed intent) implicitly assumes
- **Acknowledged in**: [limitations.md](limitations.md)

### D3. Affordance-as-relation (Gibson, original)

Affordance is a relation between world and actor, not a property. UI's essence includes "what actions the actor can take" — which requires actor presence in the API.

- **Why deferred**: Planex's Relation is between things (Estimates, Closures); adding actor-to-thing relations would be a major API revision
- **Acknowledged in**: [limitations.md](limitations.md)

### D4. Breakdown (Heidegger-Winograd/Flores)

UI's essence includes the moment of breakdown — when the tool becomes present-at-hand. This is when the user notices the UI itself (an error, a confusion, a surprise).

- **Why deferred**: requires a notion of "normal flow" vs "interrupted flow" that Planex's current abstractions don't express
- **Acknowledged in**: [limitations.md](limitations.md)

### Why these are deferred, not dismissed

Per essence-driven principle: **a deferred essence candidate must remain acknowledged**. If Planex silently drops them, it commits the same over-claim that v2 corrected. Future revisions should pick these up when use cases demand, not when completeness anxiety demands.

---

## What this replaces

Before v2, Planex claimed "4 abstractions = 4 essence axes". This was an over-claim that:
- Treated Planex's `Relation` (graph data structure) as if it were the only essence instantiation of relational ontology
- Did not acknowledge Feedback as a separate essence category (it was implicit in Closure+Perception)
- Did not acknowledge the deferred essence candidates at all

v2 (essence-derivation-v2.md) is the audit that surfaced these issues. This document is the canonical revision.

The history:
1. v0.1.0 (2026-08-22): Planex released with "3 abstractions" claim. Perception was a no-op in Closure stage 5. ADR-0001 recorded the gap.
2. v0.2 (2026-08-25): ADR-0005 promoted Perception to 4th abstraction. Closure restructured 7→5 stages.
3. v0.3 (2026-08-26): 4 abstractions fully implemented. Essence derivation v1 (`essence-derivation-v1.md`) attempted first-principles derivation; concluded "3 essence + 1 structural".
4. v0.3.1 (2026-08-27): Essence derivation v2 (`essence-derivation-v2.md`) audited v1 against a 6-tradition literature survey. Found v1 was wrong about Relation (it is essence, not structural) and missed Feedback as a separate essence category. v2 also surfaced 4 deferred essence candidates from philosophy.

The current claim is no longer "4 = 4". It is: **"4 essence categories implemented, 1 partial (Feedback), 4 deferred (Embodiment, Situatedness, Affordance-as-relation, Breakdown)."**

---

## What this is NOT

Planex is **not**:

- A general-purpose React replacement. Planex targets research-grade and embedded UI, not the broad market React serves.
- A "done" UI library. Pre-v1.0, API may break between minor versions.
- An AI-native UI runtime. Planex does not target AI-driven UI.
- A complete UI essence implementation. Per [ui-essence-layers.md](ui-essence-layers.md), Planex implements Layers 1-3 (physical + cognitive + semantic). Layers 4-6 (behavioral + evolutionary + medium) are out of scope. **Additionally**, even within layers 1-3, Planex implements 4 of 5 essence categories — Feedback is partial, and 4 philosophy-derived essence categories are deferred.

Planex **is**:

- An experiment in whether Path C (constraint-graph + time-function + speech-act UI) can be made practical, building on 60 years of prior attempts. See [path-C-lineage.md](path-C-lineage.md).
- An honest essence-driven project: the claim matches the implementation, including its gaps and deferrals.

---

## See also

- [essence-derivation-v2.md](essence-derivation-v2.md) — the audit that produced this revision
- [essence-derivation-v1.md](essence-derivation-v1.md) — v1 derivation (superseded by v2; kept for history)
- [ADR-0005](../decisions/ADR-0005-promote-perception-to-fourth-abstraction.md) — the decision that promoted Perception
- [ADR-0007](../decisions/ADR-0007-essence-derivation-v2-revision.md) — records the v2 revision
- [ADR-0001](../decisions/ADR-0001-perception-currently-noop.md) — historical record of the original gap (superseded by ADR-0005)
- [UI Essence Layers](ui-essence-layers.md) — the 6-layer essence model; Planex implements layers 1-3
- [Alternative Perspectives](alternative-perspectives.md) — four academic schools; Planex adopts Cognitive + Mathematical/Linguistic
- [Path C Lineage](path-C-lineage.md) — 60-year history of constraint-graph UI attempts
- [Limitations](limitations.md) — including the deferred essence candidates
- [Roadmap Matrix](roadmap-matrix.md) — maturity tracking per abstraction
- [examples/counter_4abs.c](../../examples/counter_4abs.c) — canonical 4-abstraction hello world
- [examples/integration_4abs.c](../../examples/integration_4abs.c) — all 4 abstractions + all features in one demo

---

## Bibliography

The 6-tradition survey that grounds this revision is archived in `research/reports/` (workspace, not in repo). Primary sources cited in v2:

- Alexander, Christopher. *A City is Not a Tree* (1965). https://www.patternlanguage.com/archive/cityisnotatree.html
- Conal Elliott. *Push-Pull Functional Reactive Programming* (2009). http://conal.net/papers/push-pull-frp
- Conal Elliott. *Denotational Design with Type Class Morphisms* (2012). http://conal.net/papers/type-class-morphisms
- Dourish, Paul. *Where the Action Is* (2001). https://www.dourish.com/embodied/
- Friston, Karl. "The free-energy principle: a unified brain theory?" *Nature Reviews Neuroscience* (2010). https://www.nature.com/articles/nrn2787
- Gibson, James J. *The Ecological Approach to Visual Perception* (1979). (affordance theory)
- Harel, David. *Statecharts: A Visual Formalism for Complex Systems* (1987).
- Hutchins, Edwin. *Cognition in the Wild* (1995). https://pages.ucsd.edu/~ehutchins/citw.html
- Norman, Don. *The Design of Everyday Things* (1988).
- Sutherland, Ivan. *Sketchpad* (1963). https://dspace.mit.edu/entities/publication/b5e8025c-c8b2-4843-84e0-76db824e07e6
- Suchman, Lucy. *Plans and Situated Actions* (1987).
- Winograd, Terry; Flores, Fernando. *Understanding Computers and Cognition* (1986).
