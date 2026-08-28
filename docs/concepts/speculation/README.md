# Speculation — Proposals Not Yet Accepted

> **Status**: These docs propose ideas that have not yet been ruled on by an ADR. See [`doc-organization.md`](../../doc-organization.md) Principle 4.

A "speculation" doc:

- Proposes something **not yet accepted** (it might be accepted, rejected, or deferred).
- Is **forward-looking** (contrasts with `state/` which is current and `history/` which is past).
- Has **no commitment** behind it — commitment requires an ADR.

## Contents

| Doc | What it proposes |
|-----|-------------------|
| [`continuous-intent-speculation.md`](continuous-intent-speculation.md) | A 5th abstraction (`Continuous Intent`) for multi-step interactions. Currently deferred per ADR-0006; this doc records the design space for when v1.0+ revisits the deferral. |

## Lifecycle

A speculation doc has one of three futures:

1. **Acceptance** — the proposal is accepted via an ADR; the doc moves to `canonical/` (if it becomes a normative claim) or `state/` (if it becomes a planned-work description), and the ADR is filed in `decisions/accepted/`.
2. **Rejection** — the proposal is rejected via an ADR; the doc moves to `history/` with a "Rejected by ADR-NNNN" header.
3. **Deferral** — the proposal is deferred; the doc stays in `speculation/` and the ADR is filed in `decisions/accepted/` if the *decision to defer* is accepted (see ADR-0006 for this pattern).

A speculation doc that stays in `speculation/` indefinitely is a signal that the proposal has no champion. After 2 minor versions without an ADR, the doc should be moved to `history/` with a "Stale; no champion" header, or removed.

## Relationship to ADRs

A speculation doc is **not** an ADR — it's the design exploration that *precedes* an ADR. The ADR records the decision; the speculation doc records the reasoning. Both can coexist: ADR-0006 (decide to defer Continuous Interaction) and `continuous-intent-speculation.md` (the design space exploration) live in different directories and serve different readers.

## When to add a doc here

If you're proposing a new abstraction, a new API, a new mechanism, or a new policy — and you want to explore the design space before committing — put the exploration here. Then open an ADR that references this doc and decides whether to accept, reject, or defer.

If the proposal is too small to warrant an ADR (e.g. a one-line API change), don't put a speculation doc — put it in an issue or in [`../staging/`](../staging/) until it's ready.

## Planned content

> Research basis: F* tutorial book (`fstar-lang.org/tutorial/book/structure.html`)
> publishes an explicit "Planned content" bullet list under a "This book is a
> work in progress" banner — the only formal-methods project surveyed that
> publicly declares unwritten chapters by name. Planex adopts the same
> pattern: visible incompleteness is itself a falsifiable claim. A speculation
> directory that hides its gaps cannot be audited for coverage.

The following speculation docs are planned but not yet written. Each entry
is a commitment to *eventually* write the doc; if a planned doc has no
champion for two minor versions, it should be removed from this list (and
the design space left to issue threads).

- **`breakdown-lifecycle-speculation.md`** — proposal for a Breakdown
  abstraction (per v4 design notes; ADR-0010 deferred it). Currently no
  champion.
- **`interpretant-speculation.md`** — proposal for an Interpretant
  abstraction (semiotic framing of the Closure/Perlocution split; currently
  discussed in `../canonical/abstraction-form.md` footnote only).
- **`complect-audit-speculation.md`** — proposal for a formal
  "complecting" detector that finds places where two abstractions are
  inappropriately intertwined (Hickey "Simple Made Easy" sense). Forward-
  looking; not currently planned for v1.0.
- **`closed-ui-corpus-speculation.md`** — proposal for a closed UI pattern
  corpus (per `../state/ui-pattern-coverage.md`'s self-acknowledged gap
  that the current 68-pattern matrix is open, not closed). Required to
  close Prerequisite 3 (falsifiability) on the completeness-test axis.

A reader who finds a gap in Planex's speculative coverage that is not
listed here should open an issue — the gap should either be added to this
list (visible incompleteness) or resolved with a doc (visible
completeness). Hidden incompleteness is the failure mode this section
prevents.
