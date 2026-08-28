# Documenting Planex's Documentation: Organization, Lifecycle, and Migration Plan

> **Status:** Proposed. Date: 2026-08-28. Author: Super Z.
>
> **Applies to:** Planex v0.5 onward. Supersedes no prior ADR; companion to ADR-0010 (honesty downgrade), ADR-0013 (v0.5 leak-budget retire), and `abstraction-form.md` Prerequisite 3 (falsifiability engineering mechanisms).
>
> **Companion documents:** [`abstraction-form.md`](concepts/canonical/abstraction-form.md) — the three prerequisites this proposal serves (specifically Prerequisite 3: falsifiability); [`../decisions/README.md`](decisions/README.md) — current ADR index; [`leak-budgets.md`](concepts/canonical/leak-budgets.md) — existing quantitative falsifiability mechanism this proposal extends to the doc layer.
>
> **Research basis:** Broad web survey of 13 projects across three categories — methodology frameworks (Diátaxis, arc42, C4, Nygard ADR, Rust RFC / TC39 / PEP / KEP lifecycles), research-grade systems projects (seL4, Lean 4, Idris 2, OpenBSD), and large-scale production systems (Rust std + rustc-dev-guide, Linux kernel, LLVM, Svelte). Full source URLs in References.

---

## Executive summary

Planex's current `docs/` tree has 19 files in `concepts/`, 13 ADRs in `decisions/`, plus `how-to/`, `reference/`, `research/`, `tutorials/` subdirectories. The structure was assembled incrementally as needs arose and shows three structural defects that compound: (1) `concepts/` mixes normative position papers, descriptive state docs, speculative proposals, historical superseded derivations, and a glossary into one flat directory with no markers — a reader looking for "what Planex canonically claims" cannot distinguish `abstraction-form.md` (normative) from `essence-derivation-v1.md` (superseded) without opening the file; (2) ADRs use a YAML `Status:` field for lifecycle (Proposed/Accepted/Superseded) but the filesystem does not enforce the state machine — a Superseded ADR looks identical to an Accepted one in `ls`; (3) no canonical glossary integration — terms like "essence", "abstraction", "leak" are defined in multiple places with subtle phrasing drift.

This proposal distills 14 signature practices from 13 surveyed projects into 8 design principles for Planex's doc organization, gives a concrete new tree, a 4-wave migration plan with file-by-file move table, and a 6-item acceptance checklist. The constraints honored throughout: pure incremental migration (no big-bang), ADR numbering preserved (ADR-0001 through ADR-0013 unchanged), and existing 6 top-level directories (`concepts/`, `decisions/`, `how-to/`, `reference/`, `research/`, `tutorials/`) preserved as the skeleton.

The single highest-leverage change is **seL4's lifecycle-as-directory pattern** applied to ADRs: replace the flat `decisions/` dir with `decisions/{proposed,accepted,deferred,deprecated,superseded}/ADR-NNNN-name.md`, making `git mv` the state transition. The second is **OpenBSD's mandatory sections pattern** extended to the ADR template: every ADR gains a `## CAVEATS` and `## HISTORY` section, distributing the epistemic-honesty posture that `limitations.md` currently centralizes. Together these two changes close the gap between Planex's stated falsifiability posture and its doc-layer mechanics, turning Prerequisite 3 from a partially-satisfied prerequisite into an engineering mechanism that operates at the doc layer in addition to the code layer.

---

## Part I: Diagnosis — current state and five problems

### 1.1 Inventory

The current `docs/` tree:

```
docs/
├── concepts/        19 files (see §1.2)
├── decisions/       ADR-0001 to ADR-0013 + TEMPLATE.md + README.md
├── how-to/          create-a-button.md, derived-estimates.md
├── reference/       api.md
├── research/        2025-08-28-abstraction-as-form-comparative-study.md
│                   2025-08-28-doc-versioning-top-solutions.md
├── tutorials/       getting-started.md
├── changelog.md
└── faq.md
```

Root-level: `README.md`, `CONTRIBUTING.md`, `PLATFORMS.md`, `TESTING.md`. The tree is small (under 30 markdown files plus 13 ADRs), which is the right moment to reorganize — before accretion makes reorganization costly.

### 1.2 The `concepts/` directory is heterogeneous

The 19 files in `concepts/` decompose by actual function into six categories that the current flat layout does not distinguish:

| Category | Files |
|----------|-------|
| **Normative position papers** | `abstraction-form.md`, `why-four-abstractions.md`, `leak-budgets.md`, `non-goals.md` |
| **Descriptive state docs** | `architecture.md`, `limitations.md`, `ui-pattern-coverage.md`, `roadmap-matrix.md`, `versioning.md`, `v0.4-roadmap.md` |
| **Speculative / proposal** | `continuous-intent-speculation.md` |
| **Historical / superseded** | `essence-derivation-v1.md`, `essence-derivation-v2.md`, `essence-derivation-v3.md`, `essence-derivation-v4-clean.md` |
| **Background / survey** | `alternative-perspectives.md`, `path-C-lineage.md`, `ui-essence-layers.md` |
| **Reference** | `glossary.md` |

A reader looking for "what Planex claims is canonically true" cannot distinguish `abstraction-form.md` (normative) from `essence-derivation-v1.md` (superseded) without opening the file and reading the status line. This is the structural defect that motivated this proposal.

### 1.3 Five specific problems

**Problem 1 — axis conflation.** `concepts/` organizes by topic ("all things conceptual") but mixes docs that answer four different reader questions: *what is claimed?* (normative), *what is the current state?* (descriptive), *what might be?* (speculative), *what was claimed historically?* (superseded). Diátaxis calls this "blur between adjacent categories" and identifies it as the failure mode that turns a doc tree into an ungrepable pile. The blur compounds because each new doc that lands in `concepts/` makes the next reader's task harder — there is no signal telling the contributor where the new doc should go.

**Problem 2 — lifecycle is invisible in the filesystem.** ADR-0007 is marked `Status: Proposed` and is effectively superseded by ADR-0009 / v4; ADR-0011 is `Status: Accepted`. Both sit side-by-side in `decisions/` with no filesystem-level distinction. `ls docs/decisions/` cannot tell you which ADRs are canonical. seL4 solves this by making the directory *be* the state: `decisions/{proposed,accepted,deferred,deprecated,superseded}/`. The Planex equivalent today requires either parsing YAML frontmatter or reading the file body — both fragile.

**Problem 3 — no canonical glossary integration.** `glossary.md` exists but is not the single source of truth for terms. "Essence" is defined in `abstraction-form.md` Prerequisite 1, `why-four-abstractions.md` §1, and `essence-derivation-v4-clean.md` Part I — with subtle phrasing differences that compound across revisions. LLVM's `Lexicon.md` solves this with `(label)=` MyST anchors that survive reorganization and provide stable cross-refs. Without this, drift is invisible until a reader notices a contradiction.

**Problem 4 — ADR template lacks distributed honesty.** The current `TEMPLATE.md` has Status / Context / Decision / Essence Check / Consequences / Alternatives Considered / References — 7 sections. OpenBSD's mandatory `STANDARDS / HISTORY / CAVEATS / BUGS` quartet distributes the honesty posture that `limitations.md` currently centralizes. `limitations.md` is project-level; an ADR-level `## CAVEATS` section would let each decision carry its own "what this does NOT cover" statement, the way ADR-0012 already does informally in its Q3 self-acknowledged gap. Today, caveats leak into "Consequences" or "Alternatives Considered" sections where they don't structurally belong.

**Problem 5 — historical derivation docs clutter the canonical dir.** `essence-derivation-v1.md` through `v4-clean.md` are historical (v1 superseded by v2, v2 by v3, v3 by v4, v4 framed as "design rationale, not essence discovery" in ADR-0010). They are valuable as intellectual history but should not sit alongside the active `abstraction-form.md`. Idris 2 keeps frozen historical docs in `updates/updates.rst` (a frozen migration description), not in the active tutorial/reference trees. The Planex equivalent would move them to `concepts/history/`.

---

## Part II: Comparative research — 13 projects surveyed

### 2.1 Surveyed projects

The three research agents surveyed 13 projects across three categories. The summary table below maps each project to its doc tree location, decision recording mechanism, and lifecycle state vocabulary.

| Project | Category | Doc tree location | Decision recording | Lifecycle state mechanism |
|---------|----------|-------------------|---------------------|---------------------------|
| Diátaxis | Methodology | 4-quadrant map (tutorial / how-to / reference / explanation) | n/a (framework) | Doc doesn't change quadrants |
| arc42 | Methodology | 12-section template (`01-introduction.md` … `12-glossary.md`) | §9 Architecture Decisions | Section persistence |
| C4 model | Methodology | Diagram levels (System → Container → Component → Code) | n/a | Diagram regeneration |
| Nygard ADR | Methodology | `doc/arch/adr-NNN.md` | The doc itself | Proposed / Accepted / Deprecated / Superseded |
| Rust RFC | Lifecycle theory | `text/NNNN-name.md` | RFC PR + tracking issue | Draft → Active → Implemented/Postponed/Closed + FCP 10-day |
| TC39 | Lifecycle theory | Per-proposal repo | Spec PR | 6 stages incl. Stage 2.7 validation gate |
| PEP | Lifecycle theory | `pep-NNNN.rst` | PEP itself | Draft / Accepted / Final / Rejected / Withdrawn / Deferred / Active / Superseded |
| KEP | Lifecycle theory | `keps/sig-*/kep-NNN-*.md` + `kep.yaml` | Tracking issue | Provisional / Implementable / Implemented / Retired / Withdrawn |
| seL4 | Research-grade | `manual/` LaTeX repo + root `CAVEATS.md` | Separate `rfcs` repo | **Lifecycle-as-directory** (`src/{proposed,active,deferred,implemented}/`) |
| Lean 4 | Research-grade | `doc/{dev,examples,make,std,latex}/` | GitHub issues + `RFC:` prefix label set | Issue state + label set |
| Idris 2 | Research-grade | Sphinx `docs/source/{tutorial,reference,backends,proofs,cookbook,implementation,typedd,updates,faq}/` | None (inline `updates.rst`) | `CHANGELOG_NEXT.md` vs `CHANGELOG.md` |
| OpenBSD | Research-grade | No `docs/` dir — man pages in `share/man/manN/` | Commit messages + per-page `.Sh HISTORY` | Per-page HISTORY section |
| Rust std + rustc-dev-guide | Large-scale | Multi-repo mdBook split | RFCs in `rust-lang/rfcs` | `#[stable]` / `#[unstable]` / `issue="..."` in source |
| Linux kernel | Large-scale | Single Sphinx `Documentation/` | Git + LKML lore | Informal; `process/deprecated.rst` registry |
| LLVM | Large-scale | Single Sphinx `llvm/docs/` | RFCs on Discourse | `{only} PreRelease` admonition + centralized `Lexicon.md` |
| Svelte | Large-scale | `documentation/docs/{NN-name}/` | RFCs in `sveltejs/rfcs` (FCP 3-day) | Numeric prefix `NN-` + `99-legacy/` dir + versioned subdomain |

### 2.2 Signature practices ranked for Planex fit

The 14 signature practices below are ranked by fit to Planex's research-grade epistemic-honesty posture, falsifiability engineering mechanism needs, and incremental-migration constraint. Practices 1–4 are the load-bearing changes; practices 5–11 are useful but lower priority; practices 12–14 are framing aids rather than structural changes.

1. **seL4's lifecycle-as-directory** — `decisions/{proposed,accepted,deferred,deprecated,superseded}/ADR-NNNN-name.md`. `git mv` *is* the state transition. The filesystem is grep-able; reviewers can `ls accepted/` to see what's canonical without parsing YAML. **Closes Problem 2 directly.** This is the single highest-leverage change in the proposal.

2. **OpenBSD's mandatory `CAVEATS / STANDARDS / HISTORY / BUGS` quartet per ADR** — distributes the honesty that `limitations.md` currently centralizes. Every decision carries its own "what this does NOT cover" statement. **Closes Problem 4.**

3. **LLVM's centralized `Lexicon.md` with `(label)=` anchors** — single source of truth for terms; cross-refs survive reorganization because they point to anchors, not file paths. **Closes Problem 3.**

4. **Idris 2's reader-intent sectioning** — split `concepts/` along the reader axis (normative / descriptive / speculative / historical / reference / background). This is the structural correction to Problem 1's axis conflation. **Closes Problems 1 and 5.**

5. **Lean's examples-as-regression-tests** — pair `examples/X.c` with `examples/X.expected`. Already partial in Planex via `test_*.c`; closes a falsifiability gap (output drift detection) that complements `leak-budgets.md`'s L2 leak audit.

6. **Linux kernel's `staging/` holding pen** — for docs that don't yet have a permanent home. Avoids the failure mode where a doc is force-fit into the wrong category just to land.

7. **Linux kernel's `process/deprecated.rst` single registry** — `leak-budgets.md` is already this for L2 leaks; formalize the parallel role for API-level deprecations (`px_closure_get_status` removed in v4 per ADR-0012 Finding 3, etc.).

8. **Svelte's `99-legacy/` for back-compat APIs in current version** — old APIs still callable in new version belong in `legacy/`, not in `v0.4/` mirror. Currently Planex has only one such case (`px_closure_get_status` removed in v4) but the pattern scales.

9. **TC39 Stage 2.7 validation gate** — promote "CI lint not yet in place" (ADR-0012 Q3 self-acknowledged gap) from a footnote to a named lifecycle stage. An ADR cannot reach `Accepted` until its enforcement mechanism is implemented. This is a real proposal but out of scope here — would amend the ADR template in a separate ADR.

10. **Svelte's numeric prefix `NN-`** for ordering within a category — eliminates separate TOC maintenance. Lower priority because Planex's current categories are small enough that TOC is not a maintenance burden.

11. **Nygard's "ADR 1 IS the README"** — auto-generated ADR-0001 doubles as index; eliminates hand-maintained `decisions/README.md` table. Planex's current `decisions/README.md` is hand-maintained; this is a maintenance cost that compounds as ADRs accrue.

12. **Diátaxis 2-D map** — recognize that a single `concepts/` dir conflates two axes (theory vs practice × acquisition vs application); the split into `explanation/` and `reference/` quadrants is the structural correction. Idris 2's reader-intent sectioning (practice 4) is the operational form of this.

13. **Rust RFC FCP 10-day window** — gate ADR acceptance with a public comment period. Useful when Planex opens to external contributors; currently single-maintainer (limitation L10) so the audience is empty.

14. **arc42 numbered-section addressing** — assign each Planex doc an arc42 section number (`§4 Solution Strategy`, `§9 Architecture Decisions`, `§10 Quality Requirements`, `§12 Glossary`); gives stable cross-refs across reorganization. A framing layer on top of the proposed structure; could be added later without restructuring.

The 14 practices compose — they are not alternatives. Practices 1–4 are the structural changes that close the five diagnosed problems; practices 5–11 are useful additions that strengthen falsifiability; practices 12–14 are framing aids that can be adopted later without restructuring.

---

## Part III: Design principles

Eight principles for Planex's doc organization, each grounded in the research and applied to Planex's specific gaps. The principles are not independent — they compose into a single coherent posture where the doc layer mirrors the code layer's falsifiability mechanisms.

### Principle 1 — Lifecycle is filesystem, not frontmatter

**Rationale:** seL4 encodes RFC state as `src/{proposed,active,deferred,implemented}/` — the directory *is* the state. `git mv` enforces the transition; reviewers can `ls accepted/` to see what's canonical without parsing YAML. Planex currently uses YAML `Status:` in ADR frontmatter, which is human-readable but machine-invisible — `ls docs/decisions/` cannot tell Accepted from Superseded. The frontmatter field becomes a redundant comment after the move, but is kept for backward-compat with any external tooling that parses it.

**Application:** Replace flat `decisions/` with `decisions/{proposed,accepted,deferred,deprecated,superseded}/`. ADR-NNNN numbering preserved; the move is purely organizational. See Part V Wave 3.

**Research basis:** seL4 RFCs repo — <https://github.com/seL4/rfcs>; OpenBSD keeps HISTORY in every man page but does not separate by state, so the seL4 pattern is the stronger fit for Planex's ADR directory.

### Principle 2 — Honesty is distributed, not centralized

**Rationale:** OpenBSD mandates every man page has `STANDARDS`, `HISTORY`, `CAVEATS`, `BUGS` sections — the honesty posture is in every document, not in a separate `CAVEATS.md` at the root. Planex currently centralizes honesty in `limitations.md` and `non-goals.md`; individual ADRs are not required to carry their own caveats. The result is that caveats leak into "Consequences" or "Alternatives Considered" sections where they don't structurally belong, and the line between "what this decision does not cover" (a caveat) and "what alternatives we rejected" (an alternatives-considered entry) blurs.

**Application:** Extend `TEMPLATE.md` with two mandatory sections: `## CAVEATS` (warnings about THIS decision that the reader should know — distinct from "Alternatives Considered" which describes rejected paths) and `## HISTORY` (when proposed, when accepted, when superseded, link to superseding ADR if applicable). See Part V Wave 2.

**Research basis:** OpenBSD `mdoc.7` style guide — <https://man.openbsd.org/mdoc.7>; seL4's root-level `CAVEATS.md` demonstrates the project-level pattern, but OpenBSD's per-page distribution is the stronger fit for ADRs.

### Principle 3 — Glossary is the canonical term source, with stable anchors

**Rationale:** LLVM's `Lexicon.md` uses `(label)=` MyST anchors so every glossary entry has a stable URL that survives file reorganization. Planex's `glossary.md` exists but terms are redefined in `abstraction-form.md`, `why-four-abstractions.md`, and `essence-derivation-v4-clean.md` with subtle drift. Without canonical anchors, the glossary is one opinion among several; with anchors, every in-text reference can link to the canonical definition and drift becomes a CI failure.

**Application:** Promote `glossary.md` to the single source of truth. Every term definition elsewhere must be a link to `glossary.md#term`. Add HTML anchors to `glossary.md` entries (e.g., `<a id="essence"></a>`) for stable cross-refs. See Part V Wave 4.

**Research basis:** LLVM `Lexicon.md` — <https://github.com/llvm/llvm-project/blob/main/llvm/docs/Lexicon.md>. The LLVM doc itself opens with "This document is a work in progress!" — a self-acknowledged weakness that Planex's glossary should adopt as a one-line caveat.

### Principle 4 — Reader intent drives top-level sectioning

**Rationale:** Idris 2 splits `docs/source/` by reader intent: `tutorial/` (learning path), `reference/` (lookup), `proofs/` (recipes), `cookbook/` (tasks), `implementation/` (maintainers), `typedd/` (book-companion), `updates/` (migration). Diátaxis reaches the same conclusion at the framework level — the four quadrants are user intents, not topic areas. Planex's `concepts/` conflates normative (what we claim), descriptive (what we have), speculative (what we might do), and historical (what we used to claim) — four different reader intents collapsed into one flat dir.

**Application:** Split `concepts/` along reader intent:
- `concepts/canonical/` — normative position papers (abstraction-form, why-four-abstractions, leak-budgets, non-goals)
- `concepts/state/` — descriptive current-state docs (architecture, limitations, ui-pattern-coverage, roadmap-matrix, versioning, v0.4-roadmap)
- `concepts/speculation/` — proposals not yet accepted (continuous-intent-speculation)
- `concepts/history/` — superseded derivation docs (essence-derivation-v1, v2, v3, v4-clean)
- `concepts/background/` — literature surveys (alternative-perspectives, path-C-lineage, ui-essence-layers)
- `concepts/glossary.md` — promoted to top-level `reference/glossary.md` (per Principle 3)

**Research basis:** Idris 2 `docs/source/` structure — <https://github.com/idris-lang/Idris2/tree/main/docs/source>; Diátaxis — <https://diataxis.fr/>.

### Principle 5 — Docs have a holding pen for incoming material

**Rationale:** Linux kernel's `Documentation/staging/` is explicitly labeled "Unsorted Documentation" and accepts docs that don't yet have a permanent home. This avoids the failure mode where a doc is forced into the wrong category just to land — a failure mode that compounds because each force-fit makes the next contributor's choice harder. Planex has no equivalent — incoming speculative docs either get force-fit into `concepts/` (contributing to Problem 1) or sit in a contributor's branch indefinitely and rot.

**Application:** Add `docs/staging/` for docs that don't yet have a permanent home. Document the expectation: a doc in `staging/` either graduates to its proper category within 2 minor versions or is removed. The policy is enforced by the acceptance checklist's orphan scan (Part VI).

**Research basis:** Linux kernel `Documentation/staging/` — <https://github.com/torvalds/linux/tree/master/Documentation/staging>. The dir's own README labels it "Unsorted Documentation" — the explicit naming is part of the pattern.

### Principle 6 — Deprecation is a registry, not a state machine

**Rationale:** Linux kernel's `process/deprecated.rst` is a single canonical registry — alphabetized, each entry has name + replacement + rationale. Rust encodes deprecation in `#[unstable]` source attributes; LLVM uses per-doc `:::{warning}` admonitions; Svelte uses `99-legacy/` directory. Of these, Linux's single-file registry is the most useful for Planex because `leak-budgets.md` already moves toward this — adding a parallel `deprecation-registry.md` would close the loop for API-level deprecations (a dimension leak-budgets does not cover).

**Application:** Add `docs/reference/deprecation-registry.md` as the canonical registry for deprecated APIs. Each entry: name, deprecated-when, replacement, rationale, link to ADR that decided the deprecation. See Part V Wave 4.

**Research basis:** Linux kernel `Documentation/process/deprecated.rst` — <https://github.com/torvalds/linux/blob/master/Documentation/process/deprecated.rst>. The doc itself opens with the admission that "new instances may sneak into the kernel while old ones are being removed" — the registry tracks drift, doesn't eliminate it.

### Principle 7 — Examples are regression tests

**Rationale:** Lean pairs every `examples/X.lean` with `examples/X.lean.out.expected` — if the example output drifts, CI breaks. The example is documentation *and* a test; the two cannot diverge silently. Planex's `examples/*.c` currently build and run, but their output is not checked into git; an example can drift from the library's actual behavior without any signal. This is a falsifiability gap that complements `leak-budgets.md` — leak budgets measure abstraction erosion; example-output checks measure executable-documentation drift.

**Application:** Add `examples/X.expected` files paired with each `examples/X.c`. CI runs each example, diffs output against `.expected`, fails on drift. The Makefile gains a `make check-examples` target. See Part V Wave 4.

**Research basis:** Lean 4 `doc/examples/` — <https://github.com/leanprover/lean4/tree/master/doc/examples>. Each `.lean` file is paired with a `.lean.out.expected` checked into git; the CI runs both and compares.

### Principle 8 — ADR-0001 is the implicit index

**Rationale:** Nygard's original ADR pattern has ADR 1 record the decision to use ADRs; the `adr-tools` CLI auto-generates ADR 1 on `adr init`. The ADR directory itself, rendered by GitHub's listing, becomes the index — no hand-maintained `README.md` table needed. Planex's `decisions/README.md` is currently hand-maintained; this is a maintenance cost that compounds as ADRs accrue, and is a falsifiability gap (the index can drift from the actual ADR set).

**Application:** Convert `decisions/README.md` into a generated artifact: a script that walks `decisions/{accepted,superseded,...}/` and emits the index table. ADR-0001 remains ADR-0001 (the implicit "we use ADRs" record). See Part V Wave 4.

**Research basis:** Nygard ADR — <https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions>; adr-tools — <https://github.com/npryce/adr-tools>.

---

## Part IV: Concrete structure

The proposed tree (after all 4 waves complete):

```
docs/
├── concepts/
│   ├── canonical/         # normative position papers (Principle 4)
│   │   ├── abstraction-form.md
│   │   ├── why-four-abstractions.md
│   │   ├── leak-budgets.md
│   │   └── non-goals.md
│   ├── state/             # descriptive current-state docs
│   │   ├── architecture.md
│   │   ├── limitations.md
│   │   ├── ui-pattern-coverage.md
│   │   ├── roadmap-matrix.md
│   │   ├── versioning.md
│   │   └── v0.4-roadmap.md
│   ├── speculation/       # proposals not yet accepted
│   │   └── continuous-intent-speculation.md
│   ├── history/           # superseded derivation docs
│   │   ├── essence-derivation-v1.md
│   │   ├── essence-derivation-v2.md
│   │   ├── essence-derivation-v3.md
│   │   └── essence-derivation-v4-clean.md
│   └── background/         # literature surveys
│       ├── alternative-perspectives.md
│       ├── path-C-lineage.md
│       └── ui-essence-layers.md
├── decisions/
│   ├── proposed/           # ADRs in Draft status (Principle 1)
│   ├── accepted/           # ADRs in Accepted status
│   ├── deferred/           # ADRs deferred to future version
│   ├── deprecated/         # ADRs whose decision is no longer active
│   ├── superseded/         # ADRs replaced by a later ADR
│   ├── TEMPLATE.md        # extended with CAVEATS + HISTORY (Principle 2)
│   └── README.md           # generated, not hand-maintained (Principle 8)
├── how-to/                 # unchanged
├── reference/
│   ├── api.md
│   ├── glossary.md         # promoted from concepts/ (Principle 3)
│   └── deprecation-registry.md  # new (Principle 6)
├── research/               # dated reports — unchanged
├── tutorials/              # unchanged
├── staging/                # holding pen for incoming docs (Principle 5)
├── changelog.md
└── faq.md
examples/
├── counter_4abs.c
├── counter_4abs.expected    # new (Principle 7)
├── hover_drag_4abs.c
├── hover_drag_4abs.expected # new
└── ... (one .expected per .c)
scripts/
└── gen_adr_index.sh         # generates decisions/README.md (Principle 8)
```

Notable constraints honored: existing 6 top-level dirs preserved (`concepts/`, `decisions/`, `how-to/`, `reference/`, `research/`, `tutorials/`); ADR-NNNN numbering preserved; all moves are `git mv`, no content rewrite required for the moves themselves. The new subdirs (`canonical/`, `state/`, `speculation/`, `history/`, `background/`, `proposed/`, `accepted/`, `deferred/`, `deprecated/`, `superseded/`, `staging/`) are additive — existing paths can be redirected via a one-line "Moved to:" placeholder file for one minor version if external links need preservation.

---

## Part V: Migration plan

Four waves, each independently shippable as a single PR. The waves are ordered by ascending risk: Wave 1 is pure file moves with no semantic change; Wave 2 is template extension; Wave 3 is the structural change with the most moving parts (but each move is mechanical); Wave 4 introduces new tooling. Each wave's acceptance criterion is a single command that produces empty output (or zero exit code).

### Wave 1 — Low-risk structural moves (no semantic change)

**Goal:** Add `docs/staging/`, `concepts/{canonical,state,speculation,history,background}/`. Move existing `concepts/*.md` into their target subdirs. No content edits. All existing links continue to work via link-rewrite pass.

**Move table (19 files):**

| Source | Destination | Category |
|--------|-------------|----------|
| `docs/concepts/canonical/abstraction-form.md` | `docs/concepts/canonical/abstraction-form.md` | normative |
| `docs/concepts/canonical/why-four-abstractions.md` | `docs/concepts/canonical/why-four-abstractions.md` | normative |
| `docs/concepts/canonical/leak-budgets.md` | `docs/concepts/canonical/leak-budgets.md` | normative |
| `docs/concepts/canonical/non-goals.md` | `docs/concepts/canonical/non-goals.md` | normative |
| `docs/concepts/state/architecture.md` | `docs/concepts/state/architecture.md` | descriptive |
| `docs/concepts/state/limitations.md` | `docs/concepts/state/limitations.md` | descriptive |
| `docs/concepts/state/ui-pattern-coverage.md` | `docs/concepts/state/ui-pattern-coverage.md` | descriptive |
| `docs/concepts/state/roadmap-matrix.md` | `docs/concepts/state/roadmap-matrix.md` | descriptive |
| `docs/concepts/state/versioning.md` | `docs/concepts/state/versioning.md` | descriptive |
| `docs/concepts/state/v0.4-roadmap.md` | `docs/concepts/state/v0.4-roadmap.md` | descriptive |
| `docs/concepts/speculation/continuous-intent-speculation.md` | `docs/concepts/speculation/continuous-intent-speculation.md` | speculative |
| `docs/concepts/history/essence-derivation-v1.md` | `docs/concepts/history/essence-derivation-v1.md` | superseded |
| `docs/concepts/history/essence-derivation-v2.md` | `docs/concepts/history/essence-derivation-v2.md` | superseded |
| `docs/concepts/history/essence-derivation-v3.md` | `docs/concepts/history/essence-derivation-v3.md` | superseded |
| `docs/concepts/history/essence-derivation-v4-clean.md` | `docs/concepts/history/essence-derivation-v4-clean.md` | superseded |
| `docs/concepts/background/alternative-perspectives.md` | `docs/concepts/background/alternative-perspectives.md` | survey |
| `docs/concepts/background/path-C-lineage.md` | `docs/concepts/background/path-C-lineage.md` | survey |
| `docs/concepts/background/ui-essence-layers.md` | `docs/concepts/background/ui-essence-layers.md` | survey |
| `docs/reference/glossary.md` | `docs/reference/glossary.md` | promoted (Principle 3) |

Plus one new file: `docs/staging/README.md` describing the holding-pen policy (graduate within 2 minor versions or remove).

**Link-update pass:** All `docs/concepts/X.md` relative links need rewriting — a single `rg -l 'docs/concepts/' docs/ examples/ tests/` run produces the list; sed substitution rewrites paths to their new locations. This is the only non-mechanical step in Wave 1, and it is bounded — under 30 files contain such links.

**Acceptance for Wave 1:** `rg -l 'docs/concepts/abstraction-form\.md$' docs/` returns 0 results with the old path (everything points to the new `canonical/abstraction-form.md`); `rg 'concepts/[a-z]' docs/ | grep -v 'concepts/\(canonical\|state\|speculation\|history\|background\)/'` returns 0 results.

### Wave 2 — ADR template revision

**Goal:** Extend `decisions/TEMPLATE.md` with mandatory `## CAVEATS` and `## HISTORY` sections (Principle 2). No content changes to existing ADRs yet — they gain the new sections when next edited (incremental backfill).

**Template delta (added after "Alternatives Considered"):**

```markdown
## CAVEATS              # NEW — what this decision does NOT cover
                       # Warnings/gotchas the reader should know about THIS decision.
                       # Distinct from "Alternatives Considered" (rejected paths)
                       # and from "Consequences" (expected downstream effects).
                       # Example: ADR-0012 Finding 3 "Closure lost get_status in v4"
                       # would live here, not in Consequences.

## HISTORY              # NEW — when proposed, accepted, superseded
                       # 2026-08-24: Proposed
                       # 2026-08-26: Accepted
                       # 2026-08-28: Superseded by ADR-NNNN (if applicable)
                       # One line per state transition; nothing else.
```

**Backfill pass:** For the 5 most-cited ADRs (ADR-0001, ADR-0007, ADR-0010, ADR-0011, ADR-0013), add `## HISTORY` and `## CAVEATS` sections now. Other ADRs gain the sections when next edited. This is the incremental pattern seL4 uses — the new format is opt-in per-ADR until backfill is complete.

**Acceptance for Wave 2:** `TEMPLATE.md` contains both new section headers; the 5 backfilled ADRs each have non-empty `## HISTORY` and `## CAVEATS` sections; `rg '## CAVEATS' docs/decisions/TEMPLATE.md` returns 1 match.

### Wave 3 — ADR lifecycle-as-directory

**Goal:** Replace flat `decisions/ADR-NNNN-*.md` with `decisions/{proposed,accepted,deferred,deprecated,superseded}/ADR-NNNN-*.md` (Principle 1). ADR numbering preserved. Each ADR's target directory is determined by its current `Status:` frontmatter field.

**Move table (13 ADRs):**

| ADR | Current status | Destination |
|-----|----------------|-------------|
| ADR-0001 | superseded (by ADR-0005) | `decisions/superseded/ADR-0001-perception-currently-noop.md` |
| ADR-0002 | accepted | `decisions/accepted/ADR-0002-relation-necessity-pending-undo.md` |
| ADR-0003 | accepted | `decisions/accepted/ADR-0003-no-ai-integration.md` |
| ADR-0004 | accepted | `decisions/accepted/ADR-0004-use-c-not-rust-zig-cpp.md` |
| ADR-0005 | accepted (supersedes ADR-0001) | `decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md` |
| ADR-0006 | accepted (decision is to defer) | `decisions/accepted/ADR-0006-continuous-interaction-deferred.md` |
| ADR-0007 | proposed (superseded by ADR-0009 / v4) | `decisions/superseded/ADR-0007-essence-derivation-v2-revision.md` |
| ADR-0008 | accepted | `decisions/accepted/ADR-0008-feedback-as-fifth-essence-category.md` |
| ADR-0009 | proposed (superseded by v4-clean) | `decisions/superseded/ADR-0009-essence-rederivation-v3.md` |
| ADR-0010 | accepted | `decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md` |
| ADR-0011 | accepted | `decisions/accepted/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md` |
| ADR-0012 | accepted | `decisions/accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md` |
| ADR-0013 | accepted | `decisions/accepted/ADR-0013-v05-leak-budget-retire.md` |

**Note on ADR-0006:** Although the decision is "defer continuous interaction abstraction to v1.0+", the ADR's own status is `Accepted` — the decision to defer was accepted. The ADR lives in `accepted/`, not `deferred/`. The `deferred/` directory is reserved for ADRs whose own acceptance is deferred (e.g., proposed but not yet ruled on).

**Note on `deprecated/` vs `superseded/`:** Superseded = replaced by a specific later ADR (bidirectional link). Deprecated = no longer active but no specific replacement named. The 3 ADRs in `superseded/` (0001, 0007, 0009) each have a named successor.

**Acceptance for Wave 3:** `ls docs/decisions/accepted/ | wc -l` returns 10 (ADR-0002, 0003, 0004, 0005, 0006, 0008, 0010, 0011, 0012, 0013); `ls docs/decisions/superseded/ | wc -l` returns 3 (ADR-0001, 0007, 0009); `ls docs/decisions/{proposed,deferred,deprecated}/ 2>/dev/null | wc -l` returns 0 (none currently in those states, dirs may be empty or `.gitkeep`-ed).

### Wave 4 — Falsifiability closure

**Goal:** Four small additions that close Principles 3, 6, 7, 8.

**4.1 — Glossary anchors (Principle 3).** Add HTML anchors to every term in `docs/reference/glossary.md` (post-Wave-1 location) — `<a id="essence"></a>`, `<a id="abstraction"></a>`, `<a id="leak"></a>`, `<a id="l1"></a>`, `<a id="l2"></a>`, `<a id="denotational"></a>`, `<a id="representamen"></a>`, etc. Update all in-text references in `canonical/abstraction-form.md`, `canonical/why-four-abstractions.md`, `history/essence-derivation-v4-clean.md` to link to `../reference/glossary.md#essence` etc. A future `scripts/check_terms.sh` (see Part VI) verifies every glossary term occurrence outside `glossary.md` links to it.

**4.2 — Deprecation registry (Principle 6).** Create `docs/reference/deprecation-registry.md`. Initial entries:

| API | Status | Deprecated when | Replacement | Rationale | ADR |
|-----|--------|-----------------|-------------|----------|-----|
| `px_closure_get_status(c)` | removed in v4 | v4 design | `px_perlocution_status(per)` | operational status is perlocutionary, not illocutionary | ADR-0012 Finding 3 |
| `px_perception_invoke_all()` | diagnostic seam (not deprecated) | n/a (v0.5 retire of L2 leak) | (kept for testing/debugging) | Phase 2 auto-invocation landed; manual invoke remains as diagnostic seam | ADR-0013 |
| `px_perception_invoke_single()` | diagnostic seam (not deprecated) | n/a | (kept) | same as above | ADR-0013 |
| `px_perception_invoke_for_estimate(e)` | diagnostic seam (not deprecated) | n/a | (kept) | same as above | ADR-0013 |

The distinction between "deprecated" and "diagnostic seam" matters: a deprecated API will eventually be removed; a diagnostic seam is permanent and exists for testing/debugging. The registry is the canonical place to record this distinction.

**4.3 — Examples as regression tests (Principle 7).** ~~Add `examples/X.expected` files for each `examples/X.c`. CI runs each example, diffs against expected, fails on drift. The Makefile gains a `make check-examples` target. The expected files are seeded from current example output; future drift becomes a CI failure that forces a deliberate update.~~ **Landed in commit (T6b).** 11 `examples/X.expected` files seeded for the headless examples (`EXAMPLES_NO_X11`); `make check-examples` target added to Makefile; 9th gate wired into `.github/workflows/docs.yml` as `examples-regression` job. Timestamps (`t=<digits>`) and addresses (`0x<digits>`) are normalized via `sed -E` before diffing so non-deterministic output (e.g., `px_now_ms()` in log perceptions) does not false-positive.

The current `examples/` directory contains 19 `.c` files; each gains a paired `.expected`. The seeding run is `for f in examples/*.c; do ./"$(basename "$f" .c)" > "${f%.c}.expected"; done` after building all examples.

**4.4 — Auto-generated ADR index (Principle 8).** Add `scripts/gen_adr_index.sh` that walks `decisions/{proposed,accepted,deferred,deprecated,superseded}/` and emits the markdown table currently hand-maintained in `decisions/README.md`. CI runs this and fails if the emitted table differs from the committed `README.md` (using `diff`). The script:

```bash
#!/usr/bin/env bash
# scripts/gen_adr_index.sh — auto-generate docs/decisions/README.md
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
echo "# Planex Architecture Decision Records"
echo
echo "Auto-generated by scripts/gen_adr_index.sh. Do not edit by hand."
echo
echo "| # | Title | Status |"
echo "|---|-------|--------|"
for state in proposed accepted deferred deprecated superseded; do
  for f in docs/decisions/$state/ADR-*.md; do
    [ -f "$f" ] || continue
    num=$(basename "$f" | grep -oE 'ADR-[0-9]+' | tr -d 'ADR-')
    title=$(grep -m1 '^# ' "$f" | sed 's/^# //')
    echo "| $num | $title | $state |"
  done
done
```

**Acceptance for Wave 4:** `make check-examples` exits 0; `scripts/gen_adr_index.sh | diff - docs/decisions/README.md` produces no output (zero-diff); `rg -l 'essence\b' docs/concepts/canonical/ | xargs rg -L '\[essence\]' | wc -l` returns 0 (every "essence" mention in canonical docs is a link to glossary).

---

## Part VI: Acceptance checklist

After all four waves ship, the following six checks must pass. Each check is a single command; each is added to CI in Wave 4 (or earlier where noted).

1. **Bidirectional link integrity** — every cross-reference in `docs/` resolves to an existing file. Command (new, `scripts/check_links.sh`): walks all `.md` files, extracts `[...](relative-path)` references, verifies each target exists. Fails on broken links. This catches the most common Wave 1 failure mode: a link-rewrite that missed a file.

2. **Orphan doc scan** — every doc is referenced by at least one other doc or by a top-level README. Command (new, `scripts/find_orphans.sh`): walks `docs/`, finds files referenced by 0 other docs. Fails on orphans with exceptions for `staging/` (which is allowed to be orphan by definition) and `changelog.md` (chronological, not referenced). This catches the Wave 5+ failure mode: a doc that lands in `staging/` and is never graduated.

3. **Term consistency scan** — every occurrence of canonical terms (`essence`, `abstraction`, `leak`, `L1`, `L2`, `denotational`, `representamen`, etc.) outside `reference/glossary.md` links to `reference/glossary.md#<term>`. Command (new, `scripts/check_terms.sh`): extracts terms from glossary, greps for occurrences elsewhere, flags those that don't link. This catches the drift that Principle 3 is designed to prevent.

4. **Lifecycle state validity** — every ADR file lives in the directory matching its `Status:` frontmatter field. Command (new, `scripts/check_adr_lifecycle.sh`): reads the `Status:` line from each ADR, compares to its directory location, fails on mismatch. This catches the Wave 3 failure mode: an ADR whose `Status:` field was not updated when moved.

5. **Examples regression** — every `examples/X.c` has a paired `examples/X.expected`; `make check-examples` passes. This is the Principle 7 falsifier — drift between examples and library behavior becomes a CI failure, not a silent bug.

6. **ADR index freshness** — `scripts/gen_adr_index.sh` output matches committed `decisions/README.md`. Command: `scripts/gen_adr_index.sh | diff - docs/decisions/README.md`. This is the Principle 8 falsifier — the hand-maintained index is replaced by a generated artifact; drift becomes a CI failure.

The six checks compose: a PR that fails any check is not mergeable. Together they form the doc-layer equivalent of `leak-budgets.md`'s quantitative L1/L2 audit — falsifiable, automated, and bounded.

---

## Part VII: Rollback path

Each wave is independently reversible. The rollback for each wave is a single PR that undoes the moves; content edits (template extensions in Wave 2, ADR `## CAVEATS` / `## HISTORY` backfills in Wave 2) are not rolled back — they are harmless if the wave's structural change is reverted.

- **Wave 1 rollback:** `git mv docs/concepts/{canonical,state,speculation,history,background}/* docs/concepts/ && rmdir docs/concepts/{canonical,state,speculation,history,background}`. Then revert the link-rewrite pass. Single PR; ~10 minutes of work.
- **Wave 2 rollback:** Remove the `## CAVEATS` and `## HISTORY` section headers from `TEMPLATE.md`. Existing ADRs that gained those sections keep them (no harm — they remain valid markdown). Single PR; trivial.
- **Wave 3 rollback:** `git mv docs/decisions/{proposed,accepted,deferred,deprecated,superseded}/* docs/decisions/ && rmdir docs/decisions/{proposed,accepted,deferred,deprecated,superseded}`. Single PR; ~5 minutes of work, all moves are mechanical.
- **Wave 4 rollback:** Delete `examples/X.expected` files; remove `make check-examples` target; remove `scripts/gen_adr_index.sh` and restore hand-maintained `decisions/README.md`; remove glossary anchors (or keep them as harmless HTML). Single PR.

The waves are designed so that any wave can be rolled back without rolling back earlier waves. Wave 1's directory split does not depend on Wave 2's template extension; Wave 3's lifecycle-as-directory does not depend on Wave 4's auto-generated index. This independence is the operational form of the "incremental migration" constraint.

---

## Part VIII: Open questions

This proposal explicitly defers the following decisions. Each deferral is intentional; the proposal's value does not depend on resolving them.

1. **Whether to adopt Svelte's `99-legacy/` pattern.** Planex does not currently have multiple shipped versions to support simultaneously; the v4 ABI break is the only candidate, and the migration path is documented in ADR-0012. The `99-legacy/` pattern becomes useful when v1.0 ships and v0.x is no longer current but still callable. Defer until v1.0.

2. **Whether to adopt TC39 Stage 2.7 as a named ADR state.** ADR-0012 Q3 already self-acknowledges the "criteria documented but not enforced" gap; making it a named lifecycle stage would mean an ADR cannot reach `Accepted` until its enforcement mechanism (e.g., a CI lint that flags new abstractions whose constructor accepts a parameter no operation reads) is implemented. This is a real proposal but out of scope for this doc — it would amend the ADR template, which is Wave 2's territory. Defer to a future ADR-0014 if the gap proves painful.

3. **Whether to adopt Rust RFC FCP.** Planex is single-maintainer (limitation L10); a 10-day public comment period has no audience. Defer until co-maintainers exist or external contribution pressure demands it.

4. **Whether to adopt arc42 numbered-section addressing.** Each Planex doc could be assigned an arc42 section number (`§4 Solution Strategy`, `§9 Architecture Decisions`, `§10 Quality Requirements`, `§12 Glossary`) for stable cross-refs that survive reorganization. This is a framing layer on top of the proposed structure and could be added later without restructuring — it composes with rather than replaces the reader-intent split of Principle 4.

5. **Migration of `docs/research/` to a separate `research/` top-level dir.** The two dated reports in `docs/research/` (`2025-08-28-abstraction-as-form-comparative-study.md`, `2025-08-28-doc-versioning-top-solutions.md`) are referenced as canonical sources from `abstraction-form.md` and from this document. They are not really "concept" docs but they are referenced as such. Leave in place; revisit if more research reports accrue.

6. **Whether to write the `scripts/check_*` and `scripts/gen_adr_index.sh` in shell or Python.** Shell is more portable (matches the existing Makefile pattern); Python is more maintainable for nontrivial logic. Defer to the Wave 4 implementer's judgment.

---

## References

Surveyed projects, grouped by research category. Each entry is a verifiable URL fetched during the research phase.

**Methodology frameworks:**
- Diátaxis framework: <https://diataxis.fr/>, <https://diataxis.fr/map>
- Discussion of Diátaxis adoption in CPython: <https://discuss.python.org/t/adopting-the-diataxis-framework-for-python-documentation/15072>
- Arc42 template: <https://docs.arc42.org/>, <https://docs.arc42.org/section-9>
- Arc42 brief introduction (innoq): <https://www.innoq.com/en/blog/2022/08/brief-introduction-to-arc42>
- C4 model: <https://c4model.com>, <https://www.infoq.com/articles/C4-architecture-model>
- Nygard ADR (canonical 2011 post): <https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions>
- ADR GitHub org: <https://adr.github.io>
- adr-tools CLI: <https://github.com/npryce/adr-tools>

**Lifecycle theory (RFC / proposal pipelines):**
- Rust RFC process: <https://github.com/rust-lang/rfcs>
- TC39 proposal stages: <https://tc39.es/process-document/>
- Python PEP 1 (lifecycle definitions): <https://peps.python.org/pep-0001/>
- Kubernetes KEP process: <https://github.com/kubernetes/enhancements/blob/master/keps/README.md>

**Research-grade systems projects:**
- seL4 RFCs repo (lifecycle-as-directory pattern): <https://github.com/seL4/rfcs>
- seL4 manual repo: <https://github.com/seL4/seL4-manual>
- seL4 CAVEATS.md (root-level honesty): <https://github.com/seL4/seL4/blob/master/CAVEATS.md>
- seL4 CHANGES.md (compatibility labels): <https://github.com/seL4/seL4/blob/master/CHANGES.md>
- Lean 4 doc/ tree: <https://github.com/leanprover/lean4/tree/master/doc>
- Lean 4 examples (paired .out.expected): <https://github.com/leanprover/lean4/tree/master/doc/examples>
- Lean 4 CONTRIBUTING (failure-mode admission): <https://raw.githubusercontent.com/leanprover/lean4/master/CONTRIBUTING.md>
- Idris 2 docs/source/ tree: <https://github.com/idris-lang/Idris2/tree/main/docs/source>
- Idris 2 implementation/overview (self-described "sketchy"): <https://raw.githubusercontent.com/idris-lang/Idris2/main/docs/source/implementation/overview.rst>
- OpenBSD mdoc.7 style guide (mandatory sections): <https://man.openbsd.org/mdoc.7>
- OpenBSD FAQ1 (man pages enforced as part of code review): <https://www.openbsd.org/faq/faq1.html#ManPages>

**Large-scale production systems:**
- Rust std lib.rs (in-source doc): <https://github.com/rust-lang/rust/blob/main/library/std/src/lib.rs>
- Rustc dev guide SUMMARY.md (mdBook TOC): <https://github.com/rust-lang/rustc-dev-guide/blob/main/src/SUMMARY.md>
- Linux kernel Documentation/ (single Sphinx tree): <https://github.com/torvalds/linux/tree/master/Documentation>
- Linux kernel deprecated.rst (single registry): <https://github.com/torvalds/linux/blob/master/Documentation/process/deprecated.rst>
- Linux kernel staging/ (holding pen): <https://github.com/torvalds/linux/tree/master/Documentation/staging>
- Linux kernel maintainer-profile (audience justification): <https://github.com/torvalds/linux/blob/master/Documentation/doc-guide/maintainer-profile.rst>
- LLVM docs/ tree: <https://github.com/llvm/llvm-project/blob/main/llvm/docs/index.md>
- LLVM Lexicon.md (centralized glossary with anchors): <https://github.com/llvm/llvm-project/blob/main/llvm/docs/Lexicon.md>
- LLVM DeveloperPolicy (RFC process, patch reversion policy): <https://github.com/llvm/llvm-project/blob/main/llvm/docs/DeveloperPolicy.md>
- Svelte docs/ tree (numeric prefix ordering): <https://github.com/sveltejs/svelte/tree/main/documentation/docs>
- Svelte 99-legacy/ (back-compat APIs in current version): <https://github.com/sveltejs/svelte/blob/main/documentation/docs/99-legacy/00-legacy-overview.md>
- Svelte v5 migration guide (diff syntax + "why we did this"): <https://github.com/sveltejs/svelte/blob/main/documentation/docs/07-misc/07-v5-migration-guide.md>
- Svelte RFCs (FCP 3-day): <https://github.com/sveltejs/rfcs>

**Planex internal cross-references:**
- [`abstraction-form.md`](concepts/canonical/abstraction-form.md) — the three prerequisites this proposal serves (specifically Prerequisite 3: falsifiability)
- [`leak-budgets.md`](concepts/canonical/leak-budgets.md) — existing quantitative falsifiability mechanism; this proposal extends the same pattern to the doc layer
- [ADR-0010](decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) — honesty downgrade framing
- [ADR-0012](decisions/accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) — Q3 self-acknowledged gap (enforcement not in place); Principle 2 distributes this honesty per-ADR
- [ADR-0013](decisions/accepted/ADR-0013-v05-leak-budget-retire.md) — first exercised migration/deprecation cycle; this proposal is the doc-layer analogue

---

## Part IX — Cross-channel augmentation (added 2026-08-28)

> **Status**: Augmentation. Date: 2026-08-28.
> Supersedes nothing in Parts I-VIII; refines with net-new principles
> drawn from 5 additional research tracks beyond the original 13 projects
> surveyed. Applies incrementally; the 4-wave migration plan in Part VI
> is unaffected.
>
> **Research basis (5 new tracks, 23 additional sources)**:
> 1. **Documentation communities** — Write the Docs (WTD), The Good
>    Docs Project (GDP), The Turing Way. ~30 sources.
> 2. **Formal-methods projects** — Coq/Rocq, Isabelle, Agda, F*, Lean
>    Mathlib. ~25 sources.
> 3. **Software engineering books & standards** — Software Engineering
>    at Google (Ch. 10 Documentation), Ousterhout's A Philosophy of
>    Software Design (Ch. 13, 15), AOSA volumes, IETF RFC 7322/7841, GDD
>    tradition.
> 4. **Documentation site generators** — Antora, Docusaurus, mdBook,
>    Sphinx, GOV.UK Service Manual + Design System.
> 5. **C-language projects** — musl libc, SQLite, curl, Redis, lwIP.
>
> Full URLs in the new References section at the end of Part IX.

Parts I-VIII drew on 13 cross-domain top-tier projects (seL4, Rust,
Linux kernel, Lean 4, Idris 2, OpenBSD, LLVM, Svelte, Diátaxis, arc42,
C4, Nygard ADR, RFC/TC39/PEP/KEP lifecycles) and produced 8 design
principles + 4-wave migration plan. Part IX adds 6 net-new principles
that the 5 additional research tracks surfaced. None of them require
revisiting the migration; all apply incrementally to the post-Wave-4
tree.

The recurring meta-finding across all 5 new tracks: **a document is a
contract between an author and an absent reader, and the contract is
enforceable only when (a) the audience is named, (b) the status is
named, (c) the limitations are named, and (d) the doc is owned and
reviewed under the same workflow as code.** Parts I-VIII partially
satisfy (a)-(d); Part IX completes the set.

### Principle 9 — Per-abstraction template: "When to use / When not to use / Known issues"

**Source**: GOV.UK Design System component-page template — every
component page on `design-system.service.gov.uk/components/<name>/`
carries mandatory headings: `## When to use this component`, `## When
not to use this component`, `## How it works` (with `###` variants and
`#### Known issues` sub-sub-sections), `## Research on this component`,
`## Help improve this component`. The "When to use / When not to use"
pair is unique among all surveyed projects — no other source makes the
*scope of applicability* a mandatory, falsifiable per-page contract.

**Why it matters for Planex**: the falsifiability posture of
`abstraction-form.md` Prerequisite 3 requires that every formal-essence
abstraction declare its scope of applicability. Currently the scope is
implicit, derivable only by reading the ADR's Context + Consequences
sections. GOV.UK's pattern makes scope explicit and falsifiable.

**Implementation (this PR)**: extended `docs/decisions/TEMPLATE.md`
with `## When to use this decision` and `## When NOT to use this
decision` sections (immediately after `## Status`) and a `## Known
issues` section (immediately after `## CAVEATS`). The "Known issues"
section is distinct from CAVEATS (non-promises) and from Consequences
(expected downstream effects) — it records *accepted leaks* the
decision explicitly tolerates.

**Implementation status**: TEMPLATE.md amended; backfill of the 5
already-backfilled ADRs (0001, 0005, 0010, 0011, 0013) deferred to a
follow-up PR.

### Principle 10 — Status-of-This-Memo boilerplate per ADR

**Source**: IETF RFC 7841 §3.2-3.5 — every RFC carries a three-paragraph
"Status of This Memo" boilerplate that is *immutable* once published;
status changes during the document's lifetime (e.g., reclassified to
Historic) are recorded in the metadata referenced from the boilerplate,
not by editing the document itself (RFC 7841 §3.6 "Noteworthy"). The
prime editorial directive (RFC 7322 §2): "do not change the intended
meaning of the text".

**Why it matters for Planex**: the current `## Status` field in ADRs
(Proposed | Accepted | Deprecated | Superseded) is *mutable in-place*,
which means a reviewer cannot tell from a git diff alone whether a
Status change was a state transition or a typo fix. The RFC pattern
makes the lifecycle transition falsifiable by separating the immutable
"Status of This Memo" boilerplate from the mutable `## HISTORY` log.

**Implementation (this PR)**: added a `## Status of This Memo` block
to `TEMPLATE.md` (immediately after the header) that declares the
initial status (Proposed) and points to `## HISTORY` for the
authoritative current status. This block is immutable after Acceptance;
lifecycle transitions are recorded as new lines in `## HISTORY`, not
as edits to this block.

**Implementation status**: TEMPLATE.md amended; backfill of existing
ADRs deferred to a follow-up PR.

### Principle 11 — Mandatory sections enforced by CI (Mathlib docBlame)

**Source**: Lean Mathlib's `docBlame` / `docBlameThm` / `tacticDocs`
linters — CI fails the build when a definition, theorem, or tactic is
missing its mandatory docstring. Run with `#lint only docBlame`. This
is the only formal-methods project surveyed with *mechanically
enforced* mandatory docstring sections; all others (Coq, Isabelle,
Agda, F*) rely on authorial discipline alone.

**Why it matters for Planex**: ADR-0012 Q3 self-acknowledges that
Planex's "essence-justified" three-criterion is not yet CI-enforced.
Mathlib's `docBlame` pattern is the falsifiable template for closing
that gap at the doc layer: a missing mandatory section should fail the
build.

**Implementation (this PR)**: added `scripts/check_doc_sections.sh`
(checks every ADR for the 9 mandatory sections: Status, Context,
Decision, Consequences, Alternatives Considered, CAVEATS, Known
issues, HISTORY, References). `--check` mode is CI-fail; `--report`
mode is human-readable. Linter is currently informational; CI
integration deferred to follow-up PR (Part VI's `check_adr_lifecycle.sh`
is the parent hook).

**Implementation status**: linter shipped; CI integration pending.

### Principle 12 — "No 3s" review rubric with six named criteria

**Source**: Write the Docs conference CFP rubric
(<https://www.writethedocs.org/organizer-guide/confs/cfp/>) — scores
each submission on six criteria (Relevance, Originality, Soundness,
Quality of Presentation, Importance, Experience) using a 1-5 scale
that *explicitly forbids* a neutral middle: "Three means you don't
have an opinion. We don't believe you. No threes."

**Why it matters for Planex**: a review process that allows neutral
scores is unfalsifiable — a "+1 looks good to me" cannot be tested
against the next ADR's outcome. Forcing a committed verdict per
criterion produces a falsifiable review record that future reviews
can be compared against.

**Implementation (this PR)**: added `docs/decisions/REVIEW-RUBRIC.md`
with the 6 criteria adapted from WTD, the "no 3s" rule, the review
comment template, and the acceptance thresholds (≥4 per criterion for
abstraction-affecting ADRs; 2 approvals required).

**Implementation status**: rubric shipped; not yet wired into the PR
template (deferred to follow-up).

### Principle 13 — Paired template + template-guide (Good Docs Project)

**Source**: The Good Docs Project (thegooddocsproject.dev) — every
documentation template ships as a paired deliverable: the template
skeleton (`TEMPLATE.md`) plus a companion "template guide" web page
explaining *how to fill each section*. The pairing turns the template
into a falsifiable contract: the template tells you *what* sections to
write; the guide tells you *how to write each section well* and *how
to know when you've written it badly*.

**Why it matters for Planex**: without the pairing, every ADR author
interprets the template's intent slightly differently, and the
template's CAVEATS / HISTORY / Known issues sections drift with each
author's reading.

**Implementation (this PR)**: added `docs/decisions/TEMPLATE-GUIDE.md`
as the paired companion to `TEMPLATE.md`. The guide has a
section-by-section entry for each TEMPLATE section, with a
"Self-check" question per section, and a "Common failure modes" table
linking back to the rubric's six criteria.

**Implementation status**: shipped.

### Principle 14 — Document freshness front-matter (SE@Google)

**Source**: Software Engineering at Google Ch. 10 — every Google
internal doc carries a freshness metadata block:
`freshness: { owner: "username", reviewed: "YYYY-MM-DD" }`. Email
reminders fire when freshness expires. Documents without owners
become stale and difficult to maintain; the freshness field is the
mechanism that makes staleness mechanically detectable.

**Why it matters for Planex**: ADR-0012 Q3 self-acknowledges that the
"essence-justified" three-criterion is not yet enforced; one
consequence is that ADRs can drift silently for years. The
freshness pattern makes staleness falsifiable — a CI gate can flag
ADRs whose `reviewed:` date is older than 12 months for re-review.

**Implementation (this PR)**: added the freshness block as an HTML
comment in `TEMPLATE.md`'s front-matter (mirrors the SE@Google
HTML-comment convention, but using `<!-- ... -->` instead of the
proprietary `<!--* ... *-->` form Google uses internally). CI
freshness check is deferred (pending the same `check_adr_lifecycle.sh`
integration as Principle 11).

**Implementation status**: TEMPLATE.md front-matter amended;
backfill of existing ADRs deferred to a follow-up PR; CI gate
deferred.

### Principle 15 — Two-file split: CHANGELOG (raw) + UPGRADING (curated breaking)

**Source**: lwIP `UPGRADING` file
(<https://git.savannah.gnu.org/cgit/lwip.git/tree/UPGRADING>) — lwIP
maintains a 189 KB raw `CHANGELOG` (every commit) and a 12.9 KB
curated `UPGRADING` (breaking changes only, grouped as `++ Application
changes:` / `++ Port changes:` / `++ Repository changes:` per version).
The two-file split is the cleanest separation of *what happened*
(changelog) from *what you need to change in your code* (UPGRADING)
among all surveyed C-language projects. curl's `docs/DEPRECATE.md` and
Linux kernel's `Documentation/process/deprecated.rst` cover
*deprecations* but not breaking migrations; lwIP's UPGRADING covers both.

**Why it matters for Planex**: Planex currently has
`docs/changelog.md` (the raw history) and
`docs/reference/deprecation-registry.md` (API-level retirements), but
no curated *breaking-migration* document. A caller upgrading from v0.4
to v4 cannot find "what do I need to change in my code?" without
reading the changelog commit-by-commit. The lwIP pattern provides the
missing artifact.

**Implementation (this PR)**: added `UPGRADING.md` at the repo root
(visible at the same level as `CHANGELOG`/`README.md`, matching lwIP's
convention) with the v4 entries grouped as `++ API changes:` / `++
Internal changes:` / `++ Build changes:`. Distinct from
`deprecation-registry.md` (which covers individual symbol lifecycle)
and from `changelog.md` (which covers every commit).

**Implementation status**: shipped; v0.5 + future entries to be
appended in the same PRs that introduce breaking changes.

### Principle 16 — Visible "Planned content" tier (F* tutorial book)

**Source**: F* tutorial book (`fstar-lang.org/tutorial/book/structure.html`)
— the only formal-methods project surveyed that publicly declares its
unwritten chapters. The structure page carries a "This book is a work
in progress" banner followed by a "Planned content" bullet list naming
the chapters the authors intend to write (User-defined Effects, State,
Extraction, FAQ, etc.). All other surveyed projects (Coq, Isabelle,
Agda, Mathlib) simply omit unwritten content — gaps are invisible to
the reader.

**Why it matters for Planex**: Planex's `concepts/speculation/` directory
contains only `continuous-intent-speculation.md`. A reader cannot tell
whether this means "we have considered all the speculation we need" or
"we have not yet written the other speculation docs". The F* pattern
makes the gap visible: a "Planned content" section in the speculation
README declares which speculation docs are intended but not yet
written, so the gap is itself auditable.

**Implementation (this PR)**: added a `## Planned content` section to
`docs/concepts/speculation/README.md` listing 4 planned-but-unwritten
speculation docs (Breakdown, Interpretant, Complect-audit,
Closed-UI-corpus). Each entry is a commitment to *eventually* write
the doc; if a planned doc has no champion for two minor versions, it
should be removed from this list.

**Implementation status**: shipped.

### Principle 17 — Layered applicability sections (RFC 7322 + Ousterhout + GOV.UK)

**Sources**:
- RFC 7322 §4.8 mandates `Security Considerations` in *every* RFC,
  including the case where the answer is "no considerations"; the
  document is *returned for further development* if the claim is
  implausible.
- Ousterhout, *A Philosophy of Software Design* 2nd ed. Ch. 13:
  interface-comment block is mandatory at the top of every public
  module header; "without comments, you can't hide complexity".
- GOV.UK Design System: "When to use / When not to use" paired
  per-component (Principle 9 above).

**Why it matters for Planex**: these three sources converge on a
layered applicability contract: every formal Planex artifact should
declare (a) what it covers, (b) what it does not cover (CAVEATS,
Principle 2 / Part III), (c) when it applies and when it does not
(Principle 9), and (d) what it cannot promise (Known issues,
Principle 9). The full layered contract is now in `TEMPLATE.md`; the
existing CAVEATS+HISTORY backfill on ADR-0001/0005/0010/0011/0013 is
the partial fulfillment of (b) and (d).

**Implementation (this PR)**: TEMPLATE.md now carries the full
layered applicability contract. ADR-level backfill of the new sections
(When to use / When not to use / Known issues) deferred to a follow-up
PR — `scripts/check_doc_sections.sh --report` currently flags these
as missing on the 5 backfilled ADRs, which is the expected interim
state.

**Implementation status**: TEMPLATE.md complete; existing-ADR
backfill pending.

### Principle 18 — One-page Intent doc per subsystem (GDD tradition)

**Source**: Stone Librande's "one-page design" GDC 2010 talk (GDD
tradition). Every game subsystem gets a single readable page
distilling: vision statement (1 sentence), 3 design pillars, core
loop, non-goals, ASCII diagram. The page is what the team hangs on
the wall.

**Why it matters for Planex**: Planex's `docs/concepts/canonical/`
directory contains 4 normative position papers (`abstraction-form.md`,
`why-four-abstractions.md`, `leak-budgets.md`, `non-goals.md`) that
together run ~10,000+ words. A new contributor needs a *one-page*
distillation that orients them before they read any of those. The GDD
tradition supplies the format.

**Implementation (this PR)**: deferred — this requires a *new* doc
(`docs/concepts/canonical/intent.md`) which would need careful
authoring. Listed here as a tracked commitment; the README in
`concepts/canonical/` already serves as a partial landing page.

**Implementation status**: deferred to follow-up PR.

### Cross-cutting observation — Mathlib's "docstrings may lie slightly about implementation"

The most surprising single finding across all 5 research tracks: Mathlib's
style guide explicitly states *"Doc strings should convey the mathematical
meaning of the definition. They are allowed to lie slightly about the
actual implementation."* — a deliberately-scoped licence that elevates
the documented essence above the implementation. This is *exactly*
Planex's formal-essence stance, stated as a writing rule. Planex should
adopt this sentence almost verbatim in its own writing guide: a docstring
that describes what the function *means* (its essence) is correct, even
if it slightly mis-describes what the function *does* (its
implementation), as long as the gap is in the caller's favor (the doc
promises less than the implementation delivers).

This is *not* a license to lie; it is a license to *abstract*. The
formal-essence stance requires that the documented surface be the
abstraction; Mathlib's rule provides the writing-theory justification for
why a documented surface can be a strict abstraction of the
implementation without being identical to it.

**Implementation (this PR)**: noted here in Part IX as research finding;
explicit adoption into `TEMPLATE-GUIDE.md` or `CONTRIBUTING.md` deferred.

### Net-new artifacts shipped in this Part IX

| Artifact | Path | Principle |
|---|---|---|
| Extended TEMPLATE.md (freshness, Status-of-This-Memo, When to use, When NOT to use, Known issues) | `docs/decisions/TEMPLATE.md` | 9, 10, 13, 14 |
| TEMPLATE-GUIDE.md (paired companion) | `docs/decisions/TEMPLATE-GUIDE.md` | 13 |
| REVIEW-RUBRIC.md ("No 3s" rubric with 6 criteria) | `docs/decisions/REVIEW-RUBRIC.md` | 12 |
| check_doc_sections.sh (docBlame-style linter) | `scripts/check_doc_sections.sh` | 11 |
| UPGRADING.md (curated breaking-migration guide) | `UPGRADING.md` (repo root) | 15 |
| Planned content section in speculation README | `docs/concepts/speculation/README.md` | 16 |

### Deferred to follow-up PRs

| Task | Principle | Why deferred |
|---|---|---|
| ~~Backfill `## When to use / When not to use / Known issues` on ADR-0001, 0005, 0010, 0011, 0013~~ | ~~9, 17~~ | **Landed in commit `422b9f2` (T4).** All 13 ADRs backfilled with the 9-section contract; check_doc_sections.sh now passes 13/13. |
| ~~Backfill `## Status of This Memo` + `freshness:` front-matter on existing 13 ADRs~~ | ~~10, 14~~ | **Landed in commit `422b9f2` (T4).** Same backfill wave; all 13 ADRs carry RFC 7841 boilerplate. |
| ~~Wire `check_doc_sections.sh` into CI~~ | ~~11~~ | **Landed in commit (T6a).** See `.github/workflows/docs.yml` — Gates 1-6 (check_doc_sections, check_adr_lifecycle, gen_adr_index, check_links, find_orphans, check_terms) + Gate 7 (make check-completeness) compose into a single `Docs` workflow alongside the existing `CI` (code-build) workflow. Path filters limit the run to docs/examples/scripts/tests/Makefile touches so doc-only changes don't re-trigger the code build. |
| ~~Wire `REVIEW-RUBRIC.md` into PR template~~ | ~~12~~ | **Landed in commit (T6a).** See `.github/PULL_REQUEST_TEMPLATE.md` — the rubric table (6 criteria, score 3 forbidden) is inlined for ADR PRs; the 7 CI gates are inlined as a pre-merge checklist for any PR touching docs/. The rubric is human-applied at review time (the scoring requires judgment, not lint logic), while the gates are CI-applied. |
| ~~Adopt Mathlib's "docstrings may lie slightly" into writing guide~~ | ~~cross-cutting~~ | **Landed in commit (T6a).** See `TEMPLATE-GUIDE.md` § "Writing theory — Mathlib's 'docstrings may lie slightly about implementation'". The rule is restated verbatim, scoped to abstraction-affecting docs only (not utility code, per ADR-0011 counterexample 1), and paired with a 4-row failure-mode table + 3-question self-check. |
| ~~Write `docs/concepts/canonical/intent.md` (one-page Intent per subsystem)~~ | ~~18~~ | **Landed in commit (T6b).** See [`docs/concepts/canonical/intent.md`](concepts/canonical/intent.md) — one-page orientation sheet per Stone Librande's GDC 2010 "one-page design" format: vision statement (1 sentence), 3 design pillars with falsifiable tests, 5-abstraction table, ASCII core-loop diagram, non-goals paired with the pillar each protects, 3 prerequisites + fallback table, 9-gate CI contract table, reading order. Read first; everything else in `canonical/` defends it at length. |
| Svelte 99-legacy pattern (back-compat APIs in current version) | (original Wave 4) | Deferred per Part VIII |
| ~~TC39 Stage 2.7 named ADR state~~ | ~~(original Wave 4)~~ | **Landed as ADR-0014 (Validated) in commit (T6b) + commit (T7).** See [`decisions/validated/ADR-0014-validated-stage-and-essence-justified-enforcement.md`](decisions/validated/ADR-0014-validated-stage-and-essence-justified-enforcement.md) — adds a `Validated` lifecycle stage between `Proposed` and `Accepted` for ADRs that propose enforcement mechanisms. The stage requires the enforcement mechanism be implemented AND proven to fire on at least one synthetic violation case before the ADR can reach `Accepted`. ADR-0014 is itself subject to its own `Validated` stage; `scripts/check_essence_admission.sh` is implemented and demonstrated to fire on `tests/synthetic_adr_0015.md` (the synthetic case). The 10th CI gate (`essence-admission` job in `.github/workflows/docs.yml`) runs both `--check` on real ADRs and `--synthetic` on the synthetic case. Promotion to `Accepted` remains pending reviewer sign-off on the synthetic case as non-trivial per REVIEW-RUBRIC.md criterion 2 (Honesty). |
| ~~CI auto-check for essence-justified admission (ADR-0011's gap)~~ | ~~11~~ | **Landed in commit (T7).** See `scripts/check_essence_admission.sh` + `tests/synthetic_adr_0015.md` + the `essence-admission` job in `.github/workflows/docs.yml` (currently Gate 12; was Gate 10 at T7 landing, was Gate 11 after T8 inserted `check_stale_abstraction_count`, became Gate 12 after T10 inserted `check_limitations_freshness` — see T8 + T10 rows for renumbering history). Closes 2 of 3 sub-criteria in ADR-0011's essence-justified three-criterion (tradition-cite + negative-consequence); the third (denotational-semantics) remains reviewer-applied per REVIEW-RUBRIC.md criterion 3. The script's `--check` mode scans `decisions/{proposed,validated}/` ADRs with `## Essence Check` sections; the `--synthetic` mode runs on the synthetic case and expects non-zero exit (the falsifiability demonstration that the lint fires). |
| Rust RFC FCP 10-day window | (original Part VIII) | Single-maintainer, no audience |
| ~~arc42 numbered-section addressing~~ | ~~(original Part VIII)~~ | **Landed in commit (T6b)** as an additive framing layer: this Part IX deferral table is renumbered (~~struck-through items~~ in their original positions, new "Landed in commit" annotations inline). See <https://arc42.de/overview/> for the source convention. Planex does not adopt full arc42 (no template, no section numbering of the entire docs/ tree); only the deferral table is renumbered for tracking. The arc42 convention is most useful when a doc set has 100+ pages and section numbers aid cross-reference; Planex's current doc set (65 .md files) does not need it. The framing layer is *available* if the doc set grows past 100 pages. |
| ~~`check_links.sh`, `check_terms.sh`, `find_orphans.sh`, `check_adr_lifecycle.sh` (CI tooling)~~ | ~~11~~ | **Landed in commit `5d24758` (T5).** See `docs/README.md` § "CI tooling — doc-organization contract enforcers". Six gates now compose with `check_doc_sections.sh` + `gen_adr_index.sh` to form the Part VI acceptance contract. |
| ~~CI auto-check for stale abstraction-count references (CONTRIBUTING.md rule 5's grep, automated)~~ | ~~11~~ | **Landed in commit (T8).** See `scripts/check_stale_abstraction_count.sh` + the `check_stale_abstraction_count` step in `.github/workflows/docs.yml` (Gate 7). Closes the gap surfaced by commit `aa752e7` (docs(spec): refresh stale 4-abstraction references in v0.5-current docs) — that commit ran CONTRIBUTING.md rule 5's grep manually and found 17 stale references; this script makes the grep self-enforcing. The script scans all v0.5-current markdown (exempting ADRs / changelog / research / `docs/concepts/history/` / `docs/concepts/background/` / `v0.4-roadmap.md` / `ui-pattern-coverage.md` / `roadmap-matrix.md` / `why-four-abstractions.md` as historical snapshots), detects two patterns (literal "4/four abstractions" and "Relation + Estimate + Closure + Perception" without "+ px_loop"), and honors the inline marker `<!-- stale-allow: reason -->` for intentional historical quotations. Markdown link text matching `[...Four abstractions...]` (any case) is auto-exempted as a reference to the `why-four-abstractions.md` filename. Six inline markers were added in the same commit (4 in CONTRIBUTING.md rule 4/5/9 LESSONs + summary row, 1 in `state/README.md` describing the v0.4 snapshot, 1 in `state/limitations.md` quoting the v0.1.0 historical claim). The gate runs as the 7th shell gate in the `doc-org-contract` job (alongside the original 6 shell gates). Renumbering history: when this gate landed in T8, it was Gate 8 and pushed the make-target gates to 9-11; when the 8th shell gate (`check_limitations_freshness`, T10) landed, it took over Gate 8 and pushed the make-target gates to 9-12. The current numbering is `7 shell + 1 fresh-check + 4 make = 12 gates` (monotonic across the YAML file). <!-- stale-allow: this deferral row describes the script's detection patterns; "4 abstractions" / "Relation + Estimate + Closure + Perception" are the literal stale-pattern strings the script grep's for --> |
| ~~Wire `test_v05_retire` into ci.yml (close ADR-0013 falsifiability gap)~~ | ~~11~~ | **Landed in commit (T9).** `tests/test_v05_retire.c` (validates that the 7 L2 leaks retired in v0.5 — 4 Estimate + 3 Perception — are actually closed, per ADR-0013 + `canonical/leak-budgets.md` §2 + §4) was built by the Makefile (`make test_v05`) but **no CI job invoked it** — a regression re-introducing an L2 leak would not break the build. This commit closes that gap by (a) registering `test_v05_retire` as a CMake executable + `add_test` entry in `CMakeLists.txt` (previously Makefile-only), and (b) wiring the test into all 4 ci.yml jobs: `linux-cmake` runs `./build/test_v05_retire`; `linux-make` + `strict-warnings` run `make test_v05`; `windows` runs `.\build\Release\test_v05_retire.exe`. Local run: 12/12 passed; v0.5 retire verified (shipping L2 count: 9 → 2 = 3.8%, target ≤8% MET). `test_v3_prototype` and `test_v4_orthogonality` are CMake-built but intentionally **not** CI-enforced — they pressure-test v3/v4 proposal-stage designs (ADR-0009 Proposed, ADR-0012 accepted-but-not-promoted); running them locally is documented in `canonical/intent.md`'s CI contract section.
| ~~CI auto-check for stale capability-claims (limitations.md / canonical/ freshness)~~ | ~~11~~ | **Landed in commit (T10).** See `scripts/check_limitations_freshness.sh` + the `check_limitations_freshness` step in `.github/workflows/docs.yml` (Gate 8). Closes the gap surfaced when the post-v0.5 audit found `limitations.md` L2 (the literal phrase `undo-via-graph has not been implemented`), L3 (the literal phrase `No anti-pattern tests for any abstraction`), and L4 (the literal phrase `no px_undo() API exists`) all stale — the implementations had landed in `examples/` and `src/` but the limitations entries had not been updated to RESOLVED. That audit fixed the entries by hand; this script makes the freshness check self-enforcing instead of human-run-on-demand. The script is the capability-status analogue of `check_stale_abstraction_count.sh` (which catches abstraction-count drift); it detects three patterns paired with reality-checks: (1) undo-via-graph + the literal phrase `has not been implemented` + `examples/undo_via_graph.c` exists; (2) the literal phrase `No anti-pattern tests` + `examples/antipattern_*.c` glob matches; (1b) the literal phrase `no px_undo() API` + `src/undo.c` declares it. Historical files (ADRs / changelog / research / `history/` / `background/` / v0.4 snapshots / `why-four-abstractions.md`) are exempt; intentional historical quotations (`previously red` past-tense) can be marked inline with `<!-- fresh-allow: reason -->` (analogous to `check_stale_abstraction_count.sh`'s `<!-- stale-allow: reason -->` marker). Two inline markers were added in the same commit: `limitations.md:41` (P2.4 demo-content caveat, 🔴 refers to demo freshness not non-existence) + `limitations.md:92` (`previously 🔴` past-tense historical narrative). The gate runs as the 8th shell gate in the `doc-org-contract` job (after `check_stale_abstraction_count` as Gate 7); the 4 make-target gates were renumbered: `check-completeness` → Gate 9, `check-compression` → Gate 10, `check-examples` → Gate 11, `check-essence` → Gate 12. The `8 shell + 4 make = 12 gates` numbering is monotonic across the YAML file. P0 P0.1 P0.2 P0.3 P0.4 (limitations.md L2/L3/L4 + ADR-0002 Resolution section + abstraction-form.md L2 rate 17%→3.8%) were all fixed in the same commit to bring the docs back into fresh-state before the gate went live. |

### Additional References (Part IX sources)

**Documentation communities:**
- Write the Docs (Docs-as-Code definition + 5-element toolchain): <https://www.writethedocs.org/guide/docs-as-code/>
- Write the Docs CFP rubric ("no 3s" rule, 6 criteria): <https://www.writethedocs.org/organizer-guide/confs/cfp/>
- Write the Docs topics 2-axis taxonomy (content type × lifecycle stage): <https://www.writethedocs.org/topics/>
- The Good Docs Project templates (paired template + template-guide): <https://www.thegooddocsproject.dev/template>
- The Good Docs Project README.md (3-type Concept/Task/Reference variant): <https://github.com/thegooddocsproject/templates>
- The Turing Way consistency checklist (hard/soft, CI-enforced): <https://book.the-turing-way.org/community-handbook/style/style-consistency>
- The Turing Way chapter anatomy (Landing → Subchapters → Checklist → Resources): <https://book.the-turing-way.org/community-handbook/style/consistency/consistency-structure>
- The Turing Way reproducible-project-template (cookiecutter scaffold): <https://github.com/the-turing-way/reproducible-project-template>

**Formal-methods projects:**
- Rocq reference manual (Sphinx build, post-migration): <https://rocq-prover.org/refman>
- Rocq `coqdoc` tool (separate API pipeline from `.glob` files): <https://rocq-prover.org/doc/V8.12.2/refman/using/tools/coqdoc.html>
- Rocq RFCs (Coq CEPs renamed): <https://github.com/rocq-prover/rfcs>
- Isabelle documentation index (audience-sliced manual set): <https://isabelle.in.tum.de/documentation.html>
- Isabelle Implementation Manual (whitespace-as-structure): <https://www.cl.cam.ac.uk/research/hvg/Isabelle/dist/Isabelle/doc/implementation.pdf>
- Agda user manual (Sphinx, literate extensions): <https://agda.readthedocs.io/en/v2.8.0/tools/literate-programming.html>
- Agda user manual `index.rst` (5-section toctree): <https://raw.githubusercontent.com/agda/agda/master/doc/user-manual/index.rst>
- F* tutorial book (Planned content + WIP banner): <https://fstar-lang.org/tutorial/book/structure.html>
- F* `book/code/` (1:1 chapter-to-source mapping): <https://github.com/FStarLang/fstarlang.github.io/tree/master/book/code>
- Mathlib contribute/doc.html (docBlame linter, ordered module docstring sections): <https://leanprover-community.github.io/contribute/doc.html>
- Mathlib contribute/style.html (`/-!` delimiter rule, "docstrings may lie slightly"): <https://leanprover-community.github.io/contribute/style.html>

**Software engineering books & standards:**
- Software Engineering at Google Ch. 10 (Documentation, freshness pattern): <https://abseil.io/resources/swe-book/html/ch10.html>
- Software Engineering at Google Ch. 3 (Knowledge Sharing, canonical sources): <https://abseil.io/resources/swe-book/html/ch03.html>
- Ousterhout APoSD 2nd ed. (Stanford book page): <https://web.stanford.edu/~ouster/cgi-bin/book.php>
- AOSA Volume I Berkeley DB chapter (per-chapter skeleton): <https://aosabook.org/en/v1/bdb.html>
- AOSA homepage (volumes I, II, POSA, 500 Lines): <https://aosabook.org/>
- RFC 7322 (Internet Style Guide, mandatory RFC structure): <https://www.rfc-editor.org/info/rfc7322>
- RFC 7841 (RFC Streams, Status of This Memo boilerplate): <https://www.rfc-editor.org/info/rfc7841>
- authors.ietf.org (mandatory Internet-Draft sections): <https://authors.ietf.org/required-content>
- GDD tradition (one-page GDD, living GDD vs snapshot GDD): <https://www.gamedeveloper.com/design/how-to-write-a-game-design-document>

**Documentation site generators:**
- Antora `antora.yml` per-component descriptor: <https://docs.antora.org/antora/latest/component-version-descriptor/>
- Antora standard family directories (pages/partials/examples/images/attachments): <https://docs.antora.org/antora/latest/standard-directories/>
- Antora playbook (separate content-free configuration repo): <https://docs.antora.org/antora/latest/playbook/>
- Docusaurus `_category_.json` per-directory metadata: <https://docusaurus.io/docs/sidebar/autogenerated>
- Docusaurus versioning convention (`versioned_docs/version-X.Y.Z/`): <https://docusaurus.io/docs/versioning>
- mdBook `src/SUMMARY.md` as single ToC source-of-truth: <https://rust-lang.github.io/mdBook/format/summary.html>
- mdBook `{{#include file.rs:2:10}}` line-range includes: <https://rust-lang.github.io/mdBook/format/mdbook.html>
- Sphinx `conf.py` as executable Python: <https://www.sphinx-doc.org/en/master/usage/configuration.html>
- Sphinx `.. toctree::` directive: <https://www.sphinx-doc.org/en/master/usage/index.html>
- Sphinx autodoc + intersphinx (`objects.inv`): <https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html>
- GOV.UK Government Design Principles (10 principles, esp. "Make things open"): <https://www.gov.uk/guidance/government-design-principles>
- GOV.UK Design System button component (mandatory headings): <https://design-system.service.gov.uk/components/button/>
- GOV.UK Design System radios component (`#### Known issues` under variants): <https://design-system.service.gov.uk/components/radios/>
- GOV.UK Design System contribution process (RFC-style per-page): <https://design-system.service.gov.uk/community/>

**C-language projects:**
- musl libc repo root (no `docs/` dir, `WHATSNEW` only): <https://git.musl-libc.org/cgit/musl/tree/>
- SQLite docs.html (master index, narrative website): <https://www.sqlite.org/docs.html>
- SQLite arch.html (architecture block diagram): <https://www.sqlite.org/arch.html>
- SQLite amalgamation.html (amalgamation build + justification): <https://www.sqlite.org/amalgamation.html>
- curl `docs/libcurl/` (one `.md` per public symbol): <https://github.com/curl/curl/tree/master/docs/libcurl>
- curl `docs/cmdline-opts/MANPAGE.md` (managen generator): <https://github.com/curl/curl/blob/master/docs/cmdline-opts/MANPAGE.md>
- curl `docs/internals/CODE_STYLE.md` (verifiable style guide): <https://github.com/curl/curl/tree/master/docs/internals>
- Redis `src/commands/get.json` (JSON-per-symbol with reply_schema): <https://github.com/redis/redis/blob/unstable/src/commands/get.json>
- Redis `src/commands/README.md` (commands.def generation): <https://github.com/redis/redis/blob/unstable/src/commands/README.md>
- Redis `MANIFESTO` plain-text design-manifesto at root: <https://github.com/redis/redis/blob/unstable/MANIFESTO>
- lwIP `UPGRADING` (research basis for Principle 15, two-file split with `CHANGELOG`): <https://git.savannah.gnu.org/cgit/lwip.git/tree/UPGRADING>
- lwIP `doc/doxygen/main_page.h` (Doxygen main page as C header): <https://git.savannah.gnu.org/cgit/lwip.git/tree/doc/doxygen/main_page.h>
- lwIP `CHANGELOG` (raw 189 KB): <https://git.savannah.gnu.org/cgit/lwip.git/tree/CHANGELOG>

---

*End of document. The proposal is open for review; comments should be filed as issues against the Planex repository. The four waves are designed to ship in order over four PRs; Wave 1 is the lowest-risk first move and can ship immediately on acceptance. Part IX (Cross-channel augmentation) layers on top of the 4-wave migration without revisiting it; the 6 net-new principles (9-18) apply incrementally to the post-Wave-4 tree.*