# Research — Dated Investigation Notes

> **Status**: These docs record investigations that informed Planex's design. They are dated because they capture the state of research at a moment in time. See [`doc-organization.md`](../doc-organization.md) Principle 4.

A "research" doc:

- Is **dated** (filename starts `YYYY-MM-DD-`).
- Is **investigative** — it surveys prior art, compares options, recommends an approach.
- Is **frozen** at its date — the research is a snapshot; later research may supersede it but the original doc is retained.
- May **inform** an ADR (an ADR records the decision; the research records the reasoning).

## Contents

| Doc | Date | What it investigates |
|-----|------|---------------------|
| [`2025-08-28-abstraction-as-form-comparative-study.md`](2025-08-28-abstraction-as-form-comparative-study.md) | 2026-08-28 | Long-form comparative study of "abstraction-as-encapsulation" vs "abstraction-as-typed-value", surveying Hickey, Tasevski, Spolsky, and others. Recommends Planex's typed-value form. Cited by [`../concepts/canonical/abstraction-form.md`](../concepts/canonical/abstraction-form.md). |
| [`2025-08-28-doc-versioning-top-solutions.md`](2025-08-28-doc-versioning-top-solutions.md) | 2026-08-28 | Survey of doc-versioning practices across 13 projects (seL4, Rust, Linux kernel, Lean, Idris 2, OpenBSD, LLVM, Svelte, etc.). Informed the design of [`../doc-organization.md`](../doc-organization.md). |

## Why a separate directory

Research docs are different from concept docs:

- A concept doc states Planex's current position; a research doc records the investigation that led to that position.
- A concept doc is updated when Planex's position changes; a research doc is frozen at its date — it captures what was known at that moment.
- A concept doc is normative (prescribes what should be true); a research doc is empirical (describes what was found).

The distinction matters because:

- A reader questioning "why is Planex shaped this way?" should find the answer in the research doc, not the concept doc.
- A reader wanting "what does Planex claim?" should find the answer in the concept doc, not wade through research.
- Future research may supersede earlier research; the doc tree must preserve the earlier work as the audit trail.

## Lifecycle

A research doc is dated and frozen. It is not edited after publication except to:

- Fix broken links when the doc tree is reorganized (via the link-rewriter scripts, not by hand).
- Add a "Superseded by YYYY-MM-DD-name.md" header if a later research doc revisits the same question.

If a research doc's conclusion is overturned, the new research doc is filed alongside (with a later date), not in place of the original.

## When to add a research doc

Add a research doc when:

- You're investigating a question whose answer will inform a design decision (not a coding decision).
- The investigation is broader than one ADR (an ADR records a decision; research records the broader survey).
- The investigation is worth preserving as an audit trail (so future maintainers can re-trace the reasoning).

If your investigation is too narrow for a research doc (one specific design choice), put it in an ADR's Context section instead.
