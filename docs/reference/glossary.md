# Glossary

> **Applies to**: v0.4. Definitions for Planex-specific terminology. Terms are listed alphabetically.

Planex uses several terms in ways that may differ from common UI library usage. This glossary is the canonical source of definitions — if a term is used in the docs without explanation, it should be defined here.

For the theoretical foundations of these terms, see [Why Four Abstractions](../concepts/canonical/why-four-abstractions.md) and the ADRs in [../decisions/](../decisions/).

---

## A

<a id="action"></a>
### Action

The third stage of a Closure. A function (`px_action_fn`) that mutates Estimates when the Closure is triggered. Receives the Intent and user data as arguments.

**Not the same as an Event** — Events in React fire callbacks that immediately mutate state; Actions are part of a structured 7-stage interaction with explicit Intent, Perception, and Evaluation.

---

## B

<a id="behavior"></a>
### Behavior

Borrowed from Conal Elliott's FRP. A time-varying value, formally `Time → a`. In Planex, an Estimate with animation is a Behavior — its value depends on the current time when sampled.

**Not a separate type** in Planex — Estimate subsumes both static values and continuous behaviors.

---

## C

<a id="closure"></a>
### Closure

One of the three core abstractions. A first-class object representing a complete interaction loop, modeled on Don Norman's 7-stage cognitive model:

1. Goal (human-readable description)
2. Intent (a typed value — see Intent)
3. Action (function that mutates state)
4. Execution (runtime invokes the action)
5. Perception (function returning visible state)
6. Interpretation (user reads the screen)
7. Evaluation (function checking if goal was achieved)

Status transitions: `IDLE → RUNNING → DONE` or `IDLE → RUNNING → FAILED`.

**Not the same as a JavaScript closure** (a function capturing lexical scope). Planex Closures are named after the "closing" of an interaction loop, not after lexical closures.

---

<a id="confidence"></a>
### Confidence

A field on every Estimate, type `double` between 0.0 and 1.0. Inspired by Karl Friston's predictive coding — state is not a known value, it's an estimate with uncertainty.

Currently most Estimates use confidence 1.0 (full certainty). The field exists for future use cases: probabilistic state, sensor fusion, Bayesian inference.

---

## D

<a id="depends-on"></a>
### DEPENDS_ON

A Relation kind. Declares that one Estimate's value is derived from another's. When the source changes, the derived Estimate is automatically recomputed.

```c
px_declare(graph, source_estimate, PX_REL_DEPENDS_ON, derived_estimate);
```

**Not the same as React's dependency arrays** — React's `useEffect([deps])` is a hint to the renderer; Planex's DEPENDS_ON is a first-class relation in a queryable graph.

---

<a id="declare"></a>
### Declare

A machine-initiated Intent kind: "X is done". Transitions a Closure from RUNNING to DONE. See also: Promise, Fail.

Used for async workflows — when the system finishes an async operation (e.g. file save), it calls `px_closure_declare()` to mark the Closure complete.

---

## E

<a id="estimate"></a>
### Estimate

One of the three core abstractions. State with time and uncertainty. Fields:

- value (current snapshot)
- confidence (0.0 to 1.0)
- timestamp (when last sampled)
- animation state (start, end, duration, easing)

Subsumes both static values and Conal's Behaviors (`Time → a`).

**Not the same as React's `useState`** — useState is a discrete snapshot with no time or confidence. Estimate carries both.

---

<a id="evaluation"></a>
### Evaluation

The seventh stage of a Closure. A function (`px_eval_fn`) that returns `bool` — true if the Goal was achieved, false otherwise. If false, the runtime auto-sets the Closure's status to `PX_CLOSURE_FAILED` and generates feedback text.

This is Norman's "Gulf of Evaluation" made machine-checkable. React callbacks return `void` — there's no built-in way to know if the action succeeded.

---

<a id="execution"></a>
### Execution

The fourth stage of a Closure. The runtime invokes the Action function. Distinct from Action itself because Execution is the runtime's responsibility — the user only writes the Action, the runtime schedules it.

---

## F

<a id="fail"></a>
### Fail

A machine-initiated Intent kind: "X failed". Transitions a Closure from RUNNING to FAILED. Generates feedback text for the user.

See also: Promise, Declare.

---

## G

<a id="goal"></a>
### Goal

The first stage of a Closure. A human-readable string describing what the interaction is trying to achieve:

```c
px_closure_new("increment counter", PX_INTENT_REQUEST, ...);
```

Goals are not just for documentation — they're shown in error messages when Evaluation fails: `"evaluation failed: goal \"increment counter\" not achieved"`.

---

## I

<a id="intent"></a>
### Intent

The second stage of a Closure. A **typed value** — one of:

- `PX_INTENT_ASSERT` — "X is true" (speaker asserts a fact)
- `PX_INTENT_REQUEST` — "do X" (speaker requests an action)
- `PX_INTENT_PROMISE` — "I will do X" (speaker promises future action)
- `PX_INTENT_DECLARE` — "X is done" (speaker declares completion)
- `PX_INTENT_EXPRESS` — "I feel X" (speaker expresses state)

These are **speech act types** from Winograd/Flores' "Conversation for Action" theory.

**Critical:** Intent is a value, not a function. This is Planex's defining design choice — see [ADR-0001](../decisions/superseded/ADR-0001-perception-currently-noop.md) and [ADR-0003](../decisions/accepted/ADR-0003-no-ai-integration.md) for consequences.

In React, `onClick={() => submit()}` — the intent is hidden inside the function. In Planex, the intent is a value that can be serialized, replayed, audited.

---

<a id="interpretation"></a>
### Interpretation

The sixth stage of a Closure. The user's cognitive act of interpreting what they perceive. Not modeled in code — this is the user's job, not Planex's. Listed for completeness (Norman's model has 7 stages; Planex implements 6 in code, with #6 being human-side).

---

## P

<a id="perception"></a>
### Perception

The fifth stage of a Closure. A function (`px_perception_fn`) that returns the visible state after the Action executes. **Currently a no-op placeholder** — see [ADR-0001](../decisions/superseded/ADR-0001-perception-currently-noop.md) and [Limitations L1](../concepts/state/limitations.md#l1-perception-is-a-no-op-architectural-gap).

---

<a id="promise"></a>
### Promise

A machine-initiated Intent kind: "I will do X". Transitions a Closure from IDLE to RUNNING. Used to express async workflows — when an async operation starts, the system calls `px_closure_promise()` to mark the Closure as in-progress.

See also: Declare, Fail.

---

<a id="px-rel"></a>
### PX_REL_*

Relation kind constants:

- `PX_REL_DEPENDS_ON` — value derivation (A's value depends on B)
- `PX_REL_TRIGGERS` — causal (A triggering causes B to fire)
- `PX_REL_BESIDE` — spatial (A is beside B in layout)
- `PX_REL_BELOW` — spatial (A is below B in layout)
- `PX_REL_CONTAINS` — structural (A contains B)
- `PX_REL_GROUP_WITH` — semantic (A and B belong to same logical group)

See `include/planex/planex.h` for the complete list.

---

## R

<a id="relation"></a>
### Relation

One of the three core abstractions. A first-class, queryable graph connecting Estimates and Closures. Modeled on Sketchpad's constraint graph and Christopher Alexander's semilattice concept.

**Not the same as React's component tree** — a tree can only express containment; a graph can express dependency, causality, spatial layout, and grouping uniformly.

See [ADR-0002](../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md) for the open question of Relation's necessity.

---

## S

<a id="semilattice"></a>
### Semilattice

A graph-theoretic structure where each node can have multiple parents, contrasting with a tree where each node has at most one parent. From Christopher Alexander's "A City is Not a Tree" (1965).

Planex's Relation is a semilattice, not a tree. This is why Relation can express multi-source dependencies (`all_valid` depending on multiple field estimates) that a tree cannot.

---

## T

<a id="triggers"></a>
### TRIGGERS

A Relation kind. Declares that triggering one Closure causes another to fire:

```c
px_declare(graph, submit_closure, PX_REL_TRIGGERS, save_closure);
```

Distinct from `DEPENDS_ON` (value derivation) — TRIGGERS is about causal flow, not data flow.

---

## See also

- [Why Four Abstractions](../concepts/canonical/why-four-abstractions.md) — manifesto
- [API Reference](api.md) — function-level documentation
- [Limitations](../concepts/state/limitations.md) — where these terms exceed current implementation
- [ADR index](../decisions/README.md) — decisions about these terms
