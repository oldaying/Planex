# Abstraction as the Organizational Form of Planex

> **Status:** Canonical position paper. Date: 2026-08-28.
>
> **Applies to:** v0.4 (5 shipping abstractions: Estimate / Perception / Closure / Relation / px_loop) and the v4 design proposal (8 abstractions: adds Interpretant / Perlocution / Breakdown).
>
> **Companion documents:**
> - [`why-four-abstractions.md`](why-four-abstractions.md) — argues *which* abstractions Planex implements.
> - [`../research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md) — the long-form research report that surveys eight alternative forms, six critique traditions, five philosophy-driven precedents, and nine production C/UI libraries. This document is the short, canonical position distilled from that report.
> - [`../decisions/ADR-0010-v4-design-rationale-not-essence-discovery.md`](../decisions/ADR-0010-v4-design-rationale-not-essence-discovery.md) — the honesty downgrade: v4 is design rationale, not essence discovery.
> - [`../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md`](../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — the formal rebuttal to the strongest external critique (Rule of Three).
>
> **What this document is:** the place a skeptical reader is sent when they ask "why abstraction as the form, instead of a DSL / component library / pattern language / data-driven config / ECS / FRP / Kay-OOP / tagless final?" It states the conditional thesis, names the three prerequisites that must hold for the thesis to remain valid, and maps each prerequisite to Planex's current satisfaction level honestly.

---

## The thesis, stated conditionally

**Abstraction is the optimal organizational form for Planex's current goals — conditional on three prerequisites.**

This is not the unconditional claim "abstraction is always best." That claim is false, as the comparative study surveys in detail. The DSL form wins for syntax-stable domains (SQL, regex, Make). The component library form wins for feature-stable, low-ceremony domains (jQuery-era DOM, classic Win32). The pattern language form wins when the domain is well-understood but essences are unstable (GoF patterns, microservice patterns). The data-driven config form wins when variance dominates essence (Kubernetes YAML, declarative UI). Each form has a domain where it is the correct answer; abstraction is not unconditionally superior.

The claim is narrower: given Planex's stated goals — (a) intent-as-value so it can be serialized and replayed, (b) multi-channel denotation so the same intent drives pixel, accessibility tree, and audit log, (c) semantic-level audit across five axes (provenance / completeness / consistency / falsifiability / cost-of-repair), (d) cognitive-bandwidth constraint on the maintainer and contributor — abstraction is the only form that satisfies all four simultaneously. The comparative study verifies this directly: every alternative form sacrifices at least one of the four.

But the claim is also conditional. Three prerequisites must hold for the conclusion to remain valid. If any one of them breaks, the conclusion flips, and Planex should switch to the alternative form that handles the broken prerequisite better. This document names the three, states Planex's current satisfaction level for each, and specifies the fallback form for each failure mode.

---

## Prerequisite 1: Ontological Stability

**The problem domain must contain stable essences that survive sustained scrutiny — not merely recurring patterns.**

This is the deepest of the three. It is the dividing line between abstraction and pattern language. Pattern languages make no ontological commitment: a pattern is "a thing that keeps happening," not "a thing that exists." Component libraries make no ontological commitment: a component is "a thing people keep calling," not "a thing that exists." Abstraction, by contrast, is an ontological commitment. To say "Estimate is one of Planex's abstractions" is to claim that prediction is a real, irreducible feature of UI interaction — not just a thing that keeps appearing in code.

The commitment is heavier than it looks. Heavier commitments demand heavier evidence. The evidence Planex relies on is *tradition*: each of the 5 shipping abstractions (and 3 proposed v4 additions) is grounded in an academic tradition with decades of survival:

| Abstraction | Tradition | Origin (rough date) |
|---|---|---|
| Estimate | Elliott's denotative FRP; Friston's predictive coding | 1997; 2010s |
| Perception | Peirce's semiotics (percept vs interpretant) | 1860s |
| Closure | Searle's speech-act theory (illocution) | 1969 |
| Relation | Heidegger's Mitsein; Simmel's relational sociology | 1927; 1908 |
| px_loop | Hoare CSP; Harel statecharts | 1978; 1987 |
| Interpretant (v4 proposed) | Peirce's interpretant | 1860s |
| Perlocution (v4 proposed) | Searle's perlocutionary act | 1969 |
| Breakdown (v4 proposed) | Heidegger's Zuhandenheit; Winograd/Flores | 1927; 1986 |

**Current satisfaction: partial, and acknowledged as partial.**

ADR-0010 is explicit about this: the v4 framing has been downgraded from "essence discovery" to "tradition-grounded design rationale." Planex does not claim to have proven these essences are stable. It claims to have inherited them from traditions that have proven stable in their parent disciplines. If cognitive science revises the prediction/perception boundary, the Estimate/Perception split must be re-cut. If semiotics revises the percept/interpretant boundary, the proposed v4 Interpretant abstraction is wrong before it ships.

This prerequisite is satisfied *to the extent tradition is a substitute for proof*, which is a weaker satisfaction than proof itself. The honesty of ADR-0010 is what keeps this partial satisfaction honest: by admitting v4 satisfies 0 of 10 constitutive demands, Planex declines to bluff stability it cannot demonstrate.

**Failure mode and fallback.**

If ontological stability breaks — i.e. if a tradition's essence is overturned by revision in its parent discipline, and Planex's abstraction turns out to have frozen a folk theory into architecture — abstraction degrades into *premature ossification*: the very thing essence-justified abstraction claims to avoid. The fallback form is **pattern language**: patterns can be replaced, can coexist with alternatives, and make no ontological claim. Planex would, in this scenario, rewrite its 5 abstractions as 5 documented patterns with no claim to exclusivity or completeness — closer to the GoF or microservice-patterns style.

---

## Prerequisite 2: Orthogonal Separability

**The abstractions must have clean, non-overlapping boundaries — each abstraction owns a constitutive question that no other abstraction answers.**

This is the dividing line between abstraction and DSL. DSLs leak; that is a known property, not a defect. The saving grace of DSLs is that their leaks are *syntactic*: you can see them, you can debug them, you can patch them at the grammar level. Abstractions, by contrast, leak *semantically*: the leak is hidden behind a vtable, behind a type cast, behind a function pointer indirection. Spolsky's "Law of Leaky Abstractions" is universal — every non-trivial abstraction leaks. The question is not whether abstraction leaks, but whether the leaks are *bounded* and *named*.

For abstraction to outperform DSL, the abstractions must be **orthogonal**: each abstraction's leaks must not bleed into another abstraction's surface. If abstractions overlap — if Interpretant and Perception answer the same question with different vocabulary — then the user is paying the cost of abstraction (cognitive overhead of the type system, indirection through vtables, the inability to grep across boundaries) without the benefit (clean composition, parallelizable denotation, serializable intent). Tangled abstractions are strictly worse than a DSL: a DSL with leaky seams at least surfaces the leak in syntax, where it can be patched; tangled abstractions hide the leak in semantics, where it cannot.

**Current satisfaction: 5 of 5 shipping abstractions pass; 3 of 3 v4 proposals are untested.**

The 5 shipping abstractions (v0.4) each bind exactly one constitutive question, and the binding is enforced by the file structure:

- `src/estimate.c` — answers "what will the world be?"
- `src/perception.c` — answers "what is the world?"
- `src/closure.c` — answers "what completed?"
- `src/relation.c` — answers "what is connected to what?"
- `src/loop.c` — answers "when does control yield?"

Orthogonality is empirically supported: each abstraction has its own test file (`tests/test_*.c`), denotation parallelizes across channels (pixel / a11y / audit log) without coordination, and intent serializes to a value that can be replayed. The test suite in `tests/test_orthogonality.c` exists precisely to catch leakage early. The leak budget per abstraction is now quantified in [`leak-budgets.md`](leak-budgets.md) — current L2 (semantic-leak) rate is 17% overall, with Relation at 0%, Closure at 8%, `px_loop` at 9%, Estimate at 24%, and Perception at 50% (the last two are the worst offenders and have retire targets through v0.5 / v0.6).

The 3 v4 proposals (Interpretant, Perlocution, Breakdown) have **not been pressure-tested for orthogonality** against the existing 5. There is a real risk that Interpretant bleeds into Perception (Peirce's own semiotic triangle makes interpretant the internal counterpart of the percept — drawing a hard line is non-trivial), and that Perlocution bleeds into Closure (Searle's perlocutionary act is a kind of closure — the question is whether it is a *distinct* kind). These risks are acknowledged, not resolved; resolving them is on the v4 roadmap and is a precondition for v4 promotion from proposal to shipping.

**Failure mode and fallback.**

If orthogonality breaks — i.e. if two abstractions cannot be cleanly separated because their constitutive questions turn out to be aspects of the same underlying phenomenon — abstraction degrades into *tangled layer cake*: a stack of abstractions whose leaks compound multiplicatively. The fallback form is **DSL**: a DSL forces the leak into the grammar, where it is at least visible and patchable. Planex would, in this scenario, flatten the tangled abstractions into a single DSL with explicit seams, closer to the QML or XUL style.

---

## Prerequisite 3: Falsifiability

**There must be a mechanism that tells Planex when an abstraction is wrong, and a revision path that does not require revolution.**

This is the dividing line between abstraction and component library. Component libraries are falsifiable by construction: if an API is wrong, it is deleted in the next major version, and the deletion is cheap because the API was never more than a function signature. Abstractions are not falsifiable by default — once a type system and a call stack grow on an abstraction, retiring it is a rewrite. Without explicit falsifiability, abstraction degenerates into *architectural dogma*: this is the failure mode of enterprise framework stacks (J2EE-era EJB, early Spring) that accumulated abstractions no one could retire, because no one had defined what would count as the abstraction being wrong.

For abstraction to outperform component library, Planex must define *in advance* what would count as one of its abstractions being wrong, and must build a revision path that does not require rewriting the whole library. There are three layers to this:

1. **Epistemic honesty (documented):** the abstractions are framed as design rationale, not as essence discovery. ADR-0010 does this. Without it, the project inherits the metaphysical baggage of "we discovered the essences of UI," which is unfalsifiable by construction.

2. **Engineering mechanism (currently missing):** there must be a test or measurement that, when it fails, indicates an abstraction is wrong or incomplete. Examples include: a class of UI interactions that cannot be expressed in the 5 abstractions (completeness failure); a class of interactions where the abstractions force more boilerplate than the equivalent component-library code (compression failure); a class of interactions where the abstractions' leak exceeds the leak budget (leakage failure). None of these mechanisms are currently in the test suite.

3. **Migration path (currently missing):** if an abstraction must be retired, the migration must be gradual. There must be a documented deprecation cycle, a way to run the old abstraction alongside the new one, and a way to migrate callers file-by-file. The ADR template's "Superseded by ADR-MMMM" field is a placeholder for this; no Planex ADR has yet exercised it.

**Current satisfaction: half-plus — epistemic honesty and one engineering mechanism (leak budget) in place; two engineering mechanisms (completeness test, migration cycle) and one metric (compression ratio) still missing.**

ADR-0010 establishes the epistemic posture: "v4 satisfies 0 of 10 constitutive demands" is a falsifiability statement, not a marketing claim. The framing downgrade ("tradition-grounded design rationale") is exactly the right epistemic move. The engineering mechanisms are partially in place: [`leak-budgets.md`](leak-budgets.md) defines the leak-budget metric per abstraction, with retire targets through v1.0 and a falsifiable failure condition (aggregate L2 rate > 30% triggers form-level fallback). Still missing: a completeness test (a class of UI interactions that cannot be expressed in the 5 abstractions indicates incompleteness), a compression metric (ratio of Planex-code to equivalent-component-library code for the same UI), and a migration/deprecation cycle exercised in at least one ADR.

**Failure mode and fallback.**

If falsifiability is not built — i.e. if the engineering mechanisms remain missing and the abstractions accrete without any way to detect or retire a wrong one — abstraction degrades into *architectural dogma*: the failure mode of every enterprise framework that promised essences and delivered ceremony. The fallback form is **component library**: components make no ontological claim, retire cheaply, and are falsifiable by construction. Planex would, in this scenario, stop claiming the 5 abstractions are essences and recast them as 5 opinionated components — closer to the GLib or libuv style.

---

## The form distinction that protects the thesis

The comparative study identifies one additional requirement that is not a prerequisite but is necessary to keep the prerequisites meaningful: **Planex must distinguish its abstraction form from the encapsulation-style abstraction that the critique literature attacks.**

Most published attacks on abstraction (Tasevski 2025, datagubbe 2021, Blow's lectures, the Over-Abstraction discourse) target a specific form: abstraction-as-encapsulation — class hierarchies that hide implementation details, abstract base classes with deep inheritance trees, frameworks that demand the user extend `AbstractWidgetAdapter`. The critique literature's complaints (inheritance tax, deep hierarchies, leaky base classes, framework inversion-of-control) are real complaints about *that* form.

Planex's abstraction form is different. Planex uses **abstraction-as-typed-value**: each abstraction is a small set of value types (estimates, intents, closures) that flow through a denotation pipeline. There is no inheritance. There is no abstract base class. There is no framework inversion-of-control. The abstractions are typed values, not encapsulated behaviors. The user composes values; the framework denotates them.

This distinction is not a defense ("ours is the good kind of abstraction"). It is a *targeting* correction: the critique literature's attacks land on a different form. Without this distinction being explicit, Planex inherits attacks it does not deserve. With the distinction explicit, the attacks that *do* land (Spolsky's leak law, the Rule of Three) are the ones Planex must actually answer — and the next section, plus ADR-0011, do exactly that.

---

## How the three prerequisites compose

The three prerequisites form a responsibility chain, not three independent checkboxes:

- **Prerequisite 1 (ontological stability)** answers "why is there an essence at all?" — provides the *legitimacy* of abstraction.
- **Prerequisite 2 (orthogonal separability)** answers "why are the boundaries here, not elsewhere?" — provides the *engineering payoff* of abstraction.
- **Prerequisite 3 (falsifiability)** answers "why can we revise this when we are wrong?" — provides the *maintainability* of abstraction.

Remove any one and the conclusion "abstraction is optimal for Planex's current goals" no longer follows. The three are also cumulative: a project that satisfies all three wins on all three axes (legitimacy, payoff, maintainability); a project that satisfies two of three is exposed on the missing axis; a project that satisfies one or none should not be using abstraction at all.

| Prerequisite broken | Abstraction degrades to | Switch to |
|---|---|---|
| 1. Ontological stability | Premature ossification | Pattern language |
| 2. Orthogonal separability | Tangled layer cake | DSL |
| 3. Falsifiability | Architectural dogma | Component library |

The table is the decision rule: if any of the three prerequisites is found broken in future audits, the row tells Planex what abstraction has degenerated into and what form to switch to. This is the content of "Planex has a fallback path" — not a vague promise, but a specific row in a specific table.

---

## Planex's current standing, honestly

| Prerequisite | Satisfaction | Evidence | Honest gap |
|---|---|---|---|
| 1. Ontological stability | Partial | 5/5 shipping abstractions trace to traditions with decades of survival | ADR-0010 admits v4 satisfies 0/10 constitutive demands; the v4 proposals (Interpretant, Perlocution, Breakdown) inherit the same partial satisfaction |
| 2. Orthogonal separability | 5/5 shipping pass; **3/3 v4 pressure-tested (ADR-0012)**; **leak budget: 9 L2 / 53 ops (17%) overall, ranging 0%–50% per abstraction**; v4 preview: 2 L2 / 11 ops (18%) across Interpretant+Perlocution | `tests/test_orthogonality.c` + [`tests/test_v4_orthogonality.c`](../../tests/test_v4_orthogonality.c) + [`leak-budgets.md`](leak-budgets.md) for quantitative leak tracking | v4 pressure test surfaced 2 L2 leaks at constructor-signature level (Interpretant.representamen_source + Perlocution.closure — both unused by any operation), 1 migration gap (Closure lost `px_closure_get_status` in v4), 1 protocol coupling (Interpretant→Breakdown, acceptable). All bounded by retire targets in [ADR-0012](../decisions/ADR-0012-v4-orthogonality-pressure-test-four-findings.md). Shipping Perception's L2 = 50% remains the worst offender and directly reflects [`limitations.md`](limitations.md) L1 Phase 2 pending |
| 3. Falsifiability | Half-plus (epistemic + 1 of 4 engineering mechanisms) | ADR-0010's honesty downgrade + [`leak-budgets.md`](leak-budgets.md) defines the leak-budget metric and retire curve | Missing: completeness test, compression metric, migration/deprecation cycle exercised in an ADR |

The standing is: **the form is correctly chosen, the prerequisites are correctly identified, two and a half of three prerequisites are currently satisfied (Prerequisite 2 evidence strengthened by [`leak-budgets.md`](leak-budgets.md); Prerequisite 3 epistemic layer in place plus one of four engineering mechanisms), and the gaps are documented rather than hidden.** The honesty of the standing is itself part of prerequisite 3 — without it, the project would already be in the architectural-dogma failure mode regardless of how the other two prerequisites scored.

---

## What this document is not

- **Not a proof that abstraction is best.** It is a conditional defense. The condition is the three prerequisites; the defense is valid only while the prerequisites hold.
- **Not a substitute for the comparative study.** The comparative study is the long-form research that surveys alternatives and verifies the alternatives do not satisfy all four of Planex's constraints simultaneously. This document is the canonical position, distilled.
- **Not a substitute for ADR-0011.** ADR-0011 answers the strongest single external critique (Rule of Three) in ADR-template form, with the mandatory Essence Check. This document frames the conditional thesis; ADR-0011 defends one specific attack on it.
- **Not a substitute for `why-four-abstractions.md`.** That document argues *which* abstractions Planex implements and why the count is what it is. This document argues *that abstraction is the form*, and is silent on which abstractions specifically.

---

## References

- Research: [`../research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md) — long-form comparative study, eight alternative forms, six critique traditions.
- Which abstractions: [`why-four-abstractions.md`](why-four-abstractions.md) — argues the count and the membership.
- Honesty downgrade: [`../decisions/ADR-0010-v4-design-rationale-not-essence-discovery.md`](../decisions/ADR-0010-v4-design-rationale-not-essence-discovery.md) — frames v4 as design rationale, not essence discovery.
- Rule-of-Three rebuttal: [`../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md`](../decisions/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — the formal ADR rebutting the strongest external critique.
- Alternative perspectives: [`alternative-perspectives.md`](alternative-perspectives.md) — the 6-tradition literature survey that grounds the essences.
- Known limits: [`limitations.md`](limitations.md) — honest statement of current implementation gaps.
- Joel Spolsky, "The Law of Leaky Abstractions" (2002) — the canonical statement of prerequisite 2's central risk.
- Karl Popper, "The Logic of Scientific Discovery" (1934/1959) — the canonical statement of prerequisite 3's central criterion.
- Aristotle, *Metaphysics* Book IV — the canonical statement of prerequisite 1's central criterion (the principle of non-contradiction as the test for whether two essences are one).
