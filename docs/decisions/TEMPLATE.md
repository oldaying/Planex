<!--
Document freshness — every ADR carries this metadata block (research basis:
Software Engineering at Google Ch. 10 "Documentation", the freshness:{owner,
reviewed} pattern). CI (planned: see doc-organization.md Part IX, Principle 13)
flags ADRs whose reviewed: date is older than 12 months for re-review.
-->
<!--
freshness: { owner: "github-handle", reviewed: "YYYY-MM-DD" }
-->

# ADR-NNNN: [Decision title — one short sentence]

> Copy this template to `decisions/<state>/ADR-NNNN-short-kebab-title.md`
> where `<state>` is `proposed` until accepted. Read [`TEMPLATE-GUIDE.md`](TEMPLATE-GUIDE.md)
> for how to fill each section well. Submit the PR; reviewer applies
> [`REVIEW-RUBRIC.md`](REVIEW-RUBRIC.md).
>
> The "Essence Check" section (5 questions) is **mandatory** for any decision
> that touches the three core abstractions (Relation / Estimate / Closure /
> Perception) or their semantics. For purely engineering decisions (new
> backend, build system change, etc.), that section may be omitted, but the
> other sections below are still required.

## Status of This Memo

> One-paragraph boilerplate (research basis: RFC 7841 §3.2-3.5, the
> three-paragraph "Status of This Memo" boilerplate). This block is
> *immutable* once the ADR is Accepted; lifecycle changes after Acceptance
> (deprecation, supersession) are recorded in `## HISTORY` below, not by
> editing this block.

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: YYYY-MM-DD (date of the decision, not the draft)

## When to use this decision

> Research basis: GOV.UK Design System component-page template ("When to use
> this component"). Adapted for ADRs: this section names the *scope of
> applicability* — under what conditions this decision applies. Distinct from
> the *scope statement* in Essence Check Q5 (which is about counterexamples);
> this section is the *positive* applicability statement.

[1-3 sentences describing when this decision applies. Example: "This
decision applies to any code path that constructs an Estimate from a
Perception feed. It does not apply to Estimates constructed from explicit
caller-supplied values, which use the unicast path covered by ADR-0007."]

## When NOT to use this decision

> Research basis: GOV.UK Design System ("When not to use this component").
> This section is the *negative* applicability statement. Pairing "when to
> use" with "when NOT to use" makes the decision's scope falsifiable — a
> future reader can cite this section when proposing to extend or restrict
> the scope.

[1-3 sentences describing when this decision does NOT apply. Example: "Do
not apply this decision to Estimates constructed from Perception feeds
that have been pre-filtered by a domain-specific denoiser; those use the
filtered-feed path covered by ADR-0008."]

## Status

Proposed | Validated | Accepted | Deprecated | Superseded by ADR-MMMM

> The `Validated` state (added by ADR-0014) is an intermediate stage
> between `Proposed` and `Accepted` for ADRs that propose an enforcement
> mechanism. The ADR's `## Validation` section (see below) records the
> synthetic violation case and the actual enforcement output as the
> falsifiability record. An ADR cannot reach `Accepted` until it has
> first been `Validated` (if it proposes an enforcement mechanism) — i.e.,
> the mechanism is implemented and proven to fire on at least one
> synthetic case. ADRs that do NOT propose an enforcement mechanism skip
> `Validated` and go directly from `Proposed` to `Accepted`.

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

## Validation (mandatory for `Validated`-track ADRs; optional otherwise)

> Research basis: TC39 Stage 2.7 — the stage where a proposal's
> reference implementation and key tests must be in place before the
> proposal can advance to Stage 3 (Accepted-equivalent). Adapted for
> ADRs by ADR-0014: an ADR that proposes an enforcement mechanism
> (a CI lint, a test, a metric, a gate) cannot reach `Accepted` until
> the mechanism is implemented in the repo AND proven to fire on at
> least one synthetic violation case included in this section.
>
> ADRs that do NOT propose an enforcement mechanism (purely-engineering
> ADRs, observation ADRs, convention-naming ADRs) OMIT this section
> entirely. See ADR-0014 counterexamples 1-3 for the scope statement.

### Synthetic violation case

[A small code or doc snippet that *should* trigger the enforcement
mechanism. Inline as a fenced code block. Example: a fake ADR that
proposes a new abstraction with no tradition citation, demonstrating
that `scripts/check_essence_admission.sh` exits non-zero on it.]

```
[synthetic violation case here]
```

### Expected enforcement behavior

[What the enforcement mechanism should do when run on the synthetic
case. Example: "exits 1 with the message '...tradition citation
missing for abstraction Memory'".]

### Actual enforcement output (preserved on YYYY-MM-DD)

[When the ADR is promoted to `Validated`, run the enforcement mechanism
on the synthetic case and preserve the actual output verbatim here.
This is the falsifiability record: a future contributor can re-run
the enforcement and verify the output matches. If the output drifts,
the ADR's `Validated` claim is broken.]

### CI encoding

[Pointer to where the synthetic case is encoded as a CI test. Example:
"The synthetic case is encoded in `tests/synthetic_adr_0015.md` and
`scripts/check_essence_admission.sh` is wired into
`.github/workflows/docs.yml` as the 10th gate (the `essence-admission`
job, running both `--check` on real ADRs and `--synthetic` on the
synthetic case)."]

## Known issues

> Research basis: GOV.UK Design System component pages (`#### Known issues`
> subsection under each variant). Adapted for ADRs: this section records the
> *known falsifications* of this decision — concrete scenarios where the
> decision produces a known-bad outcome that we accept as the cost of the
> decision. Distinct from CAVEATS (non-promises) and from Consequences
> (expected downstream effects). Known issues are *accepted leaks* the
> decision explicitly tolerates.

For each known issue, document:

- **Issue**: 1-sentence description of the known-bad outcome
- **Why accepted**: 1-sentence justification (cost-of-decision, deferred
  fix, no better alternative at this time)
- **Tracking**: link to issue, future ADR, or "deferred" / "accepted as
  permanent cost"
- **Mitigation**: what the caller can do today to avoid the issue

If no known issues, write "None identified at acceptance time." — but
expect the reviewer to push back. A decision with zero known issues at
acceptance is suspect; either we haven't looked hard enough, or the
decision is so trivial it shouldn't be an ADR.

## HISTORY

State transitions for this ADR. One line per transition; nothing else.
This section is the per-ADR analogue of the project-level `changelog.md`
and is the falsifiable record of when the ADR's status changed.

- YYYY-MM-DD: Proposed
- YYYY-MM-DD: Validated (if this ADR proposes an enforcement mechanism; per ADR-0014, the synthetic violation case in `## Validation` is in the repo and the enforcement fires)
- YYYY-MM-DD: Accepted
- YYYY-MM-DD: Superseded by ADR-MMMM (if applicable)
- YYYY-MM-DD: Deprecated (if applicable)

## References

- Code: `path/to/relevant/file.c`
- Related ADRs: ADR-MMMM
- External: [link to paper, blog post, prior art]
