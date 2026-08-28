# Background — Literature Surveys and Lineage Docs

> **Status**: These docs survey the academic traditions Planex draws from. See [`doc-organization.md`](../../doc-organization.md) Principle 4.

A "background" doc:

- Surveys **external literature** (academic traditions, prior art, related projects).
- Is **descriptive of the tradition**, not normative of Planex (that's `canonical/`).
- Is **stable** — academic traditions don't change fast, so these docs are low-churn.

## Contents

| Doc | What it surveys |
|-----|------------------|
| [`alternative-perspectives.md`](alternative-perspectives.md) | The 6 academic traditions Planex draws from: Peirce (semiotics), Winograd/Flores (Heideggerian HCI), Searle (speech acts), Friston (predictive coding), Conal Elliott (denotational design), Christopher Alexander (pattern language). |
| [`path-C-lineage.md`](path-C-lineage.md) | Planex's "Path C" lineage — the (c) path in Conal Elliott's framework, which positions Planex as a denotational-design library rather than an encapsulation-style abstraction. |
| [`ui-essence-layers.md`](ui-essence-layers.md) | The 5 essence layers (Material / Cognitive / Semantic / Behavioral / Reflective) and which Planex abstractions address each. |

## Why a separate directory

Background docs are different from canonical docs: a canonical doc states what Planex claims; a background doc describes what the literature says. The distinction matters because:

- A canonical doc can be wrong (Planex can mis-state a tradition); a background doc is empirical (the tradition is what it is).
- A canonical doc changes when Planex changes its position; a background doc changes when the literature changes (rare) or when Planex discovers a mis-citation (also rare).
- A reader looking for "what Planex claims" should not have to wade through academic surveys; a reader looking for "why Planex cites Peirce" should not have to wade through normative claims.

## Relationship to canonical docs

Canonical docs cite background docs. For example, `canonical/why-four-abstractions.md` cites `background/alternative-perspectives.md` for the 6-tradition grounding. The citation is one-way: canonical depends on background, not vice versa.

## Editing background docs

Background docs are stable. The main edits are:

- Adding a citation when a new tradition is drawn from (rare; happens when a new abstraction is proposed).
- Fixing a mis-citation when one is discovered (also rare; the audit in `research/2025-08-28-abstraction-as-form-comparative-study.md` is the source for these fixes).
- Adding a "Re-evaluation" footer if a later audit revisits the tradition's relevance.

Background docs do **not** track Planex's current implementation state — that's `state/`. They track the literature.
