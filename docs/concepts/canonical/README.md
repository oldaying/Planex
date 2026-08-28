# Canonical — Normative Position Papers

> **Status**: These docs state what Planex canonically claims. They are the source of truth for the project's posture. See [`doc-organization.md`](../../doc-organization.md) Principle 4.

A "canonical" doc:

- States a **claim** the project makes (not just describes current state — that's `state/`).
- Is **current** (not superseded — that's `history/`).
- Is **accepted** (not a proposal — that's `speculation/`).
- Is **normative** (prescribes what should be true; contrasts with `background/` which is descriptive of external literature).

## Contents

| Doc | Claim it makes |
|-----|----------------|
| [`intent.md`](intent.md) | The one-page orientation sheet (vision, 3 pillars, core loop, non-goals, 3 prerequisites, 7 CI gates, reading order). Read this first; everything else in `canonical/` defends it at length. |
| [`abstraction-form.md`](abstraction-form.md) | Planex's form is "abstraction-as-typed-value" (not encapsulation). Conditional thesis: 8 abstractions are optimal *if* three prerequisites hold (Ontological Stability / Orthogonal Separability / Falsifiability). |
| [`why-four-abstractions.md`](why-four-abstractions.md) | The 5 shipping + 3 v4-proposed abstractions are the right set, grounded in 6 academic traditions (Peirce / Winograd-Flores / Searle / Friston / Elliott / Alexander). |
| [`leak-budgets.md`](leak-budgets.md) | A quantitative L1/L2 leak audit is the falsifiability mechanism for the abstraction layer. The retire curve is committed; failures are CI-blockable. |
| [`non-goals.md`](non-goals.md) | Planex explicitly rejects: general-purpose UI library, component library, AI integration, mobile, web/DOM, GPU, styling, i18n, animation engine, multi-window, IPC, networking. (NG-1 through NG-12.) |

## Relationship to other directories

- `canonical/` answers "what Planex claims"; `state/` answers "what Planex has". They are different axes — a canonical claim can be unimplemented (the gap is in `state/limitations.md`).
- `canonical/` is current; `history/` is superseded. When a canonical doc is replaced, it moves to `history/` and a new doc takes its place in `canonical/`.
- `canonical/` is accepted; `speculation/` is proposals. A speculation doc graduates to `canonical/` (or `history/`) when its ADR is accepted (or rejected).

## Editing canonical docs

Changes to canonical docs are ADR-worthy. If the change is a clarification (no claim changes), no ADR is needed. If the change is a position shift (e.g. "we no longer claim X"), an ADR must precede the edit. See [`../decisions/TEMPLATE.md`](../../decisions/TEMPLATE.md) for the ADR template.
