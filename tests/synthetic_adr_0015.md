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

## Consequences

### Positive

- Memory is useful; we will use it.

### Negative

(none — what could go wrong?)

### Neutral

- Adds a new struct.

## Alternatives Considered

### A1. Don't add Memory

We rejected this because we need Memory.

## CAVEATS

This ADR is synthetic. It exists only to demonstrate that
`scripts/check_essence_admission.sh` correctly fires on a tradition-less,
essence-justified claim. It is the synthetic violation case encoded in
ADR-0014's `## Validation` section.

## Known issues

This ADR is intentionally broken. Do not promote to `Accepted`.

## HISTORY

- 2026-08-28: Synthetic case created (encoded in ADR-0014).

## References

- **Code:** none (synthetic; no implementation).
- **Related ADRs:** [ADR-0014](../docs/decisions/validated/ADR-0014-validated-stage-and-essence-justified-enforcement.md) — the ADR whose enforcement this case demonstrates.
