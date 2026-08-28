# Document Versioning Conventions

> **Status**: Authoritative reference for how version numbers are used across Planex docs. Read this if you see three different number systems (Arabic / Roman / no-number) and want to know why.

## Three distinct number systems

Planex docs use **three independent number systems**. They are not interchangeable; mixing them is a bug.

### 1. Document version (Arabic, no dot) — `v1`, `v2`, `v3`, `v4`

Used for documents in a **derivation lineage** — i.e., documents where each version supersedes the previous one with a re-derivation or re-audit.

Examples:
- `docs/concepts/history/essence-derivation-v1.md` — "Essence Derivation v1"
- `docs/concepts/history/essence-derivation-v2.md` — "Essence Derivation v2"
- `docs/concepts/history/essence-derivation-v3.md` — "Planex Essence Derivation v3 — A First-Principles Audit"
- `docs/concepts/history/essence-derivation-v4-clean.md` — "Planex Essence Derivation v4 — Clean-Room"

Rules:
- The version number appears in **both** the filename and the H1 title.
- Cross-references in other docs use the version label ("v1", "v2", etc.) and link to the versioned filename.
- A superseded version is **not deleted** — it gets a "⚠️ SUPERSEDED by vN" banner at the top and remains for historical reference.
- Document versions are unrelated to software release versions.

### 2. Section number within a long-form doc (Roman) — `Part I`, `Part II`, `Part VII`

Used for **section numbering** inside a single long-form document, especially derivation / audit / philosophical docs that follow the Aristotelian / philosophical tradition of structuring arguments as ordered Parts.

Examples:
- `essence-derivation-v3.md` has Parts I–VIII.
- `essence-derivation-v4-clean.md` has Parts I–IX plus an Appendix.

Rules:
- Roman numerals are **section markers, not version markers**. They never appear in a document's title or filename.
- Within a single doc, Parts are sequential and contiguous (Part I, II, III, ...).
- Cross-references between Parts use the form "Part VII.9" (Part number, dot, subsection number).
- Roman numerals are reserved for Parts; software releases and document versions use Arabic. This is intentional — the visual distinction helps a reader immediately see "this is a section pointer, not a version pointer".

### 3. Software release version (Arabic, with dot) — `v0.4`, `v0.4.0`, `v1.0`

Used for **Planex software releases**, following [Semantic Versioning](https://semver.org/).

Examples:
- `v0.4.0` — the Feedback (px_loop) release.
- `v0.5+` — a future release (used in scope statements).
- `v1.0+` — a future major release (used in scope statements).

Rules:
- Software versions always have a dot (`v0.4`, not `v04`).
- Software versions refer to **code releases**, not document versions.
- ADRs and changelog entries reference software versions to indicate which release a decision applies to (e.g., "deferred to v1.0+").
- A doc without a software version marker is either (a) not tied to a specific release (e.g., the glossary), or (b) tied to the current release via an `Applies to:` status line.

## Documents without any version marker

Many Planex concept docs have **no version marker at all** — no `v1`/`v2`/`v3` in the filename, no `v0.4` in the title. Examples:

- `architecture.md`
- `glossary.md`
- `limitations.md`
- `non-goals.md`
- `ui-essence-layers.md`
- `why-four-abstractions.md`

This is intentional. These are **living documents** that evolve continuously as the project evolves. They are not in a derivation lineage (so no `vN`), and they apply to the current release (so the release version is implicit unless stated otherwise).

To make the "applies to which release" question explicit, major concept docs include an `Applies to:` status line near the top:

```
> **Applies to**: v0.4 (Feedback release). Earlier releases differ.
```

If a concept doc does **not** have an `Applies to:` line, treat it as "applies to whatever release the repo is currently on". Check the changelog's top entry to find the current release.

## Quick reference

| Number system | Format | Where used | Example |
|---|---|---|---|
| Document version | `v1`, `v2`, `v3`, `v4` | Filename + H1 title of derivation-lineage docs | `essence-derivation-v4-clean.md` |
| Section number | `Part I`, `Part II`, `Part IX` | H2 headings inside long-form docs | `## Part VII — What a truly first-principles derivation would require` |
| Software release | `v0.4`, `v0.4.0`, `v1.0+` | Changelog, ADRs, scope statements | `Continuous interaction deferred to v1.0+` |

## When in doubt

- If a doc is in a derivation lineage, version it.
- If a doc describes the current state of Planex's design, add an `Applies to: vN.N` line.
- If a doc is purely structural (glossary, FAQ), don't version it.
- Roman numerals are for sections only — never for documents or releases.
- Arabic numerals without a dot are document versions (`v1`); Arabic numerals with a dot are software releases (`v0.4`). The dot is the disambiguator.

## See also

- [changelog.md](../../changelog.md) — release version history
- [docs/decisions/README.md](../../decisions/README.md) — ADR index and writing rules
- [ADR-0010](../../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) — formalizes that the v1–v4 essence-derivation lineage is design rationale, not essence discovery
