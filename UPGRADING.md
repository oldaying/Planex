# UPGRADING — Breaking-change migration guide

> **Applies to**: Planex v0.4 → v4 onward. Single canonical registry for
> breaking changes that require existing callers to update their code.
>
> **Companion documents**:
> - [`CHANGELOG`](docs/changelog.md) — the raw commit-level history
>   (every change, breaking or not). UPGRADING is the curated subset.
> - [`docs/reference/deprecation-registry.md`](docs/reference/deprecation-registry.md) —
>   API-level retirements (deprecated, removed, diagnostic seam). UPGRADING
>   covers breaking *migration paths*, deprecation-registry covers
>   *individual symbol lifecycle*.
> - [`docs/concepts/state/versioning.md`](docs/concepts/state/versioning.md) —
>   version-numbering policy and ABI-stability rules.
>
> **Research basis**: lwIP `UPGRADING` file —
> <https://git.savannah.gnu.org/cgit/lwip.git/tree/UPGRADING>. lwIP splits
> `CHANGELOG` (raw, every commit) from `UPGRADING` (curated, breaking
> changes only, grouped by `++ Application changes:` / `++ Port changes:`
> / `++ Repository changes:`). Planex adopts the same split with three
> Planex-specific sub-sections: `++ API changes:` / `++ Internal changes:`
> / `++ Build changes:`.

---

## Purpose

This file answers the question every upgrader asks first: *"I am on
version X, what do I need to change in my code to use version Y?"*

The answer is *not* in the changelog — the changelog is the raw commit
log, useful for understanding what happened but not for migration. The
deprecation-registry answers a different question ("this symbol I'm
calling is gone, what replaces it?"); UPGRADING answers the higher-level
question ("my build broke after upgrading, what's the systematic migration
path?").

UPGRADING is the **curated, only-breaking-changes** subset. Entries are
grouped by version and by impact category. Non-breaking changes belong in
the changelog, not here.

---

## Three impact categories (per version)

Every version that introduces a breaking change is one section. Within
each section, entries are grouped by three sub-categories, in this order:

### `++ API changes:`

Breaking changes to the public C API surface — function signatures,
struct layouts, enum values, header paths, ABI-affecting changes.

Examples: a function gained a new required parameter; a struct's member
ordering changed; a header file moved from `include/planex/v0/foo.h` to
`include/planex/v4/foo.h`.

### `++ Internal changes:`

Breaking changes to internal structure that affect contributors (not
end-users) — directory layout renames, internal helper renames, internal
data structure changes that show up in commit history but not in the
public API.

Examples: `src/estimates.c` renamed to `src/estimate.c`; internal
`px_estimate_internal_validate` renamed; `tests/` directory restructured.

### `++ Build changes:`

Breaking changes to the build system — CMake variables, target names,
required compiler version, dependency changes.

Examples: `cmake -DBUILD_SHARED_LIBS=ON` replaced by `cmake -DPLANEX_LINK=shared`;
minimum C standard bumped from C11 to C17; new required dependency on
`libfoo >= 1.2`.

---

## Entry format

```
- **[short-name-of-change]**: what changed (1 sentence)
  - Before: `code or call shape before`
  - After: `code or call shape after`
  - Why: 1 sentence (or "see ADR-NNNN")
  - Migration: concrete steps the caller must take
  - ADR: ADR-NNNN (link)
```

---

## Entries

### v4 (released 2026-08)

The v4 rederivation. ABI break from v0.4. See ADR-0012 for the
orthogonality pressure-test that motivated the redesign and ADR-0013 for
the v0.5 leak-budget retire that completed the migration.

#### `++ API changes:`

- **Closure-status accessor renamed**:
  - Before: `px_closure_get_status(c)`
  - After: `px_perlocution_status(px_perlocution_of(c))`
  - Why: illocutionary (Closure's own status) and perlocutionary (effect on
    the world) status were conflated; the rename enforces the
    Closure/Perlocution boundary
  - Migration: mechanical rename; see deprecation-registry entry for details
  - ADR: [ADR-0012](docs/decisions/accepted/ADR-0012-v4-orthogonality-pressure-test-four-findings.md) Finding 3

- **Perception auto-invocation phase**:
  - Before: caller calls `px_perception_invoke_all()` after every Estimate
    update
  - After: Phase 2 auto-invocation; the manual invoke functions are
    retained as diagnostic seams (tests, debugging) only
  - Why: manual invoke was an L2 leak — it required callers to know about
    Perception's phase, eroding the Estimate/Perception boundary
  - Migration: remove manual `px_perception_invoke_all()` calls from
    production code; keep them in tests if you need to assert against a
    specific phase ordering
  - ADR: [ADR-0013](docs/decisions/accepted/ADR-0013-v05-leak-budget-retire.md)

#### `++ Internal changes:`

- **`src/` restructured** to mirror the four-shipping-abstraction layout:
  - Before: flat `src/estimates.c`, `src/relations.c`, etc.
  - After: `src/estimate/`, `src/perception/`, `src/closure/`, `src/relation/`,
    `src/loop/` — one directory per abstraction
  - Why: directory structure mirrors the abstraction boundary; a leak in
    directory structure is now visible as cross-directory `#include` of
    internal headers
  - Migration: no caller action; contributor action — update local clones,
    re-apply any local patches to the new paths
  - ADR: implicit in ADR-0012 Finding 4

#### `++ Build changes:`

- **Minimum C standard bumped from C11 to C17**:
  - Before: `cmake -DCMAKE_C_STANDARD=11`
  - After: `cmake -DCMAKE_C_STANDARD=17`
  - Why: C17 features used in the rederivation (`_Static_assert` in
    expressions, designated-initializer improvements)
  - Migration: update CMake preset; GCC ≥ 8, Clang ≥ 7, MSVC ≥ 19.14
    required
  - ADR: [ADR-0004](docs/decisions/accepted/ADR-0004-use-c-not-rust-zig-cpp.md) (implicit extension)

---

## Maintenance protocol

When introducing a breaking change:

1. Add an entry to UPGRADING in the same PR that lands the break. CI
   (planned: see `doc-organization.md` Part VI) verifies that any `git mv`
   or signature change of a public symbol has a corresponding UPGRADING
   entry.
2. Cross-reference the ADR that decided the break in the "ADR" field.
3. If the break is a *deprecation* (symbol still callable in current
   version, to be removed later), add a row to
   `docs/reference/deprecation-registry.md` *instead of* (or in addition
   to) an UPGRADING entry. UPGRADING is for *immediate* breaks;
   deprecation-registry is for *future* removals.
4. Three sub-categories are mandatory (`++ API changes:`, `++ Internal
   changes:`, `++ Build changes:`) — if a version has no entries in one,
   write `none` rather than omitting the heading.

---

## References

- lwIP `UPGRADING` (research basis, two-file split with `CHANGELOG`):
  <https://git.savannah.gnu.org/cgit/lwip.git/tree/UPGRADING>
- curl `docs/DEPRECATE.md` (deprecation-registry analog in a docs/ tree):
  <https://github.com/curl/curl/blob/master/docs/DEPRECATE.md>
- Linux kernel `Documentation/process/deprecated.rst` (single-file
  anti-pattern registry):
  <https://github.com/torvalds/linux/blob/master/Documentation/process/deprecated.rst>
- [`docs/changelog.md`](docs/changelog.md) — raw commit-level history
- [`docs/reference/deprecation-registry.md`](docs/reference/deprecation-registry.md) — API-level retirements
- [`docs/concepts/state/versioning.md`](docs/concepts/state/versioning.md) — version-numbering policy
