<!--
PR template. Auto-loaded by GitHub when opening a PR against main.
This template wires the doc-organization contract (doc-organization.md
Part IX, Principle 12) into the PR review surface. It is the
human-applied complement to the CI gates in .github/workflows/docs.yml
and the code gates in .github/workflows/ci.yml.

Two-track review:
  1. ADR PRs (proposed -> accepted) apply REVIEW-RUBRIC.md ("No 3s",
     6 criteria). See docs/decisions/REVIEW-RUBRIC.md.
  2. All PRs (including non-ADR) apply the pre-merge checklist below.
-->

## What this PR does

<!-- 1-3 sentences. If this is an ADR, the title is the decision in one
declarative sentence; this section explains the *why*, not the *what*. -->

## Type

<!-- Check one or more. -->

- [ ] ADR (decisions/)  — applies REVIEW-RUBRIC.md (see below)
- [ ] Code (src/, include/, examples/)  — applies CONTRIBUTING.md rules
- [ ] Docs (docs/)  — applies doc-organization.md (Part VI contract)
- [ ] Test (tests/)  — closes a CI gate gap
- [ ] Build (Makefile, CMakeLists.txt, .github/)  — build-system sync rule
- [ ] Other

## Pre-merge checklist

<!-- Every box must be checked. The reviewer applies the rubric, not
the author; the author confirms the surface area below. -->

### Build sync (CONTRIBUTING.md rule 1)

- [ ] If adding/renaming/deleting any source file: `CMakeLists.txt` updated?
- [ ] Same: `Makefile` updated?
- [ ] Same: `examples/README.md` catalog updated?

### API sweep (CONTRIBUTING.md rule 2)

- [ ] If changing any function signature in `include/planex/*.h`: grep'd all call sites in `src/`, `examples/`, `tests/`?

### Doc sync (CONTRIBUTING.md rule 5)

- [ ] If this PR changes the project's claims: README.md updated?
- [ ] Same: why-four-abstractions.md / abstraction-form.md updated?
- [ ] Same: limitations.md / non-goals.md / roadmap-matrix.md updated?
- [ ] Same: changelog.md has an entry?
- [ ] grep for stale terminology: `grep -rn "<old-term>" docs/ README.md`

### Doc-org contract (only if docs/ touched)

- [ ] `scripts/check_doc_sections.sh --check` passes (ADR section completeness)?
- [ ] `scripts/check_adr_lifecycle.sh --check` passes (ADR dir state matches Status field)?
- [ ] `scripts/gen_adr_index.sh --check` passes (README index matches script output)?
- [ ] `scripts/check_links.sh --check` passes (no broken internal links)?
- [ ] `scripts/find_orphans.sh --check` passes (no unreferenced docs)?
- [ ] `scripts/check_terms.sh --check` passes (glossary term saturation)?
- [ ] `make BACKEND=headless check-completeness` passes (68-pattern corpus invariants)?
- [ ] `make check-compression` passes (no catastrophic AEL drift, LLE > 0.3)?
- [ ] If examples/ touched: `make BACKEND=headless check-examples` passes (no .expected drift)?

<!-- These 9 gates also run as .github/workflows/docs.yml on push and
PR. Checking them locally avoids round-trips; the CI gate is the
authoritative record. -->

### Code contract (only if src/, include/, or tests/ touched)

- [ ] `make BACKEND=headless` builds clean with `-Werror`?
- [ ] `make test`, `make test_ortho`, `make test_feedback`, `make test_v05` all pass?
- [ ] New abstraction added? If yes, essence-justified criteria met (ADR-0011 three-criterion: tradition-cite + orthogonality-test + denotational-semantics)?

### Platform parity (CONTRIBUTING.md rule 7)

- [ ] If adding new event types: win32.c, x11.c, cocoa.c, headless.c all updated?

---

## For ADR PRs only: REVIEW-RUBRIC.md application

> Apply [`docs/decisions/REVIEW-RUBRIC.md`](../docs/decisions/REVIEW-RUBRIC.md)
> — the "No 3s" rubric with 6 criteria. Score 3 is forbidden; a neutral
> "looks fine" means you haven't read the ADR carefully enough.

Reviewer (fill in at review time, not at PR open):

| Criterion | Score (1-5, 3 forbidden) | Notes |
|---|---|---|
| 1. Relevance | /5 | |
| 2. Honesty | /5 | |
| 3. Compressiveness | /5 | |
| 4. Falsifiability | /5 | |
| 5. Alternatives | /5 | |
| 6. References | /5 | |

**Verdict**: [ ] Accept  [ ] Request changes  [ ] Reject

<!-- A score of 1-2 on any criterion is a request-changes verdict by
default. A score of 4-5 on all six is an accept. Reviewers must commit
to a verdict per criterion; "3" is forbidden. -->

---

## Related

- ADR number (if applicable): ADR-NNNN
- Closes issue: #
- Supersedes: ADR-NNNN (if applicable)
- Related ADRs: ADR-NNNN, ADR-NNNN

## References (external prior art)

<!-- For ADRs especially: cite papers / blog posts / other projects'
ADRs. An ADR with no external references is a decision made in a vacuum
(TEMPLATE-GUIDE.md failure mode: No-external-references). -->
