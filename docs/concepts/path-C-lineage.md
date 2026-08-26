# Planex as the Latest Attempt at Path C

> **Status: Honest acknowledgment.** Planex is not an isolated project. It is the latest in a 60-year line of attempts at "constraint-graph + time-function + speech-act" UI. Most previous attempts failed. This document records the lineage honestly so future maintainers and critics can locate Planex in its real historical context.

This document is the canonical answer to "is Planex novel?" The answer is **no** — Planex is a synthesis of ideas from the 1960s, refined and combined in a way that has not been tried before, but the ideas themselves are not new.

---

## What is "Path C"?

Across the 60-year history of UI implementation, three fundamentally different paths have emerged:

### Path A: Component Tree (mainstream)

UI = tree of components + per-component state + events.

Examples: React, Vue, SwiftUI, Flutter, Qt, GTK.

Status: dominant. Captures ~95% of production UI.

### Path B: Immediate Mode (niche)

UI = sequence of draw calls per frame, no retained state.

Examples: Dear ImGui, raylib.

Status: stable niche. Captures game tools and debug UI.

### Path C: Constraint Graph + Time Function

UI = graph of constraints between typed values + time-varying behaviors + speech-act typed intents.

Examples: Sketchpad (1963), Conal Elliott's FRP (1997), Eve (2014-2018), Subtext (2005-2015), Planex (2025+), and a few others.

Status: **never reached mainstream**. Every Path C project in 60 years has either died, retreated to academia, or pivoted to a hybrid.

**Planex is on Path C.** This document records what that means.

---

## The Path C lineage — full history

| Year | Project | Author(s) | Outcome |
|---|---|---|---|
| 1963 | Sketchpad | Ivan Sutherland | PhD thesis, foundational, but hardware couldn't run it fast enough; concept shelved |
| 1968 | FLEX / Reactive Engine | Alan Kay | Tried "object-oriented + reactive" UI; abandoned for Dynabook vision |
| 1970s | Smalltalk | Alan Kay, Xerox PARC | Originally Path C; drifted to component-tree over time |
| 1980s | Garnet interactor model | Brad Myers | Tried constraint-based interactors; never mainstream |
| 1986 | Winograd/Flores | Terry Winograd, Fernando Flores | Theoretical foundation (speech-act theory) — never implemented as UI library |
| 1990s | Genera presentation types | Symbolics | Constraint-driven UI; died with Symbolics |
| 1997 | Conal Elliott FRP | Conal Elliott | Original FRP; never escaped Haskell |
| 2005 | Subtext | Jonathan Edwards (MIT) | "Programming by example"; abandoned ~2015 |
| 2006 | Lively Kernel | Dan Ingalls | Re-attempt of Smalltalk in browser; faded |
| 2011 | Reactive-Banana | Heinrich Apfelmus | Haskell FRP library; alive but niche |
| 2013 | Elm | Evan Czaplicki | Started as FRP, pivoted to "The Elm Architecture" (closer to Path A) |
| 2014 | Eve | Chris Granger et al. | "Programming designed for humans"; died 2018 |
| 2015 | Reflex | Ryan Trinkle | Haskell FRP; alive but niche |
| 2017 | Eve (final shutdown) | — | Officially archived |
| 2019 | Reactive-Banana maintenance mode | — | Mostly unmaintained |
| 2022 | Xilem | Raph Levien, Linebender | Active research project; Rust UI architecture |
| 2025 | Planex | oldaying | Current attempt — pre-v1.0 |

---

## Why Path C keeps failing

Path C projects have died from a consistent set of causes. **Every failure mode below has killed at least one previous Path C attempt.** Planex must avoid all of them.

### Failure mode 1: Simultaneously changing language + paradigm + deployment

**Worst offender: Eve (2014-2018).**

Eve tried to:
- Replace programming language (new logic-programming syntax)
- Replace UI paradigm (relational, not component)
- Replace deployment (custom IDE, not library)

The result: nothing was finished. Each axis competed for attention. Users couldn't adopt any one piece without the others. Eve died in 2018.

**Quote from Eve's own retrospective:**
> "I think Eve is tackling the wrong problem. Allow me an analogy: 'Bronk, the math designed for humans.'"

— Hacker News comment summarizing Eve's failure
https://news.ycombinator.com/item?id=12817468

### Failure mode 2: Tying the abstraction to a specific language ecosystem

**Worst offender: Conal Elliott FRP (1997+).**

FRP was conceptually elegant but implemented only in Haskell. To use FRP you had to:
- Learn Haskell
- Learn monad transformers
- Accept lazy evaluation semantics
- Work within Haskell's type class system

Outside Haskell, FRP was invisible. Inside Haskell, it was elegant but had no production use cases. The result: 25 years of academic influence, zero industry adoption.

### Failure mode 3: Theoretical ambition without engineering proof

**Worst offender: Subtext (2005-2015).**

Jonathan Edwards' Subtext promised "programming by example" with a radical new model:
- No textual code
- Programs are tables
- Execution = representation

The theoretical work was beautiful. But the implementation never reached production quality. There was no clear "Minimum Viable Subtext" that someone could ship. The project died in 2015.

### Failure mode 4: Premature ecosystem ambition

**Worst offender: Lively Kernel (2006-2010).**

Dan Ingalls (Smalltalk co-creator) tried to build a "new Smalltalk for the web." The vision: a complete programming environment in the browser, with classes, tools, versioning, all interconnected.

The result: too much, too soon. The browser wasn't ready (2006 had no WebAssembly, no Canvas2D fast enough). The project faded.

### Failure mode 5: Confusing "research" with "production"

**Worst offender: every Haskell FRP library.**

reactive-banana, Reflex, Yampa — all are beautiful research artifacts, none are production-ready. They live in their ecosystem as references, not as shipped products.

This is not a failure of the project — it's a choice. But it means the abstraction never reaches the industry that needs to learn from it.

### Failure mode 6: No incremental adoption path

**Worst offender: Eve (again).**

Eve could not be adopted incrementally. You either rewrote your whole app in Eve, or you didn't use it. This killed adoption — no one rewrites a working app to try a new paradigm.

React succeeded partly because you could embed React in one component of an existing jQuery app. Eve failed partly because you couldn't.

### Failure mode 7: Author burnout / single maintainer

**Worst offender: many.**

Subtext died when Jonathan Edwards moved on. Eve died when Chris Granger's funding ran out. Reflex has reduced activity. Single-maintainer research projects are fragile.

---

## How Planex is positioned relative to these failures

For each failure mode, here is Planex's strategy:

### Strategy 1: Don't change the language

- ✅ **Planex uses C17.** Not a new language. Anyone who programs can read C.
- ✅ **Zero external dependencies.** Can be embedded in any C project.
- ✅ **Library, not framework.** Drop in, drop out.

This avoids Eve's failure mode 1 (simultaneously changing everything).

### Strategy 2: Don't tie to a language ecosystem

- ✅ **C is universal.** Planex can be called from C++, Rust (via FFI), Zig (via FFI), Python (via cffi), JavaScript (via WASM).
- ✅ **The abstractions are language-agnostic.** Relation/Estimate/Closure could be reimplemented in any language.

This avoids Conal FRP's failure mode 2 (Haskell-only).

### Strategy 3: Provide engineering proofs

- ✅ **25 widget demos** prove the abstractions can express common UI patterns.
- ✅ **Real applications** (todo_app, counter_interactive) prove the abstractions work in real windows with real interaction.
- ✅ **Unit tests** prove the abstractions are testable.
- ✅ **Continuous integration** runs on every push.

This avoids Subtext's failure mode 3 (theoretical ambition without engineering proof).

### Strategy 4: Don't try to be a complete ecosystem

- ✅ **Planex is a widget library, not an IDE / language / runtime.**
- ✅ **No tooling ambitions.** Planex doesn't ship a debugger, profiler, or editor.

This avoids Lively Kernel's failure mode 4 (premature ecosystem ambition).

### Strategy 5: Be honest about research vs production

- ✅ **README explicitly says "not production-ready."**
- ✅ **v0.1, not v1.0.** API may break between minor versions.
- ✅ **ADRs document what's known and unknown.**

This avoids failure mode 5 (research pretending to be production).

### Strategy 6: Don't allow incremental adoption (yet)

⚠️ **This is Planex's current biggest risk.**

Planex is a library, but adopting it requires committing to its four abstractions. There's no "embed Planex in one component of an existing React app" story.

**Honest acknowledgment:** this is a known weakness. Planex cannot be incrementally adopted the way React was. Adoption options are:

1. Write a new app from scratch in Planex (high commitment)
2. Use Planex for a single subsystem that's separate from main app (possible but awkward)
3. Embed Planex via a C library boundary (possible but requires C interop)

This is the strongest reason Planex may fail to reach mainstream — even if the abstractions are right, adoption requires a greenfield commitment.

### Strategy 7: Single maintainer bus factor

⚠️ **Currently true.** Planex has one author.

- Mitigation: ADRs externalize decisions
- Mitigation: docs/decisions/ records rationale
- Mitigation: Limitations document known gaps
- No mitigation: if the author stops, the project stops

This is a real risk. It will be addressed (if at all) only when there's enough interest to attract contributors.

---

## What "the latest attempt" means

Planex is not "Path C done right this time". That would be naive. Path C's failure modes are real and Planex is exposed to most of them.

What "the latest attempt" means:

1. **Planex builds on 60 years of failure.** Each previous Path C failure teaches what to avoid.
2. **Planex's combination is new.** The specific synthesis of Norman + Winograd/Flores + Conal + Alexander, in C17, as a library, is novel — even though each piece is not.
3. **Planex may still fail.** The Path C failure rate is high. Honest acknowledgment means accepting that Planex could die like Eve or Subtext, and writing the project so that even if it dies, the lessons survive.

---

## What "honest acknowledgment" commits Planex to

By virtue of this document, Planex commits to:

### Commitment 1: Cite predecessors honestly

When documentation describes Planex's abstractions as derived from prior art, it must cite the prior art correctly. Planex did not invent:
- Constraint-driven UI (Sketchpad, 1963)
- Continuous-time FRP (Conal Elliott, 1997)
- Speech-act Intent (Winograd/Flores, 1986)
- 7-stage interaction model (Norman, 1988)
- Semilattice vs tree (Alexander, 1965)
- Predictive coding (Friston, 2010)

Planex's novelty is **the combination in C17 as a library**, not any individual abstraction.

### Commitment 2: Track failure modes

Planex's roadmap and ADRs must consider whether each decision exposes Planex to one of the seven failure modes. If a decision risks a known failure mode, the ADR must record this and justify why the risk is acceptable.

### Commitment 3: Accept that death is a possible outcome

This is the hardest commitment. Planex may die. If it does, the responsibility is to ensure:
- The abstractions are documented well enough that someone could reimplement them
- The ADRs explain why decisions were made, so future attempts can learn
- The failure (if it happens) is documented as a postmortem, not silently abandoned

This is what differentiates a research project from a startup. Startups die quietly. Research projects die with their findings intact.

### Commitment 4: Don't over-claim

Planex's README, manifesto, and docs must not claim "Planex is the future of UI" or "Planex will replace React". Such claims are inconsistent with the historical record of Path C failures.

The honest claim is: **"Planex is an experiment in whether Path C can be made practical, building on 60 years of prior attempts."** Nothing more.

---

## What this means for Planex's narrative

Previous Planex documentation sometimes implied Planex was unique or pioneering. This document corrects that:

- Planex is not unique. It is one of many Path C attempts.
- Planex is not pioneering. Sketchpad pioneered constraint UI in 1963, 62 years before Planex.
- Planex is not "the answer." It is a test of whether the answer (if Path C is the answer) can be made practical in C17.

This reframing is more honest. It also has practical benefits:

1. **Critics cannot dismiss Planex as "unproven novel ideas"** — the ideas have been studied for 60 years. Planex's question is about implementation, not theory.
2. **Contributors can locate their work** — they are contributing to a 60-year tradition, not a single project.
3. **Planex's failure (if it happens) is not a failure of the ideas** — it's a failure of one attempt at the ideas, joining a list of others.

---

## See also

- [Why Four Abstractions](why-four-abstractions.md) — Planex's manifesto (the synthesis that makes Planex's attempt different from predecessors)
- [UI Essence Layers](ui-essence-layers.md) — what layers of UI essence Planex implements
- [Alternative Perspectives](alternative-perspectives.md) — the academic schools Planex draws from
- [Limitations L10](limitations.md) — single-maintainer bus factor
- [Non-Goals](non-goals.md) — what Planex deliberately does not aspire to
- [Roadmap Matrix](roadmap-matrix.md) — current state of the attempt

---

## External sources

- Sketchpad (Sutherland 1963): https://dspace.mit.edu/entities/publication/b5e8025c-c8b2-4843-84e0-76db824e07e6
- Conal Elliott FRP: http://conal.net
- Winograd/Flores, *Understanding Computers and Cognition* (1986): https://philpapers.org/rec/WINUCA
- Eve (postmortem discussion): https://news.ycombinator.com/item?id=12817468
- Subtext (Jonathan Edwards): https://en.wikipedia.org/wiki/Subtext_(programming_language)
- Xilem (Raph Levien): https://raphlinus.github.io/rust/gui/2022/05/07/ui-architecture.html
- Christopher Alexander, *A City is Not a Tree* (1965): https://www.patternlanguage.com/archive/cityisnotatree.html
- Don Norman, *The Design of Everyday Things* (1988): https://en.wikipedia.org/wiki/The_Design_of_Everyday_Things
- Karl Friston, *Free Energy Principle* (2010): https://www.nature.com/articles/nrn2787
- Alan Kay, *Personal Dynamic Media* (1977): https://augmentingcognition.com/assets/Kay1977.pdf
