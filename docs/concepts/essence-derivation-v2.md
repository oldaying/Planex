# Essence Derivation v2

> **Status**: Replaces v1. Based on 6 parallel research agents' reports (`research/reports/00-summary.md`).
>
> **Method change from v1**: v1 was a single-author derivation from a minimal UI definition. v2 is grounded in 6 independent literature surveys covering: UI early history (Sketchpad → CUA), HCI theory (GOMS → Dourish), functional/reactive UI (Conal → re-frame), modern UI architecture (React → ImGui), phenomenology (Heidegger → Turkle), and mathematical formalization (denotational design → statecharts).
>
> **Honest verdict**: The derivation in v1 was correct in form but under-evidenced. v2 confirms v1's main finding (3 essence-derived + 1 structurally-derived abstractions) and adds 5 essence dimensions that v1 missed entirely.

---

## Why v2 was needed

v1 derived 3 essence categories (State / Communication / Presentation) from a minimal UI definition and concluded Planex's Relation is structurally-derived, not essence-derived. The derivation was correct **in form** but had no empirical grounding — it was a philosopher's argument, not an evidence-based derivation.

This matters because Planex's claim is essence-driven. If the derivation rests on a single author's intuition, "essence" becomes marketing. v2 grounds the derivation in 60 years of UI literature across 6 traditions, then asks: **what categories actually emerge from this body of evidence?**

The 6 agents' reports are archived in `research/reports/`. This document is the synthesis.

---

## Step 0: What the 6 traditions agree on (and don't)

Before deriving, observe what the literature actually says.

### No tradition gives a single UI essence

Each of the 6 traditions independently produced a different "UI essence" claim:

| Tradition | Essence claim |
|------------|--------------|
| UI history (Sketchpad→CHI) | "Real-time conversation through a shared medium" |
| HCI theory | Split into 3 incommensurable paradigms (information-processing / cognitive-mediation / phenomenological-practice) |
| Functional/reactive | 5 competing claims (Behavior, Dialogue, Derived-data, Spreadsheet, FRP-essence) |
| Modern architecture | 4-way split (declarative projection / signal-network / data-system / immediate-mode) |
| Phenomenology | 11 family-resemblance categories sharing a relational-ontology presupposition |
| Mathematical formalization | 5 orthogonal essence candidates that cannot be reduced to each other |

**This itself is a finding**: there is no historical or theoretical consensus on "UI essence". Any essence derivation must therefore be **multi-tradition convergent**, not derived from one tradition.

### But cross-tradition convergence exists

Across all 6 traditions, 5 categories appear in ≥3 traditions independently. These are the strongest essence candidates:

1. **State / something persistent that is shared** — every tradition has this. History calls it "object/statement/panel"; HCI calls it "system state"; functional calls it "Behavior/value"; modern calls it "state"; philosophy calls it "the shared something"; math calls it "variable/cell/functor carrier".

2. **Communication / human→machine intent** — every tradition has this. History: "converse", "command"; HCI: "goal/intent"; functional: "Msg/event"; modern: "event handler"; philosophy: "speech act/commitment"; math: "input channel".

3. **Presentation / machine→human denotation** — every tradition. History: "line drawings", "presentation"; HCI: "feedback"; functional: "View/render"; modern: "render"; philosophy: "affordance/evocation"; math: "interpretation/functor application".

4. **Feedback / closed-loop coupling** — independently rediscovered in every historical era, every HCI theory, every modern framework, every phenomenological critique. Even math traditions require it implicitly (causality in FRP, trace in CSP, transition in statecharts).

5. **Relational ontology / UI cannot be defined without actor + situation** — emerges from philosophy (Heidegger, Gibson, Dourish, Hutchins), distributed cognition, Activity Theory, Alexander's semilattice, Suchman's situatedness, and even math (CSP/π-calculus requires channels between processes; Constraint systems require relations between variables).

### What v2 adds that v1 missed

v1 had 3 essence categories + 1 structural (Relation). v2 adds **2 more essence categories** that v1 didn't derive:

- **Feedback / closed-loop coupling** (cross-tradition convergence point #4) — v1 didn't even mention it
- **Relational ontology** (cross-tradition convergence point #5) — v1 treated Relation as structural; v2 elevates it to essence under a relational-ontology premise, but a different premise than v1's "UI is a network"

The difference: v1's "UI is a network" was a topology claim. v2's "relational ontology" is an ontological claim — UI cannot be defined without the actor. This is stronger and comes from philosophy, not engineering.

---

## Step 1: Derivation of essence categories

### Derivation 1: The "shared something" must exist

Same as v1. From any UI definition that involves "human and machine sharing access to something", the something must have persistent existence. Call this category **State**.

Cross-tradition support: every tradition has this category. Strong essence candidate. ✓

### Derivation 2: The exchange must be structured

Same as v1. The human→machine direction must carry intent; the machine→human direction must carry denotation. Call the human→machine category **Communication** and the machine→human category **Presentation**.

Cross-tradition support: every tradition has both. Strong essence candidates. ✓

### Derivation 3: The exchange must close into a loop (v2 NEW)

v1 missed this. The shared something changes through communication; the change must be perceivable through presentation. Without this closure, there is no UI — the human cannot tell if their intent had effect.

This is not derivable from v1's 3 categories alone. It is a separate essence category: **Feedback / Closed-loop coupling**.

Cross-tradition support: feedback is independently rediscovered in every historical era (Sketchpad's rubber-band line, Engelbart's real-time display, Apple HIG's "to be in charge, the user must be informed"). It appears in every HCI theory (Norman's gulf of evaluation, KLM's system response R). Every modern framework requires it (otherwise state changes wouldn't reach the user). Phenomenology treats breakdown as the moment feedback fails. Math requires it (FRP's causality, CSP's trace, statechart's transition).

This category was hidden in v1 because v1's 3 categories (State/Communication/Presentation) implicitly assumed the loop — but the loop is itself essence, not a derived property.

### Derivation 4: UI cannot be defined without actor + situation (v2 NEW)

v1 derived State, Communication, Presentation as essence; treated Relation as structural under a "UI is a network" premise. v2 revises this.

From phenomenology (Heidegger's Zuhandenheit, Gibson's affordance-as-relation, Dourish's embodiment, Hutchins's distributed cognition, Suchman's situatedness, Alexander's semilattice): UI cannot be defined independently of the actor who uses it and the situation in which it is used. The UI's "essence" includes the relation between the UI and its user — it is not a property of the UI alone.

This is an **ontological** claim, not a topological one. v1's "UI is a network" was about topology (nodes and edges). v2's "relational ontology" is about being (UI's existence is relational).

Cross-tradition support:
- Philosophy: 11 family-resemblance categories converge on this presupposition
- HCI: Activity Theory, Distributed Cognition, Situated Action all reject the "UI as independent artefact" view
- Modern architecture: Solid/Bevy/ImGui/Genera all embody non-tree, relation-first or actor-coupled models
- Math: CSP/π-calculus makes channels (relations between processes) primitive; Constraint systems make constraints (relations between variables) primitive
- Functional: re-frame's "data coordinates functions" is a relational inversion of control
- UI history: even Sketchpad had object-constraint relations as primitive; Engelbart's "augmentation means" included methodology and training (relational, not property)

**Conclusion**: Relational ontology is essence, not structural. v1 was wrong to call it structural.

But — and this is important — relational ontology does NOT necessarily mean Planex's `px_graph` is essence. The category is "UI is relationally defined"; the implementation can be many things (a graph, a constraint system, a process network, an object graph, a semilattice). Planex's `Relation` is one engineering instance of this essence category.

### Derivation 5: v1's claim about Relation revised

v1: "Relation is structurally-derived under the 'UI is a network' premise."
v2: "Relational ontology is essence-derived. Planex's `Relation` (a graph data structure) is one engineering instantiation of this essence category — but not the only possible one."

This is a stronger essence claim than v1's. It says: any UI framework that treats UI as an independent artefact (defined without actor/situation) is missing an essence category. Planex's `Relation` happens to be a particular way of capturing relational ontology, but the essence is the ontology, not the graph.

---

## Step 2: Verify against Planex's 4 abstractions

| Planex abstraction | v1 status | v2 status | Change reason |
|--------------------|-----------|-----------|---------------|
| **Estimate** | Essence-derived (State) | Essence-derived (State) | No change — strongest cross-tradition support |
| **Closure** | Essence-derived (Communication) | Essence-derived (Communication) | No change |
| **Perception** | Essence-derived (Presentation) | Essence-derived (Presentation) | No change |
| **Relation** | Structurally-derived ("UI is a network" premise) | **Essence-derived (Relational ontology)** | v1 used topology premise; v2 uses ontology premise from phenomenology + math + HCI |

### v2 added essence categories Planex doesn't have

| Essence category | v2 status | Planex has it? |
|------------------|-----------|----------------|
| State | essence | Yes (Estimate) |
| Communication | essence | Yes (Closure) |
| Presentation | essence | Yes (Perception) |
| Relational ontology | essence | Yes (Relation, but conflated with topology) |
| **Feedback / Closed-loop coupling** | **essence** | **No** — split between Closure (intent→action) and Perception (state→view), but the loop itself is not first-class |
| **Situatedness / Embodiment** (philosophy) | essence candidate | No |
| **Affordance-as-relation** (Gibson) | essence candidate | Partially — Relation could express it, but Planex doesn't use it this way |
| **Breakdown** (Heidegger-Winograd/Flores) | essence candidate | No |

---

## Step 3: Honest verdict on Planex

### What v2 confirms

- Planex's 4 abstractions are essence-derived, not structurally-derived. v1 was wrong about Relation.
- The choice of 4 abstractions is a defensible essence partition, supported by cross-tradition convergence.

### What v2 reveals

- Planex is missing at least **1 essence category**: Feedback / closed-loop coupling. This is not a feature gap; it's an essence gap. Planex's Closure and Perception together implement feedback, but the loop itself is not first-class. This means Planex cannot natively express "the system's response changed how the user's next intent is formed" — which is the definition of interactive UI (vs. one-shot UI).
- Planex's `Relation` is one engineering instantiation of relational ontology. Other valid instantiations: constraint systems, process networks, semilattice structures. Planex should not claim `Relation` IS the essence; it should claim `Relation` instantiates the essence.
- Planex has no embodiment / situatedness / affordance-as-relation / breakdown categories. These are essence candidates from philosophy. They may be deferred (Planex implements UI essence layers 1-3, see `ui-essence-layers.md`), but they should be acknowledged as gaps.

### What v2 cannot resolve

- Whether "5 essence categories" (v2) is final. The 6 traditions don't agree on a single count. Math says 5 orthogonal candidates; philosophy says 11 family-resemblance categories; modern architecture says 3 axes. v2 takes the cross-tradition convergence (≥3 traditions) as the threshold, giving 5. But a stricter threshold (≥4 traditions) gives 4; a looser threshold (≥2 traditions) gives 8+.
- Whether Feedback is a separate essence category or a derived property of (State + Communication + Presentation). v2 argues separate, but this is debatable.

---

## Step 4: Implications for Planex

### What changes in the project

1. **`why-four-abstractions.md` must be revised again**:
   - v1 said "3 essence + 1 structural". v2 says "4 essence + 1 missing (Feedback)".
   - Relation is essence under relational-ontology premise, not topology premise.
2. **ADR-0007 should be revised**:
   - v1 ADR-0007 (if it exists) records v1's finding. v2 supersedes it.
   - v2 finding: 4 essence categories (State/Communication/Presentation/Relational-ontology) + 1 missing essence category (Feedback) + several deferred essence candidates (Embodiment, Situatedness, Affordance-as-relation, Breakdown).
3. **A new ADR should record v2's finding**: 4 essence + 1 missing essence + N deferred essence candidates.

### What doesn't change

- Planex's 4 abstractions remain a valid essence partition. v2 strengthens (not weakens) the case.
- The implementation does not need to change.
- The orthogonality test suite remains valid.

### What this enables

- Planex can now honestly answer: "Is Feedback an essence category?" — yes, and Planex doesn't have it as first-class. This is a known essence gap, like a known limitation.
- Planex can now distinguish: "Relation as graph" (engineering choice) vs "Relational ontology" (essence). The graph is one way to instantiate the essence; other ways are valid.
- Planex can now acknowledge the deferred essence candidates (Embodiment, Situatedness, etc.) without claiming to implement them. This is more honest than v1.

---

## Step 5: Open questions v2 surfaces

1. **Is Feedback really separate from State/Communication/Presentation?**
   - If separate: Planex has 5 essence categories and is missing 1.
   - If derived: Planex has 4 essence categories and is complete.
   - The literature is ambiguous. Math traditions (CSP, statecharts) treat feedback as primitive (loop, transition). Phenomenology treats breakdown (feedback failure) as essence. But modern architecture often implements feedback as derived (state→render loop).
   - **Recommendation**: treat as essence, document the choice.

2. **Are Embodiment / Situatedness / Affordance-as-relation / Breakdown essence or deferred?**
   - Philosophy strongly argues essence. Engineering has no implementation.
   - **Recommendation**: deferred essence candidates, not contingent. Document as "Planex implements UI essence layers 1-3; layers 4-6 (Embodiment, Situatedness, etc.) are deferred essence, not out of scope."

3. **Is the relational-ontology premise compatible with Planex's current `Relation` API?**
   - Planex's `px_graph` with edges like TRIGGERS/DEPENDS_ON/BESIDE is a topology implementation.
   - Relational ontology says UI cannot be defined without actor + situation. Planex's current API doesn't express this — closures and perceptions don't take "actor" or "situation" as parameters.
   - **Recommendation**: this is a real gap. Future API revision could add actor/situation as first-class, but this is a major change.

---

## Conclusion

### v1 → v2 changes

| Aspect | v1 | v2 |
|--------|----|----|
| Method | Single-author derivation from minimal definition | 6-tradition literature survey + convergence analysis |
| Essence count | 3 essence + 1 structural | 4 essence + 1 missing + N deferred |
| Relation status | Structurally-derived | Essence-derived (relational ontology) |
| Feedback | Not mentioned | Essence, missing from Planex |
| Embodiment/Situatedness/etc. | Not mentioned | Deferred essence candidates |
| Honesty | Strong in form, weak in evidence | Strong in both |

### Final honest verdict

Planex's 4 abstractions are **essence-derived under v2's stronger analysis** — v1 was wrong to demote Relation to structural. But v2 also reveals that Planex is **missing at least 1 essence category** (Feedback) and has **several deferred essence candidates** (Embodiment, Situatedness, Affordance-as-relation, Breakdown).

The honest framing is no longer "4 essence abstractions" but:

> **Planex implements 4 of 5 essence categories. The 5th (Feedback) is partially implemented but not first-class. Several additional essence categories (Embodiment, Situatedness, Affordance-as-relation, Breakdown) are deferred — acknowledged as essence but not yet implemented.**

This is a stronger and more honest claim than v1's "3 essence + 1 structural". It acknowledges what Planex has, what it's missing, and what it's deferring — all on the same essence footing.

---

## What this document deliberately does NOT do

- Does not defend the 4-abstraction count by re-labelling.
- Does not hide the missing Feedback category.
- Does not pretend deferred essence candidates don't exist.
- Does not use "essence" as marketing — only as strict cross-tradition convergence.
- Does not conflate engineering instantiations (graph, constraint system, etc.) with essence categories.
