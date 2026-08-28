# Tutorials — Learning-Oriented Documentation

> **Status**: Tutorials walk a newcomer through Planex end-to-end. See [`doc-organization.md`](../doc-organization.md) Principle 4 (Diátaxis tutorial quadrant).

A "tutorial" doc:

- Is **learning-oriented** (Diátaxis: "I want to learn, hold my hand").
- Has a **narrative arc** — start to finish, the reader builds something concrete.
- Assumes **no prior knowledge** of Planex (but assumes general programming familiarity).
- Is **strongly opinionated** — the tutorial makes specific choices so the reader doesn't have to.

## Contents

| Doc | What it teaches |
|-----|------------------|
| [`getting-started.md`](getting-started.md) | The 30-minute walkthrough: clone, build, run the counter example, modify it, run the tests. The first thing a new user reads. |

## When to add a tutorial

Tutorials are the slowest-growing doc type — they are expensive to write well and expensive to maintain (every breaking change in the API can break a tutorial). The current bar is:

- Add a tutorial only when `getting-started.md` is insufficient for a real newcomer audience.
- Tutorials target specific audiences: "Tutorial: build a slider in 50 lines", "Tutorial: port a React counter to Planex", "Tutorial: undo/redo via Relation graph". Each tutorial is one specific learning path.

If you have a recipe that doesn't walk a newcomer end-to-end (e.g. "how to create a button" — assumes you already know the basics), it goes in [`../how-to/`](../how-to/), not here.

## Relationship to how-to

Diátaxis distinguishes tutorials from how-to guides:

- **Tutorials** are learning-oriented (you don't know Planex, you want to learn).
- **How-tos** are goal-oriented (you know Planex, you want to do a specific thing).

The boundary is reader intent, not doc length. A 200-line tutorial that walks a newcomer through "build your first Planex app" is a tutorial. A 200-line how-to that walks an experienced user through "implement undo/redo via Relation" is a how-to. The difference: in the tutorial, the reader is learning the basics; in the how-to, the reader is applying the basics to a specific task.
