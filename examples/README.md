# Examples Catalog

> What each demo proves, organized by which abstraction's which capability it demonstrates.

Post-ADR-0005 redesign (commit c24bbcab57): the previous 25 3-abstraction-era demos were removed. The catalog now reflects only demos using the new 4-abstraction API (Relation + Estimate + Closure + Perception).

Demos are categorized into three tiers:

- **Tier 1 — Canonical demos**: built and run, validate the 4-abstraction API
- **Tier 2 — Prototype demos**: not in build, use pre-ADR-0005 API, kept as research reference
- **Tier 3 — Smoke tests**: built and run, validate API surface

See [roadmap-matrix.md](../concepts/roadmap-matrix.md) for the matrix view.

---

## Tier 1 — Canonical demos (in build, use new 4-abstraction API)

### Counter 4-Abs (`counter_4abs.c`)

**The canonical hello world of Planex post-ADR-0005.**

Validates that all four abstractions work together:

- **Estimate**: `count` state with value + confidence
- **Closure**: `inc` and `dec` actions (5-stage, **no perception parameter** — new API)
- **Relation**: `PX_REL_TRIGGERS` declares "inc triggers count change"
- **Perception**: `render_count_to_string` pure function denoting count

This is the minimal demo. If you want to understand Planex, read this file first.

**Key post-ADR-0005 patterns:**

- `px_closure_new` signature is 5-arg (perception parameter removed)
- Render is **NOT** a Closure (was `PX_INTENT_EXPRESS` before ADR-0005)
- Render is a Perception — registered via `px_perception_new`
- Multiple perceptions coexist for the same Estimate (text + a11y)

```bash
./build/counter_4abs
```

---

### Multi-Perception (`multi_perception.c`)

**Shows why Perception is a 4th abstraction.**

The same set of Estimates has **FOUR different Perception functions**, each producing a different denotation:

- `perceive_visual` — screen text (would drive pixels in a real app)
- `perceive_a11y` — screen reader text
- `perceive_json` — JSON snapshot (testing / serialization)
- `perceive_log` — log line (debugging)

Each is a pure function taking the same Estimates as input. They are **independent** — removing one does not affect the others.

**Why this matters:**

Mainstream UI libraries (React, SwiftUI, Flutter) have **ONE render path**. To produce multiple denotations, you'd have to either:
- Special-case each denotation inside a single render function, or
- Maintain parallel trees (DOM + a11y tree + test snapshots + ...)

Planex's Perception abstraction makes multiple denotations **first-class**. This is the strongest argument for promoting Perception to a 4th abstraction per ADR-0005.

```bash
./build/multi_perception
```

---

## Tier 3 — Smoke tests (in build, validate API surface)

### Perception Smoke Test (`perception_smoke.c`)

**Validates the Phase 1 API surface.**

9 unit tests pass:

1. `px_perception_count() == 0` initially
2. `px_closure_new` works without perception parameter (new signature)
3. Closure triggers and updates Estimate (unchanged behavior)
4. `px_perception_new` registers in global registry
5. `px_perception_name` returns correct name
6. Multiple perceptions coexist (registry supports N)
7. `px_perceptions_for_estimate` is Phase 1 stub (returns NULL, count=0)
8. `px_perception_free` removes from registry
9. After cleanup, count back to 0

**This is NOT a full app** — just API surface validation. Use it as the reference for the new API.

```bash
./build/perception_smoke
```

---

## Tier 2 — Prototype demos (NOT in build, kept as research reference)

These demos still use the pre-ADR-0005 `px_closure_new` signature (6-arg with perception parameter). They are **not in the CMake build** because they would fail to compile. They are kept as research reference and will be migrated to the new API in subsequent commits.

### Counter Denotative (`counter_denotative.c`)

**(c) route prototype — single state, headless BMP.**

Validates that pure-function render works at the simplest level:

- 4 unit tests pass
- Generates `counter_denotative.bmp` (256×96)
- Pure function: `px_fb* render(px_estimate* count)`

Validated on Linux/gcc + Windows/MSVC.

### Calculator Denotative (`calculator_denotative.c`)

**(c) route prototype — multi-state, headless BMP.**

Extends counter_denotative to a state machine:

- 4 unit tests pass
- Validated scenarios: `2+3=5`, `4*5+2=22` (chained), `5/0=ERROR`
- Generates `calculator_denotative.bmp` + error variant

Validated on Linux/gcc + Windows/MSVC.

### Counter Interactive (`counter_interactive.c`)

**(c) route prototype — real Win32/X11 window, mouse-clickable.**

Validates (c) route in a real interactive window:

- 2 unit tests pass
- Hit regions as render output (`px_hit_region` struct)
- 8 mouse clicks validated, 1273 frames at 60fps
- Pure function returns `px_fb*` + `px_hit_region[]`

Validated on Linux/gcc + Windows/MSVC (real Win32 window).

### Editor Meaning (`editor_meaning.c`)

**Phenomenological school prototype.**

Demonstrates the four phenomenological abstractions (Context, Visibility, Trace, Affordance) built on top of Planex's existing Relation+Estimate+Closure.

- Validates that the phenomenological school (per [alternative-perspectives.md](../concepts/alternative-perspectives.md)) is implementable on Planex's core
- Planex does NOT adopt this school — the prototype is research demonstration

---

## Why only 7 demos now (was 25)

Before ADR-0005, Planex had 25 demos that inflated the count by including:

- **8 backend variants** (counter_x11 / counter_fb / slider_x11 / slider_fb / ...) — same logic as base, just different backend. Information now in [PLATFORMS.md](../PLATFORMS.md).
- **3 engineering variants** (perf / resize / animate) — proved backend capability, not abstraction capability
- **11 3-abstraction-era demos** (counter / slider / checkbox / radio / dropdown / form / tabs / wizard / modal / todo / todo_app) — used the old API and the "render-as-Closure" anti-pattern (Closure with `PX_INTENT_EXPRESS`)

These 22 demos were deleted in commit c24bbcab57. The 3 remaining demos in build + 4 prototypes out of build represent the same conceptual coverage with less redundancy.

**The conceptual argument** "Planex abstractions can express common UI patterns" still holds — `counter_4abs` and `multi_perception` demonstrate it. The old demos were backend-completeness proofs, not abstraction-completeness proofs.

---

## How to add a new demo

1. Decide which abstraction's which capability you're proving
2. Use the **new 4-abstraction API**:
   - `px_closure_new(goal, intent_kind, action, evaluation, user)` — 5 args, no perception
   - `px_perception_new(name, fn, inputs, n_inputs, user)` for rendering
3. Add to `CMakeLists.txt` in `STDOUT_DEMOS` (or `WINDOWED_DEMOS` if it needs a window)
4. Update this file with a new section

**Do not** use the old 6-arg `px_closure_new` signature — it's removed. If you see code passing a `perception` parameter to `px_closure_new`, it's pre-ADR-0005 code that needs migration.

---

## See also

- [ADR-0005](../decisions/ADR-0005-promote-perception-to-fourth-abstraction.md) — why Perception was promoted
- [Why Four Abstractions](../concepts/why-three-abstractions.md) — manifesto (will be renamed to why-four-abstractions.md)
- [Roadmap Matrix](../concepts/roadmap-matrix.md) — abstraction×maturity tracking
- [UI Essence Layers](../concepts/ui-essence-layers.md) — the 6-layer essence model
- [Path C Lineage](../concepts/path-C-lineage.md) — Planex's place in 60-year Path C history
- [PLATFORMS.md](../PLATFORMS.md) — backend support matrix (replaces old backend-variant demos)
