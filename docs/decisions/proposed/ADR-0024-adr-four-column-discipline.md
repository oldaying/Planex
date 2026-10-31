<!--
Document freshness — every ADR carries this metadata block (research basis:
Software Engineering at Google Ch. 10 "Documentation", the freshness:{owner,
reviewed} pattern). CI (planned: see doc-organization.md Part IX, Principle 13)
flags ADRs whose reviewed: date is older than 12 months for re-review.
-->
<!--
freshness: { owner: "oldaying", reviewed: "2026-10-31" }
-->

# ADR-0024: The four-column discipline — every ADR states its refutation criteria and regression clause

## Status of This Memo

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: 2026-10-31

## When to use this decision

This decision applies to every ADR numbered ADR-0024 and later, across
all lifecycle states: a new ADR is not complete until its `## Refutation
Criteria` and `## Regression Clause` sections exist and are filled. It
also applies to `TEMPLATE.md` and `TEMPLATE-GUIDE.md`, which now carry
the two sections and their guidance.

## When NOT to use this decision

This decision does not apply retroactively to ADR-0001..ADR-0023: their
section contract is the one they were written under, and backfilling
their refutation criteria now would be post-hoc — exactly the failure
mode this discipline exists to prevent. It also does not govern
non-ADR documents (`docs/concepts/`, `docs/reference/`), which answer to
their own gates (freshness, limitations, term linking), and it does not
constrain code or API surface in any way.

## Status

Proposed

## Context

ADR-0014 made one class of ADR falsifiable: an ADR that proposes an
enforcement mechanism cannot be Accepted until the mechanism is
implemented and proven to fire on a synthetic violation case, with the
actual output preserved. That is the Validated state, and it works —
ADR-0014's own `check_essence_admission.sh --synthetic` demonstration
and ADR-0015's permanently retired number are the proof in the tree.

But the protection covers only the ADRs that ship a gate. The ordinary
run of decisions — language bindings, retirement cycles, promotion
ADR-0017/0018, the probe ADR-0023 — carries no written statement of
what would count against them. The result is visible in this
repository's own retire history: the leak-budget line was retired twice
([ADR-0013](../accepted/ADR-0013-v05-leak-budget-retire.md), then
[ADR-0019](../accepted/ADR-0019-v07-leak-budget-retire.md)), and both
retirements cite the same kind of evidence — budgets the codebase had
stopped obeying. That evidence was *assembled at retirement time*
because nobody had written down, at acceptance time, which observation
should trigger the retirement. The decision record was falsifiable in
principle and unfalsifiable in practice.

The project's 2026-10 research notes (the "four-column discipline",
compiled during the UI-thought archaeology program; not yet in-tree —
see References) state the general problem: a default that is never
given a testable statement can never be retired by evidence, only by
demolition. The notes name the lineage — the demarcation criterion of
Karl Popper, *Conjectures and Refutations* (1963), ch. 1: a statement
that no possible observation could count against is not an empirical
claim at all — and the repo has already internalized that lineage once:
ADR-0014's Validated state is Popper's criterion applied to gates. The
notes' fix is four columns for any axiom admitted to an ontology —
statement, defense, refutation condition, regression clause.
ADR-0014 already supplies the first two (Context defends, Decision
states) for the ADR layer. This ADR adds the missing two, restricted to
the layer the repo actually controls: its own decision records.

The timing is not idle. The paradigm-freeze program will ask this
repository to make ontology-level decisions (intent sources, time
addressability, divergence semantics) of a kind far more consequential
than a leak-budget retirement. Decisions of that weight made without
written expiry conditions are precisely how the research notes' subject
field ended up with a forty-year-frozen paradigm layer. The cheapest
moment to install the discipline is before the first such ADR is
written, not after.

## Decision

1. `TEMPLATE.md` gains two mandatory sections for ADR-0024 and later:
   `## Refutation Criteria` (the decision's expiry condition — what
   observable evidence would count against it, where it shows up, who
   notices) and `## Regression Clause` (the contract a successor
   inherits if this ADR is superseded — what must not silently return).
   `TEMPLATE-GUIDE.md` carries the how-to-fill guidance for both.
2. `scripts/check_doc_sections.sh` gains an opt-in `--four-column`
   mode. In that mode, ADRs numbered ≥ 0024 must additionally contain
   both sections, or the script exits 1. The default mode (what CI runs
   today) is behavior-identical to before — the flag exists so the
   mechanism can be demonstrated and exercised before it is wired.
3. Wiring is the promotion step: when this ADR is promoted toward
   Validated, the `docs.yml` invocation becomes
   `scripts/check_doc_sections.sh --check --four-column`, per
   [ADR-0014](../validated/ADR-0014-validated-stage-and-essence-justified-enforcement.md)'s
   rule that the enforcement mechanism must be implemented and proven
   on a synthetic case before the ADR can be Accepted. The synthetic
   case and the preserved output are in `## Validation` below.
4. ADR-0001..0023 are exempt. The exemption is not a courtesy: their
   criteria would be predictions about decisions whose outcomes are
   already known, which is theater, not falsifiability.

This ADR itself carries both sections, filled — the discipline's first
instance must pass its own gate or the gate is not worth wiring.

## Essence Check

### Q1. Which essence axis does this decision affect?

None — this is a purely engineering (documentation-process) decision.
No abstraction, API surface, or runtime semantics change. See
Alternatives Considered.

## Consequences

### Positive

- Every future decision becomes retireable by *cited evidence* rather
  than by re-architecture; the retire path ADR-0013/0019 walked by
  assembling evidence after the fact becomes a lookup of the criteria
  written before it.
- Review gains an attack surface: "is this refutation criterion
  observable and would anyone notice?" is a sharper review question
  than "does this ADR feel right?", and it is answerable by the
  REVIEW-RUBRIC without touching code.
- The ADR set becomes an experiment registry: each decision carries its
  own predicted failure signature, so drift between claim and tree has
  a per-decision tripwire instead of relying on periodic audits to find
  it wholesale.

### Negative

- Two more sections of authoring cost per ADR, on a template that is
  already the densest artifact in the repo.
- A real risk of boilerplate: criteria filled to satisfy the gate with
  no observable content ("this is wrong if it turns out to be bad").
  The gate checks presence, not meaning — meaning is review's job, and
  the rubric must grow a check for vacuity (see Known issues).
- Criteria can be *wrong* — an author may pin the decision's expiry to
  an observable that never moves even when the decision is bad, giving
  false confidence. The clause is a floor, not a guarantee.

### Neutral

- The archive becomes two-era: ADR-0001..0023 read as pre-discipline
  records, ADR-0024+ as disciplined ones. External readers will see a
  style break at exactly this number.

## Refutation Criteria

This decision is wrong if, over the next several ADR lifecycles, any of
the following observables shows up:

1. **Vacuity rate.** REVIEW-RUBRIC reviews systematically reject
   `## Refutation Criteria` sections as unobservable or tautological —
   i.e. the sections exist but authors cannot write meaningful ones
   at this granularity. Observable: the rubric's review comments and
   PR rework cycles on ADR PRs numbered ≥ 0024.
2. **Throughput cost.** ADR PR review latency grows materially and the
   delta is attributable to the two sections (not to general repo
   activity). Observable: PR timelines for ADR-numbered PRs before vs.
   after 0024.
3. **No retirement payoff.** An ADR whose written criterion *fires* is
   nonetheless not retired faster or more cleanly than the
   pre-0024 baseline (the entire point of the criterion is to make
   evidence-driven retirement cheap). Observable: `## HISTORY` logs —
   retirements citing their own fired criteria vs. demolitions.

The successor to this ADR must cite which of the three fired, with the
PR/review/HISTORY evidence, and must not overturn it on taste.

## Regression Clause

If this ADR is superseded or deprecated, the two-section contract
survives for every ADR it governs at that moment; a successor may only
exempt ADRs from the discipline by naming them individually and citing
the measured evidence (vacuity rate, throughput cost, or missing
payoff, per the Refutation Criteria above). What may not return is the
default it removes: an ADR shipping with no written expiry condition,
defended only by "the decision seemed right at the time". Any return of
that default must arrive as a *named* post-0024 decision with its own
refutation criteria — not by deleting the sections from TEMPLATE.md
and letting new ADRs grow up unfalsifiable again.

## Alternatives Considered

### Alternative 1: Status quo — ADR-0014's Validated track is enough

- What: keep falsifiability scoped to enforcement-mechanism ADRs, as
  today.
- Why rejected: ADR-0014 covers the ADRs that ship gates; the repo's
  actual decision stream (retirements, promotions, probes) ships almost
  no gates. Under the status quo the next paradigm-level ADR would
  enter the tree with the same unfalsifiable-by-practice contract that
  made the leak-budget retirements twice as expensive as they needed
  to be.

### Alternative 2: Retroactive rewrite of ADR-0001..0023

- What: backfill both sections into all 23 existing ADRs so the
  archive is uniform.
- Why rejected: those decisions' outcomes are already known; writing
  their "predictions" now produces statements that cannot fail, which
  is the theater version of falsifiability. It would also edit records
  the repo treats as append-only (`## HISTORY`), for zero retirement
  value — the old ADRs are already either vindicated or superseded.

### Alternative 3: Fold the two columns into existing instruments (Essence Check Q4/Q5)

- What: extend Q4 ("cost and who can verify it") to cover expiry, and
  Q5 (counterexamples) to cover regression.
- Why rejected: Q4 names the decision's *cost* at adoption; a refutation
  criterion names the *evidence that ends the decision later* —
  different tense, different reader, different failure mode when
  conflated. Q5 is scoped to "where this decision does not apply",
  not "what a successor inherits". Overloading the strongest existing
  instrument to save two headings would make both instruments weaker.

### Alternative 4: Enforce now, wire `--four-column` into CI in this very change

- What: ship the script mode and the workflow edit together, making the
  sections blocking for all future ADRs immediately.
- Why rejected: ADR-0014's own lifecycle rule — an enforcement
  mechanism must be implemented, demonstrated on a synthetic case, and
  *then* wired as the promotion step. Wiring first inverts the rule
  this repository already accepted, to install a discipline about
  following rules. The mode ships; the wiring waits for promotion.

## CAVEATS

- This decision does NOT make criteria meaningful — the gate checks
  structural presence. A vacuous criterion passes `--four-column`.
  Vacuity detection stays with the REVIEW-RUBRIC (which should grow a
  "would anyone notice?" check when this ADR is promoted).
- This decision does NOT extend the discipline to `docs/concepts/` or
  `docs/reference/` — those have their own freshness/limitations gates,
  and ontology-level discipline for them belongs to the paradigm
  program, not to a section linter.
- The grandfather threshold is a *number*, not a property: a
  hypothetical ADR backported with number < 0024 would escape the gate.
  Accepted as a known issue below; the alternative (content sniffing)
  is not implementable in shell.
- The two sections add authoring cost to every future ADR including
  trivial ones. The template already allows "Engineering decision, no
  essence impact" terseness for Essence Check; the same terseness is
  expected here, not essay-length philosophy for a build-system tweak.

## Validation

> Per ADR-0014: this ADR proposes an enforcement mechanism (the
> `--four-column` mode of `check_doc_sections.sh`), so it cannot be
> Accepted until the mechanism is implemented and proven to fire on a
> synthetic violation case, with the actual output preserved here.

### Synthetic violation case

A fictional ADR-0025 in `docs/decisions/proposed/` that contains all
nine currently-mandatory sections but neither new section. Recreated
from this block verbatim:

```markdown
# ADR-0025: Synthetic — missing the two four-column sections

## Status

Proposed

Date: 2026-10-31

## Context

Synthetic violation case for ADR-0024's Validation section: an ADR
numbered >= 0024 that satisfies every pre-0024 mandatory section and
omits both `## Refutation Criteria` and `## Regression Clause`.

## Decision

Ship without stating what would count against this decision or what a
successor inherits.

## Consequences

### Positive

- None (synthetic).

### Negative

- The decision carries no expiry condition (synthetic).

### Neutral

- None (synthetic).

## Alternatives Considered

### Alternative 1: none
- What: synthetic placeholder.
- Why rejected: synthetic placeholder.

## CAVEATS

Synthetic file; exists to be rejected by `--four-column`.

## Known issues

None identified at acceptance time.

## HISTORY

- 2026-10-31: Proposed

## References

- Synthetic case for ADR-0024.
```

### Expected enforcement behavior

`scripts/check_doc_sections.sh --four-column` exits 1, reporting each
synthetic ADR missing exactly `## Refutation Criteria` and `## Regression
Clause`, and reporting nothing for any ADR numbered ≤ 0023. Plain
`scripts/check_doc_sections.sh --check` — the mode CI runs today — exits
0 on the same tree, proving the default path is behavior-identical.

### Actual enforcement output (preserved on 2026-10-31)

First run of the prototype caught a defect in the prototype itself: bare
`[[ "$adr_num" -ge 24 ]]` reads zero-padded numbers as octal, so
`0009`/`0008`/`0018`/`0019` errored and silently escaped the check —
under that code a real ADR-0028 would never have been checked. Fixed by
forcing base 10 (`(( 10#$adr_num >= threshold ))`) before the runs below
were taken; the trap and its fix are recorded here because a validation
output that hides its own first failure is not a validation record.

```
$ cp <synthetic case above> docs/decisions/proposed/ADR-0025-synthetic-missing-columns.md
$ sed '1s/ADR-0025/ADR-0028/' docs/decisions/proposed/ADR-0025-synthetic-missing-columns.md \
    > docs/decisions/proposed/ADR-0028-synthetic-octal-trap.md   # zero-padded, contains an 8

$ scripts/check_doc_sections.sh --four-column; echo "exit=$?"
check_doc_sections: 4 missing mandatory section(s) across 25 ADR file(s):
  ADR-0025-synthetic-missing-columns.md :: MISSING :: ## Refutation Criteria
  ADR-0025-synthetic-missing-columns.md :: MISSING :: ## Regression Clause
  ADR-0028-synthetic-octal-trap.md :: MISSING :: ## Refutation Criteria
  ADR-0028-synthetic-octal-trap.md :: MISSING :: ## Regression Clause

Mandatory sections (per docs/decisions/TEMPLATE.md):
  - ## Status
  - ## Context
  - ## Decision
  - ## Consequences
  - ## Alternatives Considered
  - ## CAVEATS
  - ## Known issues
  - ## HISTORY
  - ## References
  (+ ## Refutation Criteria and ## Regression Clause for ADR-0024 and later — ADR-0024 four-column mode)

See: docs/doc-organization.md Part IX (Principle 11) for the rationale.
exit=1

$ scripts/check_doc_sections.sh --check; echo "exit=$?"    # the mode CI runs today
check_doc_sections: all 25 ADR file(s) have all 9 mandatory sections.
exit=0

$ rm docs/decisions/proposed/ADR-0025-synthetic-missing-columns.md docs/decisions/proposed/ADR-0028-synthetic-octal-trap.md
$ scripts/check_doc_sections.sh --four-column; echo "exit=$?"
check_doc_sections: all 23 ADR file(s) have all mandatory sections (four-column mode: ADR-0024 and later also checked for ## Refutation Criteria and ## Regression Clause).
exit=0
```

Semantics preserved above: the violation is reported by filename and
section with exit 1; the ADR-0028 line proves leading-zero numbers with
8/9 digits (the octal trap) are checked; the plain `--check` run on the
same dirty tree exits 0, so the default CI path is behavior-identical;
the restored tree passes four-column mode.

### CI encoding

The synthetic case lives inline above (recreatable verbatim); it is not
committed as `tests/synthetic_adr_0025.md` because unlike ADR-0015's
retired number, the 0025 slot should stay available for a real ADR —
the demonstration is replayable from this section alone. Upon
promotion toward Validated: change the `docs.yml` step
`check_doc_sections (ADR section completeness, 9 mandatory)` to invoke
`scripts/check_doc_sections.sh --check --four-column` and update the
step name; the synthetic replay then belongs in the promoting PR's
description, mirroring how ADR-0014's demonstration was preserved.

## Known issues

- **Number-threshold grandfathering**: the gate keys on `ADR-([0-9]+)`
  ≥ 0024, so a backported ADR numbered lower would escape. Accepted:
  numbering is sequential and repo-controlled; no such ADR exists or
  is planned.
- **Vacuity risk**: the gate verifies presence, not observability. A
  criterion like "wrong if users dislike it" passes. Mitigation is
  review-side (REVIEW-RUBRIC gains a vacuity check at promotion time);
  no shell-implementable mitigation exists.
- **Two-era archive**: readers comparing ADR-0023 with ADR-0024 will
  see different section contracts. Accepted as the honest record of
  when the discipline arrived.

## HISTORY

- 2026-10-31: Proposed

## References

- Code: `docs/decisions/TEMPLATE.md` (two new sections), `docs/decisions/TEMPLATE-GUIDE.md` (guidance), `scripts/check_doc_sections.sh` (`--four-column` mode), `.github/workflows/docs.yml` (wiring at promotion)
- Related ADRs: [ADR-0014](../validated/ADR-0014-validated-stage-and-essence-justified-enforcement.md) (the Validated state this ADR's promotion follows), [ADR-0023](ADR-0023-semantic-drift-probe.md) (predictions-registered-before-implementation precedent), [ADR-0013](../accepted/ADR-0013-v05-leak-budget-retire.md) / [ADR-0019](../accepted/ADR-0019-v07-leak-budget-retire.md) (the retire cycles whose evidence was assembled post-hoc)
- Docs: `docs/doc-organization.md` Part IX, Principle 11 (mandatory sections enforced by CI)
- External: Popper, K. *Conjectures and Refutations: The Growth of Scientific Knowledge*, Routledge & Kegan Paul, 1963, ch. 1 "Science: Conjectures and Refutations" (the demarcation criterion this section contract imports); project research notes 2026-10, "立宪：新公理表与退役清单" (the four-column discipline / meta-axiom; compiled during the UI-thought archaeology program, not yet in-tree — this ADR is its repo-side absorption).
