# Architecture Overview

> **Applies to**: v0.8. How the source files relate to each other and to the core abstractions (7 since the v0.7 promotions: the v0.4 five + intent compilation in `hit.c` + `px_interaction` in `interaction.c`).

This document describes the module structure of Planex at a level above individual files — useful for understanding where to look when modifying or extending the library.

For term definitions, see [Glossary](../../reference/glossary.md). For API specifics, see [API Reference](../../reference/api.md). For design rationale, see [Why Four Abstractions](../canonical/why-four-abstractions.md) and the [ADR index](../../decisions/README.md).

---

## Module Map (conceptual)

```
┌─────────────────────────────────────────────────────────────────────┐
│                          Application Layer                          │
│                  (user code: examples/, your app)                   │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          Public API Layer                           │
│              include/planex/*.h — what users include                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │   planex.h   │  │    app.h     │  │   a11y.h     │            │
│  │ (aggregate,  │  │ (app loop)   │  │ (a11y API)   │            │
│  │  7-abstr.    │  │              │  │              │            │
│  │  contract)   │  │              │  │              │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │   fb.h       │  │  window.h    │                               │
│  │ (framebuf)   │  │ (window/event)│                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     Core Abstractions (7)                           │
│        (these are what makes Planex Planex)                         │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │ relation.c   │  │ estimate.c   │  │ closure.c    │            │
│  │  Relation    │  │  Estimate    │  │   Closure    │            │
│  │  (graph)     │  │ (state+T)    │  │ (5-stage     │            │
│  │              │  │              │  │  execution)  │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │ perception.c │  │ feedback.c   │  │ hit.c        │            │
│  │  Perception  │  │  px_loop     │  │  intent      │            │
│  │ (machine→    │  │ (closed-loop │  │  compilation │            │
│  │  human)      │  │  coupling)   │  │ (ADR-0017)   │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │ interaction.c│  │ undo.c       │                               │
│  │ px_interaction│  │ (undo via   │                               │
│  │ (ADR-0018)   │  │  the graph) │                               │
│  └──────────────┘  └──────────────┘                               │
│                                                                     │
│  v3 prototype (Path B, proposal-stage — not promoted):             │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │ actor.c      │  │ breakdown.c  │                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Infrastructure Layer                          │
│  (platform-agnostic utilities that core abstractions depend on)     │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │   layout.c   │  │    fb.c      │  │   app.c      │            │
│  │ (relational  │  │ (framebuffer,│  │ (event loop, │            │
│  │   layout)    │  │  blit, BMP)  │  │  IME, tick,  │            │
│  │              │  │              │  │  intent      │            │
│  │              │  │              │  │  routing)    │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │  font.c      │  │ font_ttf.c   │  │   a11y.c     │            │
│  │ (bitmap font)│  │ (FreeType,   │  │ (query-side  │            │
│  │              │  │  optional)   │  │  contract)   │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        Platform Backends                            │
│         (one per OS — auto-detected at build time)                 │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │   x11.c      │  │   win32.c    │  │   cocoa.c    │            │
│  │ (Linux/BSD)│  │  (Windows)   │  │   (macOS)    │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │  headless.c  │  │ a11y_bridge_ │                               │
│  │ (no window — │  │ atspi.c      │                               │
│  │  BMP output, │  │ (AT-SPI2     │                               │
│  │  CI/testing) │  │  adapter)    │                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Core Abstractions Layer (7)

### `src/relation.c` — Relation

The graph data structure. Maintains nodes (any `void*` — Estimates, Closures, regions) and edges (Relations of various kinds: `PX_REL_TRIGGERS`, `PX_REL_DEPENDS_ON`, `PX_REL_AFFORDS`, `PX_REL_BESIDE`, ...). Provides the query API (`px_query`, `px_has_relation`) so the runtime can answer questions like "which Estimates does this Closure trigger?" — the query side of intent compilation reads the AFFORDS edges here. The v3 prototype added the 3-place form `px_declare_for` (actor-parameterized).

**Size:** ~7.3 KB
**External dependencies:** none
**Theoretical foundation:** Sketchpad constraint graph (Sutherland 1963), Alexander's semilattice (1965)
**See:** [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md)

### `src/estimate.c` — Estimate

State with time and uncertainty. Holds value, confidence, animation trajectory, and the v0.5 predictive-coding state (one-shot `px_estimate_predict`, Friston). Notifies observers when the value changes, maintains derived estimates (`px_derive_*`, with cycle detection), and carries the v0.7 describable-value schema. Also owns the **live-estimate registry** (`px__estimate_is_live`): every Estimate registers at creation and unregisters at free, so `px_undo_record` can validate TRIGGERS targets by pointer comparison instead of blind-casting them.

**Size:** ~21.6 KB — the largest core abstraction file
**External dependencies:** none
**Theoretical foundation:** Conal Elliott's FRP, Karl Friston's predictive coding
**Notable:** animation, derivation, observer notification, predictive coding, the value schema, and the undo-safety registry all live here

### `src/closure.c` — Closure

The **5-stage execution unit** (user → machine direction): Goal → Intent → Action → Execution → Evaluation. Per ADR-0005 this was reduced from the original 7-stage form: the evaluation side of Norman's model (Perception, Interpretation) moved to the Perception abstraction. Intent is a typed *value* (`px_intent`), not a callback — this is what makes undo, replay, and agent driving expressible. v0.7 added `px_closure_new_with_graph` (ADR-0019: the graph arrives with the closure; the trigger-before-bind ordering mistake is unwritable).

**Size:** ~15.6 KB
**External dependencies:** Relation (for TRIGGERS edges)
**Theoretical foundation:** Don Norman's stage model, Winograd/Flores speech-act theory
**See:** [ADR-0005](../../decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md), [ADR-0019](../../decisions/accepted/ADR-0019-v07-leak-budget-retire.md)

### `src/perception.c` — Perception

The presentation abstraction (machine → human): registered perceptions are invoked when their input Estimates change (Phase 2 auto-invocation, v0.5 — the manual invoke calls remain as diagnostic seams). Owns its own registry (the pattern the estimate registry follows). v0.6 added the `free_fn` ownership contract for cached representamens.

**Size:** ~16.9 KB
**External dependencies:** none (takes `px_estimate*` inputs)
**See:** [ADR-0005](../../decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md)

### `src/feedback.c` — Feedback (`px_loop`)

Closed-loop coupling: a `px_loop` binds a Closure (intent side) to a Perception (view side) and runs the loop with a **budget as contract** — every loop ships with `PX_LOOP_DEFAULT_BUDGET_MS` (16 ms, one 60fps frame); overruns warn once and count in `px_loop_budget_overruns()`, and audit entries carry propagation accounting (`propagation_edges` / `propagation_depth`). This is where `px_loop` lives — not in `src/loop.c` (that path is the v4 proposal tree) and not in `src/app.c`.

**Size:** ~20.7 KB
**External dependencies:** Closure, Perception, Relation
**See:** [ADR-0008](../../decisions/accepted/ADR-0008-feedback-as-fifth-essence-category.md), [ADR-0019](../../decisions/accepted/ADR-0019-v07-leak-budget-retire.md)

### `src/hit.c` — Intent compilation (6th abstraction, ADR-0017)

Region + affordance query, plus the v0.7 Line 1 routing-side compile step: `px_afford_compile` walks the AFFORDS edges and produces the afford table the app consults instead of raw-coordinate hit-testing. `px_pointer_intent` / `px_key_intent` embed the region label so the intent survives capture and replay. Landed in v0.6 as the ADR-0016 prototype; promoted by ADR-0017.

**Size:** ~14.8 KB
**External dependencies:** Relation (AFFORDS edges)
**See:** [ADR-0017](../../decisions/accepted/ADR-0017-intent-compilation-promotion.md)

### `src/interaction.c` — `px_interaction` (7th abstraction, ADR-0018)

The interaction process: an **inert hot path** (no work between transitions) with **transitions-only bridges** to the rest of the system — the invariant ADR-0018 makes normative. Landed in v0.6 as the ADR-0016 prototype; promoted in v0.7. `examples/hover_drag_interaction.c` is its regression demo.

**Size:** ~13.8 KB
**External dependencies:** Relation
**See:** [ADR-0018](../../decisions/accepted/ADR-0018-interaction-process-promotion.md)

### `src/undo.c` — Undo via the graph

ADR-0002's proof that Relation is necessary: snapshots only the Estimates reachable from a Closure via TRIGGERS edges (minimal undo — Solid must snapshot everything). Every TRIGGERS target is validated against the estimate registry before the cast (2026-10 audit fix: type-confused edges and freed Estimates are skipped, an all-filtered snapshot is not pushed, `px_undo_count()` never advertises a step that restores nothing).

**Size:** ~5.5 KB
**External dependencies:** Relation, Estimate (registry check)
**See:** [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md)

### `src/actor.c` / `src/breakdown.c` — v3 prototype (Path B)

Proposal-stage essence candidates from essence-derivation v3 (ADR-0009 Proposed): the actor parameter of the 3-place Relation, and Zuhandenheit/breakdown. Compiled and tested (`tests/test_v3_prototype.c`) but **not promoted** — they are not part of the 7 canonical abstractions.

**Size:** ~2.4 KB + ~7.3 KB
**See:** [ADR-0009](../../decisions/proposed/ADR-0009-essence-rederivation-v3.md)

---

## Infrastructure Layer

### `src/layout.c` — Layout helpers

Relation-driven layout. Provides `beside`, `below`, `center` helpers that build spatial Relations (`PX_REL_BESIDE`, `PX_REL_BELOW`). The actual layout computation reads the Relation graph and produces pixel coordinates.

**Size:** ~4.3 KB

### `src/fb.c` — Software framebuffer

CPU framebuffer with primitives: `fill_rect`, `draw_text`, `set_pixel`, `draw_line`. Includes an 8x16 bitmap font (ASCII only) as a fallback when FreeType is not available.

**Size:** ~10 KB
**External dependencies:** none (the built-in font is compiled in)

### `src/font.c` and `src/font_ttf.c` — Font loading

- `font.c`: built-in bitmap font loader and the fallback chain
- `font_ttf.c`: FreeType-based TTF/OTF loader, supports color emoji (FT_LOAD_COLOR), CJK fallback chains

`font_ttf.c` is optional — compiled only when FreeType is detected at build time.

**Size:** ~13 KB (font.c) + ~21 KB (font_ttf.c)

### `src/app.c` — Application loop

The cross-platform app loop: render, click, key, tick, IME. Holds `px_app_run()` — the main entry point most demos use. Schedules Estimate animation sampling, dispatches events to the active window, calls the user's `on_render` callback each frame, and (v0.7 Line 1) routes pointer/key events through the intent graph when `px_app_desc.intent_graph` is set: afforded → closure trigger, unresolved → raw fallback.

**Size:** ~19 KB

---

## Platform Backends

Each backend implements the same `px_window_*` API declared in `include/planex/window.h`. The build system auto-detects the platform (CMake `if(APPLE)`, `elseif(WIN32)`, `elseif(UNIX)`).

### `src/x11.c` — X11 backend (Linux / BSD)

The most mature backend. Features:
- XShm extension for fast blitting
- XIM for IME support
- HiDPI / Retina scaling
- Window resize handling

**Size:** ~26 KB
**Status:** ✅ Working

### `src/win32.c` — Win32 / GDI backend (Windows)

Uses GDI for blitting (BitBlt), IMM32 for IME. Compiles clean in CI (`/WX`); runtime verification on real hardware still wanted — see [PLATFORMS.md](../../../PLATFORMS.md).

**Size:** ~16 KB
**Status:** ✅ CI-verified compile; runtime user verification pending

### `src/cocoa.c` — Cocoa backend (macOS)

Uses NSWindow + NSBitmapImageRep for blitting, NSTextInputClient for IME. Code complete; untested on hardware.

**Size:** ~16 KB
**Status:** ⚠️ Code complete, untested on hardware

### `src/headless.c` — Headless backend

No window — writes BMP files. Used by CI and tests. Reads input from stdin (EOF auto-quit).

**Size:** ~5.4 KB
**Status:** ✅ Working

---

## Accessibility

### `src/a11y.c` — Accessibility query side

Implements the API declared in `include/planex/a11y.h`: roles (button, text, list, etc.), states (focused, disabled), announcements, focus management, `set_verbose`, and `px_a11y_set_value_estimate`. Platform-free and stable since v0.6.

**Size:** ~9 KB

### `src/a11y_bridge_atspi.c` — AT-SPI2 adapter (Linux)

The v0.7 Line 4 deliverable: an AT-SPI2 adapter behind the `PX_A11Y_ATSPI` compile flag. Without the flag it is a stub (`attach()` returns NULL). **Verified end-to-end in v0.8 Cross-cutting A** (2026-08-30): `scripts/verify_orca_e2e.sh` drives XTEST keys through Xvfb into `examples/a11y_orca_demo.c` (focus ring derived from the AFFORDS graph) and asserts orca's speech log against the app's stdout ground truth — observed with orca 48.1 / at-spi2-core 2.56 / Debian 13. Known limits are documented in the bridge source. Windows (UIA) and macOS (NSAccessibility) adapters follow the same shape; currently stubs — the deferral is capacity, not design.

**Size:** ~34 KB
**See:** [PLATFORMS.md](../../../PLATFORMS.md) for the verification record and the compile line

---

## Public API Headers (`include/planex/`)

| Header | What it provides |
|---|---|
| `planex.h` | Aggregate header — the 7-abstraction contract (Relation, Estimate, Closure, Perception, `px_loop`, intent compilation, `px_interaction`) |
| `app.h` | `px_app_desc` struct, `px_app_run()` function |
| `fb.h` | Framebuffer API, font loading, BMP I/O |
| `window.h` | Window creation, event polling |
| `a11y.h` | Accessibility API |

The four satellite headers stay small (< 16 KB each). `planex.h` itself is the exception — it carries the full 7-abstraction API documentation and has grown with the library (v0.8: ~66 KB); it is read as the contract document, not skimmed as a leaf header.

---

## Tests

Nine suites, all wired into CI (`ci.yml` build jobs + `static-analysis.yml` UBSan gate; `test_completeness` runs in `docs.yml`):

| Suite | What it covers |
|---|---|
| `tests/test_core.c` | Relation graph operations, Estimate derivation/animation, Closure status transitions, font fallback, undo safety (35 tests) |
| `tests/test_orthogonality.c` | The 4-shipping-abstraction orthogonality pressure test (20 tests, 1 skipped) |
| `tests/test_feedback.c` | The v0.4 Feedback API (13 tests) |
| `tests/test_v05_retire.c` | The 7 v0.5 L2 leak retires stay closed (12 tests) |
| `tests/test_v06_interaction.c` | `px_interaction` invariants + v0.6 retires (27 tests) |
| `tests/test_v07.c` | v0.7 line-by-line verification (21 tests) |
| `tests/test_v08.c` | v0.8 keyboard channel (31 tests) |
| `tests/test_v3_prototype.c` | v3 essence candidates expressible without breaking shipping API |
| `tests/test_v4_orthogonality.c` | v4 clean-room pressure test (CMake-built, not CI-enforced — proposal-stage) |

`tests/test_completeness.c` (the 68-pattern corpus check, 75 checks) is built by both systems and run via `make check-completeness`.

---

## How the modules talk to each other

```
                    user code
                       │
                       ▼
              px_app_run() (app.c)
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
   Event dispatch  Tick (sample  Render (call
   → intent        animations)   user on_render)
   routing (app.c       │              │
   + hit.c afford       │              │
   compile)             │              │
        │              │              │
        ▼              ▼              │
   Closure.trigger  Estimate         │
   (closure.c,      .advance         │
   5 stages)        (estimate.c)     │
        │              │              │
        ▼              ▼              │
   Action mutates  Relation graph    │
   Estimates       notifies          │
   (estimate.c)    dependents        │
        │         (relation.c)       │
        ▼              ▼              ▼
   Undo snapshots  Perception      Framebuffer
   (undo.c,        presents         (fb.c)
   registry-       (perception.c)     │
   checked)           │                ▼
        │              ▼           Window blit
        └──────► px_loop budgets  (x11.c / win32.c / ...)
                  the propagation
                  (feedback.c)
```

Key insight: **Relation is the communication backbone.** When an Estimate changes, it doesn't directly notify observers — it asks the Relation graph "who depends on me?" and the graph notifies them. This is why Relation is a first-class abstraction, not a hidden implementation detail. The second backbone is the **feedback budget**: `px_loop` owns the per-frame deadline that propagation must fit inside.

---

## Build dependency order

If you're modifying Planex and need to understand the build dependency order:

1. **Core abstractions** (`relation.c`, `estimate.c`, `closure.c`, `perception.c`, `feedback.c`, `hit.c`, `interaction.c`, `undo.c`) — `relation.c` is independent; the others use Relation edges; `feedback.c` additionally binds Closure+Perception.
2. **v3 prototype** (`actor.c`, `breakdown.c`) — depends on core; proposal-stage.
3. **Infrastructure** (`layout.c`, `fb.c`, `font.c`, `font_ttf.c`, `app.c`, `a11y.c`) — depends on core abstractions.
4. **Backends** (`x11.c`, `win32.c`, `cocoa.c`, `headless.c`, `a11y_bridge_atspi.c`) — depend on infrastructure and core.
5. **Public headers** declare the API surface; the implementation order doesn't matter for users.

Both build systems must list every source and every example (enforced since 2026-10 by `make check-build-sync`, Gate 13). If you're adding a new module, put it in `src/`, add it to BOTH `CMakeLists.txt` and the `Makefile`, and run `scripts/check_build_sync.sh`.

---

## See also

- [Why Four Abstractions](../canonical/why-four-abstractions.md) — design rationale
- [Roadmap Matrix](roadmap-matrix.md) — per-abstraction maturity
- [Limitations](limitations.md) — known gaps
- [ADR index](../../decisions/README.md) — architecturally significant decisions
- [API Reference](../../reference/api.md) — function-level docs
- [PLATFORMS.md](../../../PLATFORMS.md) — per-backend feature matrix
