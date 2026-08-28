# How-To — Goal-Oriented Recipes

> **Status**: How-to guides assume you know the basics and want to do a specific thing. See [`doc-organization.md`](../doc-organization.md) Principle 4 (Diátaxis how-to quadrant).

A "how-to" doc:

- Is **goal-oriented** (Diátaxis: "I want to do X, show me the recipe").
- Assumes **basic familiarity** with Planex (if not, start with [`../tutorials/getting-started.md`](../tutorials/getting-started.md)).
- Is **modular** — each how-to is self-contained; you don't read them in order.
- Is **practical** — concrete code, concrete steps, minimal theory.

## Contents

| Doc | What it shows |
|-----|----------------|
| [`create-a-button.md`](create-a-button.md) | How to implement a clickable button using Closure + Perception + Estimate. The button is the canonical "hello world" of UI; this how-to shows how Planex does it. |
| [`derived-estimates.md`](derived-estimates.md) | How to use Relation's `DEPENDS_ON` to declare that one Estimate is derived from another. Includes the auto-recomputation behavior and the cycle-detection edge case. |

## When to add a how-to

Add a how-to when:

- A user asks "how do I do X in Planex?" and the answer is non-obvious.
- The recipe is reusable — multiple users will want to do this.
- The recipe fits in one sitting (under 30 minutes of reading + coding).

If the recipe is too small (one-line API call), it goes in [`../reference/api.md`](../reference/api.md), not here. If the recipe is too large (multi-step architectural pattern), it goes in [`../concepts/state/architecture.md`](../concepts/state/architecture.md) or warrants its own concept doc.

## Relationship to tutorials

How-tos assume basic familiarity; tutorials don't. If your doc needs to explain "what is an Estimate?" before showing the recipe, it's a tutorial, not a how-to. If your doc jumps straight to "to declare a derived estimate, call `px_declare(graph, source, PX_REL_DEPENDS_ON, derived)`", it's a how-to.

The boundary matters: putting how-tos in `tutorials/` overwhelms newcomers with specifics; putting tutorials in `how-to/` bores experienced users with basics.
