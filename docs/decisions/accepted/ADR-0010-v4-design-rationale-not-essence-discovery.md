# ADR-0010: v4 essence derivation is design rationale, not essence discovery

## Status

**Accepted.** Date: 2026-08-27.

Formalizes the conclusion already reflected in
[essence-derivation-v4-clean.md](../../concepts/history/essence-derivation-v4-clean.md)
Part VII (the first-principles audit) and the rewritten Appendix
(honest framing). This ADR is the institutional record of that
conclusion so that future ADRs, README, manifesto, and external
communications use the downgraded framing consistently.

**Relationship to prior ADRs**:

- **ADR-0009** (Proposed) — recorded the v3 Path B essence
  coverage claim ("6 implemented + 2 partial + 2 deferred"). This
  ADR does not supersede ADR-0009's *implementation* decisions
  (the v3 API surface, the test suite, the examples); those stand.
  What changes is the *epistemic status* of the coverage claim:
  ADR-0009 framed it as essence coverage; ADR-0010 reframes it as
  abstraction coverage.
- **ADR-0008** (Accepted) — `px_loop` as 5th essence category.
  The `px_loop` implementation stands; the essence-category framing
  is downgraded to "abstraction with strong tradition support".
- **ADR-0007** (Accepted) — v2 essence derivation. Treated as
  research record, not metaphysical result. Same downgrade.
- **ADR-0005** (Accepted) — Perception as 4th abstraction. Same
  pattern: implementation stands, essence framing downgraded.

## Context

The v4 essence-derivation document
([essence-derivation-v4-clean.md](../../concepts/history/essence-derivation-v4-clean.md))
was written as a clean-room re-derivation from UI's essence,
following the v3 Path B re-derivation. After completing the v4
document, a methodological audit (Part VII of that document) was
performed against the actual methodology literature on
"first-principles derivation" — fetching 60 Wikipedia primary
articles covering 6 methodological traditions (Aristotelian,
Cartesian, Husserlian, Popperian, Quinean, Wittgensteinian plus
Kuhn / Lakatos / Brooks for software-specific demands).

**The audit found v4 meets 0 of 10 constitutive demands** of
first-principles derivation in the strong sense. The demands are
not nitpicks — they are the constitutive criteria of "derivation
from first principles" across 2500 years of methodology literature.
Meeting none of them means v4 is not, by any classical standard, a
first-principles derivation. The 10 demands, summarized from Part
VII.9 of the v4 doc, are:

1. **State first principles explicitly and own their constructedness**
   (Aristotle *Posterior Analytics* I.2–I.6; Quine *Epistemology
   Naturalized*).
2. **Demonstrate that the principles are prior to and independent of
   the conclusions** (Aristotle I.2; Descartes *Meditations* III).
3. **Provide a dependency-ordered demonstration chain where each step
   follows necessarily** (Aristotle I.2–I.3; Euclid *Elements*).
4. **Use methodological doubt to test each principle** (Descartes
   *Discourse on the Method* Part IV).
5. **Distinguish clear-and-distinct principles from derived results**
   (Descartes *Meditations* III–IV).
6. **Perform eidetic variation on concrete instances** (Husserl
   *Ideas I* §3; *Cartesian Meditations*).
7. **State falsifiers — observations that would refute the essence
   claim** (Popper *Logic of Scientific Discovery*).
8. **Address anti-essentialist counter-arguments** (Wittgenstein
   *Philosophical Investigations* §§66–71, family resemblance).
9. **Separate essential from accidental complexity in the problem
   domain** (Brooks *No Silver Bullet*).
10. **Acknowledge derivation direction** — if code preceded
    derivation, say "this is reverse-engineering" (Lakatos
    *Methodology of Scientific Research Programmes*; Quine
    *Word and Object* on web of belief).

v4 meets none of these. The "essence-derived" framing was stronger
than the methodology warrants. The v4 document itself now uses the
honest framing in its Appendix:

> "8 abstractions, each justified by N of M sampled traditions,
> where N≥3 is an arbitrary threshold and M is undersampled (9
> Western-Anglophone traditions out of a larger space). The set is
> sample-dependent, partly retroactive, not falsifiable, and
> contested by family-resemblance anti-essentialism. The code is a
> design proposal; the essence-derived framing should be read as
> design rationale, not metaphysical discovery."

**Why this needs an ADR**: Without an ADR, the conclusion lives
only inside the v4 doc. Future contributors reading ADR-0009
(still marked Proposed) or the README's tagline will encounter
the old "essence-derived" framing and may re-assert it. The
honest framing needs to be the project's institutional position,
enforceable in code review, referenced from the README, and
binding on future ADRs. An ADR is how Planex formalizes
essence-level decisions.

**Why now**: The audit is fresh. The v4 doc has been pushed to
`main`. If the next ADR (0011+) is written before this conclusion
is formalized, that ADR may inherit the over-claim. Freezing the
honest framing now is cheaper than retrofitting it across
multiple future ADRs.

## Forces

1. **Honesty vs ambition tension**: The "essence-derived" framing
   is rhetorically powerful — it positions Planex as a
   research-grade UI library that derives its abstractions from
   first principles, not from intuition. Downgrading to "design
   rationale" weakens the positioning. The force pulling toward
   honesty is that the audit shows the stronger claim is false; the
   force pulling toward ambition is that the weaker claim is harder
   to communicate to new users and adopters. Resolution: own the
   downgrade, communicate it clearly in the README's manifesto, and
   let the code speak for itself — the 8-abstraction surface
   remains implementable, orthogonal, and tested regardless of
   epistemic framing.

2. **Code preservation vs framing revision**: The v4 code (8
   abstractions, 9 test binaries, ~133 assertions, all green) is a
   real artifact. It cannot be deleted without losing working
   design work. But the framing attached to that code is
   misleading. The force here is to separate the *code's* value
   (implementability, orthogonality) from the *framing's* value
   (essence-discovery). The two have no logical connection —
   implementability is an engineering property; essence-discovery
   is an epistemological property. Resolution: code stands,
   framing downgrades, the two are explicitly decoupled in
   documentation.

3. **Future path: v5 vs accept-and-stop**: If Planex accepts that
   "essence derivation" is a design-rationale posture (not a
   metaphysical discovery), there are two sub-paths: (a) stop
   overclaiming and treat the 8 abstractions as the project's
   chosen design, full stop; or (b) commit to actually performing
   the v5 work (eidetic variation on 30+ concrete UIs, falsifiers,
   Wittgenstein engagement, essential/accidental separation, Lakatos
   risky prediction) before re-claiming essence-derivation. The
   force toward (a) is cost — v5 is months of work and no current
   implementation pressure exists for it. The force toward (b) is
   intellectual honesty — if "essence-derived" is the project's
   identity, eventually it must be earned. Resolution: this ADR
   takes path (a) — downgrade framing now — and explicitly leaves
   path (b) as a future option, with the work-items documented in
   Part VII.11 of the v4 doc as the gate.

4. **Backward compatibility**: This is a framing-only change. No
   API breaks. No code changes. No tests change. The force is
   toward ensuring the change is *only* framing — any code
   revision would require a separate ADR. Resolution: this ADR
   touches no `.c` / `.h` files; it touches only documentation and
   future-ADR language conventions.

5. **Tradition of ADR-0005 framing correction**: ADR-0005
   (Promote Perception to 4th abstraction) was later "framing
   corrected by ADR-0007" — the original essence claim was
   re-stated with caveats. This established the precedent that
   framing corrections get their own ADR. ADR-0010 follows that
   precedent at a larger scope: instead of correcting one ADR's
   framing, it corrects the framing of the entire v1→v4 essence
   derivation lineage.

## Decision

### D1. The v4 essence-derived framing is downgraded to design-rationale framing

The single declarative sentence: **Planex's v4 8-abstraction surface
is a design proposal arrived at by tradition-sampling and analogical
reasoning, retrofitted to existing code; it is not an essence
discovery and not a first-principles derivation in the strong
(epistemological) sense.**

Implications:

- The v4 document's Appendix (already revised) is the canonical
  framing and is binding on all derivative materials.
- The phrase "essence-derived" is replaced by "design-rationale"
  or "tradition-grounded" in:
  - README.md's tagline / manifesto section
  - `why-four-abstractions.md` (slated for rename to
    `why-eight-abstractions.md` per ADR-0009; the rename can
    proceed but the document's framing must use design-rationale
    language)
  - Future ADRs and changelog entries
- The 10 constitutive demands of first-principles derivation
  (listed in Context above and in Part VII.9 of the v4 doc) are
  the project's audit bar. Any future re-claim of "essence-derived"
  must demonstrate meeting all 10 demands in a v5 derivation
  document.

### D2. Future ADRs and docs use "8-abstraction design proposal" not "8 essence categories"

Language convention (binding on future PRs):

| Old (over-claim) | New (honest) |
|---|---|
| "8 essence categories" | "8 abstractions, each justified by N of M sampled traditions" |
| "essence-derived" | "tradition-grounded design rationale" |
| "v4 implements 8 of 9 essence" | "v4 implements 8 abstractions with 2 candidates unimplemented" |
| "deferred essence" | "essence-but-unimplemented" / "not-essence" / "undecided" — pick one, do not use "deferred" |
| "the essence derivation" | "the design proposal" or "the tradition-sampling derivation" |
| "first-principles derivation" (of v1-v4) | "tradition-sampling derivation with retroactive fitting" |

The word "deferred" is specifically banned for new writing because
it conflates three epistemic states (essence-but-unimplemented /
not-essence / undecided) per Part VIII.4 of the v4 doc. The
Appendix of the v4 doc already uses the three-state labeling
explicitly for Adaptation ("judged essence, not yet implemented")
and Medium-ness ("judged essence, but out of scope").

### D3. The 10 constitutive demands of first-principles derivation become the project's audit bar

The 10 demands listed in Context (sourced from Part VII.9 of the
v4 doc, drawn from Aristotle / Descartes / Husserl / Popper /
Quine / Wittgenstein / Lakatos / Brooks) are adopted as the
project's institutional bar for any future re-claim of
"first-principles derivation". They are listed in this ADR's
Context so that contributors do not have to read the v4 doc to
discover them.

A future ADR (call it ADR-00NN, "v5 essence re-derivation")
claiming first-principles status must:

1. Reference this ADR's 10 demands.
2. For each demand, either demonstrate compliance or explicitly
   accept non-compliance with rationale.
3. Be reviewed by at least one contributor other than the author.
4. Include eidetic variation on at least 30 concrete UIs (Husserl).
5. State at least one falsifier per essence claim (Popper).
6. Engage Wittgenstein's family-resemblance counter-argument
   explicitly — either rebut it or accept it and abandon essence.

### D4. ADR-0009's essence coverage claim is downgraded from "essence coverage" to "abstraction coverage"

ADR-0009's D4 table currently reads as essence-coverage tracking.
Re-reading:

| Essence category | Pre-ADR-0009 | Post-ADR-0009 |
|---|---|---|
| Object | implemented (Estimate) | implemented (Estimate) |
| ... | ... | ... |

This table is still accurate as *abstraction coverage* — the
abstractions exist, the APIs work, the tests pass. What changes
is the table's framing label: not "essence category coverage"
but "abstraction coverage with stated tradition support".

This downgrade does **not** invalidate ADR-0009's implementation
decisions (D1, D2, D3 — the v3 prototype API surface, the
backward-compatible wrappers, the examples and test suite). Those
are engineering decisions and stand. Only ADR-0009's D4 and D5
essence-claim language is downgraded.

### D5. The v5 path is explicitly left open but not committed

The v4 doc Part VII.11 lists 8 work items v5 would need to perform
to honestly claim first-principles derivation:

1. Pick a single concrete UI (e.g., a slider, or a specific chat
   application's send-button).
2. Perform Husserl's eidetic variation on it: list every feature,
   vary each, observe what survives.
3. Generalize across N concrete UIs (where N is large enough to be
   statistically meaningful, not just 3; consider 30+).
4. For each surviving invariant, state what observation would
   falsify its essence-status.
5. Address Wittgenstein: if no invariant survives across all UIs,
   accept family-resemblance and abandon the essence project.
6. Address Brooks: separate essential from accidental in the
   problem domain.
7. Address Lakatos: state the hard core and make a risky
   prediction (e.g., "any UI framework that lacks an Interpretant
   abstraction will exhibit bug class X").
8. Address the descriptive/normative gap explicitly.

This ADR does **not** commit Planex to performing v5. The decision
is: until v5 is performed and meets the 10 demands, all v1–v4
essence claims are design rationale, not metaphysical discovery.
If a future maintainer wishes to start v5, they should open
ADR-00NN that supersedes this ADR's "design rationale" framing
and references the v5 work.

## Verification

### Build

No code changes. No build verification needed. The v4 code
prototype continues to build and pass all tests per ADR-0009's
verification section.

### Documentation review

The following documents were checked for consistency with this
ADR's downgraded framing:

- `docs/concepts/history/essence-derivation-v4-clean.md` — Part VII
  (audit) and Appendix (honest framing) already reflect this
  ADR's conclusion. No change required.
- `docs/concepts/history/essence-derivation-v3.md` — predates this ADR;
  v3 doc's framing stands as historical record. Future v3
  references in new docs must use the downgraded language.
- `docs/concepts/history/essence-derivation-v2.md` — same as v3.
- `docs/concepts/history/essence-derivation-v1.md` (v1) — marked SUPERSEDED
  by v2; no change.
- `docs/decisions/proposed/ADR-0009-essence-rederivation-v3.md` —
  implementation decisions stand; D4 and D5 essence-claim
  language is downgraded by this ADR (see D4 above).
- `docs/concepts/canonical/why-four-abstractions.md` — uses the older "4
  abstractions" framing; ADR-0009 deferred the rename to
  `why-six-abstractions.md` until ADR-0009 moves to Accepted.
  This ADR does not force the rename. The rename, when it
  happens, must use design-rationale framing.
- `README.md` (repo root) — must be updated separately to use
  design-rationale framing. This ADR is the binding reference for
  that update.
- `docs/concepts/state/limitations.md`, `docs/concepts/canonical/non-goals.md`,
  `docs/concepts/background/ui-essence-layers.md`, `docs/concepts/background/path-C-lineage.md`
  — to be updated to use design-rationale framing in a follow-up
  documentation PR; this ADR is the binding reference for those
  updates.

### Audit

The 60-source audit is recorded at
`/home/z/my-project/research/firstprinciples/body/*.txt` (not
committed to this repo — single-source Wikipedia, sufficient for
audit; deeper audit would consult primary texts). The audit
summary is in Part VII of the v4 doc. This ADR incorporates that
audit by reference.

## Essence Check

### Q1. Which essence axis does this decision affect?

This is a **meta-decision about essence framing**, not a direct
essence-axis decision. It does not affect Intent space, State
space, or Semantic interface directly — the abstractions
themselves (Estimate, Closure, Perception, Relation, Interpretant,
Perlocution, Breakdown, Loop) are unchanged in their semantics
and implementation.

What changes is the *epistemic label* attached to those
abstractions: from "essence-derived" (strong claim) to
"tradition-grounded design rationale" (weaker, accurate claim).

If forced to pick an axis: the closest is **Semantic interface**
— but at the meta level (how the project communicates its
abstractions' status to readers), not at the implementation
level (how abstractions encode/decode intent↔state). Even that is
a stretch; this is really a documentation/framing decision that
happens to be about essence.

### Q2. Does it compress or increase human cognitive bandwidth?

**Compressions (cognitive bandwidth reduced):**

- New readers no longer need to verify the "essence-derived"
  claim — the project tells them up front it's design rationale.
  This removes the (heavy) burden of auditing the derivation
  themselves.
- Contributors writing future ADRs no longer need to decide
  whether their decision touches "essence" — they describe which
  abstraction it touches and what tradition support exists, no
  metaphysical labeling required.
- Reviewers no longer need to argue whether a proposed
  abstraction is "essence" or "not essence" — the framing
  question becomes "does this abstraction have ≥3 tradition
  support, or is it utility?" which is answerable without
  metaphysical commitment.
- The 10 constitutive demands of first-principles derivation are
  now listed in one place (this ADR + v4 doc Part VII.9) — future
  contributors don't need to re-derive them from Aristotle /
  Descartes / Husserl / etc.

**Increases (cognitive bandwidth added):**

- Contributors must learn the new language convention (D2 above):
  "design rationale" not "essence-derived", "abstraction" not
  "essence category", three-state labeling not "deferred".
  Initial cost: re-reading the v4 doc's Part VII + this ADR's D2
  table. Amortized cost: small, the new language is more honest
  and less metaphorical.
- The 10 demands become a thing contributors must understand if
  they want to claim "first-principles derivation" in the future.
  This is a real cost — but only paid if/when v5 is undertaken.
  For contributors who don't pursue v5, the cost is zero.
- Future external communication (blog posts, conference talks,
  README tagline) must be careful with the word "essence". Some
  audience-facing material may need rephrasing. This is a
  marketing-cost, not an engineering-cost.

### Q3. Is there a gap between the claim and the implementation?

**Before this ADR:**

- Claim (in README, ADR-0007, ADR-0008, ADR-0009, v2/v3/v4
  derivation docs): "Planex derives its abstractions from UI's
  essence, sampled across N traditions; the implemented set
  matches the derived essence set."
- Implementation (in code): 8 abstractions that work, are
  orthogonal, pass tests, were written before the "essence"
  derivation was constructed to match them.
- Gap: large. The claim is stronger than the implementation
  warrants — the implementation is solid engineering, but the
  claim of essence-derivation is unsupported by the methodology
  literature.

**After this ADR:**

- Claim: "Planex's 8 abstractions are a design proposal arrived
  at by tradition-sampling and analogical reasoning, retrofitted
  to existing code; the design rationale is documented but the
  abstractions are not claimed as essence discoveries."
- Implementation: unchanged (8 abstractions, working code).
- Gap: closed. The claim now matches what was actually done.

This is a Conal Elliott "claim = implementation" improvement.
The implementation didn't change; the claim was downgraded to
match.

### Q4. What is the cost, and who can verify it?

- **Cost**: future contributors and external communicators must
  use the new language convention. Some existing documents
  (README, why-four-abstractions, limitations, non-goals) need
  framing updates in follow-up PRs — the work is mechanical
  (search-and-replace of "essence-derived" → "design rationale"
  + spot-check context) but non-trivial in scope (~5-10 documents).
- **Who can verify**: any contributor reading the future ADR
  pipeline and the README after the framing updates land. The
  verification scenario is: "Open the README. Does it claim
  Planex is essence-derived? If yes, this ADR was not applied.
  If no (uses design-rationale language), this ADR was applied."
- **Verification scenario for the 10 demands**: if a future ADR
  claims "first-principles derivation", the reviewer must check
  the ADR against the 10 demands listed in this ADR's Context.
  The 10 demands are the audit bar; they are reviewable by any
  contributor who reads this ADR + Part VII of the v4 doc.
- **Cost of the audit itself**: already paid (60 Wikipedia
  sources fetched, parsed, audited, written up in Part VII of v4
  doc). Not a recurring cost unless v5 is undertaken.

### Q5. What are the counterexamples?

- **Counterexample 1 (over-claim and hope nobody notices)**:
  Keep the "essence-derived" framing and assume readers won't
  audit. Rejected: this is dishonest, and the audit is now
  published in Part VII of the v4 doc — anyone who reads it
  will notice the over-claim.
- **Counterexample 2 (delete v4 and re-derive properly)**: Throw
  away the v4 code and the v1-v4 derivation lineage, start v5
  fresh with eidetic variation on 30+ UIs. Rejected: too
  expensive (months of work, no implementation pressure), and
  throws away working code that solves real conflations in v0.4
  (Closure mixed illocution+perlocution; Perception mixed
  representamen+interpretant). The 8-abstraction surface stands
  on its engineering merits even without essence-discovery
  framing.
- **Counterexample 3 (mark v4 as "experimental" without
  addressing the framing)**: Add an "experimental" sticker to
  the v4 doc without changing the essence framing. Rejected:
  doesn't address the metaphysical over-claim. "Experimental"
  implies the *code* is provisional; the issue is the *framing*
  is over-strong, regardless of code status.
- **Counterexample 4 (defer the decision until v5 is done)**:
  Keep the over-claim and downgrade it later when v5 completes.
  Rejected: each future ADR written before the downgrade inherits
  the over-claim. The cost compounds. Better to freeze the honest
  framing now and let v5 (if it happens) re-claim essence status
  by meeting the 10 demands.

## Scope

This ADR is a framing-only change. It does NOT:

- Change any code (no `.c` / `.h` modifications).
- Change any test (no test files modified).
- Break any API (existing callers unaffected).
- Force the rename of `why-four-abstractions.md` to
  `why-eight-abstractions.md` — that's still deferred to a
  follow-up PR (per ADR-0009's scope).
- Commit Planex to performing v5 — v5 is left open as a future
  option, not a commitment.
- Update the README's tagline — that's a follow-up documentation
  PR that references this ADR.
- Update `limitations.md`, `non-goals.md`, `ui-essence-layers.md`,
  `path-C-lineage.md` — same, follow-up PRs.
- Retroactively rewrite ADR-0007, ADR-0008, ADR-0009 — ADRs are
  immutable once Accepted. The framing correction is recorded
  here; future readers consult this ADR to see the corrected
  framing.
- Modify the v1/v2/v3 essence-derivation documents — those are
  historical records. Future references to them in new docs use
  the downgraded framing.

## Consequences

### Positive

- **Planex stops over-claiming**. The README, ADRs, and manifesto
  will no longer describe the v1–v4 derivation lineage as
  essence-discovery. New readers get an accurate picture of what
  the project is and what it is not.
- **The 10 demands become the project's institutional bar** for
  any future "first-principles" re-claim. Contributors don't have
  to re-derive the audit criteria; they're listed in this ADR.
- **Future ADRs are easier to write**. Authors describe which
  abstraction their decision touches and what tradition support
  exists — no metaphysical labeling, no "is this essence?" debate.
- **The v5 path is explicitly documented** (Part VII.11 of v4 doc
  + this ADR's D5). A future maintainer who wants to genuinely
  earn "first-principles derivation" status has a clear checklist.
- **Code and framing are decoupled**. The 8-abstraction surface
  stands on its engineering merits (implementable, orthogonal,
  tested) regardless of epistemic framing. Contributors can
  improve the code without re-debating the essence claim.
- **Auditability is improved**. Any contributor can read this
  ADR + Part VII of v4 doc and verify the project's claim
  matches what was done. The gap is closed per Q3 above.

### Negative

- **Rhetorical power is reduced**. "Planex derives its
  abstractions from UI's essence" is a strong positioning
  statement; "Planex's 8 abstractions are a tradition-grounded
  design proposal" is weaker. External communications (blog posts,
  conference talks, GitHub README) lose some of the prior
  marketing edge.
- **The word "essence" becomes complicated to use**. Future
  contributors must be careful: "essence" still appears in
  document titles (`essence-derivation-v4-clean.md`), in ADR
  filenames, in tradition-support discussions — but the strong
  metaphysical reading is banned. The word is reserved for
  describing the project's *aspiration* (some future v5 may earn
  it) and the *audit target* (the 10 demands), not for claiming
  current status.
- **Some external material may need explicit correction**. If
  anyone has cited Planex as an "essence-derived UI library" in
  external writing, that citation is now inaccurate. We can't
  retract external citations; the best we can do is update the
  README and let future citations use the corrected framing.
- **Risk of meta-debate fatigue**. Contributors new to the
  project may find the "design rationale vs essence discovery"
  distinction subtle and prefer the simpler (over-claim) framing.
  There is a small ongoing cost to explaining the distinction in
  code review and PR discussion.
- **No new essence work is committed**. This ADR does not start
  v5. If the project never undertakes v5, "essence-derived" framing
  never returns. Some contributors may find this permanent
  down-grade discouraging.

### Neutral

- **No code change** — `git diff` against the previous commit
  shows only documentation files modified.
- **No test change** — all 9 v4 test binaries continue to pass
  (~133 assertions, zero warnings) per ADR-0009's verification.
- **No API break** — old callers continue to work; the
  8-abstraction public surface is unchanged.
- **ADR-0009's Status remains Proposed** — ADR-0009 was Proposed
  when written; this ADR does not flip ADR-0009 to Accepted or
  Superseded. ADR-0009's implementation decisions stand; its
  essence-claim language is downgraded by this ADR.
- **v1/v2/v3 derivation documents remain as historical records**.
  This ADR doesn't rewrite them; it changes how future writing
  refers to them.

## Alternatives Considered

### A1. Keep the "essence-derived" framing and ignore the audit

Rejected. The audit (Part VII of v4 doc) shows v4 meets 0 of 10
constitutive demands of first-principles derivation. The demands
are not nitpicks — they span 2500 years of methodology literature
(Aristotle through Brooks). Keeping the over-claim after the audit
is published would be dishonest: any reader who opens Part VII
sees the over-claim and the audit side by side. The project's
research-grade positioning depends on intellectual honesty; this
is non-negotiable.

### A2. Delete v4 entirely and start v5 fresh

Rejected. The v4 code solves real conflations in v0.4 (Closure
mixed illocution+perlocution; Perception mixed
representamen+interpretant; 2-place Relation missed Situatedness).
Throwing away that work to start v5 from scratch loses engineering
value for an epistemic cleanup that doesn't require code
deletion. The code is implementable and orthogonal — those are
engineering merits independent of essence framing. Keep the code,
downgrade the framing.

### A3. Mark v4 as "experimental" without addressing the framing

Rejected. "Experimental" implies the *code* is provisional —
unstable, may be removed, not yet production-ready. The actual
issue is that the *framing* is over-strong, regardless of code
status. The v4 code is stable (9 test binaries, ~133 assertions,
all green). Marking it "experimental" would communicate the wrong
thing (code unstable) instead of the right thing (framing was
over-claimed, now corrected).

### A4. Defer the downgrade until v5 is done

Rejected. Each future ADR written before the downgrade inherits
the over-claim. The compounding cost: ADR-0011, ADR-0012, etc.
would all use "essence category" language; when v5 (if ever)
completes and re-claims essence status, all those ADRs would need
retroactive framing correction. Freezing the honest framing now is
cheaper — future ADRs use the new language from the start.

### A5. Make this a soft framing note in the v4 doc, not an ADR

Rejected. A note in the v4 doc is invisible to contributors
writing future ADRs. ADR-0009 (Proposed) makes the old essence
claim at ADR level; if the downgrade is not also at ADR level,
the old framing wins by default (the ADR index shows the strong
claim, not the v4 doc's softer note). The downgrade must be at
the same institutional level as the over-claim it corrects.

### A6. Commit to v5 in this ADR

Rejected. v5 is months of work (eidetic variation on 30+ UIs,
falsifier specification, Wittgenstein engagement, Lakatos risky
prediction) with no current implementation pressure — no domain
UI demands it. Committing to v5 in this ADR would (a) be dishonest
about timeline, (b) block shipping v0.5+ features that don't
depend on essence-derivation, and (c) tie the framing downgrade
to a future deliverable that may not happen. Better to downgrade
framing now (this ADR) and leave v5 as an explicit option (D5)
that a future maintainer can choose to undertake.

## CAVEATS

This ADR is a framing downgrade. It does NOT cover:

- The substantive question of whether the v4 essence-rederivation proposal (Interpretant / Perlocution / Breakdown) is *correct* — this ADR is silent on the proposal's truth. It only downgrades the framing from "essence discovery" to "design rationale".
- Whether the v4 proposal is the *only* design proposal that satisfies the three prerequisites (Ontological Stability / Orthogonal Separability / Falsifiability) — it is one proposal among possible others; alternative essence-rederivations may be proposed in future ADRs.
- The indispensability test for the 8-abstraction set — `abstraction-form.md` Prerequisite 3 names the three-layer verification (Layer 1 traditional sourcing / Layer 2 orthogonal separability / Layer 3 falsifiability) and explicitly notes that Layer 3c (completeness test via closed UI-pattern corpus + bidirectional coverage proof) remains an open gap as of v0.5. This ADR does not close that gap.
- The leak-budget mechanism — see `leak-budgets.md` and ADR-0013 for the quantitative L1/L2 audit that tracks v4-proposal leak retirement. This ADR does not touch that machinery.
- The decision of whether v4 actually ships — ADR-0006 (defer Continuous Interaction to v1.0+) shows the precedent of deferring proposals; v4's promotion to shipping is its own future decision and is out of scope here.

The ADR's only commitment: as of v0.5, v4 is framed as "design rationale, not essence discovery" in all canonical docs (abstraction-form.md, why-four-abstractions.md, the README). Future ADRs may upgrade or retire this framing.

## HISTORY

- 2026-08-27: Accepted (with the v4 derivation already in repo as `essence-derivation-v4-clean.md`)

## References

- [essence-derivation-v4-clean.md](../../concepts/history/essence-derivation-v4-clean.md)
  — the v4 design analysis (Part VII audit + Appendix honest
  framing); the source document for this ADR's conclusion.
- [ADR-0009](../proposed/ADR-0009-essence-rederivation-v3.md) — v3 Path B
  implementation ADR (Proposed). Implementation decisions stand;
  D4/D5 essence-claim language downgraded by this ADR.
- [ADR-0008](ADR-0008-feedback-as-fifth-essence-category.md) —
  `px_loop` ADR (Accepted). Implementation stands; "5th essence
  category" framing downgraded to "5th abstraction with tradition
  support".
- [ADR-0007](ADR-0007-essence-derivation-v2-revision.md) — v2
  essence derivation (Accepted). Historical record; framing
  downgraded by this ADR.
- [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md)
  — Perception as 4th abstraction (Accepted, framing corrected
  by ADR-0007). Precedent for framing corrections getting their
  own ADR.
- [ADR-0003](ADR-0003-no-ai-integration.md) — unchanged by this
  ADR. The "no AI integration" decision is engineering, not
  essence; the framing of "essence categories Planex does NOT
  include" is downgraded, but the no-AI decision itself stands.
- Audit sources (60 Wikipedia articles, 6 methodological
  traditions): listed in Part VII.12 of the v4 doc. Local copies
  at `/home/z/my-project/research/firstprinciples/body/*.txt`
  (not committed to this repo).

## See also

- [ADR-0005](ADR-0005-promote-perception-to-fourth-abstraction.md)
  — historical precedent for framing corrections getting their
  own ADR (the "framing corrected by ADR-0007" note on ADR-0005's
  status line is the same pattern this ADR applies at larger
  scope).
- [ADR-0009](../proposed/ADR-0009-essence-rederivation-v3.md) — the ADR most
  affected by this ADR's downgrade. Future contributors reading
  ADR-0009 should also read this ADR to see the framing
  correction.
- [essence-derivation-v4-clean.md](../../concepts/history/essence-derivation-v4-clean.md)
  Part VII — the audit that grounds this ADR's conclusion. Not
  for the casual reader; ~190 lines covering 6 methodological
  traditions' demands on "first-principles derivation".
- [essence-derivation-v4-clean.md](../../concepts/history/essence-derivation-v4-clean.md)
  Appendix — the honest framing already in use; this ADR makes
  that framing binding on all derivative materials.
- [non-goals.md](../../concepts/canonical/non-goals.md) — the project's
  non-goals include NG-3 (no kitchen-sink API). This ADR does not
  affect non-goals; it only affects how the project describes its
  abstractions' epistemic status.
- [why-four-abstractions.md](../../concepts/canonical/why-four-abstractions.md)
  — the manifesto document, slated for rename to
  `why-eight-abstractions.md` per ADR-0009's scope. When the
  rename happens, the new document must use design-rationale
  framing per this ADR's D2.
- Michael Nygard's original ADR article
  (https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions)
  — ADR format reference.
- Conal Elliott, "Denotational Design with Type Class Morphisms"
  — the "claim = implementation" ideal this ADR moves the project
  toward (per Q3 above).
