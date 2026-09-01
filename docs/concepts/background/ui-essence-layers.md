# UI Essence — A Layered View

> **Status: Authoritative reference for Planex's stance on UI essence.**
>
> **Applies to**: v0.8. The six-layer model itself is version-independent; the per-layer implementation census below reflects the seven canonical abstractions (the 6th — intent compilation — and 7th — px_interaction — were promoted in v0.7 by ADR-0017/ADR-0018). ADR-0009's Breakdown-as-6th claim was never admitted: the v3 additions (actor/breakdown/perlocution/interpretant) remain prototype status, and ADR-0010 downgraded the essence-discovery framing to design rationale.
> Replaces the implicit "3 abstractions = UI essence" framing with an explicit layered model that admits Planex only implements part of UI essence, not all of it.

This document records what UI essence is, in a layered form grounded in academic literature. It is the honest answer to "what is UI fundamentally?" — surveying the question across HCI, cognitive science, philosophy, and computing history.

The key admission: **UI essence is not a single proposition. It is a six-layer nested structure.** Planex implements some layers; others are documented as future work. This is more honest than claiming to fully capture UI essence.

---

## The six layers

From outermost (most concrete) to innermost (most abstract):

```
┌────────────────────────────────────────────────────────────────┐
│  Layer 6: Medium — UI is a dynamic medium for human expression   │  (Kay, Engelbart)
├────────────────────────────────────────────────────────────────┤
│  Layer 5: Evolutionary — UI guides useful action, not truth   │  (Hoffman)
├────────────────────────────────────────────────────────────────┤
│  Layer 4: Behavioral — UI presents action possibilities         │  (Gibson, Heidegger)
├────────────────────────────────────────────────────────────────┤
│  Layer 3: Semantic — UI is the medium of conversation           │  (Winograd/Flores)
├────────────────────────────────────────────────────────────────┤
│  Layer 2: Cognitive — UI bridges two gulfs (execution+eval)     │  (Norman)
├────────────────────────────────────────────────────────────────┤
│  Layer 1: Physical — UI is the space where interaction occurs   │  (Wikipedia, ACM Survey)
└────────────────────────────────────────────────────────────────┘
```

Each layer nests inside the next: physical is the substrate of cognitive, cognitive is the substrate of semantic, and so on. **All six are real. All six must be answered to claim "we capture UI essence".**

---

## Layer 1: Physical — UI is the space where interaction occurs

### Definition

> "In the industrial design field of human–computer interaction, a user interface (UI) is the space where interactions between humans and machines occur."
> — Wikipedia, "User interface"
> https://en.wikipedia.org/wiki/User_interface

### What this layer says

UI has a physical substrate: input device, output device, pixel grid, audio, haptics. Without physical I/O, there is no UI — there is only computation.

### What this layer does NOT say

It does not specify the form of the substrate (screen, speaker, force feedback), nor the meaning of what passes through it. It only requires that something physical happens.

### Planex status

✅ **Implemented.** Planex has framebuffer (`px_fb`), window abstraction (`px_window`), and event polling (`px_event`). This is the most basic layer and is fully covered.

---

## Layer 2: Cognitive — UI bridges two gulfs

### Definition

> Don Norman, *The Design of Everyday Things* (1988). The model was originally articulated in Hutchins, Hollan, Norman, *Direct Manipulation Interfaces* (1985).
>
> Two gulfs define the cognitive cost of UI:
> - **Gulf of Execution**: gap between user's goal and the system state change required to achieve it
> - **Gulf of Evaluation**: gap between system state and the user's understanding of that state
>
> "Each gulf is unidirectional: The gulf of execution goes from goals to system state; the gulf of evaluation goes from system state to goals."
> — Hutchins, Hollan, Norman 1985
> https://www.lri.fr/~mbl/ENS/FundHCI/2013/papers/Hutchins-HCI-85.pdf (cited 2620+ times)

### What this layer says

UI design quality = how narrow these two gulfs are. UI is fundamentally a *bridge* — its job is to reduce the cognitive cost of translating goals into actions, and the cognitive cost of translating state into understanding.

### What this layer does NOT say

It assumes meaning is pre-existing in the user's head and in the machine's state. UI's job is to *transfer* this meaning across the gulfs, not to *create* it.

### Planex status

✅ **Implemented.** Planex's Closure abstraction directly implements Norman's 7-stage model (Goal → Intent → Action → Execution → Perception → Interpretation → Evaluation). The `px_closure_new` API takes a goal string, an intent kind, an action function, an evaluation function — all seven stages are addressable in code.

### Source

- *The Design of Everyday Things* (Norman 1988): https://en.wikipedia.org/wiki/The_Design_of_Everyday_Things
- *Direct Manipulation Interfaces* (Hutchins, Hollan, Norman 1985): https://www.lri.fr/~mbl/ENS/FundHCI/2013/papers/Hutchins-HCI-85.pdf
- NN/g explainer: https://www.nngroup.com/articles/two-ux-gulfs-evaluation-execution/

---

## Layer 3: Semantic — UI is the medium of conversation

### Definition

> Terry Winograd, Fernando Flores, *Understanding Computers and Cognition: A New Foundation for Design* (1986).
>
> "Conversations for action, commitment management protocol."
> — Wikipedia, "Language/action perspective"
> https://en.wikipedia.org/wiki/Language/action_perspective
>
> Computers are not information processors — they are **participants in conversation**, performing speech acts (requests, promises, declarations, assertions, expressions).
>
> The book draws on Heidegger, Maturana, and Austin's speech-act theory. Cited 11,000+ times.

### What this layer says

UI is not just a transfer mechanism — it is a *medium of conversation*. Every interaction is a speech act: a request, a promise, a declaration. The user and system are **conversation partners**, not sender-and-receiver.

### What this layer does NOT say

It does not address what makes conversation meaningful (that is Layer 4). It only specifies that interaction has the *form* of speech acts.

### Planex status

✅ **Implemented.** Planex's Intent abstraction is directly the five speech-act types:

```c
typedef enum {
    PX_INTENT_ASSERT,    // "X is true"
    PX_INTENT_REQUEST,   // "do X"
    PX_INTENT_PROMISE,   // "I will do X"
    PX_INTENT_DECLARE,   // "X is done"
    PX_INTENT_EXPRESS,   // "I feel X"
} px_intent_kind;
```

This is a one-to-one mapping with Winograd/Flores. The Closure abstraction makes Intent a typed value (not a callback), which is the technical expression of "intent is a speech act, not a function call".

### Source

- *Understanding Computers and Cognition* (Winograd, Flores 1986): https://en.wikipedia.org/wiki/Language/action_perspective
- Dubberly's diagram of the Language/Action Model: https://www.dubberly.com/articles/language-action-model.html
- PhilPapers (11,218+ citations): https://philpapers.org/rec/WINUCA

---

## Layer 4: Behavioral — UI presents action possibilities

### Definition

> James J. Gibson, *The Ecological Approach to Visual Perception* (1979).
>
> "The affordances of the environment are what it offers the animal."
> — Wikipedia, "Affordance"
> https://en.wikipedia.org/wiki/Affordance
>
> Martin Heidegger, *Being and Time* (1927).
>
> Tools are encountered **ready-to-hand** (Zuhandenheit) when skillfully used — they "disappear" and we engage the task, not the tool. Tools become **present-at-hand** (Vorhandenheit) when they break or are reflected upon.
> — Heideggerian terminology
> https://en.wikipedia.org/wiki/Heideggerian_terminology

### What this layer says

UI's behavioral essence is not "transfer information" — it is **presenting action possibilities** (affordances) and supporting the user's fluid engagement with them (ready-to-hand).

A button "affords" pressing. A slider "affords" dragging. The user perceives these possibilities *directly*, not by inference. UI's job is to make the right affordances perceivable and the wrong ones invisible.

### What this layer does NOT say

It does not specify how affordances evolve or adapt over time (that is Layer 5).

### Planex status

⚠ **Partially implemented — the query half is canonical, the presentation half is not.** v0.7 promoted intent compilation (region + `PX_REL_AFFORDS` + `px_afford_at`/`px_afford_compile`) to the 6th canonical abstraction (ADR-0017): affordances are now a first-class, queryable surface — "which affordance contains (x, y)" and "which closure does this input afford" are graph queries, and the v0.8 a11y focus ring is derived from the AFFORDS graph. What remains open:

- Rendering does not yet visually differentiate available affordances to the user
- Ready-to-hand / breakdown (the Heidegger half) remains v3 prototype (`px_breakdown`, not admitted — see ADR-0010)
- Affordance *adaptation* over time is Layer 5's job, out of scope here

**Layer 4's action-possibilities vocabulary is canonical; its perception-side surfacing is not.** See [ADR-0017](../../decisions/accepted/ADR-0017-intent-compilation-promotion.md) for the promotion record.

### Source

- *The Ecological Approach to Visual Perception* (Gibson 1979): https://en.wikipedia.org/wiki/Affordance
- Affordance theory overview (Newcastle University): https://open.ncl.ac.uk/theories/22/affordances-theory
- Heideggerian terminology: https://en.wikipedia.org/wiki/Heideggerian_terminology
- "A Demonstration of the Transition from Ready-to-Hand to Unready-to-Hand" (Dotov et al., 2010): https://pmc.ncbi.nlm.nih.gov/articles/PMC2834739/
- Paul Dourish, *Where the Action Is: The Foundations of Embodied Interaction* (2001): https://direct.mit.edu/books/monograph/3875/

---

## Layer 5: Evolutionary — UI guides useful action, not truth

### Definition

> Donald Hoffman, *The Interface Theory of Perception* (2015).
>
> "Perception is a product of evolution. An interface serves to guide useful actions, not to resemble truth."
> — Hoffman et al., PubMed
> https://pubmed.ncbi.nlm.nih.gov/26384988/
>
> Hoffman's claim: human perception is not a window onto reality. It is an *interface* evolved to guide useful action. The desktop UI is the same kind of thing — it doesn't reflect the machine's true state, it presents a *useful fiction* that guides the user.

### What this layer says

UI is not a representation of machine state — it is an *adapted interface* that guides the user's actions. The same machine state can be presented many ways; the right presentation is the one that **helps the user act well**, not the one that "accurately reflects" state.

This implies UI should be **adaptive** — different users, different contexts, different histories should produce different presentations of the same state. A static UI is an evolutionary dead-end.

### What this layer does NOT say

It does not specify what "useful action" means in different contexts (that is Layer 6, where action becomes creative expression).

### Planex status

❌ **Not implemented.** Planex's UI is static — the same Estimate always renders to the same pixels. There is no:

- Adaptation to user history (learning what they do often)
- Adaptation to context (showing different views of the same state)
- Adaptation to skill level (simplifying for novices, exposing detail for experts)
- Recognition of user intent gradient (low-confidence intent vs high-confidence intent)
- Predictive presentation (showing what the user is likely to need next)

**Layer 5 is documented as future work** — see the `continuous-intent-speculation.md` and the `Estimate.confidence` field (currently decorative, future predictive use). Planex's confidence field is a placeholder for Layer 5 functionality.

### Source

- Hoffman, Singh, Prakash, *The Interface Theory of Perception* (2015): https://pubmed.ncbi.nlm.nih.gov/26384988/
- Wikipedia: https://en.wikipedia.org/wiki/Donald_D._Hoffman
- Karl Friston's *Free Energy Principle* (related, predictive coding): https://www.nature.com/articles/nrn2787

---

## Layer 6: Medium — UI is a dynamic medium for human expression

### Definition

> Alan Kay, Adele Goldberg, *Personal Dynamic Media* (1977).
>
> "We design, build, and use dynamic media which can be used by human beings of all ages as a medium for expression through drawing, painting, animating pictures..."
> — Kay 1977
> https://augmentingcognition.com/assets/Kay1977.pdf
>
> Douglas Engelbart, "A Research Center for Augmenting Human Intellect" (1968, the "Mother of All Demos").
> The demonstration introduced the mouse, hypertext, collaborative real-time editing, graphical windows — all framed not as *tools* but as *augmentations of human intellect*.
> — Wikipedia, "The Mother of All Demos"
> https://en.wikipedia.org/wiki/The_Mother_of_All_Demos

### What this layer says

UI's deepest essence is **medium** — a substrate for human expression and thought. The computer is not a calculator; it is a *dynamic medium*, and UI is how humans engage with this medium.

This is the most ambitious layer. It says UI should not just "help users do tasks" but should **enable new forms of thought, expression, and creativity**. A spreadsheet is a medium (it lets people think new thoughts). A piano is a medium. A good UI is one that becomes a *medium* for its users.

### What this layer does NOT say

It does not specify which mediums to support (text, image, sound, simulation, etc.) — only that UI's ultimate purpose is to be a medium.

### Planex status

❌ **Not implemented.** Planex is currently a *widget toolkit*, not a *medium*. There is no:

- Programmability by end users (no scripting, no extension)
- Compositional primitives that let users build new things
- Support for non-textual expression (no canvas, no audio, no simulation)
- Vision of UI as something the user *shapes* rather than *uses*

**Layer 6 is far future work.** Planex's current scope (widget library for embedded/desktop apps) is incompatible with the medium layer. Acknowledging this honestly means Planex does not aspire to be a medium — it aspires to be a good widget library. That is a defensible scope choice.

### Source

- Kay, Goldberg, *Personal Dynamic Media* (1977): https://augmentingcognition.com/assets/Kay1977.pdf
- Wikipedia, "Mother of All Demos": https://en.wikipedia.org/wiki/The_Mother_of_All_Demos
- Engelbart's archive: https://www.dougengelbart.org/theDemo
- Bret Victor, *Inventing on Principle* (2012): https://worrydream.com/dbx

---

## Summary table

| Layer | What UI is at this layer | Planex status | Theoretical source |
|---|---|---|---|
| 1. Physical | Space where interaction occurs | ✅ Implemented | Wikipedia, ACM Survey |
| 2. Cognitive | Bridge across two gulfs | ✅ Implemented (Closure 7-stage) | Norman 1988 |
| 3. Semantic | Medium of conversation | ✅ Implemented (Intent = speech act) | Winograd/Flores 1986 |
| 4. Behavioral | Presentation of action possibilities | ⚠️ Seed only (`PX_REL_AFFORDS`) | Gibson 1979, Heidegger 1927 |
| 5. Evolutionary | Adaptive interface for useful action | ❌ Not implemented | Hoffman 2015, Friston 2010 |
| 6. Medium | Dynamic medium for expression | ❌ Not implemented | Kay 1977, Engelbart 1968 |

---

## What this means for Planex

### The honest claim

Planex implements Layers 1–3 of UI essence fully, plus Layer 4's query half (intent compilation — the AFFORDS graph surface). It does **not** implement Layers 5 or 6, nor Layer 4's perception-side surfacing. The README's "seven abstractions" claim addresses exactly this scope; the per-layer census below is the fine print.

When the manifesto says "Planex is built on UI essence", what it actually means is "Planex is built on the cognitive + semantic layers of UI essence, plus the behavioral layer's query half (intent compilation)". The behavioral layer's presentation half, and the evolutionary and medium layers, are not in scope.

### What this changes

Previously, Planex's documentation framed "UI essence" as a single proposition ("intent space ↔ state space semantic interface"). This document corrects that — UI essence is **six nested layers**, and Planex implements Layers 1–3 fully plus Layer 4's query half (intent compilation).

This is more honest for three reasons:

1. **Future contributors** can see where Planex's scope ends and where extensions would go.
2. **Critics** cannot dismiss Planex for "claiming to capture UI essence but not implementing affordance adaptation" — the document explicitly admits this.
3. **Researchers** can locate Planex in the academic literature: it is a Layer 1–3 implementation, comparable to other Layer 1–3 libraries (most mainstream UI libraries are also Layer 1–3, with rare exceptions like Dynamicland which targets Layer 6).

### What this does NOT change

Planex's canonical set is seven abstractions (Relation, Estimate, Closure, Perception, px_loop, intent compilation, px_interaction — per ADR-0005/0008/0017/0018). The first five implement Layers 1–3; the 6th (intent compilation) is Layer 4's action-possibilities query. The v0.1-era "three core abstractions" framing this section originally recorded was superseded first by ADR-0005 (Perception) and ADR-0008 (px_loop), then by the v0.7 promotions.

Extending to Layer 5 (predictive adaptation) remains future work (the v4 derivation's essence #9, deferred); Layer 6 (medium) is explicitly out of scope (see [limitations](../state/limitations.md)).

---

## Comparison to other UI libraries

Where other UI libraries sit on this layered model:

| Library | Layers implemented |
|---|---|
| Planex | 1, 2, 3, 4-query (intent compilation, ADR-0017) |
| React / Vue / SwiftUI | 1, 2, 3 (no Layer 4 first-class, no 5/6) |
| Dear ImGui | 1, 2 (limited 3, no 4/5/6) |
| Qt | 1, 2, 3 + limited 4 (via QAction) |
| GTK | 1, 2, 3 + limited 4 (via GAction) |
| Emacs / Vim | 1, 2, 3, 6 (Layer 6 — Emacs is a medium) |
| Dynamicland | 1, 2, 3, 4, 5, 6 (full medium) |
| Bevy / Unity UI | 1, 2 (limited 3) |

**No mainstream UI library implements Layers 4–6.** This is not a Planex deficiency — it is an industry-wide gap. Dynamicland is the rare exception, and it requires physical-space hardware (projectors, cameras) that Planex does not target.

Planex is therefore **on par with industry leaders** for Layers 1–3, holds a first-class Layer 4 affordance query (rare in the industry — the strongest prior claim, CLIM's presentation-typed interaction, died with its host for economic reasons, not because the idea was refuted), and remains **acknowledged-but-not-implementing** for Layers 5–6. This is honest.

---

## See also

- [Why Four Abstractions](../canonical/why-four-abstractions.md) — Planex's manifesto (Layers 1–3)
- [Alternative Perspectives](alternative-perspectives.md) — the four academic schools
- [Continuous Intent Speculation](../speculation/continuous-intent-speculation.md) — Layer 5 future work
- [Limitations L1–L11](../state/limitations.md) — known gaps
- [ADR-0001](../../decisions/superseded/ADR-0001-perception-currently-noop.md) — Perception gap (Layer 4 implementation question)
- [Roadmap Matrix](../state/roadmap-matrix.md) — maturity tracking
- External sources linked inline at each layer

---

## Bibliography

- Alexander, Christopher. *A Pattern Language* (1977) and *The Nature of Order* (2003-2004). https://www.patternlanguage.com/
- Card, Moran, Newell. *The Psychology of Human-Computer Interaction* (1983). GOMS model.
- Conal Elliott. *Push-Pull Functional Reactive Programming* (2009). http://conal.net/papers/push-pull-frp
- Dourish, Paul. *Where the Action Is: The Foundations of Embodied Interaction* (2001). MIT Press.
- Engelbart, Douglas. "A Research Center for Augmenting Human Intellect" (1968). https://en.wikipedia.org/wiki/The_Mother_of_All_Demos
- Friston, Karl. "The free-energy principle: a unified brain theory?" *Nature Reviews Neuroscience* (2010). https://www.nature.com/articles/nrn2787
- Gibson, James J. *The Ecological Approach to Visual Perception* (1979).
- Heidegger, Martin. *Being and Time* (1927).
- Hoffman, Donald. "Interface Theory of Perception." *Psychonomic Bulletin & Review* (2015). https://pubmed.ncbi.nlm.nih.gov/26384988/
- Hutchins, Hollan, Norman. *Direct Manipulation Interfaces* (1985). https://www.lri.fr/~mbl/ENS/FundHCI/2013/papers/Hutchins-HCI-85.pdf
- Kay, Alan; Goldberg, Adele. "Personal Dynamic Media." *Computer* 10(3) (1977). https://augmentingcognition.com/assets/Kay1977.pdf
- Ko, Andrew. "User Interface Software and Technology: A Theory." https://faculty.washington.edu/ajko/books/user-interface-software-and-technology/theory
- Morales Díaz, LV, Toon, A. "What is a User Interface, again? A Survey of Definitions." *CHI* (2022). https://dl.acm.org/doi/fullHtml/10.1145/3565494.3565504
- Norman, Don. *The Design of Everyday Things* (1988).
- Sutherland, Ivan. *Sketchpad: A Man-Machine Graphical Communication System* (1963). https://dspace.mit.edu/entities/publication/b5e8025c-c8b2-4843-84e0-76db824e07e6
- Winograd, Terry; Flores, Fernando. *Understanding Computers and Cognition* (1986).
