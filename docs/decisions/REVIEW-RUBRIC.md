# ADR Review Rubric — "No 3s" with six named criteria

> **Status**: Reference. Date: 2026-08-28.
>
> **Applies to**: every ADR PR (proposed → accepted) and every
> abstraction-affecting PR (per `TEMPLATE.md` Essence Check).
>
> **Research basis**: Write the Docs conference CFP rubric
> (<https://www.writethedocs.org/organizer-guide/confs/cfp/>). The WTD
> review scale forbids a neutral "3" — "Three means you don't have an
> opinion. We don't believe you. No threes." Planex adopts the same rule:
> an ADR review cannot be "looks fine, ship it" without a committed
> verdict on each of the six criteria below.

---

## Purpose

The falsifiability posture of `abstraction-form.md` Prerequisite 3
requires that *every* decision be reviewable against a fixed rubric —
otherwise the gap between claim ("we are falsifiable") and implementation
("reviews are vibes-based") is itself an L2 leak. This rubric is the
falsifiable review contract.

The rubric is also a *de-clutttering* mechanism: a reviewer who is forced
to commit to a verdict per criterion cannot hide behind a generic "+1".
It surfaces the dimension along which the reviewer is uneasy, which makes
the ADR author's revision loop tighter.

---

## The six criteria

Each criterion is scored on a 1-5 scale. **Score 3 is forbidden** — if
you cannot commit to "this is good (4-5)" or "this needs work (1-2)", you
have not read the ADR carefully enough.

### 1. Relevance

Does this ADR address a real problem the project faces? Or is it a
solution in search of a problem?

| Score | Meaning |
|---|---|
| 5 | Solves a documented problem; the Context section names the concrete pain |
| 4 | Solves a real problem; Context is concrete but could be tighter |
| 2 | Marginally relevant; Context is hand-wavy or strawman |
| 1 | Not relevant; reject and move to changelog or issue |

### 2. Originality

Is this decision a real *choice*? Or is it a fact dressed up as a
decision? ("We considered two approaches..." when only one was viable is
a fact, not a decision — see `TEMPLATE-GUIDE.md` failure modes.)

| Score | Meaning |
|---|---|
| 5 | At least two real alternatives that a reasonable engineer could advocate for in a design review |
| 4 | Two alternatives; one is weak but defensible |
| 2 | One strawman + chosen |
| 1 | No alternatives; this is a fact, not a decision |

### 3. Soundness

Is the reasoning correct? Are the claims verifiable? Does the ADR name
a cost and a verification scenario (Q4 in Essence Check)?

| Score | Meaning |
|---|---|
| 5 | Claims are tight; costs are named with verification scenarios; Q3 gap is explicit |
| 4 | Sound; minor claims could be tighter |
| 2 | Some claims are vague; Q4 verification scenario is hand-wavy |
| 1 | Reasoning is broken; unverifiable claims; reject |

### 4. Quality of presentation

Is the ADR readable? Does the title communicate the decision? Does the
structure follow `TEMPLATE.md`?

| Score | Meaning |
|---|---|
| 5 | Title is a declarative sentence; sections are tight; no prose-bloat |
| 4 | Mostly clean; minor bloat in one section |
| 2 | Title is a topic not a decision; sections are bloated |
| 1 | Unreadable; reject with structural notes |

### 5. Importance

Does this decision matter? Is it worth an ADR, or should it be a
changelog entry?

| Score | Meaning |
|---|---|
| 5 | Affects the project's core abstractions or ABI; lasting impact |
| 4 | Affects a subsystem; lasting impact within that subsystem |
| 2 | Engineering decision; no essence impact; could be a changelog entry |
| 1 | Trivial; reject and move to changelog |

### 6. Experience

Has the author done the homework? Are external references cited? Does
the ADR show awareness of prior art?

| Score | Meaning |
|---|---|
| 5 | External prior art cited (papers, blog posts, other projects' ADRs); alternative-perspectives doc referenced where relevant |
| 4 | Some external references; one or two key citations |
| 2 | Only code paths + related ADRs; no external prior art |
| 1 | No references at all; "original work" claimed without justification |

---

## Reviewer workflow

1. Read the ADR end-to-end.
2. Read [`TEMPLATE-GUIDE.md`](TEMPLATE-GUIDE.md) and check the ADR against
   the "common failure modes" table.
3. Run `scripts/check_doc_sections.sh --report` to verify mandatory
   sections are present.
4. Score each of the six criteria on 1-5. **No 3s.**
5. Post the scores as a single comment on the PR (see template below).
6. If any score is ≤2, the review is "request changes"; the ADR author
   revises and re-requests review.
7. If all scores are ≥4, the review is "approve"; merge requires two
   approvals (maintainer + reviewer with relevant expertise).

### Review comment template

```
## ADR review — <ADR-NNNN>

| Criterion | Score | Notes |
|---|---|---|
| Relevance | N | (one-line justification) |
| Originality | N | (one-line justification) |
| Soundness | N | (one-line justification) |
| Quality of presentation | N | (one-line justification) |
| Importance | N | (one-line justification) |
| Experience | N | (one-line justification) |

Verdict: approve / request changes / reject

Top concern: (one sentence)
```

---

## Acceptance thresholds

| PR type | Minimum per criterion | Required approvals |
|---|---|---|
| Abstraction-affecting ADR (Essence Check triggered) | 4 | 2 (maintainer + reviewer) |
| Engineering ADR (Essence Check skipped) | 4 | 1 (maintainer) |
| Doc-only PR (no ADR) | (rubric not applied) | 1 (maintainer) |

A single "5" does not outweigh a "2" elsewhere — the minimum-per-criterion
rule applies. This prevents a reviewer from giving a "5" on Importance to
compensate for a "2" on Soundness.

---

## Why "no 3s"

A neutral score is unfalsifiable. If a reviewer says "looks fine", they
have made no claim that can be tested against the next ADR's outcome.
Forcing a verdict per criterion — even when the verdict is "this is fine"
(4) vs "this needs work" (2) — produces a falsifiable record that
*future* reviews can be compared against. Without this discipline, the
review process drifts into "+1 looks good to me", which is the
documentation equivalent of an L2 leak: the surface looks rigorous, the
semantics aren't.

---

## References

- Write the Docs CFP rubric (the "no 3s" rule + 6 criteria, adapted):
  <https://www.writethedocs.org/organizer-guide/confs/cfp/>
- RFC 7322 §4.8 (mandatory RFC sections, the editorial-style precedent):
  <https://www.rfc-editor.org/info/rfc7322>
- Ousterhout, *A Philosophy of Software Design* 2nd ed. Ch. 13 ("Comments
  should describe things that aren't obvious from the code") — the
  analog: a review that doesn't say what's wrong is itself an unfalsifiable
  artifact.
- [`TEMPLATE.md`](TEMPLATE.md) — the ADR skeleton this rubric grades
- [`TEMPLATE-GUIDE.md`](TEMPLATE-GUIDE.md) — how to write each section
- [`../doc-organization.md`](../doc-organization.md) Part IX Principle 12 — research basis
