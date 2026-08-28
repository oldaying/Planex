# Deprecation Registry

> **Applies to**: v0.4 → v4 onward. Single canonical registry for deprecated or removed APIs.
>
> **Companion documents**: [`leak-budgets.md`](../concepts/canonical/leak-budgets.md) — quantitative L2 leak audit (abstraction-eroding); this registry records API-level retirements that the leak audit triggered. [`abstraction-form.md`](../concepts/canonical/abstraction-form.md) — Prerequisite 2 (orthogonal separability) and Prerequisite 3 (falsifiability); every entry below retires an abstraction-eroding leak that violates one or both prerequisites.
>
> **Research basis**: Linux kernel `Documentation/process/deprecated.rst` — <https://github.com/torvalds/linux/blob/master/Documentation/process/deprecated.rst>. The Linux registry is a single alphabetized file, each entry has name + replacement + rationale. Planex uses the same shape, extended with the ADR that decided the deprecation (Planex ADRs are more central to project decisions than Linux commits, so the ADR column is mandatory).

---

## Purpose

This document is the single source of truth for APIs that have been deprecated or removed. It exists for two reasons:

1. **Reader lookup** — a user encountering `px_closure_get_status` in old example code can look here to find the replacement and the ADR that decided the removal.
2. **Falsifier for retire discipline** — the registry grows monotonically. A retire that does not appear here is an incomplete retire; CI can flag any `git rm` of a public symbol whose name is not in this registry. (CI hook is the same shape as the auto-generated ADR index in `scripts/gen_adr_index.sh` — see Part VI of [`doc-organization.md`](../doc-organization.md).)

The registry does NOT track:

- **Internal helper functions** (`px_*_internal_*`, static functions) — these are not public API and their lifecycle is governed by the leak budget, not by ADR.
- **L1 leaks (host tax)** — platform-specific includes in cross-platform headers are L1 leaks, tracked in `leak-budgets.md` Section "L1 audit". This registry is L2-only (abstraction-eroding).
- **v0.4 → v4 migration mechanics** — the v4 ABI break is a one-time event recorded in ADR-0012 Finding 3. The deprecation registry records its individual API-level retirements, not the break itself.

---

## Distinction: "deprecated" vs "diagnostic seam" vs "removed"

The registry distinguishes three states, mirroring the ADR lifecycle states (`decisions/{proposed,accepted,deferred,deprecated,superseded}/`):

| State | Meaning | Future action |
|-------|---------|----------------|
| **deprecated** | API still callable in current version; will be removed in a future version. | Track for removal; update examples to use replacement; remove at named version boundary. |
| **removed** | API no longer exists in current version. Migration path documented. | No future action; entry retained for historical lookup. |
| **diagnostic seam** | API is permanent, exists for testing/debugging only. Not on the deprecation path. | No future action; documented here to prevent well-intentioned "cleanup" PRs from removing it. |

The distinction matters: a "deprecated" API will eventually be removed; a "diagnostic seam" is permanent. The registry is the canonical place to record this distinction so that a contributor opening a PR to "remove dead code" can be pointed at the registry entry that justifies the API's continued existence.

---

## Registry

| API | Status | Deprecated / removed when | Replacement | Rationale | ADR |
|-----|--------|---------------------------|-------------|----------|-----|
| `px_closure_get_status(c)` | removed | v4 design | `px_perlocution_status(per)` | Operational status is perlocutionary (effect on the world), not illocutionary (the speech act itself). Mixing them in one accessor was an L2 leak: Closure's illocutionary status got conflated with perlocutionary operational status, eroding the Closure/Perlocution boundary that ADR-0012 orthogonality pressure-test exposed. | [ADR-0012](../decisions/accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) Finding 3 |
| `px_perception_invoke_all()` | diagnostic seam | n/a (v0.5 retire of L2 leak) | (kept for testing/debugging) | Phase 2 auto-invocation landed in v0.5 (closing the L2 leak where Perception was a no-op that callers had to invoke manually). The manual invoke functions remain as a diagnostic seam: tests and debugging benefit from being able to trigger Perception outside the auto-invocation path. Removing them would break tests without cleaning up any abstraction-eroding leak. | [ADR-0013](../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) |
| `px_perception_invoke_single(...)` | diagnostic seam | n/a | (kept) | Same as above. | [ADR-0013](../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) |
| `px_perception_invoke_for_estimate(e)` | diagnostic seam | n/a | (kept) | Same as above. | [ADR-0013](../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) |

---

## Maintenance protocol

When retiring an API:

1. Add a row to the registry in the same PR that removes the API. The CI hook (planned, see [`doc-organization.md`](../doc-organization.md) Part VI) verifies that any `git rm` of a public symbol has a corresponding registry entry.
2. Cross-reference the ADR that decided the retirement in the "ADR" column.
3. If the retirement is a *deprecation* (not a removal), the entry's status is "deprecated" and the "future action" is to remove the API at the next ABI break (typically the next minor version).
4. If the retirement is a *diagnostic seam* (API kept for testing/debugging), document the rationale explicitly. Future contributors will read this column before opening a "cleanup" PR.

The registry is monotonic — entries are never deleted. A "deprecated" entry transitions to "removed" when the API is finally removed, but the entry itself persists for historical lookup. This mirrors the ADR lifecycle (`superseded/` retains the superseded ADR rather than deleting it).

---

## References

- Linux kernel `Documentation/process/deprecated.rst` — <https://github.com/torvalds/linux/blob/master/Documentation/process/deprecated.rst>
- [`leak-budgets.md`](../concepts/canonical/leak-budgets.md) — L1/L2 audit mechanism
- [`abstraction-form.md`](../concepts/canonical/abstraction-form.md) — Prerequisites 2 and 3 that every retirement here addresses
- [ADR-0012](../decisions/accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) — the v4 orthogonality pressure test whose Finding 3 produced the registry's first entry
- [ADR-0013](../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) — the v0.5 leak-budget retire that produced the diagnostic-seam entries
