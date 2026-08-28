# Documentation Versioning & Knowledge Management — Top-tier Industry Solutions Survey

> **Author**: Planex docs engineering
> **Date**: 2025-08-28
> **Status**: Reference (research output, not a decision)
> **Applies to**: v0.4
> **Scope**: Survey of industry frameworks, platforms, and project practices for documentation versioning and engineering knowledge management, with gap analysis and recommendations for Planex.

---

## Executive Summary

This report surveys eight foundational traditions in technical documentation (Diátaxis, Docs as Code, SemVer for Documents, ADRs, C4 + arc42, Literate Programming, RFC/PEP process, SSoT/DRY), six major documentation platforms (MkDocs Material, Docusaurus, GitBook, Sphinx, Backstage TechDocs, Docsy), and five large-project case studies (Linux Kernel, Kubernetes, Python, Rust, arXiv). The survey confirms that Planex's three-system numbering convention (document version v1–v4 / section number Part I–IX / software version v0.4) is *not* a defect — it is the natural state of any project that separates concept history from software releases. arXiv, Python PEPs, Rust RFCs, and ADRs all use the same pattern: immutable snapshots for concept history, separate versioning for software. The key gap is not numbering but **stability signaling**: Planex lacks a rust-lang-style "experimental/unstable/stable/legacy" field on concept docs, and lacks "Superseded by" cross-links on ADRs. Three tiers of recommendations follow, ranging from immediate low-cost additions to long-term platform migration.

---

## 1. Context: Planex's Three-System Problem

Task 12 (commit `f4fdc85`) formalized Planex's three numbering systems:

1. **Document version** (`v1`–`v4`): tracks revisions of a single concept document, e.g., `essence-derivation-v1.md` through `essence-derivation-v4-clean.md`. Each version is an immutable snapshot — v4 does not replace v1, it sits alongside it as a separate attempt.
2. **Section number** (`Part I`–`Part IX`): tracks chapters within a single document. Roman numerals signal "this is a section identifier, not a version".
3. **Software version** (`v0.4`): tracks the released C library. Arabic-with-dot signals "this is a software version, not a document version".

The dot is the disambiguator. ADR-0010 then formalized that v4 essence-derivation is *design rationale*, not *essence discovery* — meaning the four v1–v4 documents are conceptually distinct attempts, not incremental refinements. The question this report answers: **does industry practice validate this three-system model, and what does industry do that Planex does not?**

---

## 2. Methodology

This survey used the `web_search` SDK function to query 17 distinct topics, returning 8–10 results per query (≈150 candidate sources). Six canonical sources were then fetched in full via the `page_reader` function for deeper extraction: diataxis.fr, docs.divio.com/documentation-system, semverdoc.org, adr.github.io, arc42.org/overview, and writethedocs.org/guide/docs-as-code. Search topics spanned: Diátaxis, Divio, Docs as Code, SemVer for docs, ADR ecosystem, C4 model, arc42, literate programming, RFC/PEP/TC39 process, SSoT/DRY, Linux kernel docs, Kubernetes multi-version docs, MkDocs/Docusaurus/GitBook/Backstage comparison, Doxygen for C, documentation lifecycle and debt, arXiv versioning, and Zettelkasten/Second Brain knowledge graphs. Selection criteria for inclusion in the synthesis: (a) the source is the canonical/authoritative origin of the practice, (b) the practice is widely adopted by ≥3 major projects, (c) the practice is directly applicable to a single-maintainer C library project (filtering out enterprise-scale-only practices).

---

## 3. Eight Foundational Traditions

### 3.1 Diátaxis / Divio Documentation System

Diátaxis (originally the "Divio Documentation System", created by Daniele Procida around 2017–2019, renamed "Diátaxis" in 2020) classifies documentation into four types along two axes: **action vs knowledge** and **acquisition vs use**. The four types are: *tutorials* (learning-oriented, action, acquisition), *how-to guides* (problem-oriented, action, use), *reference* (information-oriented, knowledge, use), and *explanation* (understanding-oriented, knowledge, acquisition). Diátaxis is adopted by Django, Cloudflare, Qt, Gatsby, and the Python documentation overhaul proposal (PEP-style discussion in 2022). The framework's central insight is that documentation is not a monolith — different reader needs require different writing styles, and conflating them produces mediocre docs.

Planex already has the four directories: `docs/tutorials/`, `docs/how-to/`, `docs/reference/`, `docs/concepts/`. The mapping is natural: `concepts/` is the "explanation" quadrant. The gap is that this mapping is *implicit* — there is no README in each directory stating "this is the X quadrant of Diátaxis" and the boundaries are not enforced. A low-cost recommendation is to add a single sentence per directory header naming the Diátaxis type, so contributors know which style to write in.

### 3.2 Docs as Code

Docs as Code is a philosophy (popularized by the Write the Docs community around 2013–2016, and by GitHub's own practice) that documentation should be written with the same tools and workflows as source code: plain-text markup (Markdown/AsciiDoc/rST), version control in Git, code review through pull requests, CI/CD for building and deploying, and issue tracking for doc work. The Write the Docs declaration states it captures "a philosophy that you should be writing documentation with the same tools as code: Issue Trackers, Version Control Systems, plain text markup". Adopters include Stripe, Spotify (Backstage TechDocs), GitLab, and most large open-source projects.

Planex is already a Docs as Code project — all documentation lives in the same Git repository as the C source, is reviewed in PRs, and is plain Markdown. The remaining gap is *automation*: there is no CI step that validates links, builds a preview site, or lints the Markdown. Adding a `markdown-link-check` step to the existing `.github/workflows/ci.yml` is the single highest-leverage Docs-as-Code improvement available.

### 3.3 SemVer for Documents (semverdoc.org)

This is the most directly relevant finding for Planex's version-numbering question. The SemVerDoc project (semverdoc.org) explicitly adapts Semantic Versioning for documents, recognizing that software SemVer (`MAJOR.MINOR.PATCH` where `MINOR` is "new functionality added in a backwards-compatible manner") does not fit documents. The SemVerDoc rule is: **MAJOR** when the document has undergone significant changes; **MINOR** when information has been added or removed; **PATCH** when minor changes (e.g., fixing typos). The critical difference from software SemVer is that "information removed" triggers a MINOR bump — because in documents, removing information is a meaningful change for readers, even if it is not a "breaking change" in the software sense.

For Planex, this is the key insight: the v1–v4 integer versioning on `essence-derivation-*.md` does not need to migrate to SemVerDoc, because v1 through v4 are *conceptually different attempts* (different methodologies, different framings) — they are not minor revisions of the same document. They are more like arXiv preprint versions, where each version is a distinct intellectual artifact preserved for the historical record. However, SemVerDoc is the right convention for the *software version*: bumping `v0.4` → `v0.5` should follow the SemVerDoc "MINOR = information added/removed" rule when essence categories change, not the software SemVer "MINOR = backwards-compatible feature added" rule.

### 3.4 Architectural Decision Records (ADRs)

ADR is the most mature practice Planex has already adopted. The format originates in Michael Nygard's 2011 article "Documenting Architecture Decisions" (Reactive Systems blog). The canonical home is `adr.github.io`, which defines: "An Architectural Decision (AD) is a justified design choice that addresses a functional or non-functional requirement that is architecturally significant. An Architectural Decision Record (ADR) captures a single AD and its rationale; the collection of ADRs created and maintained in a project constitute its decision log." The Nygard template is: Title, Status, Context, Decision, Consequences — five sections, deliberately lightweight.

Planex's ADRs (ADR-0001 through ADR-0010) already *exceed* the Nygard baseline. ADR-0010 in particular adds: Forces, Verification, Essence Check, Scope, Alternatives, References — a richer template appropriate for a project where every decision is contested. The gap relative to industry is small: adr.github.io recommends explicit "Superseded by" / "Supersedes" cross-links when an ADR is replaced, and Planex currently uses Status text ("Proposed", "Accepted", "Superseded by ADR-0010") without a machine-readable field. A low-cost improvement is to add a `Superseded by:` field to the ADR template.

### 3.5 C4 Model + arc42 Template

The C4 model (created by Simon Brown, c4model.com) is a hierarchical approach to software architecture visualization: Context, Container, Component, Code — four levels of zoom, each with its own diagram type. The arc42 template (arc42.org, created by Gernot Starke and Peter Hruska) is a 12-section architecture documentation template: Introduction, Context, Context Scope, Solution Strategy, Building Block View, Runtime View, Deployment View, Cross-cutting Concepts, Architectural Decisions, Quality Requirements, Risks, Glossary. Both are widely adopted in enterprise architecture documentation.

For Planex — a single C library with ~6 essence categories — full C4 or arc42 is overkill. But the *discipline* of arc42 is valuable: separating "context" (what surrounds Planex) from "building blocks" (the 4+1 abstractions) from "decisions" (ADRs) from "glossary" is exactly what Planex already does informally. The recommendation is not to adopt arc42 wholesale, but to use its section list as a *checklist* when reviewing whether `docs/concepts/` is complete: if any arc42 section has no Planex equivalent, that is a coverage gap.

### 3.6 Literate Programming

Donald Knuth introduced Literate Programming in his 1984 book of the same name. The core idea: programs are written as documents that explain the program to humans, with code embedded as fragments in a sequence chosen for pedagogical clarity (not compiler order). Tools include WEB (Knuth's original), CWEB (C variant), noweb (simpler), and modern descendants like Org-babel, Jupyter notebooks, and R Markdown. Knuth's claim: "literate programming provides a first-rate documentation system, which is not an add-on, but is grown naturally in the process of writing the program".

Planex's v4 essence derivation already moves in this direction: the prose-first structure of `essence-derivation-v4-clean.md` (rationale block → code that emerges from rationale) is literate programming in spirit, even if the implementation is two separate files. The recommendation is to formalize this for v5: every essence category's "why does this exist" should be a prose block that the corresponding C header comment references, so the rationale is co-located with the code rather than in a separate doc.

### 3.7 RFC / PEP / TC39 Process

The RFC (Request for Comments) tradition in software design predates software itself (IETF RFCs since 1969). Within modern open source, three variants are canonical: **Python PEPs** (Python Enhancement Proposals, since 2000 — PEP 0 is the index, PEP 1 is the process, PEPs are immutable once Final); **Rust RFCs** (rust-lang/rfcs repo, with FCP "final comment period" mechanism, since 2014); **TC39 proposals** (JavaScript, staged 0→4, since 2014). All share: a numbered proposal document, a status lifecycle (Draft → Review → Accepted/Rejected → Implemented), an immutable record once accepted, and a separate "decision log" that records the final disposition. The proposal template typically includes: Summary, Motivation, Detailed design, Drawbacks, Alternatives, Prior art, Unresolved questions.

Planex currently uses ADRs for both *proposing* and *deciding*. As the project grows and proposals become more contentious (the v3 → v4 transition, captured in ADR-0009 → ADR-0010, is a case study in this), it may be worth splitting the two: an RFC track for "we are proposing this" and the ADR track for "we have decided this". The RFC document would carry the design exploration; the ADR would record the decision and reference the RFC. This is exactly the Rust pattern: an RFC text/NNNN.md file, then once accepted, the corresponding ADR/decision-record updates.

### 3.8 Single Source of Truth & DRY

"Every piece of knowledge must have a single, unambiguous, authoritative representation within a system" — this is the DRY principle from Hunt & Thomas's *The Pragmatic Programmer* (1999). The Single Source of Truth (SSoT) pattern extends DRY from code to data and documentation: there is one canonical place where each fact lives, and every other reference points to it rather than duplicating. Wikipedia notes: "Single source of truth (SSOT) architecture is the practice of structuring information models and associated data schemas such that every data element is stored exactly once".

Planex's Task 12 implementation of "Applies to: v0.4" front-matter on every concept doc is exactly an SSoT move: it makes the software version the single source of truth that every concept doc references, rather than each concept doc carrying its own implicit version assumption. The remaining gap is cross-document: when `architecture.md` mentions "the four abstractions", it should link to `why-four-abstractions.md` rather than re-explaining. A low-cost improvement is a CI link-checker that catches dead cross-references.

---

## 4. Platform Survey

Six major documentation platforms were surveyed:

| Platform | Stack | Strength | Adopter Profile |
|---|---|---|---|
| **MkDocs Material** | Python + Jinja2 | Low overhead, GitHub Pages friendly, mature | FastAPI, FastAPI-Contrib, many Python projects |
| **Docusaurus** | React + Node | Rich React components, MDX support | Meta projects, Babel, Jest |
| **GitBook** | SaaS / Markdown | Hosted, no build step, multi-language | Many startups, open-source projects |
| **Sphinx** | Python + reST | Mature, multi-version, kernel-grade | Python, Linux kernel, LLVM |
| **Backstage TechDocs** | Node + MkDocs | Integrated with developer portal | Spotify, Expedia, Netflix |
| **Docsy** | Hugo + Go | Multi-version per release branch, K8s-style | Kubernetes, Helm, OpenTelemetry |

For Planex (single-maintainer C library, GitHub-hosted, all docs currently Markdown): **MkDocs Material is the right platform if a generated site is ever needed**. It has the lowest setup cost (one `mkdocs.yml` + GitHub Action), supports C Doxygen integration via `mkdoxy` plugin, and aligns with the existing Markdown corpus without conversion. Sphinx would be the right choice if Planex grows into a multi-component ecosystem needing Doxygen-grade API reference generation. Backstage TechDocs is overkill — it is designed for engineering organizations managing dozens of services, not single libraries. GitBook and Docusaurus both work but add a non-trivial build dependency for limited gain over GitHub-rendered Markdown.

---

## 5. Large Project Case Studies

### 5.1 Linux Kernel

The Linux kernel documentation system (docs.kernel.org) uses Sphinx to build reStructuredText under `Documentation/`. Two key practices: (a) inline `kernel-doc` comments in C source files are extracted as structured API reference (a literate-programming-adjacent pattern), and (b) `Documentation/` serves as the maintainer handbook. Multi-version hosting uses `sphinx-contrib/multiversion` to build all release branches into a single site with version switching. The kernel's lesson for Planex: the API reference should be generated *from* source comments where possible (Doxygen for C), rather than hand-maintained in `docs/reference/api.md`.

### 5.2 Kubernetes

Kubernetes uses Docsy (Hugo) for kubernetes.io/docs, with multi-version sites served per release branch (e.g., `v1-33.docs.kubernetes.io`). The pattern: each release branch gets its own doc snapshot, and the URL prefix encodes the version. This is heavyweight infrastructure for a small project like Planex, but the underlying principle — "readers can reach the doc version that matches their installed software version" — is achievable at small scale simply by tagging the repo at each release and pointing readers at the tag.

### 5.3 Python

Python combines PEPs (Python Enhancement Proposals) for design proposals with Sphinx-built docs.python.org for reference and tutorials. PEPs are immutable once Final; the PEP 0 index tracks all PEPs and their statuses. This is the RFC pattern (§3.7) plus a strong Diátaxis-aligned doc site. The PEP 0 index is the canonical model for an "ADR index page" — Planex's `docs/decisions/README.md` already mirrors this pattern correctly.

### 5.4 Rust

Rust combines the RFC process (rust-lang/rfcs, with FCP "final comment period") with three published books: The Rust Programming Language, The Rust Reference, and The Rustonomicon. RFCs are immutable text files (`text/NNNN-description.md`) with a status field. Once accepted, the implementation work proceeds as normal PRs, and the RFC remains as the historical record of the design rationale. The Rust model is the closest industry analog to what Planex's v4 essence derivation already does — prose-first design rationale, preserved as immutable snapshot, with the code emerging from the rationale rather than the other way around.

### 5.5 arXiv

arXiv's versioning model (info.arxiv.org/help/versions.html) is *exactly* the pattern Planex's `essence-derivation-v1` through `v4` files follow. Each arXiv submission has a version number (v1, v2, v3, ...), every version is preserved permanently, and each version is essentially a separate intellectual artifact (revisions of a paper, not patches to it). The arXiv model explicitly avoids SemVer: the version is just an integer, and the historical record is preserved. This validates Planex's choice of integer versioning for essence-derivation documents — SemVerDoc (§3.3) is the wrong fit here, because v1–v4 are *re-derivations*, not minor edits.

---

## 6. Cross-cutting Patterns (Synthesis)

Five cross-cutting patterns recur across all surveyed projects:

1. **Layered versioning**: every mature project separates document version from software version. Planex's three-system rule (Task 12) is industry-standard, not a defect.
2. **Immutable snapshots for concept history**: arXiv, PEPs, Rust RFCs, and Planex's `essence-derivation-v1..v4` all preserve every version forever. Never overwrite a concept doc — write a new version alongside.
3. **Status lifecycle fields**: Proposed → Accepted → Deprecated/Superseded. Planex's ADRs have this; concept docs do not.
4. **Decision-log + concept-doc pairs**: ADRs record *what was decided and why*; concept docs record *what is true now*. Planex already has this separation (`docs/decisions/` vs `docs/concepts/`).
5. **"Applies to" front-matter linking docs to specific software versions**: Task 12 added this to Planex; this is also the rust-lang/cfg pattern (each unstable feature doc carries `tracking issue: NNN` and `stability: unstable`).

---

## 7. Gap Analysis: Planex vs Industry

### What Planex already does at industry standard

- **ADRs** (10 records, Nygard+ template with Forces/Verification/Essence Check/Scope/Alternatives/References) — exceeds baseline
- **Docs as Code** (Markdown in Git, PR-reviewed, plain text)
- **"Applies to" front-matter** on 10 concept docs (Task 12)
- **Multi-version concept docs** (essence-derivation-v1..v4 — arXiv model)
- **Versioning.md rule book** (Task 12 — three-system disambiguation)
- **Decision-log / concept-doc separation** (`docs/decisions/` vs `docs/concepts/`)
- **Diátaxis-aligned directory structure** (implicit: tutorials/how-to/reference/concepts)

### What Planex lacks vs industry

1. **No explicit "Stability" field** on concept docs (rust-lang-style: experimental/unstable/stable/legacy). Without this, a reader cannot tell at a glance whether `essence-derivation-v4-clean.md` is the current canonical truth or an experimental proposal.
2. **No machine-readable "Superseded by" / "Supersedes" cross-link** on ADRs. Planex uses Status text ("Superseded by ADR-0010") but no structured field. adr.github.io recommends an explicit field.
3. **No CI doc validation**: no link-checking, no Markdown linting, no preview-site build. This is the single highest-leverage Docs-as-Code improvement.
4. **No formal RFC track separate from ADRs**: as proposals grow more contentious (the v3→v4 transition is a preview), the absence of a "proposal" document type forces all design exploration to happen in ADR-0009-style Proposed records, which mixes two roles.
5. **No generated API reference**: `docs/reference/api.md` is hand-maintained; the Linux kernel and Python ecosystems generate API docs from source comments (Doxygen for C). This risks drift between source and reference.

---

## 8. Recommendations

### Tier 1 — Immediate (low cost, high value, ≤1 day each)

1. **Add "Stability" field to concept docs**. Convention: `experimental | unstable | stable | legacy`. Apply to all 10 concept docs: most are `stable`, `essence-derivation-v4-clean.md` is `stable` (current truth), `essence-derivation-v1/v2/v3.md` are `legacy` (historical record), `v0.4-roadmap.md` is `unstable` (forward-looking).
2. **Add "Superseded by" / "Supersedes" structured field to ADR template**. ADR-0009 should carry `Superseded by: ADR-0010` as a YAML-style front-matter field, not just as Status text.
3. **Add Diátaxis directory headers**. One README or one-line header in each of `docs/{tutorials,how-to,reference,concepts}/` naming the Diátaxis quadrant and the writing style.
4. **Add CI link-checker**. Add a `markdown-link-check` step to `.github/workflows/ci.yml`. Catches dead cross-references before merge.

### Tier 2 — Medium-term (process formalization, 1–2 weeks each)

1. **Adopt SemVerDoc rule for software version bumps**. When essence categories are added/removed, bump MINOR (v0.4 → v0.5). When a category's interface breaks (e.g., `px_breakdown` signature change), bump MAJOR (v0.x → v1.0). When only docs/comments change, bump PATCH (v0.4 → v0.4.1). This replaces the current ad-hoc "v0.4" with a more informative version string.
2. **Introduce RFC track separate from ADRs**. New `docs/rfcs/` directory. RFCs are proposals (Draft → FCP → Accepted/Rejected); ADRs are decisions (Proposed → Accepted → Superseded). An RFC, once accepted, spawns an ADR that records the decision and links back. Use this for v5 essence derivation work.
3. **Literate-programming-lite for v5 derivation**: each essence category's C header comment should reference the prose rationale in `docs/concepts/essence-derivation-v5.md` by section anchor, so the rationale is the single source of truth.
4. **Generate API reference from Doxygen**. Add a `doxygen` build step that consumes structured C comments from `include/planex/planex.h` and `src/*.c`, outputting Markdown to `docs/reference/api-autogenerated.md`. Keep the hand-written `docs/reference/api.md` for narrative examples, but use the generated file for the function-by-function reference.

### Tier 3 — Long-term (platform migration, multi-week)

1. **Set up MkDocs Material site via GitHub Pages**. Single `mkdocs.yml`, single GitHub Action that builds and deploys on push to `main`. Gives Planex a proper doc site without leaving Markdown.
2. **Multi-version site**. Use `mike` (MkDocs versioning tool) to host snapshots per software release tag, so readers can reach the docs that match their installed version.
3. **Backstage TechDocs only if Planex grows into a multi-package ecosystem**. For a single C library, this is overkill; for a future "Planex platform" with UI bindings for GTK/Qt/ImGui, it becomes relevant.

---

## 9. Open Questions

1. **Software version format**: should Planex migrate from `v0.4` (dot-less, no patch) to `v0.4.0` (full SemVerDoc)? Pros: more informative for downstream consumers. Cons: more ceremony for a small library; the current `v0.4` is sufficient if PATCH releases are rare.
2. **Concept doc stability migration**: should `essence-derivation-v1.md` and `v2.md` carry `Stability: legacy` *now*, or only after v5 is fully derived? Marking them legacy too early may discourage reference reading; marking too late risks readers trusting outdated reasoning.
3. **RFC vs ADR split**: is the v3→v4 transition (ADR-0009 Proposed, then ADR-0010 Accepted and effectively supersedes 0009) enough of a case for splitting RFC from ADR? Or is the existing single-track ADR with Proposed/Accepted status sufficient for a single-maintainer project?
4. **Doxygen adoption scope**: should Doxygen run on the full `src/` and `include/`, or only on the public API in `include/planex/planex.h`? The latter is simpler; the former is more thorough but adds noise from internal functions.

---

## 10. References

### Frameworks (canonical sources)

- Diátaxis — https://diataxis.fr/
- Divio Documentation System — https://docs.divio.com/documentation-system/
- Write the Docs: Docs as Code — https://www.writethedocs.org/guide/docs-as-code/
- SemVerDoc (Semantic Versioning for Documents) — https://semverdoc.org/
- SemVer 2.0.0 — https://semver.org/
- Architectural Decision Records — https://adr.github.io/
- Michael Nygard, "Documenting Architecture Decisions" (original 2011 article) — referenced via adr.github.io
- C4 Model — https://c4model.com/
- arc42 Template Overview — https://arc42.org/overview
- Literate Programming (Donald Knuth, Stanford) — https://www-cs-faculty.stanford.edu/~knuth/lp.html
- Wikipedia: Literate programming — https://en.wikipedia.org/wiki/Literate_programming
- Rust RFC process — https://github.com/rust-lang/rfcs/blob/master/text/0002-rfc-process.md
- Single Source of Truth (Wikipedia) — https://en.wikipedia.org/wiki/Single_source_of_truth
- DRY principle (webel.com summary) — https://www.webel.com.au

### Platforms

- MkDocs Material — https://squidfunk.github.io/mkdocs-material/
- Docusaurus — https://docusaurus.io/
- Sphinx — https://www.sphinx-doc.org/
- Backstage TechDocs FAQ — https://backstage.io/docs/features/techdocs/faqs
- Docsy (versioning) — https://www.docsy.dev/docs/content/versioning/
- GitBook vs Docusaurus vs MkDocs comparison — https://unmarkdown.com/blog/gitbook-vs-docusaurus-vs-mkdocs

### Large project case studies

- Linux Kernel documentation (Sphinx) — https://docs.kernel.org/doc-guide/sphinx.html
- sphinx-contrib/multiversion — https://github.com/sphinx-contrib/multiversion
- Linux kernel-doc comments — https://www.infradead.org/~mchehab/kernel_docs/doc-guide/kernel-doc.html
- Kubernetes multi-version docs — https://kubernetes.io/docs/tasks/extend-kubernetes/custom-resources/custom-resource-definition-versions/
- Python PEP 0 index — https://peps.python.org/
- Rust RFCs — https://rust-lang.github.io/rfcs/
- arXiv submission version availability — https://info.arxiv.org/help/versions.html

### C library documentation

- Doxygen — https://www.doxygen.nl/
- KDE Doxygen guidelines — https://community.kde.org/Guidelines_and_HOWTOs/API_Documentation

### Knowledge management traditions

- Zettelkasten / Second Brain — https://en.wikipedia.org/wiki/Zettelkasten
- Documentation lifecycle and technical debt — https://www.ibm.com/think/topics/technical-debt
- The Pragmatic Programmer (Hunt & Thomas, 1999) — DRY principle origin

### Internal Planex references

- `docs/decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md` — the decision that motivated this survey
- `docs/concepts/state/versioning.md` — Planex's three-system rule book (Task 12 output)
- `docs/changelog.md` — `[Unreleased]` section documenting the v4 / ADR-0010 / versioning work
