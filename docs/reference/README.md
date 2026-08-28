# Reference — Lookup Documentation

> **Status**: These docs are lookup-oriented. They work in isolation — you should be able to read any one entry without reading the others. See [`doc-organization.md`](../doc-organization.md) Principle 4 (Diátaxis reference quadrant).

A "reference" doc:

- Is **information-oriented** (Diátaxis: "I need to know this fact to do my work").
- Works **in isolation** — no narrative, no walkthrough, no learning curve.
- Is **stable** — the API and glossary change less often than tutorials or how-tos.

## Contents

| Doc | What you look up |
|-----|-------------------|
| [`api.md`](api.md) | The Planex C API: every public function, struct, and enum. Stable across minor versions; ABI breaks recorded in [`deprecation-registry.md`](deprecation-registry.md). |
| [`glossary.md`](glossary.md) | The canonical source of Planex-specific terminology. Every term has an HTML anchor (e.g. `#closure`, `#estimate`) for stable cross-refs. Other docs should link to glossary anchors rather than re-define terms. |
| [`deprecation-registry.md`](deprecation-registry.md) | The single canonical registry for deprecated or removed APIs. Each entry: name, status (deprecated / removed / diagnostic-seam), replacement, rationale, ADR. Monotonic — entries are never deleted. |

## Why a separate directory

The Diátaxis framework identifies reference as a distinct documentation quadrant with its own reader intent ("I need to know this fact"). Mixing reference with tutorials (learning-oriented) or how-tos (goal-oriented) produces docs that serve neither reader well — the tutorial reader is overwhelmed by completeness, the reference reader is overwhelmed by narrative.

Reference docs are stable across versions, which makes them the right place for the glossary and the deprecation registry. The API reference changes more often, but the changes are additive (new symbols) rather than narrative.

## Cross-references

The glossary is the **canonical source** of Planex-specific terminology (Principle 3 of [`doc-organization.md`](../doc-organization.md)). Other docs should link to glossary anchors rather than re-define terms. The glossary has HTML anchors (e.g. `<a id="closure"></a>`) that survive reorganization; cross-refs use `glossary.md#closure` rather than a file-relative path that breaks when docs move.

The deprecation registry is the **canonical source** of API lifecycle. Other docs (especially `state/limitations.md` and `decisions/accepted/ADR-*` files) link to the registry when they mention a deprecated or removed API, rather than re-stating the replacement and rationale inline.

## Editing reference docs

Reference docs are edited when:

- The API changes (additive: new symbols added to `api.md`; subtractive: removed symbols added to `deprecation-registry.md`).
- A glossary term is added or its definition is corrected (rare — terms should be stable).
- An API is deprecated or removed (new row in `deprecation-registry.md`; the corresponding entry in `api.md` is updated to note the deprecation).

The expectation is that reference docs are the **most-linked** directory in the doc tree — every other doc cross-references glossary and API entries. Breaking these cross-refs (by moving files) is the failure mode that the link-rewriter scripts in `/home/z/my-project/scripts/fix_doc_links.py` and `/home/z/my-project/scripts/fix_doc_prose_refs.py` are designed to prevent.
