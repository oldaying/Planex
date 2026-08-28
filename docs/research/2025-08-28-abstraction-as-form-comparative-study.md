# Abstraction as Organizational Form — Comparative Study (Is Abstraction the Best Form for Planex?)

> **Author**: Planex docs engineering
> **Date**: 2025-08-28
> **Status**: Reference (research output, not a decision)
> **Applies to**: v0.4 (5 shipping abstractions: Estimate / Perception / Closure / Relation / px_loop) and v4 design proposal (8 abstractions: adds Interpretant / Perlocution / Breakdown)
> **Scope**: Survey of organizational forms alternative to "abstraction" for a UI library; cross-validation against Planex's chosen form; gap analysis and recommendations.
> **Companion document**: [`why-four-abstractions.md`](../concepts/canonical/why-four-abstractions.md) argues *which* abstractions; this report argues *whether abstraction is the right form*.

---

## Executive Summary

This report surveys eight alternative organizational forms a UI library could adopt instead of "abstraction" (DSL, component library, pattern language, ECS, FRP, data-driven config, Kay-OOP, tagless final), six major critique traditions that attack abstraction-heavy designs (Worse-is-Better, Simple Made Easy, Leaky Abstractions, Abstraction Inversion, Rule of Three, Over-Abstraction/End-of-Civ), five philosophy-driven design precedents where theory was operationalized into software (Winograd/Flores Coordinator, Conal Elliott's Fran/Pan, Dourish/Ishii embodied UI, Friston's free-energy principle, Kay's Smalltalk), and nine production C/UI libraries for calibration (SDL, GLib/GObject, Cairo, Wayland, libuv, GTK, Qt, Dear ImGui, Redis). The survey yields a clear verdict: **abstraction is the correct primary form for Planex's stated goals** (intent-as-value, multi-channel denotation, semantic audit, cognitive-bandwidth constraint), and no equally-good alternative satisfies all of those constraints simultaneously. The verdict comes with three caveats: (1) Planex must explicitly distinguish its "abstraction-as-typed-value" form from the "abstraction-as-encapsulation" form that the critique literature attacks; (2) Planex must rebut the Rule of Three, which is the strongest external critique of essence-justified abstraction; (3) Planex must quantify per-abstraction leak budgets, since Spolsky's law guarantees leaks in any non-trivial abstraction. Seven specific gaps are identified, with three tiers of recommendations ranging from immediate documentation fixes to long-term fallback-form planning.

---

## 1. Context: Planex's Form Choice

Planex's API is organized around a small fixed set of named concepts chosen by essence, not by inspiration or by duplication-removal. v0.4 ships 5 such abstractions: `Estimate` (state with confidence, after Friston's predictive coding and Conal Elliott's `Behavior = Time → α`), `Perception` (machine-to-human denotation, after Peirce's representamen), `Closure` (human-to-machine speech act, after Winograd/Flores and Searle), `Relation` (constraint graph, after Sutherland's Sketchpad and Alexander's semilattice), and `px_loop` (closed-loop coupling, after CSP and statecharts). Each abstraction is a *type* + *denotation* + *operations preserving the denotation* — Conal Elliott's denotational design pattern. The form is explicitly NOT a component library, NOT a DSL, NOT a pattern catalog, NOT a data-driven configuration. The v4 essence-rederivation proposal (see [essence-derivation-v4-clean.md](../concepts/history/essence-derivation-v4-clean.md)) extends this to 8 abstractions by adding `Interpretant` (Peirce's interpretant vs representamen distinction), `Perlocution` (Searle's illocution vs perlocution distinction), and `Breakdown` (Heidegger's Zuhandenheit). [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) reframes v4 as "design rationale, not essence discovery" — a downgrade acknowledging that the 8-abstraction proposal is one design proposal among possible others.

The question this report answers: **is abstraction the best organizational form for Planex, or should it be a DSL, a component library, a pattern language, or something else?**

---

## 2. Methodology

The survey used the `web_search` SDK function across 32 distinct query topics, returning 8–10 results per query (~280 candidate sources). Queries spanned five dimensions: (A) alternative organizational forms (DSL, component library, pattern language, ECS, FRP, data-driven, Kay-OOP, tagless final, algebraic effects); (B) critique movements (Worse-is-Better, Simple Made Easy, Leaky Abstractions, Abstraction Inversion, Rule of Three, SOLID criticism, over-abstraction); (C) philosophy-driven design precedents (Winograd/Flores, Conal Elliott denotational design, Peirce semiotics, Heidegger phenomenology, Friston free-energy, Brooks No Silver Bullet); (D) UI library form comparison (React hooks vs classes, Qt model-view, SwiftUI vs UIKit, Elm architecture, Dear ImGui retained vs immediate, Qt Widgets vs QML, GTK hierarchy, XUL, OpenLaszlo); (E) production C library calibration (SDL, GLib/GObject, Cairo, Wayland, libuv, Plan 9, Redis, D language). Seven canonical sources were fetched in full via `page_reader`: jwz.org's "Worse Is Better" archive, datagubbe.se's "Is Software Abstraction Killing Civilization?", InfoQ's transcript of Hickey's "Simple Made Easy", Eric Normand's podcast notes on denotational design, Wikipedia's "No Silver Bullet", Wikipedia's "Entity Component System", and Wikipedia's "Speech Act". Selection criteria for inclusion in the synthesis: (a) the source is the canonical/authoritative origin of the practice, (b) the practice is widely cited (≥100 citations for academic sources, ≥3 major adopters for industry practices), (c) the practice is applicable to a single-maintainer C library with explicit essence-chosen abstractions.

---

## 3. Eight Alternative Organizational Forms (Compared to Planex's Abstraction)

### 3.1 Domain-Specific Language (DSL)

A DSL is a small programming language specialized to a domain — SQL for relational queries, regex for pattern matching, XUL for Mozilla UI, QML for Qt Quick, OpenLaszlo's XML dialect for rich web apps, and Redis's command language which antirez explicitly calls "a DSL that manipulates abstract data types". DSLs make the domain readable to non-programmers and replaceable at the semantics level, but they move parser, syntax, versioning, and debugging into the library, and they suffer from Greenspun's Tenth Rule ("any sufficiently complicated imperative language ends up with a half-implemented Common Lisp"). For Planex, a DSL would require a C-side parser (the user could not just call `px_estimate_new(...)` in C), would lock users out of using C's type system to compose Planex with other libraries, and would break Planex's "integrate as a C type" promise. Verdict: inferior as the primary form; could serve as a tier-3 *alternative API surface* (a declarative DSL layered on top of the 5 abstractions for users who want hot-reloadable UI definitions).

### 3.2 Component Library (GTK widgets, Qt Widgets, React component libraries)

A component library ships ready-made `Button`, `Slider`, `Menu`, `Form` objects that users instantiate. The strengths are immediate ergonomics (copy-paste a button, it works) and a clear mental model (everything is a widget). The weaknesses are opacity (the "widget" hides the underlying meaning; you cannot ask "what does this button *denote*") and implicit relations (the connection between two widgets is captured in event handler code, not in a queryable structure). For Planex, a component library would contradict [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md)'s framing — it would put the WHAT (a button) before the WHY (the essence categories). Planex *does* ship components (`px_button` is built on top of Estimate + Closure + Relation), but components are the second-class form, not the primary one. Verdict: orthogonal — components compose with abstractions, they do not replace them.

### 3.3 Pattern Language (Christopher Alexander, 1977)

Alexander's *A Pattern Language* introduced the idea of cataloguing recurring design problems with their contextual solutions, naming each pattern (e.g., "Light on Two Sides of Every Room"). The pattern-language tradition deeply influenced the GoF *Design Patterns* book, Extreme Programming, Ward Cunningham's wiki, and the early agile movement. Patterns are pedagogical, contextual, and lived-in — they capture *what works in practice*. But patterns are recipes, not types: they cannot be denotationally specified, they cannot be composed algebraically, and as Tomas Petricek argues in *The Timeless Way of Programming* (2022), they leave the "what does it mean" question unanswered. For Planex, the abstractions are *documented* as quasi-patterns in `docs/concepts/`, but the form is not pattern-based — the abstractions are first-class typed values that compose algebraically; patterns in `docs/concepts/state/ui-pattern-coverage.md` are descriptive, not prescriptive. Verdict: complementary as the documentation layer, not viable as the primary form.

### 3.4 Entity-Component-System (ECS)

ECS is the dominant abstraction in modern game engines (Unity DOTS, Bevy, Unreal Mass). The form splits the world into Entities (IDs), Components (plain-old-data structs attached to entities), and Systems (functions that iterate over all entities with a given component set). ECS is data-oriented, cache-friendly, parallelizable, and Mike Acton's 2014 C++ talk "Data-Oriented Design" is its canonical manifesto. For Planex, an ECS could in principle express `px_actor` + `px_relation` (entities are actors, components are estimates and perceptions, systems are closure dispatchers). But ECS optimizes for throughput (frames per second), not semantic clarity (cognitive bandwidth per UI). Planex's stated constraint per `why-four-abstractions.md` is *cognitive bandwidth*, not *frame rate*. Verdict: wrong optimization target for Planex's scope.

### 3.5 Functional Reactive Programming (FRP, Conal Elliott 1997)

FRP defines `Behavior = Time → α` (continuous values) and `Event = [(Time, α)]` (discrete occurrences), and provides a denotational semantics for time-varying values. FRP's strengths: it is denotational, compositional, and provides a clear "what does state mean" answer. Its weaknesses: original Haskell FRP is academic; Conal's "Push-pull FRP" (2009) remains research-grade; the RxJS/Reactive Streams family is *called* FRP but Conal explicitly disowns them as "FRP-wannabes" that lost the continuous-time semantics. For Planex, `Estimate` IS Conal's `Behavior` (with a `confidence` field added from Friston). But `Closure` is *not* FRP `Event` because Closure has speech-act semantics (ASSERT/REQUEST/PROMISE/DECLARE/EXPRESS), not just "discrete time-stamped value". `Relation` and `px_loop` are also outside FRP's scope. Verdict: FRP is *part of* Planex's form (it's the lineage of Estimate), but FRP alone does not cover Closure/Relation/px_loop.

### 3.6 Data-Driven Configuration (Kubernetes manifests, GtkBuilder UI files, JSON configs)

Data-driven form expresses the system as declarative data (JSON/YAML/XML) consumed by a generic engine. Strengths: tool-friendly, hot-reloadable, language-agnostic. Weaknesses: loses type safety; the ITNEXT article "Abstraction is the wrong way to simplify configuration" (Dec 2025) argues config DSLs fail when they need conditional logic, and Tomas Petricek's "Library patterns: Multiple levels of abstraction" (2015) shows that data-only APIs force users to escape back to code for any non-trivial case. For Planex, `px_intent_*` is already a typed value, but exposing it as serialized JSON would lose the C type guarantees that make Planex safe to embed. Data-driven could be a *serialization format* (e.g., for snapshot/replay) but not the *form*. Verdict: tier-3 layer for tooling, not primary form.

### 3.7 Object-Oriented Framework (Alan Kay's Smalltalk vision)

Kay's 2003 statement: "OOP to me means only messaging, local retention and protection and hiding of state-process, and extreme late-binding of all things." Kay-OOP is message-passing between encapsulated stateful objects, with late binding at every level. Strengths: encapsulation, polymorphism, runtime extensibility. Weaknesses: modern Java/C# OOP "complects state+identity+value" (per Hickey, see §4.2); GObject in C shows the cost — roughly 3000 lines of boilerplate per class, manual reference counting, and a 30+ entry type system that took years to stabilize. For Planex in C17, Kay-OOP requires either GObject-scale ceremony or a custom preprocessor (neither acceptable). The closest Planex gets is `px_actor` (a context, not an object — it holds an Estimate + a Closure dispatch entry but does not encapsulate methods). Verdict: rejected by [ADR-0004](../decisions/accepted/ADR-0004-use-c-not-rust-zig-cpp.md) implicitly (Kay-OOP requires language-level support that C does not offer).

### 3.8 Tagless Final / Algebraic Effects

Tagless final (Okasaki, the Haskell/Scala world) is a technique for embedding DSLs in a typed host language without defining a tag datatype — different interpreters can give different meanings to the same code. Algebraic effects (Plotkin, Levy) generalize monads: programs express effect operations that are handled by user-defined handlers, allowing effect composition without monad transformer stacks. Strengths: zero-cost abstraction, fully extensible without modifying the core. Weaknesses: both require higher-kinded types and language-level support; C17 has no equivalent. Verdict: not feasible in Planex's language; noted as theoretical inspiration for the *form* (Planex's `px_closure` with a `kind` discriminant is the closest C can get).

### 3.9 Summary of alternatives

No single alternative form covers all of Planex's constraints (typed values, denotational meaning, C17 feasibility, cognitive-bandwidth focus, no AI-as-driver). The closest precedents are: (a) antirez's Redis, which explicitly identifies as "a DSL that manipulates abstract data types" — a *hybrid* of DSL and abstraction; (b) libuv, which ships "2 abstractions: handles and requests" on top of an event loop — the closest *pure-abstraction* precedent in C; (c) Cairo, which ships ~6 denotationally-designed concepts (surface/context/path/pattern/operators) — the most *philosophically-grounded* C library in widespread production use.

---

## 4. Six Critique Movements — Defending Abstraction Against Its Critics

### 4.1 Worse Is Better (Richard Gabriel, 1989)

Gabriel's essay contrasts the "MIT/Stanford right-thing" style (correctness, consistency, completeness — interface simplicity more important than implementation simplicity) with the "New Jersey worse-is-better" style (implementation simplicity above all, interface simplicity sacrificed if needed, completeness sacrificed whenever implementation simplicity is at risk). Gabriel's thesis: the NJ style wins in the marketplace because "Unix and C work fine on [worse-than-median] machines" and "the viral spread is assured as long as [the virus] is portable." Planex's exposure: the v4 8-abstraction proposal is MIT-style (correctness across all 10 constitutive demands, even if ADR-0010 concedes v4 satisfies 0 of them); v0.4's 5-abstraction shipping is closer to NJ (pragmatic subset that demonstrably runs 25+ demos). Defense: Planex's *essence-chosen* abstractions are not over-engineered — they are 5 of the 5 essence categories identified by the 6-tradition literature survey. v0.4 is NJ-pragmatic; v4 is MIT-aspirational and explicitly labeled as design rationale, not shipping commitment. Gap: [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) already concedes this; the gap is that no ADR exists documenting *when* Planex will promote an abstraction from v4-aspiration to v0.x-shipping. ADR-0011 or 0012 should add this gate.

### 4.2 Simple Made Easy (Rich Hickey, 2011)

Hickey's Strange Loop talk distinguishes "simple" (from Latin *sim* + *plex*, meaning "one twist" — unfurled, single-role) from "easy" (from Latin *adjacency*, meaning "near, familiar, in your skillset"). Hickey lists constructs that *complect* (braid together) and generate complex artifacts: state, objects, methods, syntax, inheritance, switch/matching, vars, imperative loops, actors, ORM, conditionals, inconsistency. He lists constructs that generate *simple* artifacts: values, functions, namespaces, data, polymorphism, managed refs, set functions, queues, declarative data manipulation, rules, consistency. Planex exposure: `px_actor` has *state* in Hickey's sense (it ties value and time); `px_closure`'s `kind` discriminant is a *switch on multiple who/what pairs* (Hickey calls switch complex). The other three Planex abstractions are mostly in Hickey's "simple" list: `Estimate` is a *value* (with a confidence field), `Perception` is a *function* (denotation), `Relation` is *data* (a semilattice of named relations). Defense: Planex's abstractions are largely values/functions/data — the Hickey-favored category. The one known exception (`px_closure.kind`) is documented as a known issue. Gap: Planex should conduct a Hickey-style "complect audit" across all 5 abstractions and document findings. The audit should produce, per abstraction, a count of "complected" constructs and a justification or refactor plan for each.

### 4.3 Law of Leaky Abstractions (Joel Spolsky, 2002)

Spolsky's law: "All non-trivial abstractions, to some degree, are leaky." The canonical example is TCP over IP — TCP promises reliable ordered streams, but IP drops packets, so TCP must leak retransmission state to the user when the network fails badly. Planex exposure: Planex is in C17 — every abstraction eventually leaks to `px_actor` + malloc + `px_loop` tick. The leak is *worse* in C than in Haskell because C has no type-system backstop (no `IO` monad, no `ST` region). Defense: Planex's [`limitations.md`](../concepts/state/limitations.md) already documents 11 known leaks (L1–L11) including Perception Phase 2 pending, Relation necessity unproven, multi-frame interaction gap, etc. This is *honest* about leaks but only *qualitative*. Gap: Planex should add a "Leak Budget" table per abstraction, counting how many of its operations require the user to touch the underlying implementation layer. For example: `Estimate` (8 operations: 2 leak to malloc, 0 leak to px_loop) vs `px_loop` (5 operations: 5 leak to OS scheduler). The leak budget gives a falsifiable metric for "is this abstraction too leaky to justify?"

### 4.4 Abstraction Inversion (c2 wiki)

Abstraction Inversion is the anti-pattern of "a simple abstraction built on a complicated mechanism, when it would be possible (and practical) to do it the other way around." Planex exposure: `px_loop` is the suspect — its semantics are simple ("tick the world once") but its implementation has to coordinate Estimate subscription, Closure dispatch, Relation graph traversal, and Perception denotation in one tick. By the c2 definition, this is abstraction inversion *if* a simpler implementation exists. Defense: closed-loop coupling genuinely requires this coordination; the inversion is *necessary* and is acknowledged in code structure (px_loop lives in its own translation unit, isolated from the abstractions it coordinates). Gap: Planex should add an ADR documenting why `px_loop` is the legitimate locus of necessary complexity, distinguishing it from accidental abstraction inversion elsewhere.

### 4.5 Rule of Three (Wikipedia, Randy Shoup, Holden Rehg)

The Rule of Three is a code-refactoring heuristic: "do not abstract until you have copy-pasted the same code three times." Variants appear in Randy Shoup's LinkedIn post (2025) and Holden Rehg's blog (2021), and the Wikipedia article treats it as canonical. Planex exposure: Planex shipped 5 abstractions with fewer than 3 concrete uses each in v0.3 — `px_loop` had 0 uses in v0.3, became 1-of-5 in v0.4 with one demonstration. By the Rule of Three, this is "premature abstraction." Defense: Planex's counter-argument is essence-justified, not duplication-justified: the 5 abstractions are justified by the 6-tradition literature survey (Cognitive, Math/Linguistic, Phenomenological, Neural, Semiotic — see [`alternative-perspectives.md`](../concepts/background/alternative-perspectives.md)), not by 3+ code duplications. The Rule of Three is a *sufficient* condition for abstraction (when 3+ duplications exist, abstract), not a *necessary* one (essence-justified abstractions are exempt). Gap: this is the *strongest* external critique. [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) partially addresses it by framing v4 as design rationale; but no ADR exists that explicitly rebutts the Rule of Three by articulating when essence-justified overrides duplication-justified. ADR-0011 (or 0013, depending on numbering) should fill this gap.

### 4.6 Over-Abstraction / End-of-Civ (Darkø Tasevski 2025, datagubbe 2021, Jonathan Blow)

Tasevski's "A Case Against Abstraction" (dev.to, Aug 2025) argues that "abstraction no longer reduces cognitive load; it multiplies it. It takes effort that should go into problem-solving and reroutes it into archaeology." The piece's distinguishing claim is the *AI-era* critique: "LLMs can plow through raw detail without ever getting tired ... but they choke on hidden indirection." Tasevski recommends: "If you can't explain what an abstraction does in 5 minutes to a new developer (or AI), it's too complex." raganwald's maxim: "Sufficiently advanced abstractions are indistinguishable from obfuscation." datagubbe's "Is Software Abstraction Killing Civilization?" (2021) responds to Jonathan Blow's claim that abstraction-induced ignorance of low-level programming will lead to civilizational collapse — datagubbe mostly debunks Blow's specifics (the "five nines" metric was never used for laptops; COBOL runs the banking system and is a high-level language; etc.) but concedes that "abstraction can lead to detrimental ignorance." Planex exposure: Planex is *intentionally* abstraction-heavy — 5 abstractions for what could be a single `button` struct is exactly what Tasevski's critique targets. Defense: Planex's abstractions are *typed values*, not encapsulation boxes. Tasevski's critique is specifically against *encapsulation-style* abstraction (Provider/Manager/Factory hierarchies where state changes ripple through hidden chains). Planex's form is closer to *algebraic data types* — the very thing Tasevski's article recommends as the alternative. `Estimate` is a value (struct + confidence); `Closure` is a value (kind + payload); `Relation` is data (a semilattice). None of these hide implementation; they expose typed values that compose. Gap: Planex does not currently *say* this distinction. The fix is a new concept document `abstraction-form.md` distinguishing "abstraction-as-encapsulation" (rejected) from "abstraction-as-typed-value" (Planex's form), citing Hickey and Tasevski.

---

## 5. Philosophy-Driven Design — Precedents and Falsifiability

Planex is *the most philosophy-driven C library surveyed* (see §6). This is a feature — but it places Planex in a small peer group with mixed commercial outcomes. Five precedents must be examined for what they teach about *philosophy-driven design's longevity*.

### 5.1 Winograd/Flores (1986) — Speech Act Theory → Coordinator

Winograd and Flores's *Understanding Computers and Cognition* applied Searle's speech act theory to office automation. They shipped a product, Coordinator (Action Technologies, 1980s–90s), that organized email around speech-act types (REQUEST, COMMIT, etc.). Coordinator sold well initially, then declined. Winograd himself shifted in later work (*Bringing Design to Software*, 1996) toward design-oriented HCI and away from the strong speech-act framing. Lesson: philosophy-driven design CAN ship, but commercial longevity depends on ecosystem fit (Coordinator lost to ordinary email that didn't constrain users to speech-act types). Planex parallel: Planex is open-source and small-scope; the ecosystem risk is *maintainer burnout* and *niche irrelevance*, not market share. But the parallel warns that theory purity can alienate users who just want a button.

### 5.2 Conal Elliott — Denotational Design → Fran / Pan

Conal Elliott's FRP work (Fran, 1997; Pan, 2003; "Denotational Design with Type Class Morphisms", 2009) is the most direct precedent for Planex's denotational approach. Fran and Pan were academic; Haskell FRP libraries (reflex, reactive-banana) are alive but niche. Roman Cheplyaka's blog post "Denotational design does not work" (2014) demonstrates that Conal's image-denoising example grows unmanageably complex when requirements change (adding variables, adding errors) — the denotation's simplicity collapses under real-world perturbations. Lesson: pure denotational design is *hard to evolve*. Each new requirement can require re-derivation rather than extension. Planex parallel: Planex's v4 essence-rederivation (`essence-derivation-v1.md` through `essence-derivation-v4-clean.md`) is exactly this pattern — 4 attempts over 4 versions. [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md)'s framing downgrade to "design rationale" is the right response: it acknowledges that v4 is *one possible re-derivation*, not the truth.

### 5.3 Heidegger / Dourish — Phenomenology → Tangible UI

Dourish's *Where the Action Is* (2001) and Ishii's *Tangible Bits* (1997) attempted to operationalize Heidegger's *Zuhandenheit* (ready-to-hand) for HCI. The research line is alive in academic HCI but never became mainstream GUI. The closest commercial descendant is multi-touch (iPhone 2007), which is *not* Heideggerian — it's ergonomic and pragmatic, with no philosophical commitment. Lesson: philosophical schools rarely ship directly; they fertilize other traditions. Planex parallel: Planex's choice to *adopt* Cognitive + Math/Linguistic schools and *acknowledge* Phenomenological + Neural schools (per [`alternative-perspectives.md`](../concepts/background/alternative-perspectives.md)) is the right pattern — adopt the schools whose abstractions match Planex's scope (screens, pointers, discrete UIs), acknowledge the others as seeds for future growth. Gap: Planex should add a "fertilization path" doc: which non-adopted school feeds which adopted abstraction as a seed (e.g., Heidegger → `PX_REL_AFFORDS` relation, which is documented in [`ui-essence-layers.md`](../concepts/background/ui-essence-layers.md) as a seed of Affordance).

### 5.4 Friston — Free Energy Principle → Predictive Coding

Friston's free-energy principle (Nature, 2010) proposes that the brain is a prediction machine minimizing "surprise" (variational free energy). Planex's `Estimate.confidence` field is directly borrowed from Friston's predictive coding. Outcome: FEP is contested in neuroscience; no production software has shipped "active inference UI" commercially. Lesson: adopting a contested theory as implementation is risky; adopting it as a *field on a struct* (Planex's current approach) is safe. Planex parallel: Planex's `confidence` field is decorative per [`alternative-perspectives.md`](../concepts/background/alternative-perspectives.md) — "the field exists but is not yet used in a prediction loop ... a placeholder for future predictive functionality." This is the correct level of commitment.

### 5.5 Alan Kay — OOP → Smalltalk

Kay's 1970s vision of OOP (messaging, late binding, encapsulation) was philosophy-driven — and as Kay himself has said repeatedly (e.g., his 2003 email, the 1997 OOPSLA keynote), modern Java/C# OOP is "not what I meant." Lesson: a philosophy-driven form can be *misappropriated* by the mainstream and lose its meaning. If "Planex-style abstraction" became a buzzword, it might come to mean "any UI library with named concepts" — losing the denotational meaning that makes Planex what it is. Planex parallel: Planex should write a "What Planex is NOT" doc to prevent misappropriation. The existing [`non-goals.md`](../concepts/canonical/non-goals.md) says what Planex does not *do*; it does not say what the *abstraction form* is not. A FAQ entry specifically distinguishing "Planex abstraction" from "named concepts in a component library" would address this.

---

## 6. Production C/UI Library Calibration

To calibrate Planex against production peers, the survey examined nine production C or UI libraries and their primary organizational form. The table summarizes the comparison.

| Library | Primary form | # top-level concepts | Philosophy-driven? | Longevity |
|---|---|---|---|---|
| SDL | Hardware abstraction layer | ~10 subsystems (video/audio/input/time/filesys) | No (pragmatic) | 27+ years, ubiquitous |
| GLib/GObject | OOP framework in C | 4 core (GObject/GInterface/GBoxed/GParamSpec) + ~30 GType subtypes | Partial (documented design philosophy) | 25+ years, GNOME backbone |
| Cairo | Denotational design (surface/context/path/pattern/operators) | ~6 | Yes (PostScript-inspired denotational) | 22+ years, stable |
| Wayland | Wire protocol + proxy + registry | ~5 (display/registry/surface/seat/compositor) | Yes (protocol-as-API) | 17+ years, replacing X11 |
| libuv | Two abstractions: handles + requests | 2 + event loop | Yes (explicit "2 abstractions" design doc) | 14+ years, Node.js backbone |
| GTK | Widget hierarchy (GtkWidget derived) | ~50 widget classes | Partial (object-oriented C) | 27+ years, evolving |
| Qt Widgets | Object hierarchy (QObject derived) | ~50 widget classes + model/view framework | Partial (object-oriented C++) | 30+ years, evolving |
| Dear ImGui | Immediate-mode GUI (no retained state) | ~30 immediate-mode functions | No (pragmatic) | 10+ years, ubiquitous in games |
| Redis | DSL + ADT (per antirez manifesto) | ~10 ADTs (string/list/hash/set/zset/stream/hyperloglog/bitmap/geo) | Yes (explicit manifesto) | 16+ years, ubiquitous |
| **Planex v0.4** | Essence-chosen typed abstractions | 5 | **Yes (most philosophy-driven of set)** | 0.4 release, early |

Three observations follow. First, Planex is the *most philosophy-driven C library surveyed* — the only one whose abstraction count is justified by an explicit 6-tradition literature survey. This is a feature (intellectual honesty, falsifiable design) but places Planex in a small peer group (Cairo, libuv, Wayland, Redis). Second, libuv's design page explicitly states "2 abstractions: handles and requests" — this is the *closest pure-abstraction precedent* in production C. libuv ships 2 abstractions for an event loop; Planex ships 5 for UI. The density ratio (2.5x) is defensible because UI has more essence categories than async I/O (5 vs 2 in the literature survey). Third, GObject is the cautionary tale: its 4-abstraction core ballooned into a 30+ entry type system, with GObject's reference-counting semantics a famously painful API surface for new developers. Planex's 8-abstraction v4 proposal is in the same risk zone; [ADR-0010](../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md)'s framing downgrade ("design rationale, not essence discovery") is the right defense, but it must be paired with a *gate* that prevents v4 abstractions from silently becoming shipping commitments without independent justification.

---

## 7. Gap Analysis — Seven Specific Gaps

The survey identified seven concrete gaps between Planex's current documentation and the defense it owes its chosen form.

1. **"Abstraction-as-encapsulation" vs "abstraction-as-typed-value" is not distinguished.** The critique literature (Tasevski, Hickey, Spolsky) is overwhelmingly *against* encapsulation-style abstraction (Provider/Manager/Factory). Planex's form is *for* typed-value-style abstraction (Estimate is a value, Closure is a value, Relation is data). Without this distinction made explicit, Planex inherits the attacks of the wrong target. Fix: add `docs/concepts/canonical/abstraction-form.md` with the distinction, citing Hickey's "values" list and Tasevski's recommended alternative.

2. **Rule-of-Three rebuttal is missing.** Rule of Three (Wikipedia, Shoup, Rehg) is the strongest external critique of essence-justified abstraction. Planex's defense (essence-justified overrides duplication-justified) is implicit in `why-four-abstractions.md` but not articulated as a rebuttal. Fix: add ADR-0011 "Essence-justified vs Duplication-justified Abstraction" with the Rule-of-Three rebuttal and the conditions under which each kind is appropriate.

3. **Leak budget per abstraction is not measured.** Spolsky's law is universal; Planex's `limitations.md` is honest but qualitative. Fix: add a "Leak Budget" table per abstraction (operations that leak to underlying layer / total operations), giving a falsifiable metric for "is this abstraction too leaky."

4. **Complect audit is not done.** Hickey's complect list (state, objects, methods, syntax, inheritance, switch, vars, loops, actors, ORM, conditionals) should be applied to each Planex abstraction. `px_closure.kind` switch is a known complect; are there others? Fix: a `docs/concepts/complect-audit.md` document per abstraction.

5. **Falsifiability path is missing.** ADR-0010 frames v4 as "design rationale" — good. But no documented criterion exists for when an abstraction is *falsified* (i.e., when should Planex drop an abstraction?). Fix: add to `why-four-abstractions.md` a "Falsifiability criteria" section (e.g., "Interpretant is falsified if no Planex user writes an extension within 3 years of v0.5").

6. **Misappropriation defense is partial.** `non-goals.md` says what Planex does not *do*; it does not say what the *abstraction form* is not (Kay precedent — see §5.5). Fix: add "What Planex is NOT" FAQ entry, specifically distinguishing Planex-style abstraction from component-library named concepts.

7. **Tier-2 fallback forms are not on the shelf.** Brooks's *No Silver Bullet* (1986) argues that no single development gives a 10x improvement; the same applies to organizational forms. Planex commits fully to abstraction. If a future use case (e.g., declarative skinning, AI-driven UI) needs a different form, Planex has no documented fallback. Fix: add a `docs/concepts/fallback-forms.md` doc that pre-documents when and how Planex would add a DSL layer or data-driven format on top of the 5 abstractions, citing antirez's Redis as the closest commercial hybrid precedent.

---

## 8. Verdict — Is Abstraction the Best Form for Planex?

**Yes, for Planex's stated goals. The form is correct, with three caveats.**

The form is correct because:

1. Planex's stated constraint is *cognitive bandwidth* (per `why-four-abstractions.md`), and the only form that delivers *typed values* (not callbacks, not configs, not opaque widgets) is abstraction. The alternatives either sacrifice typing (DSL, data-driven), sacrifice denotational meaning (component library, pattern language), or sacrifice C17 feasibility (Kay-OOP, tagless final).

2. Planex's stated non-goal is *AI integration* (per [ADR-0003](../decisions/accepted/ADR-0003-no-ai-integration.md)). Pure data-driven or DSL forms would invite AI-as-driver — the AI manipulates data or DSL text, and the library becomes a passive interpreter. Abstraction-as-typed-value keeps AI as a value-constructor, with the C type system as a guardrail. This is exactly the "AI-era critique" reversal of Tasevski: typed values are *more* AI-friendly than encapsulated state chains, because the AI can reason about the values without un-encapsulating hidden indirection.

3. Planex's stated language is C17 ([ADR-0004](../decisions/accepted/ADR-0004-use-c-not-rust-zig-cpp.md)). Of the eight alternative forms surveyed, only four are expressible in C17 (DSL, component library, ECS, data-driven). None of these four simultaneously satisfies (1) and (2). The other four alternatives (Kay-OOP, tagless final, algebraic effects, FRP-as-primary-form) require language-level features C does not offer.

The three caveats are:

1. **Planex must distinguish its form from encapsulation-style abstraction** (Gap 1 above). Without this distinction, Planex inherits the critique literature's attacks on the wrong target.

2. **Planex must rebut the Rule of Three** (Gap 2). Essence-justified abstraction is defensible, but the defense must be explicit — currently it is implicit.

3. **Planex must measure leaks** (Gap 3). Spolsky's law is universal; Planex's honesty about leaks (in `limitations.md`) is good but qualitative. Quantify the leak budget per abstraction.

---

## 9. Three-Tier Recommendations

### Tier 1 — Immediate (low effort, high defense)

- **T1.1**: Add `docs/concepts/canonical/abstraction-form.md` distinguishing "abstraction-as-encapsulation" (rejected) from "abstraction-as-typed-value" (Planex's form), citing Hickey's "Simple Made Easy" transcript, Tasevski's "A Case Against Abstraction," and Spolsky's "Law of Leaky Abstractions."
- **T1.2**: Add ADR-0011 "Essence-justified vs Duplication-justified Abstraction" — the explicit Rule-of-Three rebuttal. Conditions: essence-justified abstraction is appropriate when (a) the abstraction is grounded in a 3+ source academic tradition, (b) the abstraction's denotation can be specified independently of its implementation, (c) the abstraction's removal would create a documented expressiveness gap. Duplication-justified abstraction is appropriate when (a) 3+ concrete duplications exist, (b) the duplications differ only in data, not in semantics.
- **T1.3**: Add a "Leak Budget" table to `limitations.md` — per-abstraction count of leak-prone operations / total operations.

### Tier 2 — Medium-term (next 2 release cycles)

- **T2.1**: Apply Hickey's complect audit to all 5 shipping abstractions; document findings in `docs/concepts/complect-audit.md`. For each abstraction, list which Hickey-complected constructs it uses and either justify or refactor.
- **T2.2**: Add "Falsifiability criteria" section to `why-four-abstractions.md` — per-abstraction, the criterion under which Planex would drop it (e.g., "Interpretant is falsified if no Planex user writes an Interpretant extension within 3 years of v0.5").
- **T2.3**: Add "What Planex is NOT" FAQ entry to `docs/faq.md`, defending against the Alan Kay misappropriation precedent — distinguishing Planex-style essence-chosen abstraction from "any UI library with named concepts."
- **T2.4**: Add ADR-0012 "Why px_loop is the locus of necessary complexity" — documenting the abstraction-inversion defense (per §4.4).

### Tier 3 — Long-term (v0.5+)

- **T3.1**: Add `docs/concepts/fallback-forms.md` — pre-documenting when and how Planex would add a DSL layer or data-driven format on top of the 5 abstractions. Cite antirez's Redis as the closest commercial hybrid (DSL + ADT).
- **T3.2**: Add a "philosophy-driven design patterns" doc series — what other C libraries (libuv's "2 abstractions," Cairo's denotational operators, Wayland's protocol-as-API) do right that Planex should emulate.
- **T3.3**: Track the v4 falsifiability: if no v4-proposed abstraction (Interpretant, Perlocution, Breakdown) is shipped by v0.6, formally mark v4 as "deferred indefinitely" in ADR-0010's status field.

---

## 10. Conclusion

Planex's choice of abstraction as its organizational form is the best option *for its stated goals* — but only if Planex can defend the form against the six specific critique traditions surveyed in this report. The defense is half-written: `why-four-abstractions.md` argues *which* abstractions and *why these five*; it does not argue *why abstraction as a form*. The seven gaps identified in this report are the second half of the defense. Closing them costs roughly 3–5 days of doc work and prevents Planex from inheriting the criticisms that killed (or marginalized) Winograd/Flores's Coordinator, Conal Elliott's Fran/Pan, and GObject's type system. The cost of *not* closing the gaps is that future critics of Planex can point to Tasevski, Hickey, Spolsky, and Gabriel and say "Planex is the thing these essays warn against" — and Planex will have no documented answer.

The deeper lesson of the survey: abstraction is not a single thing. It is a *family* of forms, ranging from encapsulation-style (which the critique literature rightly attacks) to typed-value style (which Hickey's "Simple Made Easy" actually endorses as the *simple* alternative). Planex's form is the typed-value style. The single most important framing fix is Gap 1: making this distinction explicit, so that Planex stops inheriting attacks aimed at the wrong target.

---

## 11. References (categorized by section)

### Alternative forms (§3)

- Wikipedia: Domain-specific language — https://en.wikipedia.org/wiki/Domain-specific_language
- antirez: Redis Manifesto (Mar 2011) — https://oldblog.antirez.com/post/redis-manifesto.html
- Wikipedia: A Pattern Language — https://en.wikipedia.org/wiki/A_Pattern_Language
- Tomas Petricek: The Timeless Way of Programming (2022) — https://tomasp.net/blog/2022/timeless-way
- Maggie Appleton: Pattern Languages in Programming and Interface Design — https://maggieappleton.com/pattern-languages
- SanderMertens: Entity Component System FAQ — https://github.com/SanderMertens/ecs-faq
- Mike Acton: Data-Oriented Design and C++ (CPP 2014) — https://neil3d.github.io/assets/img/ecs/DOD-Cpp.pdf
- Wikipedia: Functional reactive programming — https://en.wikipedia.org/wiki/Functional_reactive_programming
- Conal Elliott: Why program with continuous time? (2010) — http://conal.net/blog/posts/why-program-with-continuous-time
- Akka: Reactive programming vs. reactive systems (2023) — https://akka.io/blog/reactive-programming-versus-reactive-systems
- ITNEXT: Abstraction is the wrong way to simplify configuration (Dec 2025) — https://itnext.io/abstraction-is-the-wrong-way-to-simplify-configuration-81ac4bad02ab
- Tomas Petricek: Library patterns — Multiple levels of abstraction (2015) — https://tomasp.net/blog/2015/library-layers
- Wikipedia: GObject — https://en.wikipedia.org/wiki/GObject
- GLib Type System Concepts — https://docs.gtk.org/gobject/concepts.html
- Alan Kay on OOP (2003 email, via StackExchange) — https://softwareengineering.stackexchange.com/questions/46592/
- C2 wiki: AlanKayOnMessaging — https://wiki.c2.com/?AlanKayOnMessaging
- Okasaki: Tagless-Final Style — http://okmij.org/ftp/tagless-final/index.html
- Wikipedia: Leaky abstraction — https://en.wikipedia.org/wiki/Leaky_abstraction

### Critique movements (§4)

- Richard Gabriel: The Rise of "Worse Is Better" (1989) — https://www.jwz.org/doc/worse-is-better.html and https://dreamsongs.com/WorseIsBetter.html
- Wikipedia: Worse is better — https://en.wikipedia.org/wiki/Worse_is_better
- Rich Hickey: Simple Made Easy (Strange Loop 2011), InfoQ transcript — https://www.infoq.com/presentations/Simple-Made-Easy
- matthiasn talk-transcripts: Hickey Simple Made Easy — https://github.com/matthiasn/talk-transcripts/blob/master/Hickey_Rich/SimpleMadeEasy.md
- Joel Spolsky: The Law of Leaky Abstractions (2002) — https://www.joelonsoftware.com/2002/11/11/the-law-of-leaky-abstractions
- c2 wiki: Abstraction Inversion — https://wiki.c2.com/?AbstractionInversion
- Wikipedia: Rule of three (computer programming) — https://en.wikipedia.org/wiki/Rule_of_three_(computer_programming)
- Randy Shoup: Not DRY, but The Rule of Three instead (LinkedIn 2025) — https://www.linkedin.com/posts/randyshoup_not-dry-but-the-rule-of-three-instead-activity-7308790229690134528-Sk-d
- Holden Rehg: The Rule of Three (2021) — https://holdenrehg.com/blog/2021-09-20_rule-of-three
- Darkø Tasevski: A Case Against Abstraction (dev.to Aug 2025) — https://dev.to/puritanic/a-case-against-abstraction-118o
- datagubbe: Is Software Abstraction Killing Civilization? (2021) — https://datagubbe.se/endofciv/
- Fred Brooks: No Silver Bullet — Essence and Accident in Software Engineering (1986) — https://en.wikipedia.org/wiki/No_Silver_Bullet and https://www.cs.unc.edu/techreports/86-020.pdf
- Mark Seemann: Yes silver bullet (2019) — https://blog.ploeh.dk/2019/07/01/yes-silver-bullet

### Philosophy-driven precedents (§5)

- Terry Winograd, Fernando Flores: Understanding Computers and Cognition (1986) — https://philpapers.org/rec/WINUCA
- Winograd/Flores: Conversation for Action (ACM) — https://dl.acm.org/doi/pdf/10.1145/1125944.1125973
- Conal Elliott: Denotational Design — From Meanings To Programs (YOW! 2022) — https://www.youtube.com/watch?v=rlyqoYoUumc
- Conal Elliott: Denotational design with type class morphisms (2009) — http://conal.net/papers/type-class-morphisms
- Roman Cheplyaka: Denotational design does not work (2014) — https://ro-che.info/articles/2014-12-31-denotational-design-does-not-work
- Eric Normand: Why do I like Denotational Design? (2019) — https://ericnormand.me/podcast/why-do-i-like-denotational-design
- Paul Dourish: Where the Action Is (MIT Press 2001) — https://direct.mit.edu/books/monograph/3875/
- Hiroshi Ishii: Tangible Bits (1997) — https://dl.acm.org/doi/10.1145/258549.258715
- Wikipedia: Heideggerian terminology — https://en.wikipedia.org/wiki/Heideggerian_terminology
- Dotov et al.: A Demonstration of the Transition from Ready-to-Hand (PMC 2010) — https://pmc.ncbi.nlm.nih.gov/articles/PMC2834739
- Karl Friston: The free-energy principle (Nature 2010) — https://www.nature.com/articles/nrn2787
- Friston: Predictive coding under the free-energy principle (PMC) — https://pmc.ncbi.nlm.nih.gov/articles/PMC2666703/

### Production C/UI library calibration (§6)

- SDL Homepage — https://www.libsdl.org
- Wikipedia: Simple DirectMedia Layer — https://en.wikipedia.org/wiki/Simple_DirectMedia_Layer
- libuv: Design overview — https://docs.libuv.org/en/v1.x/design.html
- Wikipedia: Libuv — https://en.wikipedia.org/wiki/Libuv
- Cairo Graphics — https://www.cairographics.org
- Wikipedia: Cairo (graphics) — https://en.wikipedia.org/wiki/Cairo_(graphics)
- Wayland: Protocol design — https://wayland-book.com/protocol-design.html
- Wikipedia: Wayland (protocol) — https://en.wikipedia.org/wiki/Wayland_(protocol)
- GTK: GtkWidget — https://docs.gtk.org/gtk3/class.Widget.html
- Qt 6: Model/View Programming — https://doc.qt.io/qt-6/model-view-programming.html
- Wikipedia: XUL — https://en.wikipedia.org/wiki/XUL
- Wikipedia: OpenLaszlo — https://en.wikipedia.org/wiki/OpenLaszlo
- Wikipedia: Entity component system — https://en.m.wikipedia.org/wiki/Entity_component_system
- Wikipedia: Speech act — https://en.m.wikipedia.org/wiki/Speech_act
