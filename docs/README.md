# Planex Documentation

> **Doc tree organization**: see [`doc-organization.md`](doc-organization.md) for the proposal that introduced this structure. The four-wave migration plan, design principles, and acceptance checklist are all there.

## Reader intent map (Diátaxis-aligned)

Planex docs are split by what you, the reader, are trying to do. Pick your entry point:

| You want to… | Start here |
|---------------|------------|
| **Learn** Planex from zero (oriented, walks you through) | [`tutorials/`](tutorials/) — start with [`getting-started.md`](tutorials/getting-started.md) |
| **Do** a specific task (button, derived estimate) — recipe-style, assumes you know the basics | [`how-to/`](how-to/) — [`create-a-button.md`](how-to/create-a-button.md), [`derived-estimates.md`](how-to/derived-estimates.md) |
| **Look up** an API, a glossary term, or a deprecated symbol | [`reference/`](reference/) — [`api.md`](reference/api.md), [`glossary.md`](reference/glossary.md), [`deprecation-registry.md`](reference/deprecation-registry.md) |
| **Understand** why Planex is shaped the way it is (normative claims, leak budgets, non-goals) | [`concepts/canonical/`](concepts/canonical/) — [`abstraction-form.md`](concepts/canonical/abstraction-form.md), [`leak-budgets.md`](concepts/canonical/leak-budgets.md), [`non-goals.md`](concepts/canonical/non-goals.md), [`why-four-abstractions.md`](concepts/canonical/why-four-abstractions.md) |

## Doc tree at a glance

```
docs/
├── concepts/              # Theory & position papers — split by reader intent
│   ├── canonical/         #   What Planex claims is true (normative)
│   ├── state/             #   What Planex currently has (descriptive)
│   ├── speculation/       #   What Planex might do (proposals)
│   ├── history/           #   What Planex used to claim (superseded derivations)
│   └── background/        #   Literature surveys (academic context)
├── decisions/             # ADRs — lifecycle is the directory
│   ├── accepted/          #   Active ADRs (govern the codebase today)
│   ├── proposed/          #   Draft ADRs (not yet ruled on)
│   ├── deferred/          #   ADRs whose own acceptance is deferred
│   ├── deprecated/        #   Decisions no longer active, no successor
│   └── superseded/        #   Replaced by a named later ADR
├── how-to/                # Recipe-style task guides
├── reference/             # Lookup: API, glossary, deprecation registry
├── research/              # Dated research notes (comparative studies)
├── tutorials/             # Learning-oriented, walk-throughs
├── staging/               # Holding pen for docs that don't have a home yet
├── changelog.md           # Per-version deltas
├── doc-organization.md   # This tree's own design rationale
└── faq.md                 # Quick answers to common questions
```

## Decision recording

ADRs use Nygard format with mandatory `## CAVEATS` and `## HISTORY` sections (Principle 2 of [`doc-organization.md`](doc-organization.md)). The ADR index in [`decisions/README.md`](decisions/README.md) is **auto-generated** by [`scripts/gen_adr_index.sh`](../scripts/gen_adr_index.sh) — drift between the script output and the committed README is a CI failure.

## Lifecycle as directory

An ADR's lifecycle state is its filesystem location, not a YAML field. To move an ADR from Proposed to Accepted, `git mv docs/decisions/proposed/ADR-NNNN-*.md docs/decisions/accepted/`. ADR numbering (ADR-0001 onward) is permanent; only the directory changes.

## Where to start

- **New to Planex**: [`tutorials/getting-started.md`](tutorials/getting-started.md)
- **Evaluating Planex for a project**: [`concepts/canonical/why-four-abstractions.md`](concepts/canonical/why-four-abstractions.md), then [`concepts/canonical/non-goals.md`](concepts/canonical/non-goals.md)
- **Contributing**: [`../CONTRIBUTING.md`](../CONTRIBUTING.md), then [`decisions/TEMPLATE.md`](decisions/TEMPLATE.md)
- **Investigating an old example**: search [`reference/deprecation-registry.md`](reference/deprecation-registry.md) for the symbol you encountered
