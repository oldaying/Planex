# ADR-0007: Essence Derivation v2 — revised essence claim

## Status

Accepted

Date: 2026-08-27

Supersedes: the essence framing in [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md) (the "4 abstractions = 4 essence axes" claim). ADR-0005 is not deprecated — its decision to promote Perception stands. But ADR-0005's implicit framing that "this completes the essence coverage" is corrected by this ADR.

## Context

ADR-0005 framed Planex as "4 abstractions = 4 essence axes". This was the canonical claim in `why-four-abstractions.md` and across the project's documentation.

This claim was derived from `essence-derivation.md` (v1), a single-author derivation from a minimal UI definition. v1 concluded:
- 3 abstractions (Estimate, Closure, Perception) are essence-derived
- 1 abstraction (Relation) is structurally-derived under an additional "UI is a network" premise

A subsequent 6-tradition literature survey (archived in `research/reports/`) revealed v1 was **correct in form but under-evidenced**:

1. v1 was wrong about Relation. Relational ontology is essence, not structural — grounded in Heidegger (Zuhandenheit), Gibson (affordance-as-relation), Dourish (embodiment), Hutchins (distributed cognition), Alexander (semilattice). v1's "topology premise" missed the ontology premise.

2. v1 missed Feedback as a separate essence category. Feedback (the closed loop of intent → action → state → perception → next intent) is independently rediscovered in every historical era, every HCI theory, every modern framework, every phenomenological critique, and every mathematical formalization (CSP trace, statechart transition, FRP causality). It is essence — but Planex implements it only implicitly via Closure+Perception, not as a first-class concept.

3. v1 did not acknowledge deferred essence candidates. Philosophy traditions (Heidegger, Suchman, Dourish, Gibson, Winograd/Flores) describe essence categories Planex does not implement:
   - Embodiment (Dourish)
   - Situatedness (Suchman)
   - Affordance-as-relation (Gibson, original)
   - Breakdown (Heidegger-Winograd/Flores)
   
   v1 didn't list these as deferred — it didn't acknowledge them at all. This is the same over-claim pattern v1 itself was trying to correct.

The full v2 derivation is in [essence-derivation-v2.md](../concepts/essence-derivation-v2.md). The 6 research reports that grounded it are:
- 01-early-history.md (Sketchpad → CUA → CHI)
- 02-hci-theory.md (GOMS → Dourish, 3 incommensurable paradigms)
- 03-functional-reactive.md (Conal → re-frame, with Eve postmortem)
- 04-modern-architecture.md (React → Dear ImGui, 4-way split)
- 05-phenomenology.md (Heidegger → Turkle, 11 family-resemblance categories)
- 06-mathematical.md (denotational design → statecharts, 5 orthogonal essence candidates)

### Forces

1. **Planex's essence-driven commitment**: A research-grade UI library must keep its claim aligned with its implementation. The "4 = 4" framing is an over-claim because (a) Feedback is partial, (b) 4 essence categories are silently deferred.

2. **v1's own methodology demanded this**: v1 explicitly said "If this document concludes an abstraction does NOT emerge from essence, the project's canonical claim must be revised." v2 surfaced two such cases (Relation's status revised; Feedback's gap surfaced). Revision is therefore required by v1's own rule.

3. **The honest-claim requirement**: `path-C-lineage.md` and `limitations.md` both commit Planex to "honest acknowledgment — no over-claiming". The "4 = 4" framing over-claims in two directions: it claims Relation is structural (under-claim, v2 says it's essence), and it claims essence coverage is complete (over-claim, v2 says Feedback is missing + 4 deferred).

4. **No code change is required**: This is a documentation/claim revision. The 4 abstractions remain implemented. Their essence status is what changes.

### Constraints

- The 4 implemented abstractions (Estimate, Closure, Perception, Relation) remain as-is. This ADR revises the **claim**, not the **code**.
- The Feedback gap is acknowledged but not closed in this ADR. A future ADR will design + implement Feedback if essence-driven pressure demands it.
- The 4 deferred essence candidates remain deferred. They are acknowledged, not implemented.

## Decision

### D1. Revise the canonical claim

The canonical claim changes from:
- **Old (ADR-0005 era)**: "Planex is built on 4 abstractions = 4 essence axes"
- **New (this ADR)**: "Planex implements 4 of 5 essence categories (State, Communication, Presentation, Relational ontology). The 5th (Feedback / closed-loop coupling) is partial — implemented implicitly via Closure+Perception, not as a first-class concept. 4 additional essence categories (Embodiment, Situatedness, Affordance-as-relation, Breakdown) are acknowledged as essence but deferred."

### D2. Revise `why-four-abstractions.md`

The canonical manifesto is rewritten to reflect the new claim. The file name is kept (renaming would break links), but the framing changes from "4 = 4" to "4 implemented + 1 partial + 4 deferred".

### D3. Mark v1 as superseded

`essence-derivation.md` (v1) gets a "⚠️ SUPERSEDED by v2" banner at the top, with a brief summary of what v2 changed. The document body is kept for historical reference.

### D4. Acknowledge deferred essence candidates

`limitations.md` is updated (or a new section is added) listing the 4 deferred essence candidates with a one-line definition each and a "why deferred" note. This makes the deferral explicit, not silent.

### D5. Do NOT close the Feedback gap in this ADR

Closing the Feedback gap requires designing what "first-class Feedback" means in Planex's API. That is a separate decision requiring its own ADR (future ADR-0008, if pursued). This ADR only acknowledges the gap exists.

## Essence Check

### Q1. Which essence axis does this decision affect?

**All of them, by reframing.** This ADR revises how each abstraction's essence status is described:
- Estimate: essence-derived (unchanged from v1)
- Closure: essence-derived (unchanged from v1)
- Perception: essence-derived (unchanged from v1)
- Relation: revised from "structural" (v1) to "essence-derived under relational-ontology premise" (v2)
- Feedback: newly identified as a separate essence category, currently partial
- Embodiment/Situatedness/Affordance-as-relation/Breakdown: newly acknowledged as deferred essence

### Q2. Does it compress or increase human cognitive bandwidth?

**Compresses**: maintainers and contributors now have an honest map of what Planex does and doesn't claim. Reading `why-four-abstractions.md` no longer leaves a false impression of complete essence coverage.

**Increases**: the claim is now more nuanced ("4 + 1 partial + 4 deferred") instead of the simpler "4 = 4". This is the cost of honesty — the simpler claim was wrong.

### Q3. Is there a gap between the claim and the implementation?

**Yes, and this ADR is the gap-closing action.** Before this ADR:
- Claim: "4 abstractions = 4 essence axes, complete coverage"
- Implementation: 4 abstractions + implicit Feedback + 4 unacknowledged deferred essence categories
- Gap: over-claim in two directions

After this ADR:
- Claim: "4 essence + 1 partial + 4 deferred" (matches v2 derivation)
- Implementation: unchanged (4 abstractions + implicit Feedback + 4 deferred)
- Gap: claim matches implementation; the Feedback partial-implementation is now visible as a known gap, not hidden

### Q4. What is the cost, and who can verify it?

- **Cost**: future contributors reading the revised claim will encounter 5 essence categories where Planex has 4 abstractions. They may ask "when will Feedback become first-class?" or "when will Embodiment be implemented?" These are fair questions and the answers ("when essence-driven pressure demands; not before") require maintainer judgment.
- **Verifier**: any contributor who reads `why-four-abstractions.md` and `essence-derivation-v2.md` and asks "does the implementation match this claim?"
- **Verification scenario**: a contributor tries to use Planex for an application that needs first-class Feedback (e.g., interruptible batch updates, loop auditing, breakdown detection). They will find Feedback is implicit, not first-class. The gap is now visible.

### Q5. What are the counterexamples?

- **Counterexample 1**: A maintainer might argue "v2 is over-engineering — Planex is pre-v1.0, the simple '4 = 4' claim is fine for now." Rebuttal: essence-driven projects cannot defer honesty. If the claim is wrong, it's wrong now, not later.

- **Counterexample 2**: A maintainer might argue "v2's deferred essence candidates (Embodiment etc.) are too speculative — they belong to philosophy, not engineering." Rebuttal: the 6-tradition survey found these categories independently in HCI theory, philosophy, and math traditions. They are not speculation — they are documented essence claims Planex does not address. Acknowledging them is the minimum honesty requirement.

- **Counterexample 3**: A maintainer might argue "Feedback isn't a separate essence category — it's a derived property of State+Communication+Presentation+Relation." Rebuttal: cross-tradition convergence disagrees. Every tradition that discusses feedback treats it as primitive (CSP's trace, statechart's transition, FRP's causality, KLM's system response R, Norman's gulf of evaluation, breakdown as feedback-failure). If Feedback were derived, these traditions wouldn't need it as primitive.

### Scope Statement

This ADR applies to Planex's canonical essence claim and documentation. It does **not** apply to:
- Planex's code (unchanged)
- The 4 implemented abstractions' APIs (unchanged)
- Future decisions about whether to close the Feedback gap (separate ADR)
- Future decisions about whether to implement any deferred essence candidate (separate ADRs)

## Consequences

### Positive

- **Claim matches implementation**. Planex's canonical claim is now honest — it states what's implemented, what's partial, what's deferred. This is the essence-driven minimum.
- **Future contributors get an honest map**. They can see which essence categories Planex covers and which it doesn't, without having to reverse-engineer from code.
- **v1's methodology is validated**. v1 explicitly said "if this document concludes X, the claim must be revised". v2 concluded X. Following through on v1's rule demonstrates the methodology works.
- **Pressure to close Feedback gap is now legitimate**. With the gap acknowledged, future maintainers can ask "should we close this?" without pretending it doesn't exist.

### Negative

- **The claim is more complex**. "4 essence + 1 partial + 4 deferred" is harder to communicate than "4 = 4". This may make Planex harder to pitch in a single sentence.
- **Contributors may push to implement deferred categories prematurely**. Acknowledging Embodiment/Situatedness/etc. as essence may create "completeness anxiety" pressure. Must resist: deferred categories should be implemented when use cases demand, not when anxiety demands.
- **v2 may itself be revised**. A future v3 could surface more essence categories or revise v2's status of Relation/Feedback. This is a feature (the methodology is self-correcting), but it means the canonical claim is not final.

### Neutral

- **No code change**. The 4 abstractions remain implemented. The orthogonality test suite remains valid. Existing demos remain functional.
- **v1 is preserved as historical record**. Researchers can trace how the derivation evolved.

## Alternatives Considered

### A1. Keep "4 = 4" framing, treat v2 as internal research

Rejected because it violates the essence-driven minimum: if v2 found over-claims, the canonical claim must be revised. Hiding v2 as internal research is the same over-claim pattern v2 itself corrected.

### A2. Close the Feedback gap in this ADR

Rejected because designing first-class Feedback is a separate decision requiring its own ADR. Mixing "acknowledge the gap" with "close the gap" would rush the design. This ADR is about honesty; closing the gap is about engineering.

### A3. Demote Relation back to "structural" (keep v1's framing)

Rejected because v2's literature survey found strong cross-tradition support for relational ontology as essence (Heidegger, Gibson, Dourish, Hutchins, Alexander, π-calculus, constraint systems). v1's "topology premise" was a misframing. Reverting to v1 would be ignoring evidence.

### A4. Acknowledge only Feedback, not the 4 deferred candidates

Rejected because it's incomplete honesty. If Embodiment/Situatedness/Affordance-as-relation/Breakdown are essence (per v2's survey), not acknowledging them is the same as v1's omission. Half-honesty is not honesty.

## References

- [essence-derivation-v2.md](../concepts/essence-derivation-v2.md) — the v2 derivation that grounds this ADR
- [essence-derivation.md](../concepts/essence-derivation.md) — v1 (superseded by v2)
- [why-four-abstractions.md](../concepts/why-four-abstractions.md) — canonical manifesto, revised per this ADR
- [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md) — Perception's promotion (framing corrected, decision stands)
- [limitations.md](../concepts/limitations.md) — where deferred essence candidates are listed
- Research reports (workspace, not in repo): `research/reports/00-summary.md` through `06-mathematical.md`

## See also

- [ADR-0001](ADR-0001-perception-currently-noop.md) — historical record of the original Perception gap (superseded by ADR-0005)
- [ADR-0006](ADR-0006-continuous-interaction-deferred.md) — another deferred-essence ADR (continuous interaction)
