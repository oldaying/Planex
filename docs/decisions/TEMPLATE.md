# ADR-NNNN: [Decision title — one short sentence]

> Copy this template to `ADR-NNNN-short-kebab-title.md`. Replace every section.
>
> The "Essence Check" section (5 questions) is **mandatory** for any decision
> that touches the three core abstractions (Relation / Estimate / Closure /
> Perception) or their semantics. For purely engineering decisions (new
> backend, build system change, etc.), that section may be omitted, but the
> other six sections are still required.

## Status

Proposed | Accepted | Deprecated | Superseded by ADR-MMMM

Date: YYYY-MM-DD

## Context

What is the issue we're facing? What forces are at play? What constraint forced this decision to be made now rather than later?

This section is the most important part of the ADR. A reader six months from now should be able to understand **why this was a real question**, not just what was answered. Include:

- The problem (what's wrong without this decision)
- The constraints (what can't we change)
- The forces (what pulls in different directions)

## Decision

What did we choose? State it as a single declarative sentence first, then explain.

## Essence Check (mandatory for abstraction-affecting decisions)

> The five questions every essence-driven decision must answer.
> See `README.md` → "Thinking from essence — the five questions" for guidance.

### Q1. Which essence axis does this decision affect?

Pick one or more:

- [ ] Intent space (user → machine direction)
- [ ] State space (machine-side state, its time/uncertainty)
- [ ] Semantic interface (the bidirectional encoding/decoding surface)
- [ ] None — this is a purely engineering decision

If "None", skip Q2-Q5 and write: "Engineering decision, no essence impact. See Alternatives Considered."

### Q2. Does it compress or increase human cognitive bandwidth?

Be specific. List concrete compressions and concrete increases.

**Compressions (cognitive bandwidth reduced):**
- ...

**Increases (cognitive bandwidth added):**
- ...

**Net assessment:** Compresses / Increases / Balanced

### Q3. Is there a gap between the claim and the implementation?

- **Claim** (what the README/manifesto says): ...
- **Implementation** (what the code actually does): ...
- **Gap**: ...

If "No gap" — write "Claim = Implementation" explicitly. This is the strongest possible state.

### Q4. What is the cost, and who can verify it?

Every essence-driven decision has a cost. The cost must be **verifiable** — i.e. there must be a concrete scenario where the cost shows up. Vague "it might be slower" is not allowed.

- **Cost**: ...
- **Who can verify**: ...
- **Verification scenario**: ...

If you cannot name a verification scenario, the cost is not real, it's fear. Either find the scenario or reconsider the decision.

### Q5. What are the counterexamples?

What kinds of use cases or scenarios where this decision **fails** or **does not apply**? List them concretely.

- Counterexample 1: ...
- Counterexample 2: ...

If you cannot think of any counterexamples, write "None identified" — but be honest. A decision with no counterexamples is suspect; it usually means you haven't thought hard enough about its scope.

**Scope statement:** This decision applies to [...]. It does NOT apply to [...].

## Consequences

### Positive
- ...

### Negative
- ...

### Neutral
- ...

## Alternatives Considered

For each alternative, document:
- What it is
- Why we didn't choose it

If you cannot name at least one real alternative, this is not a decision — it's a fact. Move it to the changelog, not the ADR.

### Alternative 1: [name]
- What: ...
- Why rejected: ...

### Alternative 2: [name]
- What: ...
- Why rejected: ...

## CAVEATS

What this decision does NOT cover. Warnings/gotchas the reader should know about THIS decision.

This section is **distinct from "Alternatives Considered"** (rejected paths)
and from **"Consequences"** (expected downstream effects). CAVEATS is the
"this is what the decision does not promise" list. Examples:

- "This decision reorganizes the abstractions but does NOT promise that
  no further reorganization will happen — see `leak-budgets.md` for the
  retire mechanism."
- "The v4 essence rederivation is one design proposal; it does NOT
  close the falsifiability gap on whether 8 abstractions are
  indispensable. See ADR-0012 Q3."

Every Accepted ADR should have at least one caveat. A decision with
zero caveats is suspect; it usually means the author has not thought
hard enough about what the decision does not promise. If you genuinely
cannot name a caveat, write "None identified" and explain why.

- Caveat 1: ...
- Caveat 2: ...

## HISTORY

State transitions for this ADR. One line per transition; nothing else.
This section is the per-ADR analogue of the project-level `changelog.md`
and is the falsifiable record of when the ADR's status changed.

- YYYY-MM-DD: Proposed
- YYYY-MM-DD: Accepted
- YYYY-MM-DD: Superseded by ADR-MMMM (if applicable)
- YYYY-MM-DD: Deprecated (if applicable)

## References

- Code: `path/to/relevant/file.c`
- Related ADRs: ADR-MMMM
- External: [link to paper, blog post, prior art]
