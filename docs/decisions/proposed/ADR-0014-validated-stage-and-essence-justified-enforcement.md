<!--
freshness: { owner: "oldying", reviewed: "2026-08-28" }
-->

# ADR-0014: Add a Validated lifecycle stage between Proposed and Accepted (TC39 Stage 2.7 analog + essence-justified enforcement)

## Status of This Memo

This ADR records a decision of the Planex project. The initial status is
**Proposed**. Upon acceptance, the status becomes **Accepted** and the
file moves from `decisions/proposed/` to `decisions/accepted/`. The
current authoritative status is in the `## HISTORY` log and the lifecycle
subdirectory under which the file is filed.

Date: 2026-08-28 (date of the decision, not the draft)

## When to use this decision

This decision applies to any ADR PR that proposes a new abstraction
admitted under the essence-justified criterion (ADR-0011), or any ADR
PR that proposes a new CI gate or enforcement mechanism whose
*falsifiable claim* ("this lint will catch X") is verifiable before
acceptance. It does not apply to ADRs that record purely historical
decisions (e.g., ADR-0006's deferral of continuous-interaction
abstraction), nor to ADRs that name conventions without enforcement
claims.

## When NOT to use this decision

Do not apply this decision to ADRs that document engineering tradeoffs
without enforcement claims (e.g., "we chose C over Rust" — ADR-0004 —
has no enforcement mechanism to validate), nor to ADRs that record
post-hoc observations (e.g., ADR-0012's pressure-test findings — the
findings are observations, not mechanisms that need pre-acceptance
validation). This ADR carves out a specific gap: the gap between
"decision is documented" and "decision's enforcement mechanism is
implemented and proven to fire on its target."

## Status

Proposed

## Context

ADR-0011 Known issues names a specific gap: the essence-justified
criterion (tradition-cite + orthogonality-test + denotational-semantics)
is documented but not CI-enforced. A contributor can in principle add a
new abstraction that fails any of the three criteria, and the test suite
will not flag it. ADR-0011's Q3 self-acknowledged gap states:

> The criterion is a paper gate, not a code gate.

ADR-0011 names `scripts/check_essence_admission.sh` as the future
enforcement tool, deferred to "a future ADR-0014." This is that ADR.

Independently, `doc-organization.md` Part VIII deferral list names
"TC39 Stage 2.7 named ADR state" as a deferred proposal:

> ADR-0012 Q3 already self-acknowledges the 'criteria documented but
> not enforced' gap; making it a named lifecycle stage would mean an
> ADR cannot reach Accepted until its enforcement mechanism (e.g., a
> CI lint that flags new abstractions whose constructor accepts a
> parameter no operation reads) is implemented. This is a real
> proposal but out of scope for this doc — it would amend the ADR
> template, which is Wave 2's territory. Defer to a future ADR-0014
> if the gap proves painful.

Both deferrals point at the same gap from different angles: the gap
between a decision's *claim* and the decision's *enforcement*. Closing
the gap requires (a) a CI lint that catches essence-justified
violations, and (b) a named lifecycle stage that says "the decision is
documented, the lint exists, but we have not yet verified that the lint
fires correctly on a real violation."

The TC39 Stage 2.7 design (per
<https://tc39.es/process-document/>) is the model: a proposal at Stage
2.7 has a draft specification and an in-progress reference
implementation; it cannot reach Stage 3 (Accepted-equivalent) until
implementations have validated that the spec is implementable and the
key tests pass. The Planex analog: an ADR at "Validated" has a draft
decision and an in-progress enforcement mechanism; it cannot reach
"Accepted" until the enforcement mechanism is implemented and proven to
fire on at least one synthetic violation case.

## Decision

**Add a `Validated` lifecycle stage between `Proposed` and `Accepted`
for any ADR that claims an enforcement mechanism. The stage means: the
ADR's decision is documented, the enforcement mechanism exists in the
repo, and the mechanism has been proven to fire on at least one
synthetic violation case. An ADR cannot reach `Accepted` until it is
first `Validated`.**

This decision has three parts:

### Part 1 — New lifecycle stage

The ADR lifecycle state machine becomes:

```
Proposed ──> Validated ──> Accepted ──> Deprecated / Superseded
              │
              └─> (revert to Proposed if validation fails)
```

- `Proposed`: the ADR is documented but its enforcement mechanism (if
  any) is not yet implemented. Existing ADRs without enforcement claims
  skip the `Validated` stage.
- `Validated`: the ADR is documented, its enforcement mechanism is
  implemented in the repo (`scripts/check_*.sh`, `make check-*` target,
  or equivalent), and the mechanism has been demonstrated to fire on at
  least one synthetic violation case included in the ADR's `##
  Validation` section (a new mandatory section for `Validated`-track
  ADRs).
- `Accepted`: the ADR's decision is canonical; the enforcement mechanism
  is CI-wired (in `.github/workflows/*.yml`); the synthetic violation
  case is preserved in the ADR's history as the falsifiability record.

The directory structure becomes `decisions/{proposed,validated,accepted,deferred,deprecated,superseded}/`.
The `validated/` directory is new; the `## Status` field gains a
`Validated` value; `check_adr_lifecycle.sh` is updated to recognize
the new state.

### Part 2 — New ADR section: `## Validation`

For any ADR that proposes an enforcement mechanism (a lint, a test, a
metric), the `## Validation` section is mandatory and contains:

- A concrete **synthetic violation case** — a small code or doc snippet
  that *should* trigger the enforcement mechanism. The snippet is
  inlined as a code block in the ADR.
- The expected output of the enforcement mechanism on the synthetic
  case (e.g., "`check_essence_admission.sh` exits 1 with the message
  '...tradition citation missing for abstraction Memory'").
- A pointer to where the synthetic case is encoded as a CI test (so
  the validation is reproducible, not just documented in prose).

If the ADR does not propose an enforcement mechanism, the `## Validation`
section is omitted (just as `## Essence Check` is omitted for purely
engineering ADRs).

### Part 3 — `scripts/check_essence_admission.sh` (the first concrete application)

This ADR's own `## Validation` section (below) demonstrates the
mechanism on a synthetic violation: an ADR that proposes a new
abstraction `Memory` with no tradition citation. The script
`scripts/check_essence_admission.sh` is the enforcement mechanism,
implemented in the same commit as this ADR's promotion to `Validated`.

The script checks, for each ADR proposing a new abstraction:

1. The ADR's `## Essence Check` section (or equivalent) cites at least
   one external tradition source (paper, book, or named tradition from
   the 6-tradition list in `why-four-abstractions.md`).
2. The ADR's `## Alternatives Considered` section is non-empty and
   contains at least one real alternative (not a strawman).
3. The ADR's `## Consequences` section names at least one negative
   consequence (per `TEMPLATE-GUIDE.md`'s "Zero-negative-consequences"
   failure mode).

If any check fails, the script exits 1 with a message naming the
missing criterion.

This is **not a complete enforcement of ADR-0011's three-criterion**
(tradition-cite + orthogonality-test + denotational-semantics). The
orthogonality test is already enforced by `tests/test_orthogonality.c`
(shipping v0.4) and `tests/test_v4_orthogonality.c` (v4 proposals).
The denotational-semantics test is harder to automate (it requires
verifying that the abstraction's product is a typed value) and remains
a reviewer-applied check per `REVIEW-RUBRIC.md`'s criterion 2
(Honesty). This ADR's script closes only the tradition-cite +
negative-consequence halves automatically.

## Essence Check

> This decision touches the semantic interface axis: it defines the
> criterion by which ADRs that propose new abstractions are admitted
> to or rejected from the semantic interface.

### Q1. Which essence axis does this decision affect?

- [x] **Semantic interface** — the criterion by which ADRs proposing
  new abstractions are validated before acceptance.
- [ ] Intent space (the user → machine direction)
- [ ] State space (machine-side state)
- [ ] None — engineering decision

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions (cognitive bandwidth reduced):**

- Contributors can no longer push an ADR with a missing tradition
  citation through to Accepted; the `Validated` stage gates it. This
  eliminates the recurring review-loop cost of catching missing
  citations at human review time.
- Reviewers can rely on `check_essence_admission.sh` for the
  tradition-cite + negative-consequence halves of ADR-0011's three
  criteria, freeing reviewer attention for the
  denotational-semantics half (which is harder to automate).
- The synthetic violation case in the `## Validation` section makes
  the enforcement mechanism's behavior reproducible — a future
  contributor can run the ADR's validation case and see it fire,
  without having to read the enforcement script.

**Increases (cognitive bandwidth added):**

- Contributors proposing new abstractions now have an additional
  lifecycle stage to navigate (`Proposed` → `Validated` → `Accepted`,
  not just `Proposed` → `Accepted`). The `## Validation` section is
  additional writing work.
- The synthetic violation case is real writing effort: the
  contributor must construct a falsifying example, which requires
  thinking about what would count as the ADR being wrong. This is the
  cost of falsifiability (per `abstraction-form.md` Prerequisite 3):
  the cost is real, and the cost is the point.

**Net assessment:** Compresses. The one-time cost of writing the
`## Validation` section is less than the recurring cost of catching
essence-justified violations at human review time.

### Q3. Is there a gap between the claim and the implementation?

- **Claim (this ADR):** an ADR at `Validated` stage has its enforcement
  mechanism implemented and proven to fire on a synthetic violation.
- **Implementation (in this commit):** `scripts/check_essence_admission.sh`
  implements the tradition-cite + negative-consequence halves. The
  synthetic violation case (a fake `Memory` abstraction with no
  tradition citation) is encoded in `tests/test_essence_admission.c`
  (or `.sh`, depending on implementation language).
- **Gap:** the denotational-semantics half of ADR-0011's three-criterion
  is NOT enforced by this ADR's script. It remains a reviewer-applied
  check per `REVIEW-RUBRIC.md` criterion 2 (Honesty). This is the
  same gap ADR-0011 Q3 self-acknowledges; this ADR closes two of
  three sub-criteria, not all three.

### Q4. What is the cost, and who can verify it?

- **Cost:** the `Validated` stage adds friction to ADR acceptance. A
  contributor who wants to land a new abstraction must now also write
  a synthetic violation case and demonstrate that the lint fires on
  it. This is real up-front work. The cost is most painful for
  time-pressured changes (e.g., a v1.0 feature rush) where the
  contributor wants to ship fast.
- **Who can verify:** any reviewer can challenge a `Validated` claim
  by running `check_essence_admission.sh` on the ADR's synthetic
  violation case and verifying the lint fires. If the lint does not
  fire (i.e., the synthetic case does not actually trigger the
  enforcement), the ADR cannot progress to `Accepted`.
- **Verification scenario:** a contributor proposes `ADR-0015:
  Memory abstraction`. The ADR's `## Validation` section claims
  `check_essence_admission.sh` will exit 1 on a synthetic `Memory`
  ADR with no tradition citation. The reviewer runs the script on
  the synthetic case; if the script exits 0 (no failure), the claim
  is false, the ADR stays at `Validated` (or reverts to `Proposed`),
  and the contributor must fix either the lint or the synthetic case
  before re-promotion.

### Q5. What are the counterexamples?

This ADR's scope is bounded by the three parts above.

- **Counterexample 1: purely engineering ADRs.** ADR-0004 (use C not
  Rust/Zig/C++) has no enforcement mechanism; it is a historical
  decision. Such ADRs skip the `Validated` stage entirely; they go
  `Proposed` → `Accepted` directly. Applying this ADR to them would
  be a category error.
- **Counterexample 2: ADRs that record observations.** ADR-0012's
  four findings from the v4 orthogonality pressure test are
  observations, not enforcement mechanisms. The ADR is canonical
  Accepted because the pressure test exists; the ADR itself does not
  propose a new lint. The `Validated` stage does not apply.
- **Counterexample 3: ADRs that name conventions without enforcement
  claims.** ADR-0006 (continuous interaction deferred) names a
  deferral decision; it has no enforcement mechanism to validate.
  The `Validated` stage does not apply.
- **Counterexample 4: this ADR itself.** This ADR proposes an
  enforcement mechanism (`check_essence_admission.sh`), so it is
  itself subject to the `Validated` stage. The synthetic violation
  case in `## Validation` below demonstrates the lint firing on a
  fake `Memory` ADR. This ADR cannot reach `Accepted` until the
  lint is implemented and the synthetic case is reproduced.

**Scope statement:** This decision applies to any ADR that proposes
an enforcement mechanism (a lint, a test, a metric, a CI gate). It
does NOT apply to purely engineering ADRs, observation ADRs, or
convention-naming ADRs.

## Consequences

### Positive

- Closes the gap named in ADR-0011 Known issues and doc-organization.md
  Part VIII deferral list. The "essence-justified criterion documented
  but not enforced" gap is reduced to "essence-justified criterion
  partially enforced (2 of 3 sub-criteria automated, 1 remains
  reviewer-applied)."
- The `Validated` stage is a TC39 Stage 2.7 analog: it forces a
  pre-Acceptance demonstration that the enforcement mechanism works.
  This closes a class of "the lint exists but never fires" bugs that
  are otherwise undetectable.
- The synthetic violation case is a falsifiable record: a future
  contributor can run the ADR's validation case and verify the lint
  fires. The ADR becomes an executable contract, not just prose.

### Negative

- Adds friction to ADR acceptance. Contributors proposing new
  abstractions must now also write a synthetic violation case and
  demonstrate the lint fires on it. This is real work and may
  discourage casual contribution.
- The `validated/` directory is new; existing 13 ADRs (which are all
  `Proposed`, `Accepted`, or `Superseded`) are not affected
  retroactively. But future ADRs that propose new abstractions must
  traverse the new stage.
- The denotational-semantics half of ADR-0011's three-criterion is
  NOT closed by this ADR. The criterion-3 (does the abstraction's
  product form a serializable value?) remains a reviewer-applied
  check. A future ADR may close this with a more sophisticated
  lint; this ADR does not.

### Neutral

- The `## Validation` section is added to the ADR template as an
  optional section (mandatory only for `Validated`-track ADRs).
- `check_adr_lifecycle.sh` is updated to recognize the new state; no
  existing ADR moves as a result of this ADR's acceptance.

## Alternatives Considered

### Alternative 1: Do not add a `Validated` stage; add only `check_essence_admission.sh` as a CI lint

- **What:** Implement the enforcement script and wire it into CI, but
  do not change the lifecycle state machine. An ADR with an
  enforcement mechanism goes `Proposed` → `Accepted` directly; the
  CI lint runs on every PR.
- **Why rejected:** This alternative closes the *enforcement* gap but
  not the *demonstration* gap. A lint that exists in CI but has never
  been proven to fire on a synthetic violation is not a falsifiable
  mechanism — it is a hope. The `Validated` stage forces the
  contributor to demonstrate the lint fires on at least one
  synthetic case before the ADR is Accepted. Without the stage, the
  lint can be added but its falsifiability is unverifiable.

### Alternative 2: Use the existing `## Known issues` section to record the synthetic violation case, no new stage

- **What:** Extend the ADR template's `## Known issues` section to
  include a synthetic violation case for any ADR that proposes an
  enforcement mechanism. No new lifecycle stage.
- **Why rejected:** The `## Known issues` section is for accepted
  leaks, not pre-acceptance validation. Conflating the two would
  degrade the semantic distinction between "this ADR has known
  limitations we accept" (Known issues) and "this ADR's enforcement
  has been demonstrated to work" (Validation). The two are
  structurally different: Known issues go in Accepted ADRs;
  Validation goes in Validated-track ADRs before they reach Accepted.

### Alternative 3: Make `Validated` mandatory for ALL ADRs, not just those with enforcement mechanisms

- **What:** Every ADR must pass through `Validated` before
  `Accepted`. For ADRs without enforcement mechanisms, the
  `## Validation` section is replaced with a `## No enforcement
  mechanism` statement.
- **Why rejected:** This would force every purely-engineering ADR
  (ADR-0004, ADR-0006, etc.) to write a `## No enforcement mechanism`
  section, which is pure ceremony. The TC39 Stage 2.7 model applies
  only to proposals that have a specification to validate; proposals
  without a spec skip the validation stage. The Planex analog: ADRs
  without an enforcement mechanism skip the `Validated` stage.

## CAVEATS

This ADR closes only the **tradition-cite + negative-consequence** halves
of ADR-0011's three-criterion. The **denotational-semantics** half is
NOT closed; it remains a reviewer-applied check per REVIEW-RUBRIC.md.

This ADR's `Validated` stage is **not retroactive**: existing 13 ADRs
do not need to be re-validated. The stage applies only to ADRs proposed
after this ADR's acceptance.

This ADR does NOT address the broader TC39 lifecycle (Stage 0 → Stage
4). The `Validated` stage is a single-step addition between Proposed
and Accepted; it does not introduce Stage 0 (Strawman) or Stage 4
(Finished). Those would require deeper template amendments and are out
of scope here.

## Validation

This ADR is itself subject to the `Validated` stage it defines. The
enforcement mechanism is `scripts/check_essence_admission.sh`. The
synthetic violation case is below.

### Synthetic violation case

Consider a hypothetical `ADR-0015: Memory abstraction` with the
following (intentionally broken) structure:

```
# ADR-0015: Memory is a 6th abstraction

## Status
Proposed

## Context
We need a Memory abstraction because working memory is fundamental.

## Decision
Add `px_memory_new() / px_memory_recall() / px_memory_forget()` as
the 6th abstraction alongside Estimate/Perception/Closure/Relation/px_loop.

## Essence Check
### Q1. Which essence axis does this decision affect?
- [x] Semantic interface
### Q2-Q5: (left blank — "no impact")
```

This ADR is missing:

1. A tradition citation (no Friston, no Elliott, no Peirce, no
   Heidegger — just "we need it because working memory is
   fundamental").
2. A non-empty `## Alternatives Considered` section.
3. A non-empty `## Consequences` → Negative sub-section.

### Expected enforcement behavior

`scripts/check_essence_admission.sh tests/synthetic_adr_0015.md` should
exit 1 with a message like:

```
check_essence_admission: ADR-0015 (Memory abstraction) fails criterion 1
(tradition citation): no tradition source cited in Essence Check or
Context. ADR-0011 requires a specific paper or book citation, not a
general vibe.
```

### CI encoding

The synthetic case is encoded in `tests/synthetic_adr_0015.md` (a
literal copy of the snippet above) and `check_essence_admission.sh` is
wired into `.github/workflows/docs.yml` as the 10th gate, running on
every PR that touches `decisions/proposed/`. The synthetic ADR is
excluded from the ADR index (`gen_adr_index.sh` skips files in
`tests/synthetic_*`), so the synthetic does not pollute the real ADR
registry.

This ADR cannot progress from `Validated` to `Accepted` until the
above enforcement is demonstrated to fire on the synthetic case. The
demonstration is in the commit that promotes this ADR.

## Known issues

- **Issue**: The `Validated` stage adds friction to ADR acceptance.
  Contributors proposing time-pressured abstractions may bypass the
  stage by writing a weak synthetic case (e.g., a trivial violation
  that the lint catches trivially) and proceeding to Accepted without
  real validation.
- **Why accepted**: The cost is bounded by reviewer vigilance (per
  REVIEW-RUBRIC.md criterion 2 — Honesty). A reviewer who reads the
  synthetic case can challenge it as too trivial; the contributor
  must then write a stronger case. The friction is the point; the
  cost is the falsifiability tax.
- **Tracking**: deferred to v1.0+ when external contribution pressure
  is real and the friction may need re-tuning.
- **Mitigation**: the REVIEW-RUBRIC.md criterion 2 (Honesty)
  explicitly asks "is the synthetic violation case non-trivial?";
  a reviewer applying the rubric will catch a strawman synthetic.

- **Issue**: The denotational-semantics half of ADR-0011's
  three-criterion is not closed by this ADR. The script
  `check_essence_admission.sh` enforces only the tradition-cite +
  negative-consequence halves.
- **Why accepted**: Automating denotational-semantics verification
  (does the abstraction's product form a serializable value?)
  requires either a type-system extension or a runtime
  serialization test, both of which are non-trivial and out of
  scope for this ADR.
- **Tracking**: deferred to a future ADR-0016+ if the gap proves
  painful.
- **Mitigation**: REVIEW-RUBRIC.md criterion 3 (Compressiveness)
  asks "is the abstraction's output a typed value?"; a reviewer
  applying the rubric catches a non-value abstraction.

## HISTORY

- 2026-08-28: Proposed

## References

- **Code:** `scripts/check_essence_admission.sh` (to be implemented in
  the commit that promotes this ADR to Validated)
- **Code:** `tests/synthetic_adr_0015.md` (synthetic violation case)
- **Related ADRs:** [ADR-0010](../accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) — the honesty downgrade this ADR
  operationalizes at the lifecycle level; [ADR-0011](../accepted/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — the three-criterion this ADR partially automates (Known issues names the gap); [ADR-0012](../accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) — Q3 self-acknowledged gap this ADR's `Validated` stage would have caught at ADR-0011 acceptance time.
- **Related docs:** [`../../doc-organization.md`](../../doc-organization.md) Part VIII — names "TC39 Stage 2.7 named ADR state" as the deferred proposal this ADR addresses; [`../TEMPLATE.md`](../TEMPLATE.md) — gains the optional `## Validation` section per this ADR's Part 2; [`../TEMPLATE-GUIDE.md`](../TEMPLATE-GUIDE.md) — gains guidance for the `## Validation` section; [`../REVIEW-RUBRIC.md`](../REVIEW-RUBRIC.md) — criterion 2 (Honesty) and criterion 3 (Compressiveness) are the reviewer-applied complements to the automated halves of this ADR's enforcement.
- **External:** TC39 process document, Stage 2.7 — <https://tc39.es/process-document/>
- **External:** Rust RFC FCP (final comment period) — <https://github.com/rust-lang/rfcs#the-rfc-process> (related but different; FCP is a comment period, not a validation gate)
- **External:** PEP 1, PEP lifecycles (Draft / Accepted / Final / Rejected / Withdrawn / Deferred / Active / Superseded) — <https://peps.python.org/pep-0001/>
