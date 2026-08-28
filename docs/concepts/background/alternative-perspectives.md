# Alternative Perspectives on UI Essence

> **Applies to**: v0.4. UI essence is not a single theory. It is a contested concept with **four academic schools**, each making different ontological assumptions about what UI fundamentally is. Planex has chosen two of the four; the other two are recorded here as alternative perspectives that Planex does not adopt but acknowledges as legitimate.

This document exists to keep Planex honest about its philosophical commitments. If a future maintainer asks "why does Planex model UI this way and not that way?", the answer is here — not in intuition.

---

## The four schools

| School | Core claim | UI essence is... | Primary sources |
|---|---|---|---|
| **Cognitive** | UI is a bridge across two gulfs (execution + evaluation) | Information transfer between intent space and state space | Norman 1988, Hutchins/Hollan/Norman 1985, Card/Moran/Newell 1983 |
| **Mathematical/Linguistic** | UI is a denotation; intent is speech act | State's mathematical meaning; intent as typed value | Conal Elliott, Winograd/Flores 1986, Sutherland 1963 |
| **Phenomenological** | UI is embodied meaning formation | Meaning emerges in user-environment coupling, not pre-existing | Dourish 2001, Ishii 1997, Heidegger 1927 |
| **Neural/Predictive** | UI is a prediction loop | State is a predictive model; perception minimizes surprise | Friston 2010, Predictive coding |

Planex adopts the **Cognitive** and **Mathematical/Linguistic** schools. It does not adopt the Phenomenological or Neural/Predictive schools. This document records all four so the choice is explicit, not implicit.

---

## School 1: Cognitive (Planex adopts)

### Core claim

UI is a **bridge** between human cognition and machine state. There are two "gulfs" to cross:

- **Gulf of Execution**: from user's goal to system state change
- **Gulf of Evaluation**: from system state to user's understanding

UI design quality = how narrow these gulfs are.

### Foundational sources

- **Don Norman, *The Design of Everyday Things* (1988)** — 7-stage action model
  - Wikipedia: https://en.wikipedia.org/wiki/The_Design_of_Everyday_Things
  - MIT Press: https://mitpress.mit.edu/9780262640374/the-design-of-everyday-things

- **Hutchins, Hollan, Norman, *Direct Manipulation Interfaces* (1985)** — Gulfs of execution/evaluation
  - Paper: https://www.lri.fr/~mbl/ENS/FundHCI/2013/papers/Hutchins-HCI-85.pdf
  - Cited 2620+ times

- **Card, Moran, Newell, *The Psychology of Human-Computer Interaction* (1983)** — GOMS model
  - Wikipedia: https://en.wikipedia.org/wiki/CMN-GOMS
  - Treats interaction as goal-directed; discrete stages

### What this school says UI is

- UI is **information transfer** — meaning exists in user's head and machine's state, UI transfers it across
- UI abstractions should model **the transfer** (intent → action → state → perception)
- Norman's 7 stages are the canonical decomposition

### How Planex uses this school

- **Closure** is a direct implementation of Norman's 7-stage model
  - Goal → Intent → Action → Execution → Perception → Interpretation → Evaluation
- Planex's essence statement 1 ("UI is a semantic interface between intent space and state space") is the cognitive school's stance
- Planex's essence statement 3 ("constraint is human cognitive bandwidth") is also cognitive school — it's about reducing the cognitive cost of crossing the gulfs

### What this school does NOT address

- **Meaning formation** — assumes meaning is pre-existing and transferred, not constructed in interaction
- **Time continuity** — GOMS treats interaction as discrete stages; doesn't model continuous intent gradient
- **Embodiment** — assumes UI is on a screen, not in physical space

---

## School 2: Mathematical/Linguistic (Planex adopts)

### Core claim

UI is a **mathematical denotation**. Every UI type has a mathematical meaning; every operation preserves that meaning. Intent is a **speech act** — a typed value, not a callback.

### Foundational sources

- **Conal Elliott — Denotational Design + FRP**
  - Home: http://conal.net
  - "Denotational Design: From Meanings To Programs" — https://www.youtube.com/watch?v=rlyqoYoUumc
  - "The Essence & Origins of Functional Reactive Programming" — https://www.youtube.com/watch?v=rfmkzp76M4M
  - Push-pull FRP paper (2009)

- **Terry Winograd, Fernando Flores, *Understanding Computers and Cognition* (1986)** — speech-act theory applied to computation
  - PhilPapers: https://philpapers.org/rec/WINUCA
  - Cited 11218+ times
  - Draws on Heidegger + Maturana + Austin

- **Ivan Sutherland, *Sketchpad* (1963)** — constraint-driven UI
  - MIT thesis: https://dspace.mit.edu/entities/publication/b5e8025c-c8b2-4843-84e0-76db824e07e6
  - Cited 4787+ times

### What this school says UI is

- UI is **mathematical structure** — types have denotations, operations preserve them
- State is `Behavior = Time → α` (Conal)
- Intent is a typed value (Winograd/Flores): ASSERT / REQUEST / PROMISE / DECLARE / EXPRESS
- Relations are constraint graphs (Sutherland)

### How Planex uses this school

- **Estimate** is Conal's `Behavior = Time → α` (plus Friston's confidence — see school 4)
- **Closure's Intent kinds** are directly Winograd/Flores speech acts
  - `PX_INTENT_ASSERT / REQUEST / PROMISE / DECLARE / EXPRESS`
- **Relation** is Sketchpad's constraint graph + Alexander's semilattice
  - See Alexander, *A City is Not a Tree* (1965): https://www.patternlanguage.com/archive/cityisnotatree.html

### What this school does NOT address

- **Meaning's origin** — assumes meaning is in the type's denotation; doesn't ask how the user forms that meaning
- **Phenomenological coupling** — UI is abstracted away from the body
- **Empirical uncertainty** — pure denotations are deterministic; reality isn't

---

## School 3: Phenomenological (Planex does NOT adopt)

### Core claim

UI is **embodied meaning formation**. Meaning is not pre-existing in the user's head or the machine's state — it **emerges** in the coupling of body, environment, and tools. UI's job is to support this emergence.

### Foundational sources

- **Paul Dourish, *Where the Action Is: The Foundations of Embodied Interaction* (2001)**
  - MIT Press: https://direct.mit.edu/books/monograph/3875/
  - "Embodied interaction locates the formation of meaning and action in ongoing engagement among bodies, people, objects, and spatial settings"

- **Hiroshi Ishii, *Tangible Bits* (1997)**
  - MIT Tangible Media: https://tangible.media.mit.edu/project/tangible-bits/
  - ACM paper: https://dl.acm.org/doi/10.1145/258549.258715
  - "Towards seamless interfaces between people, bits and atoms"

- **Martin Heidegger, *Being and Time* (1927)** — ready-to-hand (Zuhandenheit)
  - Discussion: https://pmc.ncbi.nlm.nih.gov/articles/PMC2834739/
  - Tools disappear when used skillfully ("ready-to-hand"); they become visible only when they break down ("present-at-hand")

### What this school says UI is

- UI is **a site of meaning formation**, not a transfer channel
- State has no inherent meaning — meaning is **constructed by the user** in a specific context
- Same state, different context → different meaning
- Good UI "disappears" — user is absorbed in the task, not aware of UI

### What this school would propose as abstractions

If Planex adopted this school, the abstractions would be different:

- **Context** — the user's current activity frame (what they're doing)
- **Visibility** — how state's history is made perceivable (not just current value, but trajectory)
- **Trace** — record of how the user got here (cognitive history, not just state history)
- **Affordance** — what the user can do now (as a perceived possibility, not a button)

These four would replace or augment Relation/Estimate/Closure/Perception. The semantic shift is from "state and transfer" to "context and meaning formation".

### Why Planex does NOT adopt this school

1. **Planex's chosen language (C17) and target (embedded/desktop) are not phenomenologically native**. Phenomenology assumes rich embodiment — touch, gesture, spatial coupling. Planex targets screens + pointers + keyboards.

2. **Planex's 25 demos are all discrete state-manipulation UIs** (counter, slider, form, todo). Phenomenological abstractions are overkill for these — they're designed for creative/exploratory UIs (IDE, design tools, data analysis).

3. **Planex's manifesto emphasizes explicit abstractions**, which is opposite to "tools disappear". Planex wants users to see and control the abstractions; phenomenology wants abstractions to vanish.

### Implications Planex acknowledges but does not address

- Planex's abstractions cannot express **how meaning forms** — only how it transfers
- Planex UIs will feel "explicit" rather than "absorbing" — users will be aware of the abstractions, not just the task
- For creative/exploratory UIs (IDE, design tools, data analysis), Planex will need to extend its abstractions — see `meaning-formation-speculation.md` (if it exists)

---

## School 4: Neural/Predictive (Planex partially adopts)

### Core claim

UI is a **prediction loop**. The brain is a prediction machine that minimizes "surprise" (free energy). State is not a fact — it's a **predictive model** with confidence. Perception is the brain comparing prediction to sensory input and updating the model.

### Foundational sources

- **Karl Friston, *The free-energy principle: a unified brain theory?* (Nature, 2010)**
  - Nature: https://www.nature.com/articles/nrn2787
  - "Different global brain theories all describe principles by which the brain optimizes value and surprise"

- **Friston, *Predictive coding under the free-energy principle* (PMC)**
  - https://pmc.ncbi.nlm.nih.gov/articles/PMC2666703/
  - "The brain models the world... perception and action minimize free energy"

- Wikipedia overview: https://en.wikipedia.org/wiki/Free_energy_principle

### What this school says UI is

- State is a **predictive model**, not a fact — `Estimate = value + confidence`
- Perception is **prediction error minimization** — the brain compares prediction to input and updates
- UI should expose the **prediction loop**, not just the value
- "Surprise" (low confidence) is the signal that drives learning and attention

### How Planex partially uses this school

- **Estimate has a `confidence` field** (0.0 to 1.0) — directly borrowed from Friston's predictive coding
- The field exists but is **not yet used in a prediction loop**
  - Most Estimates set confidence = 1.0 (full certainty)
  - No API for "predict X → observe Y → update confidence"
- The field is a placeholder for future predictive functionality

### What Planex does NOT use from this school

- **Active inference** — Friston's full theory says the brain not only predicts but **acts** to minimize surprise. Planex's Closure does intent, but doesn't tie intent to "minimize prediction error".
- **Hierarchical priors** — Friston's predictive models are hierarchical (low-level predictions feed high-level). Planex's Estimate is flat — no hierarchy.
- **Free energy minimization as a UI principle** — UI could be designed to minimize user surprise (sudden changes are surprising; smooth transitions reduce prediction error). Planex doesn't use this.

### Implications Planex acknowledges

- The `confidence` field in Estimate is currently **decorative** — it has a theoretical foundation but no functional use
- A future Planex could implement "predictive Estimates" — Estimates that forecast future values and update based on observation
- This is recorded as future-research, not current commitment

---

## Why Planex chose Cognitive + Mathematical/Linguistic

### The combination is natural

The two adopted schools are complementary:

- **Cognitive school** provides the *structural* model — 7 stages, two gulfs
- **Mathematical/Linguistic school** provides the *formal* model — denotations, speech acts, constraint graphs

Together they answer:
- *What structure does UI have?* (Norman's 7 stages)
- *What do the parts mean mathematically?* (Conal's denotations, Winograd's speech acts)
- *How do parts relate?* (Sutherland's constraint graph)

### The two non-adopted schools would force different commitments

- **Phenomenological** would require Planex to target embodied/tangible UIs, not screen UIs
- **Neural/Predictive** would require Planex to implement full prediction loops, not just static confidence fields

Planex's current scope (screen UIs, discrete abstractions) is incompatible with these schools' full demands. So Planex acknowledges them but doesn't adopt them.

---

## What this means for Planex's future

### If Planex ever extends to creative/exploratory UIs

It will need to absorb parts of the Phenomenological school. The four phenomenological abstractions (Context, Visibility, Trace, Affordance) are not current Planex abstractions, but Planex's `PX_REL_AFFORDS` relation is a seed of Affordance. See `meaning-formation-speculation.md` (if it exists) for details.

### If Planex ever implements predictive state

It will need to extend Estimate with active inference — the prediction-update loop. This is currently speculation. The `confidence` field is the placeholder.

### If Planex never does either

That's also fine. Planex's current scope (discrete state-manipulation UIs for embedded/desktop) is well-served by Cognitive + Mathematical/Linguistic schools. The two non-adopted schools are recorded here to keep the choice honest, not to force future adoption.

---

## Summary table

| School | Adopted by Planex? | Used in which abstraction | What it provides |
|---|---|---|---|
| Cognitive | ✅ | Closure (7 stages), essence statements 1+3 | Structural model: 7 stages, two gulfs |
| Mathematical/Linguistic | ✅ | Estimate (FRP), Closure Intent (speech acts), Relation (constraint graph) | Formal model: denotations, typed intent, semilattice |
| Phenomenological | ❌ | (none, but `PX_REL_AFFORDS` is a seed) | Meaning formation: Context, Visibility, Trace, Affordance |
| Neural/Predictive | ⚠️ partial | Estimate.confidence field only | Prediction loop, free energy minimization |

---

## See also

- [Why Four Abstractions](../canonical/why-four-abstractions.md) — Planex's manifesto
- [Continuous Intent Speculation](../speculation/continuous-intent-speculation.md) — related speculation about intent gradient
- [Limitations L11](../state/limitations.md) — multi-frame interaction process gap (related to phenomenological school)
- [ADR-0001](../../decisions/superseded/ADR-0001-perception-currently-noop.md) — Perception gap (related to all four schools)
- External sources linked inline throughout this document
