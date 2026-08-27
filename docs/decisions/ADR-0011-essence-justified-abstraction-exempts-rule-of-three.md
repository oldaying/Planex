# ADR-0011: Essence-justified abstraction is exempt from the Rule of Three

## Status

**Accepted.** Date: 2026-08-28.

Formalizes the rebuttal to the strongest external critique of Planex's abstraction route. The rebuttal was implicit in [`why-four-abstractions.md`](../concepts/why-four-abstractions.md) and was identified as a known gap (Gap 2) in the comparative study [`docs/research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md). This ADR is the institutional record so that future contributors, reviewers, and external skeptics can be pointed to one place when the Rule-of-Three critique is raised.

**Relationship to prior ADRs:**

- **ADR-0010** (Accepted) — downgraded v4 from "essence discovery" to "design rationale." That downgrade is necessary but not sufficient: it tells the world Planex is honest about not having proven the essences; it does not tell the world *why* Planex is still allowed to use essence-style abstraction despite having fewer than three concrete uses per abstraction. This ADR answers that "why."
- **ADR-0007, ADR-0008** (Accepted) — recorded the v2 essence-derivation revisions and the promotion of `px_loop` to 5th abstraction. These ADRs grounded each abstraction in a tradition but did not articulate the *general criterion* under which tradition-grounding exempts an abstraction from the duplication-justified rule. This ADR supplies the general criterion.
- **ADR-0009** (Proposed, framing downgraded by ADR-0010) — recorded the v3 Path B essence claim. Its implementation decisions stand; its essence framing is downgraded. This ADR's criterion is the test the v4 proposals (Interpretant, Perlocution, Breakdown) must pass before promotion from proposal to shipping.

## Context

The Rule of Three is the canonical code-refactoring heuristic in mainstream software engineering: *do not abstract until you have copy-pasted the same code three times.* Variants appear in Randy Shoup's 2025 LinkedIn post ("Not DRY, but The Rule of Three instead"), Holden Rehg's 2021 blog, and Wikipedia's article on the rule treats it as canonical. The rule is widely cited, widely practiced, and almost universally invoked as the first line of attack against any design that introduces abstractions "early."

Planex's exposure to this critique is severe. The v0.4 shipping set has 5 abstractions (Estimate, Perception, Closure, Relation, `px_loop`); the v4 design proposal adds 3 more (Interpretant, Perlocution, Breakdown). Across these 5+3, the concrete-use count is uneven. `px_loop` had 0 uses in v0.3, was promoted to 5th abstraction in v0.4 with one demonstration, and remains single-use. The v4 proposals are not yet in shipping code at all — they exist only as `v4/tests/test_*.c` and `v4/src/*.c` prototypes. By a strict reading of the Rule of Three, every one of these abstractions is "premature," and the entire abstraction-driven route of Planex is suspect.

This critique is the strongest external attack on Planex's chosen form. It is stronger than Worse-is-Better (which argues against perfectionism, not against abstraction per se), stronger than Simple Made Easy (which argues against intertwining, not against abstraction per se), and stronger than the Leaky Abstractions law (which accepts abstraction but warns about leaks). The Rule of Three is the only critique that attacks the *timing* of abstraction — that says "you did this too early, before the evidence warranted it." It cannot be dismissed, because it is grounded in a real engineering truth: most abstractions introduced before three duplications turn out to be wrong, because the first two duplications did not yet reveal the actual common structure.

What this ADR must do is articulate *why* Planex's abstractions are exempt from this rule, *under what conditions* the exemption holds, and *what would count as the exemption failing*. Without this articulation, the rebuttal remains implicit and any external reviewer can dismiss Planex's route with one Wikipedia citation.

## Decision

**Essence-justified abstractions are exempt from the Rule of Three; duplication-justified abstractions obey it. The two routes to abstraction are independent sufficient conditions, not graded rungs on a single ladder.**

A Planex abstraction is essence-justified when it meets three criteria simultaneously:

1. **Tradition traceability.** The abstraction is traceable to one of the six traditions surveyed in [`alternative-perspectives.md`](../concepts/alternative-perspectives.md): Cognitive science, Math/Linguistic, Phenomenological, Neural predictive coding, Semiotic, or Speech-Act theory. The traceability must be a citation to a specific paper or book, not to a general vibe. For shipping v0.4 abstractions: Estimate → Elliott's denotative FRP and Friston's free-energy principle; Perception → Peirce's percept; Closure → Searle's illocutionary act; Relation → Heidegger's Mitsein and Simmel's relational sociology; `px_loop` → Hoare's CSP and Harel's statecharts.

2. **Constitutive orthogonality.** The abstraction answers a constitutive question that no other abstraction answers. This is the orthogonality test enforced in `tests/test_orthogonality.c` for shipping v0.4: each abstraction owns exactly one question (Estimate owns "what will the world be?", Perception owns "what is the world?", Closure owns "what completed?", Relation owns "what is connected to what?", `px_loop` owns "when does control yield?"). An abstraction that does not own a unique question is not essence-justified, regardless of how many traditions it cites.

3. **Denotational semantics.** The abstraction's product is a value (not a callback, not a configuration struct, not an opaque widget) that can be serialized, replayed, and denotated across multiple channels (pixel, accessibility tree, audit log). This is the engineering payoff of essence-justified abstraction: the abstraction's output is a typed value, which is what enables the four stated goals of Planex (intent-as-value, multi-channel denotation, semantic audit, cognitive-bandwidth constraint). An abstraction whose output is not a value cannot deliver the payoff, and therefore cannot claim the exemption.

An abstraction that meets all three criteria is exempt from the Rule of Three. It may be admitted with zero concrete uses (as `px_loop` was in v0.4) provided the three criteria are documented in its admission ADR. An abstraction that does not meet all three criteria must obey the Rule of Three: it must demonstrate three concrete duplications in `examples/` or `tests/` before promotion.

This distinction is not a defense of "abstraction in general" — it is a targeting correction. The Rule of Three is correct for duplication-justified abstraction, where the rule's premise (the abstraction is inferred from observed repetition) holds. Essence-justified abstraction has a different premise: the abstraction is inferred from a tradition's account of the domain's structure, not from observed repetition. The Rule of Three's evidence criterion (three duplications) does not apply to a different evidence criterion (one tradition citation plus orthogonality plus denotational semantics). The two routes are independent.

## Essence Check

> This decision touches the semantic interface axis: it defines the criterion by which abstractions are admitted to or rejected from the semantic interface. It does not add a new abstraction or change an existing one's denotation.

### Q1. Which essence axis does this decision affect?

- [x] **Semantic interface** — the criterion by which new entries are admitted to or rejected from the bidirectional encoding/decoding surface.
- [ ] Intent space (the user → machine direction)
- [ ] State space (machine-side state)
- [ ] None — engineering decision

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions (cognitive bandwidth reduced):**

- Contributors can stop defending each abstraction from Rule-of-Three attacks individually. One citation to this ADR suffices to retire the critique.
- Reviewers can dismiss Rule-of-Three critiques of essence-justified abstractions with a single reference, instead of re-arguing the distinction in every code review.
- The admission criterion for new abstractions is now explicit: tradition-cite, orthogonality-test, denotational-semantics-check. Contributors proposing a new abstraction know in advance what they must show.

**Increases (cognitive bandwidth added):**

- Contributors must learn to distinguish essence-justified from duplication-justified abstraction before proposing new ones. The distinction is documented in this ADR but is not yet enforced in CI, so the burden is on the contributor to read and apply it.
- Reviewers must verify each essence-justified claim by checking the tradition citation, the orthogonality test, and the denotational semantics. This is real review work, not rubber-stamping.

**Net assessment:** Compresses. The contributor's one-time cost of learning the distinction (perhaps an hour of reading this ADR plus [`abstraction-form.md`](../concepts/abstraction-form.md) plus [`alternative-perspectives.md`](../concepts/alternative-perspectives.md)) is less than the recurring cost of defending each abstraction from Rule-of-Three critiques in every external review and every internal code review for the lifetime of the project.

### Q3. Is there a gap between the claim and the implementation?

- **Claim (in [`why-four-abstractions.md`](../concepts/why-four-abstractions.md), ADR-0007, ADR-0008, ADR-0010):** the 5 shipping abstractions are essence-justified, grounded in the 6-tradition literature survey.
- **Implementation:** there is no formal criterion in code or CI that distinguishes essence-justified from duplication-justified. The distinction lives only in ADRs. A contributor proposing a new abstraction could in principle claim "essence-justified" without meeting the three criteria, and CI would not catch it. The `tests/test_orthogonality.c` enforces the orthogonality half of the criterion but not the tradition-traceability or denotational-semantics halves.
- **Gap:** documented criterion (this ADR) but no enforcement mechanism. The gap is real and acknowledged. Closing it is on the roadmap: a CI check that requires any new abstraction's admission ADR to cite (a) a specific tradition paper, (b) the result of the orthogonality test, (c) the value type the abstraction produces and the channels it denotates through. Until that check exists, the gap is closed only by reviewer vigilance.

### Q4. What is the cost, and who can verify it?

- **Cost:** contributors may prematurely invoke "essence-justified" to defend bad abstractions. The 6-tradition citation could become a checkbox rather than a real grounding — a contributor cites Peirce's semiotics without having read Peirce, and reviewers rubber-stamp. This is the cost of any criterion that relies on documentation rather than mechanism: the criterion can be gamed.
- **Who can verify:** any reviewer can challenge an essence-justified claim by demanding the specific tradition citation. Which of the six traditions names this essence? Which paper, which passage, which argument? If the proposer cannot answer, the claim is invalid and Rule of Three applies. This verification is cheap: it takes one minute of reviewer time per claim.
- **Verification scenario:** a contributor proposes a new abstraction "Memory" in ADR-0015, claiming essence-justified. The reviewer asks: which of the six traditions names memory as a UI essence? The proposer cites Friston's free-energy principle and the predictive-processing account of working-memory maintenance. The reviewer reads the citation, finds that Friston's account does treat active maintenance of beliefs as a core feature of predictive systems, and accepts the citation as legitimate — at which point the orthogonality test (does Memory answer a question no other abstraction answers?) and the denotational-semantics test (does Memory produce a serializable value?) still must be passed. If any of the three checks fails, the abstraction is not admitted and must obey Rule of Three until it has three duplications. The cost (rubber-stamping) is measurable in principle: count the ADRs that cite a tradition but do not show how the abstraction's denotation follows from the cited tradition's account. A count greater than zero indicates the cost is being realized.

### Q5. What are the counterexamples?

This ADR's scope is bounded by the three criteria above. The exemption does not extend to everything in the Planex repository.

- **Counterexample 1: utility code.** Planex's memory-allocation wrappers, string utilities, file-I/O helpers, and similar infrastructure code are not abstractions in the essence-justified sense. They obey Rule of Three strictly: abstract a utility only after three duplications. Misapplying this ADR to utility code would be a category error — utility code has no tradition citation, no constitutive question, and no denotational value type.
- **Counterexample 2: experimental prototypes.** The v4 proposed abstractions (Interpretant, Perlocution, Breakdown) are not yet shipping and have not yet passed the three criteria in a reviewable ADR. Until they do, they obey Rule of Three: do not promote from prototype to abstraction until either (a) three concrete uses in `examples/` or `tests/` exist OR (b) the three essence-justified criteria are documented in an admission ADR. The current v4 prototypes are in `v4/tests/` and `v4/src/` precisely because they have not yet met either bar.
- **Counterexample 3: third-party bindings and adapters.** When Planex eventually ships bindings to other languages (Python, Rust, Zig — per ADR-0004 Alternative 4, deferred), the binding code obeys Rule of Three strictly. A binding is not essence-justified; it is engineering glue.
- **Counterexample 4: widget implementations.** The concrete widget code (counter, slider, button) is not essence-justified abstraction; it is the *use* of the abstractions. Rule of Three applies to widget-internal code normally: if three widgets end up with the same event-handling boilerplate, that boilerplate may be abstracted into a helper.

**Scope statement:** This decision applies to Planex's 5 shipping abstractions (v0.4) and any future abstraction proposed for admission to the semantic interface. It does NOT apply to utility code, internal helpers, widget-internal code, third-party bindings, or experimental prototypes that have not yet met the essence-justified bar in a reviewable ADR.

## Consequences

### Positive

- One-citation rebuttal to the strongest external critique. Any reviewer who dismisses Planex's abstractions as "premature" by Rule of Three can be pointed to this ADR, which articulates the principled distinction between essence-justified and duplication-justified routes and the three criteria the former must meet.
- Clear admission criterion for new abstractions going forward. Future ADRs that propose new abstractions must include an Essence-Justified Check (tradition citation, orthogonality test, denotational semantics) or fall back to duplication-justified (three concrete uses in `examples/` or `tests/`). There is no third route.
- Protects v0.4 and v4 from being dismissed as premature architecture. The five shipping abstractions each have documented tradition citations in [`why-four-abstractions.md`](../concepts/why-four-abstractions.md); the three v4 proposals each have documented tradition citations in [`essence-derivation-v4-clean.md`](../concepts/essence-derivation-v4-clean.md); the criterion is now uniform across both.
- Establishes a falsifiability hook (per Prerequisite 3 of [`abstraction-form.md`](../concepts/abstraction-form.md)): if a future audit finds that an abstraction admitted as essence-justified does not in fact meet the three criteria, the admission ADR can be superseded and the abstraction retired. The criterion is the test.

### Negative

- The "essence-justified" label could be abused by contributors who cite a tradition without understanding it. The 6-tradition citation could become ceremonial rather than substantive — a contributor writes "Peirce, 1860s" in their ADR without having read Peirce, and reviewers who have also not read Peirce accept the citation. This is the recurring failure mode of any criteria system that relies on documentation rather than mechanism.
- Adds a non-trivial conceptual burden on new contributors. A contributor who wants to propose a new abstraction must learn the 6-tradition survey, the orthogonality test, and the denotational-semantics requirement before they can write a serious admission ADR. This is real up-front cost and may deter casual contributors.
- Does not help if the parent tradition itself is later overturned. If cognitive science revises the prediction/perception boundary, the Estimate/Perception split breaks regardless of how well this ADR's criteria were applied at admission time. That failure mode is handled by Prerequisite 1 (ontological stability) in [`abstraction-form.md`](../concepts/abstraction-form.md), not by this ADR.

### Neutral

- Future ADRs proposing new abstractions must include essence-justified reasoning or fall back to duplication-justified. The ADR template may eventually be amended to include an explicit "Justification Route" field; until then, the route is implicit in the Context section.
- Rule of Three remains the default for all non-abstraction code. This ADR does not displace the rule for utility code, prototypes, or bindings; it carves out a specific exemption for abstractions that meet three documented criteria.

## Alternatives Considered

### Alternative 1: Apply Rule of Three strictly to all Planex code, including abstractions

- **What:** Every abstraction, including the 5 shipping v0.4 abstractions and the 3 v4 proposals, must demonstrate three or more concrete uses in `examples/` or `tests/` before being admitted. Essence-style justification is not accepted as a substitute.
- **Why rejected:** This alternative would force Planex to ship v0.4 with zero abstractions, because none of the five has three or more real consumers in shipping code today. (Estimate is used in `counter_4abs.c` and `slider` examples; Perception in `perception_smoke.c` and `multi_perception.c`; Closure in `calculator_denotative.c` and `counter_denotative.c`; Relation in `undo_via_graph.c` and `editor_meaning.c`; `px_loop` in one demonstration.) It would also block all v4 proposals indefinitely. This contradicts the entire premise of essence-driven design: that an abstraction can be justified by tradition-grounded reasoning about the domain's structure, not only by observed repetition. The Rule of Three is a *sufficient* condition for abstraction (when three duplications exist, abstracting is safe); it is not a *necessary* one (essence-justified abstractions can be admitted without three duplications, provided the essence criterion is met). Applying Rule of Three strictly assumes the only legitimate route to abstraction is duplication, which begs the question against essence-driven design. The Rule of Three's evidence criterion is appropriate for duplication-justified abstraction and inappropriate for essence-justified abstraction; conflating them is a category error.

### Alternative 2: Drop essence framing entirely; recast Planex as a component library with 5 opinionated components

- **What:** Stop claiming the 5 abstractions are essence-justified. Treat them as opinionated components in the style of GLib's `GObject` or libuv's `uv_loop_t`. They are components because they are useful, not because they are essences.
- **Why rejected:** This alternative would lose the engineering payoff that motivates Planex's existence. Components do not give you serializable intent; only typed-value abstractions do. The four stated goals of Planex (intent-as-value so it can be replayed, multi-channel denotation so the same intent drives pixel and accessibility and audit, semantic-level audit across five axes, cognitive-bandwidth constraint on the maintainer) require the abstraction-as-typed-value form. Component library form sacrifices at least two of these (intent-as-value, multi-channel denotation) because components are encapsulated behaviors, not typed values. Furthermore, ADR-0010 already downgraded the framing from "essence discovery" to "design rationale," which is the honest epistemic move; dropping the framing further to "opinionated component" would be more conservative than the evidence warrants. The traditions Planex cites (Elliott, Peirce, Searle, Heidegger, Hoare, Harel) are real and provide real grounding for the abstraction-as-typed-value form; pretending they do not exist in order to dodge the Rule-of-Three critique would be intellectually dishonest.

### Alternative 3: Hybrid — grandfather v0.4 shipping abstractions, apply Rule of Three to v4 proposals and all future abstractions

- **What:** The 5 shipping abstractions are exempted by grandfathering (they are already shipped, reversing them is costly). All future abstractions, including the v4 proposals, obey Rule of Three strictly until they have three duplications.
- **Why rejected:** This alternative creates two classes of abstractions with no principled boundary between them. The v4 proposals (Interpretant, Perlocution, Breakdown) have the same kind of tradition-grounding as the v0.4 five — in fact, the same traditions: Interpretant is Peirce (the same tradition as Perception), Perlocution is Searle (the same tradition as Closure), Breakdown is Heidegger and Winograd/Flores (the same tradition as Relation). Applying Rule of Three to the v4 proposals but exempting the v0.4 five would be arbitrary: it would exempt abstractions because they were earlier, not because they were better-grounded. The correct distinction is not temporal (shipped vs proposed) but epistemic (essence-justified vs duplication-justified). This ADR's criterion applies uniformly: the v0.4 five pass the three criteria today; the v4 proposals must pass them before promotion, and if they do, they are exempt on the same grounds. The hybrid alternative would freeze Planex's abstraction set at v0.4 and prevent legitimate essence-justified additions, which would be the wrong outcome if the v4 proposals turn out to meet the bar.

## References

- **Code:** `tests/test_orthogonality.c` — the orthogonality check for the 5 shipping abstractions (criterion 2 enforcement, partial).
- **Code:** `v4/tests/test_*.c` — v4 proposal tests; not yet part of shipping test suite; pending criterion review.
- **Code:** `examples/counter_4abs.c`, `examples/calculator_denotative.c`, `examples/undo_via_graph.c` — concrete uses of the shipping abstractions (criterion relevance: duplication-justified route, not the route the shipping abstractions were admitted by).
- **Related ADRs:** [ADR-0010](ADR-0010-v4-design-rationale-not-essence-discovery.md) — the honesty downgrade this ADR extends; [ADR-0007](ADR-0007-essence-derivation-v2-revision.md) — v2 essence-derivation; [ADR-0008](ADR-0008-feedback-as-fifth-essence-category.md) — `px_loop` as 5th abstraction; [ADR-0009](ADR-0009-essence-rederivation-v3.md) — v3 essence re-derivation (Proposed, framing downgraded by ADR-0010).
- **Related docs:** [`../concepts/abstraction-form.md`](../concepts/abstraction-form.md) — the conditional thesis with three prerequisites; this ADR is one of the three caveats identified there. [`../concepts/why-four-abstractions.md`](../concepts/why-four-abstractions.md) — argues which abstractions; this ADR argues the form and admission criterion. [`../concepts/alternative-perspectives.md`](../concepts/alternative-perspectives.md) — the 6-tradition literature survey that grounds criterion 1. [`../research/2025-08-28-abstraction-as-form-comparative-study.md`](../research/2025-08-28-abstraction-as-form-comparative-study.md) — the long-form research that prescribes this ADR (Gap 2).
- **External:** Wikipedia, "Rule of three (computer programming)" — https://en.wikipedia.org/wiki/Rule_of_three_(computer_programming)
- **External:** Randy Shoup, "Not DRY, but The Rule of Three instead" (LinkedIn, 2025) — https://www.linkedin.com/posts/randyshoup_not-dry-but-the-rule-of-three-instead-activity-7308790229690134528-Sk-d
- **External:** Holden Rehg, "The Rule of Three" (2021) — https://holdenrehg.com/blog/2021-09-20_rule-of-three
- **External:** Conal Elliott, "Denotational Design with Type Class Morphisms" — the abstraction-as-typed-value form Planex uses, which is the form this ADR's criterion 3 (denotational semantics) operationalizes.
