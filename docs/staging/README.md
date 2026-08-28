# Staging — Holding Pen

> **Status**: This directory is the holding pen for incoming documentation that does not yet have a permanent home. See [`doc-organization.md`](../doc-organization.md) Principle 5.

## Policy

A document lands in `staging/` when:

- It is useful enough to commit (it has a reader), but
- It does not yet have a clear category fit in the canonical tree (it's not obviously a tutorial, how-to, reference, or concept).

The expectation is **graduation within 2 minor versions**. Either:

1. **Graduate** — the doc moves to its proper category (`canonical/`, `state/`, `tutorials/`, `how-to/`, `reference/`, etc.) when its permanent home becomes clear, OR
2. **Remove** — the doc is deleted if no home is found within 2 minor versions. A doc that stays in `staging/` indefinitely is a signal that the doc tree is missing a category; the right response is to propose a new category (via an ADR), not to let the staging dir grow unbounded.

## Why a holding pen

Without a staging dir, the failure mode is force-fit: a contributor writes a doc that doesn't match any existing category, but needs to put it *somewhere*, so it lands in the closest category and dilutes that category's signal. The next contributor's task gets harder because the category is now less clean. The staging dir breaks this cycle by giving force-fit docs a temporary home while the proper category is being decided.

This pattern is borrowed from Linux kernel's `Documentation/staging/` — <https://github.com/torvalds/linux/tree/master/Documentation/staging>.

## Audit

CI runs an orphan scan (planned, see [`doc-organization.md`](../doc-organization.md) Part VI Check 2). Files in `staging/` are allowed to be orphans by definition; files in other directories that are orphans trigger a CI failure. This catches the failure mode where a doc lands in `staging/` and is never graduated — the audit does not block the staging itself, but it does block the doc from "leaking" into the canonical tree without being properly placed.

## Current contents

*(empty — this dir's `.gitkeep` placeholder is here so the directory survives in git)*

If you're adding a doc here, replace this section with a one-line description of your staging doc and the conditions under which it should graduate.
