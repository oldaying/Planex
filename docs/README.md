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

### ADR author kit

When writing or revising an ADR, four files work together (Principle 13 of [`doc-organization.md`](doc-organization.md) Part IX):

| File | Role |
|------|------|
| [`decisions/TEMPLATE.md`](decisions/TEMPLATE.md) | Skeleton — what sections to write |
| [`decisions/TEMPLATE-GUIDE.md`](decisions/TEMPLATE-GUIDE.md) | Companion — how to fill each section well; failure-mode checklist |
| [`decisions/REVIEW-RUBRIC.md`](decisions/REVIEW-RUBRIC.md) | Reviewer rubric — "No 3s" with six criteria; what graders look for |
| [`scripts/check_doc_sections.sh`](../scripts/check_doc_sections.sh) | CI-ready linter — fails the build if a mandatory section is missing. Run `--report` for a human-readable audit. |

The ADR lifecycle, applicability, freshness, and review contract are all in these four artifacts; no separate governance document is needed.

### CI tooling — doc-organization contract enforcers

The doc-organization proposal (see [`doc-organization.md`](doc-organization.md) Part VI) commits six falsifiability gates. Each is a single command in `scripts/`; each fails the build on drift. Together they form the doc-layer equivalent of `leak-budgets.md`'s quantitative L1/L2 audit.

| Command | What it enforces | Failure mode caught |
|---------|------------------|---------------------|
| [`scripts/check_doc_sections.sh`](../scripts/check_doc_sections.sh) | Every ADR has all 9 mandatory sections (Status / Context / Decision / Consequences / Alternatives Considered / CAVEATS / Known issues / HISTORY / References). | Mathlib docBlame-style mandatory-section drift (Principle 11). |
| [`scripts/gen_adr_index.sh`](../scripts/gen_adr_index.sh) | `docs/decisions/README.md` matches the script's auto-generated output. | Hand-maintained index drifting from the actual ADR set (Principle 8). |
| [`scripts/check_adr_lifecycle.sh`](../scripts/check_adr_lifecycle.sh) | Each ADR file lives in the lifecycle directory matching its declared Status. | An ADR moved between directories without updating its Status field (Principle 1). |
| [`scripts/check_links.sh`](../scripts/check_links.sh) | Every internal markdown link resolves to an existing file. | Wave 1 link-rewrite drift — wrong path depth after a doc was moved between subdirectories. |
| [`scripts/find_orphans.sh`](../scripts/find_orphans.sh) | Every doc is linked from at least one other doc (allowlist aside for `staging/`, `changelog.md`, and entry-point `README`s). | A doc that lands in `staging/` and is never graduated (Principle 5). |
| [`scripts/check_terms.sh`](../scripts/check_terms.sh) | Each glossary term used in `docs/concepts/canonical/` is linked to its glossary anchor at least once. Pass `--scope=all` for doc-wide coverage. | A canonical doc mentions a Planex abstraction without linking to its definition (Principle 3, Wave 4.1 scope). |
| [`scripts/check_stale_abstraction_count.sh`](../scripts/check_stale_abstraction_count.sh) | No stale "4 abstractions" references in v0.5-current docs; no "Relation + Estimate + Closure + Perception" without "+ px_loop". | ADR-0008 (v0.4) added `px_loop` as 5th abstraction; v0.5-current docs that still say "4 abstractions" are post-ADR-0008 drift. Historical files (ADRs / changelog / research / v0.4 snapshots) are exempt; intentional quotations can be marked `<!-- stale-allow: reason -->`. (CONTRIBUTING.md rule 5, automated.) <!-- stale-allow: this row describes the script's detection targets; "4 abstractions" is the literal stale-pattern the script grep's for --> |

Run all seven locally before pushing doc edits:

```
for s in scripts/check_doc_sections.sh scripts/gen_adr_index.sh \
         scripts/check_adr_lifecycle.sh scripts/check_links.sh \
         scripts/find_orphans.sh scripts/check_terms.sh \
         scripts/check_stale_abstraction_count.sh; do
    "$s" --check || exit 1
done
```

If `check_links.sh` reports broken internal-link depth after a reorganization wave, fix the depths by hand: open the file, find the broken link, and insert one more `../` segment until the link resolves. The previous one-shot `fix_doc_link_depths.py` helper has been removed — it had done its one-time job (repaired 34 broken depths in commit `de2b669` during Wave 4.2) and was no longer needed; keeping it around was just language-distribution noise.

### Breaking-change migration

Beyond the per-version changelog and per-symbol deprecation registry, Planex maintains a curated breaking-migration guide at the repo root:

| File | Role |
|------|------|
| [`../docs/changelog.md`](changelog.md) | Raw per-version delta log (every change, breaking or not) |
| [`../docs/reference/deprecation-registry.md`](reference/deprecation-registry.md) | Per-symbol retirements (deprecated / removed / diagnostic-seam) |
| [`../UPGRADING.md`](../UPGRADING.md) | **Curated breaking-migration guide** — what you need to change in your code when upgrading. Grouped by version, sub-grouped as `++ API changes:` / `++ Internal changes:` / `++ Build changes:`. (Research basis: lwIP `UPGRADING` file; see Part IX Principle 15.) |

If your build broke after a Planex upgrade, start at `UPGRADING.md`; if a symbol you're calling is gone, start at the deprecation registry; if you want a per-version change history, start at the changelog.

## Lifecycle as directory

An ADR's lifecycle state is its filesystem location, not a YAML field. To move an ADR from Proposed to Accepted, `git mv docs/decisions/proposed/ADR-NNNN-*.md docs/decisions/accepted/`. ADR numbering (ADR-0001 onward) is permanent; only the directory changes.

## Where to start

- **New to Planex**: [`tutorials/getting-started.md`](tutorials/getting-started.md)
- **Evaluating Planex for a project**: [`concepts/canonical/why-four-abstractions.md`](concepts/canonical/why-four-abstractions.md), then [`concepts/canonical/non-goals.md`](concepts/canonical/non-goals.md)
- **Contributing**: [`../CONTRIBUTING.md`](../CONTRIBUTING.md), then [`decisions/TEMPLATE.md`](decisions/TEMPLATE.md)
- **Investigating an old example**: search [`reference/deprecation-registry.md`](reference/deprecation-registry.md) for the symbol you encountered
