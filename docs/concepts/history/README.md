# History — Superseded Docs

> **Status**: These docs are superseded. They record how Planex's thinking evolved. See [`doc-organization.md`](../../doc-organization.md) Principle 4 and Problem 5.

A "history" doc:

- Is **superseded** by a later version or by an ADR.
- Is **retained** for intellectual-history value — readers can trace how the current position was reached.
- Is **not actionable** — the current canonical position is in `canonical/`, not here.

## Contents

| Doc | Status | Superseded by |
|-----|--------|----------------|
| [`essence-derivation-v1.md`](essence-derivation-v1.md) | Superseded | `essence-derivation-v2.md` (single-author derivation → multi-tradition grounding) |
| [`essence-derivation-v2.md`](essence-derivation-v2.md) | Superseded | `essence-derivation-v3.md` (first-principles audit added) |
| [`essence-derivation-v3.md`](essence-derivation-v3.md) | Framing downgraded | `essence-derivation-v4-clean.md` + ADR-0010 (v3 implementation proposed but framing of "essence discovery" downgraded to "design rationale") |
| [`essence-derivation-v4-clean.md`](essence-derivation-v4-clean.md) | Current derivation | — (current; ADR-0010 downgraded the framing but retained the doc as design rationale) |
| [`v0.4-roadmap.md`](v0.4-roadmap.md) | Superseded (moved from `state/` 2026-08-30, v0.6.0 release) | shipped reality (v0.4–v0.6) + later roadmaps |
| [`v0.7-roadmap.md`](v0.7-roadmap.md) | Superseded (moved from `state/` 2026-08-30, v0.7.0 release) | shipped reality (v0.7, all lines) + [`v0.8-roadmap.md`](../state/v0.8-roadmap.md) (the current roadmap) |

## Why retain superseded docs

Superseded docs are not deleted. They serve three purposes:

1. **Intellectual history** — readers can trace how Planex's thinking evolved, which is valuable for understanding why current canonical claims are shaped the way they are.
2. **Reversibility** — if a later version is itself superseded, the earlier version is still available for reference. Deleting would make this impossible.
3. **Falsifiability audit trail** — the leak-budget mechanism in `canonical/leak-budgets.md` tracks retire curves over versions; the history docs are the substrate that the audit runs against.

This pattern is borrowed from Idris 2's `docs/source/updates/updates.rst` — frozen migration descriptions kept separate from the active tutorial/reference trees.

## Editing history docs

History docs are **frozen**. They are not edited for content; they are edited only for:

- Adding a "Superseded by" header at the top (when a later version supersedes them).
- Fixing broken links when the doc tree is reorganized (this happens via the link-rewriter scripts, not by hand).
- Adding a "Re-evaluation" footer if a later ADR re-examines the supersession (rare).

If you find a typo or factual error in a history doc, **do not fix it in place**. File an issue and let the maintainer decide whether the correction warrants a new doc (a re-evaluation) or is too minor to record.

## Relationship to ADRs

ADRs that supersede a history doc are filed in `../../decisions/superseded/` if the ADR itself is later superseded, or `../../decisions/accepted/` if the ADR is still active (as with ADR-0010, which downgraded v4's framing but is itself an Accepted ADR). The ADR-numbering is permanent; only the directory changes.
