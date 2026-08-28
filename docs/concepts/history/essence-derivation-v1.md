# Essence Derivation v1

> **Status:** ⚠️ SUPERSEDED by [essence-derivation-v2.md](essence-derivation-v2.md).
>
> This document (v1) was a single-author derivation from a minimal UI definition. v2 supersedes it by grounding the derivation in a 6-tradition literature survey (UI history, HCI theory, functional/reactive UI, modern UI architecture, phenomenology, mathematical formalization).
>
> v1's main findings were correct in form but under-evidenced:
> - v1 said Relation is "structurally-derived under a topology premise". v2 revised this: Relation is **essence-derived under a relational-ontology premise** (grounded in Heidegger, Gibson, Dourish, Hutchins, Alexander).
> - v1 missed Feedback as a separate essence category. v2 surfaced it via cross-tradition convergence.
> - v1 did not acknowledge the deferred essence candidates (Embodiment, Situatedness, Affordance-as-relation, Breakdown). v2 lists them explicitly.
>
> This document is kept for historical reference. The canonical essence derivation is v2.
>
> ---
>
> Original abstract (below) preserved as-is:
>
> Re-examines Planex's "4 abstractions = 4 essence axes" claim using first-principles derivation. Written after the project's implementation had solidified — its job is to honestly verify whether the abstractions emerge from UI essence, not to defend them post-hoc.
>
> If this document concludes an abstraction does NOT emerge from essence, the project's canonical claim must be revised.

---

## Why this document exists

The original `why-four-abstractions.md` lists **inspirations** for each abstraction:

| Abstraction  | Inspiration cited                        |
| ------------- | ----------------------------------------- |
| Estimate      | Conal FRP, Friston, spreadsheets         |
| Relation      | Sketchpad rings, Bevy ECS, Alexander      |
| Closure       | Norman 7-stage, Winograd/Flores, re-frame |
| Perception    | Conal denotational, Haskell purity, FRP   |

This is **pattern matching to inspirations**, not essence derivation. It shows *where the idea came from*, not *why the idea is necessary*.

The difference matters. An essence-driven project must answer:

> If we had never read Conal, never read Norman, never read Alexander — could we still derive these 4 abstractions from the mere definition of "user interface"?

This document attempts that derivation. Inspirations are cited only as **independent confirmation** of results we already reached.

---

## Step 0: Define "user interface" without using any abstraction names

We strip the definition to the minimum that any UI must have. No mention of "component", "state", "event", "graph" — those are conclusions to be derived.

> **A user interface is a structure through which a human and a machine share access to something, and exchange signals about it.**

Three elements appear in this minimal definition:

1. **A human** (the user) — the entity capable of intent and perception.
2. **A machine** (the computer) — the entity capable of computation and persistence.
3. **A shared something** — the subject matter the interface is *about* (a document, a counter, a list of todos, an image being edited).

And one relation between them:

4. **Exchange of signals** — bidirectional: human→machine (acting on the something) and machine→human (presenting the something).

Nothing else is in the definition. No tree, no component, no event handler, no virtual DOM.

---

## Step 1: What must exist for this definition to hold?

We now derive necessary categories. A category is **necessary** if removing it makes the definition collapse — i.e., if you remove it, the structure is no longer a UI.

### Derivation 1: The "shared something" must have an existence

The interface is *about* something. That something must exist somewhere, in some form the machine can hold.

- If the something does not exist, there is nothing to share → not a UI.
- If it exists only transiently (e.g., a voltage on a wire that vanishes), the human cannot act on it again → not a UI.

**Conclusion 1:** The shared something must have **persistent existence** in a form the machine can read and write. Call this category **State**.

### Derivation 2: The exchange must be structured

A signal exchange exists. But unstructured signal exchange (e.g., raw bytes) is not an interface — it is a wire.

- The human must send signals that mean "do X to the something" — i.e., the signal must carry **intent**.
- The machine must send signals that mean "the something is now Y" — i.e., the signal must carry **denotation**.
- Without this structure, neither side can interpret the other → not a UI.

**Conclusion 2:** The human→machine direction must carry **Intent**. We call this category **Communication**.

### Derivation 3: The machine→human direction must produce a sensible form

The machine sends signals back. For the human to perceive them, the signals must take a form the human senses can interpret:

- Visual (pixels, text, shapes)
- Auditory (speech, sound)
- Tactile (haptics)
- Or even structured data the human reads through another tool (JSON for a test, log lines for an audit)

The machine's signals cannot be raw machine state — they must be **denoted** into a form fit for the human's perceptual channel.

**Conclusion 3:** The machine→human direction requires a function that maps state to a denotation. Call this category **Presentation**.

### What we have so far

Three categories are derived:
- **State** — necessary for "shared something" to exist.
- **Communication** — necessary for human→machine intent exchange.
- **Presentation** — necessary for machine→human denotation.

Each is **necessary**: remove any one, the structure stops being a UI.

---

## Step 2: Is a fourth category derivable?

We now ask the harder question. Is **Relation** (Planex's 4th abstraction) derivable from the definition alone, or does it require an additional premise?

### Attempt A: Derive Relation from "shared something"

The shared something is state. Could "state having relations to other state" be necessary?

- Consider a UI with a single piece of state (a counter).
- The counter has no relations to other state (there is no other state).
- This is still a UI — a single-counter calculator is a UI.

So Relation is not necessary in this minimal case. But UIs in practice are not single-state. Does multi-state *require* Relation?

- Consider two pieces of state A and B with no declared relation.
- The user changes A. B does not change.
- This is still a UI — many UIs have unrelated state (e.g., a sound setting independent of the document being edited).

So multi-state does not require Relation either. The user may **observe** that A and B are independent, but the UI works without modeling their independence.

**Attempt A fails.** Relation does not derive from "shared something".

### Attempt B: Derive Relation from "Communication"

Could relations between *intents* be necessary?

- Consider an "increment" action and a "reset" action.
- They both affect the same state but no relation between them is declared.
- Triggering increment, then reset, still works — the UI is functional.

Without relations between intents, the UI loses **undo scoping** (you cannot know what an action touched), but the UI is still a UI. Undo is a feature, not essence.

**Attempt B fails.** Relation does not derive from "Communication" alone.

### Attempt C: Derive Relation from "Presentation"

Could relations between state and presentation be necessary?

- A Perception is a pure function from state to denotation.
- It already "knows" which state it depends on (via its source array).
- A graph edge from state to perception is redundant with the perception's own source list.

So Relation is not necessary for Presentation — the dependency is already expressed in the perception function itself.

**Attempt C fails.** Presentation does not require Relation.

### Attempt D: Derive Relation from an additional premise about UI structure

The only way Relation becomes essence-necessary is if we add an additional premise:

> **Premise (networked):** UI structure is a network, not a tree.

Under this premise, relations between nodes are first-class — there is no other way to express the network.

But this premise is **independent of the UI definition**. The definition ("a structure for sharing something") is silent on whether the structure is a tree, a network, or any other topology.

This premise can be defended on its own (Alexander's "A City is Not a Tree" argues that natural complex systems are semilattices, not trees; Sketchpad's ring structure made relations first-class; React's component tree forces parent-child data flow that is awkward for cross-cutting concerns). But the defense is **a separate essence claim**, not a derivation.

**Conclusion:** Relation is essence-necessary *only if* you accept the additional premise "UI is a network". That premise is defensible but not derivable from the minimal UI definition.

---

## Step 3: Verify each Planex abstraction against the derivation

| Planex abstraction | Derivation status                       |
| ------------------ | --------------------------------------- |
| **Estimate**       | Directly maps to derived category **State**. Essence-necessary. ✓ |
| **Closure**        | Directly maps to derived category **Communication**. Essence-necessary. ✓ |
| **Perception**     | Directly maps to derived category **Presentation**. Essence-necessary. ✓ |
| **Relation**       | Does not derive from the minimal definition. Essence-necessary only under the *additional* premise "UI is a network". Ambiguous. ? |

### Implication

Planex's "4 abstractions = 4 essence axes" claim is **partially correct**:

- 3 abstractions (Estimate, Closure, Perception) are unconditionally essence-necessary.
- 1 abstraction (Relation) is essence-necessary only under an additional structural premise that is itself a separate essence claim.

This is not a flaw in Planex — it is honest about the architecture. But the project's canonical framing should be revised:

> **Planex has 3 essence-derived abstractions + 1 structurally-derived abstraction (Relation, justified by the "UI is a network" premise).**

The "UI is a network" premise is strong and well-defended, but it is a separate claim from "these are what UI essence requires". Conflating them weakens the essence argument.

---

## Step 4: Sanity check — do the inspirations support this conclusion?

We derived 3 categories from first principles. Do Planex's cited inspirations agree?

| Inspiration          | Aligns with which derived category? |
| -------------------- | ----------------------------------- |
| Conal FRP (Behavior = Time → a) | State — Behavior is the formalization of state-over-time |
| Friston predictive coding        | State — the brain maintains posterior estimates, not snapshots |
| Spreadsheets                     | State — cells are state, formulas are derived state |
| Norman 7-stage                   | Communication — Goal → Intent → Action → Execution |
| Winograd/Flores                  | Communication — Conversation for Action |
| re-frame interceptors             | Communication — intent flows through interceptor chain |
| Conal denotational               | Presentation — pure function from semantic to syntactic |
| Haskell purity                   | Presentation — pure function = no side effects |
| Sketchpad rings                  | **Relation** (not derivable from essence alone) |
| Bevy ECS                          | **Relation** (not derivable from essence alone) |
| Alexander semilattice            | **Relation** (not derivable from essence alone — argues for the network premise) |

The pattern is consistent: every inspiration for **State / Communication / Presentation** is a formalization of *what those categories must be*. Every inspiration for **Relation** is an argument for the *network premise* — not a derivation of Relation from UI essence.

This confirms the derivation: 3 categories are essence-derived, Relation is structurally-derived.

---

## Step 5: Implications for Planex

### What stays

- The 4 abstractions remain the right **engineering choice**. Relation's structural justification (network premise) is strong, and undo-via-graph (ADR-0002) gives Relation a clear purpose.
- The implementation does not need to change.
- The documentation about *why each abstraction exists* remains correct.

### What changes

- The canonical framing "4 abstractions = 4 essence axes" must be revised.
- `why-four-abstractions.md` should be updated to:
  - Acknowledge that Relation is structurally-derived, not essence-derived.
  - Make the "UI is a network" premise explicit and argue for it as a *separate* essence claim.
- A new ADR (ADR-0007) should record this finding: 3 essence-derived abstractions + 1 structurally-derived abstraction.

### What this clarifies

- Planex's "essence-driven" claim is honest **if** we revise the framing. The 3 essence-derived abstractions genuinely *cannot be removed*. The 4th (Relation) can be removed in principle — the cost is losing the network premise's benefits.
- This is a *stronger* essence claim than the original, because it is honest about where each abstraction's necessity comes from.

---

## Step 6: Open questions this raises

This derivation surfaces questions that should be addressed next:

1. **Is the network premise actually essential?**
   - "UI is a network" is a strong claim. But trees are a subset of networks.
   - React's component tree *is* a network (parent-child edges) — React just doesn't make non-tree edges first-class.
   - A more careful argument: are non-tree relations essential, or just convenient?

2. **Should Relation be merged into another abstraction?**
   - If Relation is structurally-derived, could it be a feature of Estimate (state-state dependencies) or Closure (intent-intent relations)?
   - The current design keeps it separate to allow cross-abstraction relations (closure TRIGGERS estimate).
   - This separation is an engineering choice, not essence.

3. **Are there essence categories we missed?**
   - The derivation found 3. Could there be a 4th essence category we missed (not Relation)?
   - Candidate: **Time** — UI happens over time. But Time is folded into Estimate (Estimate has a time dimension via animation).
   - Candidate: **Identity** — UI elements have identity across state changes. But identity is folded into Closure (intent history).
   - No 4th essence category emerges cleanly. If Planex claims 4 essence axes, it must either:
     - Promote the network premise to a 4th essence category (defensible but unusual).
     - Or admit the count is "3 essence + 1 structural".

---

## Conclusion

### Honest answer

- **3 abstractions** (Estimate, Closure, Perception) are essence-derived.
- **1 abstraction** (Relation) is structurally-derived under a defensible-but-separate premise.
- The "4 abstractions = 4 essence axes" claim is **too strong** as written.
- The corrected claim is: **"3 essence-derived abstractions + 1 structurally-derived abstraction, justified by the 'UI is a network' premise."**

### What this means for the project

This does not invalidate Planex. It strengthens it:

- The 3 essence-derived abstractions are unimpeachable — they cannot be removed without breaking the UI definition.
- The 4th (Relation) is justified by a *separate, defensible* essence claim about UI structure.
- Both kinds of justification are legitimate; conflating them weakens the argument.

The next action is to revise `why-four-abstractions.md` and add ADR-0007 recording this finding.

---

## What this document deliberately does NOT do

- Does not list inspirations as derivations (the original failure mode).
- Does not defend the 4-abstraction count by re-labeling.
- Does not hide the gap between essence-derived and structurally-derived.
- Does not use "essence" as a marketing term — only as a strict derivation target.
