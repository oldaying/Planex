# State — Descriptive Current-State Docs

> **Status**: These docs describe what Planex currently has, not what it claims. See [`doc-organization.md`](../../doc-organization.md) Principle 4.

A "state" doc:

- Describes the **current** state (architecture, limitations, roadmap, versioning).
- Is **descriptive** (contrasts with `canonical/` which is normative).
- Is **current** (not superseded — `history/` is for that).
- May include planned future work (in `roadmap-matrix.md` and `v0.4-roadmap.md`) but the work is described as "planned", not as "committed" — commitment requires an ADR.

## Contents

| Doc | What it describes |
|-----|---------------------|
| [`architecture.md`](architecture.md) | The shipping code layout: `src/`, `include/`, `examples/`, `tests/`; the relationship between abstractions; backend structure (font, fb, x11, win32, cocoa, etc.). |
| [`limitations.md`](limitations.md) | L1 through L14 — known limitations, single-maintainer bus factor (L10), Perception Phase 2 not yet enforced, leak budget gap on L1 leaks, etc. |
| [`ui-pattern-coverage.md`](ui-pattern-coverage.md) | 68 UI patterns × 4 abstractions coverage matrix. The completeness-test corpus that `abstraction-form.md` Prerequisite 3c names as an open gap. |
| [`roadmap-matrix.md`](roadmap-matrix.md) | Cross-tab of abstractions × verification dimensions (tradition citation, orthogonality test, leak budget, example count). |
| [`versioning.md`](versioning.md) | Planex's three-system versioning rule book (semantic version, abstraction epoch, leak budget epoch). |
| [`v0.4-roadmap.md`](v0.4-roadmap.md) | The v0.4 minor-version roadmap. Items here describe planned work; commitment is via ADR. |

## Relationship to other directories

- `state/` is paired with `canonical/`: canonical docs describe what Planex *claims*; state docs describe what Planex *has*. The gap between them is the falsifiability surface (see `canonical/abstraction-form.md` Prerequisite 3).
- `state/` is paired with `decisions/accepted/`: when a state doc's planned work ships, an ADR records the commitment; when a state doc's limitation closes, an ADR records the closure (see ADR-0013 for the leak-budget retire example).
- `state/` is current; `history/` is superseded. When a state doc is no longer current (e.g. `v0.4-roadmap.md` after v0.4 ships), it moves to `history/` and a new doc takes its place.

## Editing state docs

State docs are edited freely as the project state changes. The expectation is:

- When a limitation closes, the line in `limitations.md` is updated in the same PR that closes the leak.
- When a roadmap item ships, the row in `roadmap-matrix.md` flips from 🔴 to 🟢 in the same PR that ships it.
- When the architecture changes materially (new backend, new abstraction shipped), `architecture.md` is updated in the same PR.
