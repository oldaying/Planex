# ADR-TEMPLATE-GUIDE — How to Fill `TEMPLATE.md`

> **Status**: Reference. Date: 2026-08-28. Companion to [`TEMPLATE.md`](TEMPLATE.md).
>
> **Research basis**: The Good Docs Project's "paired template + template-guide"
> convention (<https://www.thegooddocsproject.dev/template>). Every Planex
> skeleton ships with a sibling file explaining *how to fill each section* —
> the template becomes a falsifiable contract, not a blank page.
>
> **Why this file exists**: `TEMPLATE.md` tells you *what* sections to write.
> This file tells you *how to write each section well* and *how to know when
> you've written it badly*. Without this pairing, the template's intent can
> drift with every author's interpretation, and CI cannot meaningfully grade
> the section beyond "is the heading present?"

---

## How to use this guide

1. Copy `TEMPLATE.md` to `decisions/<state>/ADR-NNNN-<short-kebab-title>.md`
   where `<state>` is `proposed` until accepted.
2. For each section in the template, read the matching entry below.
3. Write the section. Then re-read this entry and check your section against
   the "Self-check" question. If the answer is "no", rewrite.
4. Run `scripts/check_doc_sections.sh --report` to verify the mandatory
   structural sections are present.
5. Open the PR. Reviewer applies the rubric in [`REVIEW-RUBRIC.md`](REVIEW-RUBRIC.md).

---

## Section-by-section guidance

### Title (`# ADR-NNNN: [Decision title — one short sentence]`)

The title is the only part of the ADR most readers will see in indices,
changelogs, and grep results. Make it a declarative sentence ("Promote
Perception to fourth abstraction"), not a topic ("Perception"). Reviewer
self-check: *could someone scanning the ADR index reconstruct what was
decided from the title alone?*

### Status block

Single line: `Proposed | Accepted | Deprecated | Superseded by ADR-MMMM`.
Followed by `Date: YYYY-MM-DD`. The date is the **decision** date, not the
draft date; for "Proposed" the date is the date the PR was opened.

The lifecycle subdirectory already encodes the state machine, but the
in-file field is the falsifiable record — a `git mv accepted/ADR-NNNN
superseded/ADR-NNNN` requires updating this field in the same commit. CI
does not yet enforce this (planned: see `doc-organization.md` Part VI).

### Context

Three bullets, no more: the problem, the constraints, the forces. The
common failure mode is to write a 1000-word history lesson — that belongs
in the ADR's predecessor speculation doc, not in Context. The reviewer
self-check: *could someone six months from now understand why this was a
real question, not just what was answered?* If yes, Context is done. If
no, you've written a decision in search of a problem.

### Decision

One declarative sentence first. Then explanation. The common failure mode
is to start with "After extensive discussion..." or "We considered several
approaches..." — these belong in Alternatives Considered, not Decision.
Reviewer self-check: *can the first sentence be quoted verbatim into the
changelog entry?*

### Essence Check (mandatory for abstraction-affecting)

The five questions (`Q1` through `Q5`) are Planex's analog of RFC 7322's
"Security Considerations" — every abstraction-affecting decision must
answer them, including the case where the answer is "no impact". The
common failure mode is to write "no impact" and skip Q2-Q5. **Don't.** If
Q1 is "None", write the no-impact sentence and link to the engineering
justification — that is the falsifiable claim.

For Q3 ("gap between claim and implementation"), the strongest possible
state is "Claim = Implementation" written explicitly. Anything weaker
must name the concrete gap and name the planned retire. See
[`leak-budgets.md`](../concepts/canonical/leak-budgets.md) for the retire
mechanism.

### Consequences (Positive / Negative / Neutral)

Three sub-sections. "Neutral" is mandatory even if empty — its presence
signals "we looked for neutral effects and didn't find any", not "we
didn't think about it". Reviewer self-check: *did you name at least one
negative consequence?* An ADR with zero negatives is suspect.

### Alternatives Considered

At least two alternatives. For each: what it is, why rejected. The common
failure mode is to list a strawman ("we could do nothing") and reject it
with "doing nothing would not solve the problem". A real alternative is
one that a reasonable engineer could advocate for in a design review.

### CAVEATS (distinct from Alternatives Considered)

CAVEATS answers *what does this decision NOT promise?*. It is the
epistemic-honesty section, distinct from Alternatives (rejected paths) and
Consequences (expected downstream effects). The common failure mode is to
write caveats that are actually alternatives ("we could have done X") or
consequences ("this will make Y slower"). Those belong in their own
sections.

Every Accepted ADR should have at least one caveat. If you genuinely
cannot name one, write "None identified" and explain why. A decision with
zero caveats usually means the author has not thought hard enough about
what the decision does not promise — see `doc-organization.md` Part IX,
Principle 10 (OpenBSD mandatory sections) for the rationale.

### HISTORY

One line per state transition. YYYY-MM-DD: action. The common failure mode
is to write prose ("we discussed this over several weeks..."). HISTORY is
not a narrative — it's a falsifiable record of *when* the status changed.
If the narrative matters, write it in Context.

### References

Three categories: code path(s), related ADR(s), external prior art (papers,
blog posts, other projects' ADRs). The common failure mode is to omit
external prior art — an ADR with no external references is a decision
made in a vacuum, which is suspect for a research-grade library. If there
is genuinely no external prior art, write "None identified — original
work" explicitly.

---

## Common failure modes (reject the PR if you see these)

| Failure mode | What it looks like | Fix |
|---|---|---|
| **Title-as-topic** | "Perception" instead of "Promote Perception to fourth abstraction" | Rewrite as declarative sentence |
| **Context-as-history** | 1000+ words on the history of the project | Move to speculation doc; keep 3 bullets here |
| **Decision-as-narrative** | "After discussion, we chose..." | Open with the decision: "Promote Perception to fourth abstraction." |
| **Essence-Check-no-impact-skip** | Q1=none, Q2-Q5 omitted | Write the no-impact sentence; link to engineering justification |
| **Zero-negative-consequences** | Only positive + neutral | Name at least one real negative; "none identified" is suspect |
| **Strawman-alternatives** | Only strawman + chosen | List at least two real alternatives that a reasonable engineer could advocate for |
| **CAVEATS-as-alternatives** | "We could have done X instead" | Move to Alternatives; CAVEATS is for what the decision does NOT promise |
| **CAVEATS-as-consequences** | "This will make Y slower" | Move to Consequences; CAVEATS is for non-promises, not effects |
| **HISTORY-as-narrative** | "We discussed for several weeks..." | One line per transition; move narrative to Context |
| **No-external-references** | Only code paths + related ADRs | Add papers, blog posts, or other projects' ADRs |

---

## References

- The Good Docs Project — paired template + template-guide convention:
  <https://www.thegooddocsproject.dev/template>
- Mathlib docBlame linter (CI-enforced mandatory docstring sections):
  <https://leanprover-community.github.io/contribute/doc.html>
- RFC 7322 §4.8 (mandatory RFC sections, esp. "Security Considerations"):
  <https://www.rfc-editor.org/info/rfc7322>
- [`TEMPLATE.md`](TEMPLATE.md) — the skeleton this guide explains
- [`REVIEW-RUBRIC.md`](REVIEW-RUBRIC.md) — the PR review rubric for ADRs
- [`../doc-organization.md`](../doc-organization.md) Part IX — research basis
