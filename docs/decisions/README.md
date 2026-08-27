# Architecture Decision Records (ADR)

> Record of architecturally significant decisions: each ADR documents **one** decision, its context, its alternatives, and its consequences — and, for any abstraction-affecting decision, an explicit **essence check** that forces the author to think from UI's essence rather than from intuition.

ADR is a standard practice for research-grade and architecturally significant projects (Rust RFCs, seL4 design notes, MLIR Rationale, Lean RFCs). Planex is a research-grade UI library — most decisions are about **what the abstractions mean**, not about engineering details. Those decisions need to survive maintainer turnover, community pressure, and future refactoring.

---

## Where ADRs live

```
docs/decisions/
├── README.md             # this file — index + writing rules
├── TEMPLATE.md           # copy this to start a new ADR
├── ADR-0001-*.md
├── ADR-0002-*.md
└── ...
```

## When to write an ADR

Write an ADR when a decision is:

- **Architecturally significant** — affects the core abstractions (Relation / Estimate / Closure / Perception) or their semantics
- **Hard to reverse** — once shipped, reverting requires breaking public API or user code
- **Has alternatives** — if there's no real alternative, it's not a decision, it's a fact
- **Easy to forget why** — decisions that look "obvious" in hindsight but had genuine debate

## When NOT to write an ADR

- Bug fixes
- Performance optimizations (those go in changelog)
- Documentation polish
- New widget demos (those are engineering)
- Anything that doesn't affect the abstractions

## ADR format (mandatory)

See `TEMPLATE.md`. The required sections:

1. **Status** — Proposed / Accepted / Deprecated / Superseded by ADR-MMMM
2. **Context** — what forces are at play? what constraint forced this decision?
3. **Decision** — what did we choose?
4. **Essence Check** (mandatory for abstraction-affecting decisions) — five questions, see below
5. **Consequences** — Positive / Negative / Neutral
6. **Alternatives Considered** — what else was on the table, why was it rejected
7. **References** — external sources, related ADRs, code paths

For purely engineering decisions (e.g. "use XShm for blitting"), the Essence Check may be omitted — but mark this explicitly by writing "Engineering decision, no essence impact" in the Q1 slot.

---

## Thinking from essence — the five questions

> A research-grade UI library must think from UI's essence, not from implementation convenience. The five questions below are mandatory for any ADR that touches the core abstractions.

Planex's statement of UI essence:

1. UI is a **semantic interface** between human intent space and machine state space
2. UI does two things: encode intent → machine instructions; decode state → perceivable form
3. UI's fundamental constraint is **human cognitive bandwidth**, not machine compute

From these three statements, every essence-driven decision should be answerable through five questions:

### Q1. Which essence axis does this decision affect?

Pick one (or "None — engineering decision"):
- **Intent space** (the user → machine direction: how intent is encoded)
- **State space** (machine-side state, its time evolution, its uncertainty)
- **Semantic interface** (the bidirectional encoding/decoding surface)

If the answer is "None", this is an engineering decision — fine, but don't dress it up as essence. Stop here and go to "Alternatives Considered".

### Q2. Does it compress or increase human cognitive bandwidth?

Be specific. List concrete compressions and concrete increases. A decision that adds a new concept compresses one dimension but expands another — name both.

**Example (good):** "Adding Intent-as-value compresses cognitive bandwidth by making intent serializable (no need to mentally track side effects). It increases cognitive bandwidth by adding 5 intent kinds to remember."

**Example (bad):** "It makes the API cleaner." — vague, not essence-driven.

### Q3. Is there a gap between the claim and the implementation?

- **Claim**: what the README/manifesto says
- **Implementation**: what the code actually does
- **Gap**: the distance between them

The strongest possible state is "Claim = Implementation" — no gap. This is Conal Elliott's denotative ideal: every abstraction's denotation matches its implementation.

**Example:** Planex currently claims "3 abstractions" but Perception is a no-op placeholder. There IS a gap. ADR-0001 records this gap explicitly. Any decision that closes the gap (like the (c) route) is essence-positive; any decision that widens it is essence-negative.

### Q4. What is the cost, and who can verify it?

Every essence-driven decision has a cost. The cost must be **verifiable** — there must be a concrete scenario where the cost shows up.

- **Cost**: what gets harder / slower / less flexible
- **Who can verify**: who would encounter this cost in what scenario
- **Verification scenario**: the specific use case where the cost becomes measurable

Vague "it might be slower" is not allowed. If you cannot name a verification scenario, the cost is fear, not fact.

**Example (good):** "Cost: GPU backend is harder. Verifier: future GPU backend implementer. Scenario: each frame's PixelBuffer allocation costs ~1-2ms for texture upload + destruction on a Vulkan backend at 1080p/60fps."

**Example (bad):** "Cost: maybe performance issues."

### Q5. What are the counterexamples?

What use cases does this decision **fail** or **not apply** to? List them concretely. A decision with no identified counterexamples is suspect — it usually means you haven't thought hard enough about its scope.

After listing counterexamples, write a **Scope Statement**: "This decision applies to X. It does NOT apply to Y."

**Example:** "(c) route applies to simple state-driven UI (counter, slider, form). It does NOT apply to hover/drag preview (state is transient, not in Estimate) or video playback (rendering comes from external decoder, not from Estimate)."

---

## Numbering rules

- Numbers are monotonically increasing, never reused
- File name format: `ADR-NNNN-short-kebab-title.md`
- Zero-padded to 4 digits: `ADR-0001`, `ADR-0042`
- Superseding an ADR: create a new ADR, set old one's Status to `Superseded by ADR-MMMM`, **do not delete the old one**

## ADR index

| # | Title | Status |
|---|---|---|
| [0001](ADR-0001-perception-currently-noop.md) | Perception is currently a no-op placeholder | Superseded by ADR-0005 |
| [0002](ADR-0002-relation-necessity-pending-undo.md) | Relation's necessity is not yet proven | Accepted |
| [0003](ADR-0003-no-ai-integration.md) | No AI integration in Planex core | Accepted |
| [0004](ADR-0004-use-c-not-rust-zig-cpp.md) | Implementation language is C17, not Rust/Zig/C++ | Accepted |
| [0005](ADR-0005-promote-perception-to-fourth-abstraction.md) | Promote Perception to the fourth abstraction | Accepted (framing corrected by ADR-0007) |
| [0006](ADR-0006-continuous-interaction-deferred.md) | Continuous interaction abstraction deferred to v1.0+ | Accepted |
| [0007](ADR-0007-essence-derivation-v2-revision.md) | Essence Derivation v2 — revised essence claim | Accepted |
| [0008](ADR-0008-feedback-as-fifth-essence-category.md) | Feedback as 5th essence category (v0.4) | Accepted |
| [0009](ADR-0009-essence-rederivation-v3.md) | Essence re-derivation v3 + Path B prototype | Proposed (essence-claim framing downgraded by ADR-0010; implementation decisions stand) |
| [0010](ADR-0010-v4-design-rationale-not-essence-discovery.md) | v4 essence derivation is design rationale, not essence discovery | Accepted |
| [0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) | Essence-justified abstraction is exempt from the Rule of Three | Accepted |

## Style rules

- One decision per ADR. If you're writing two decisions, split it.
- Write prose, not bullets. ADR is for reasoning, not checklists.
- The "Alternatives Considered" section is mandatory — if you can't name at least one real alternative, the decision wasn't a decision.
- The "Essence Check" is mandatory for abstraction-affecting decisions. If you skip it for such decisions, the ADR is incomplete.
- Keep ADRs immutable once Accepted. New info → new ADR that supersedes.
- Link to code, not to lines. Code lines move; file/function names don't.

## See also

- Michael Nygard's original ADR article (https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions)
- adr.github.io
- Rust RFC process (https://github.com/rust-lang/rfcs)
- Conal Elliott, "Denotational Design with Type Class Morphisms" — the "claim = implementation" ideal
