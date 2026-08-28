# Architecture Overview

> **Applies to**: v0.4. How the source files relate to each other and to the core abstractions.

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
│  │ (aggregate)  │  │ (app loop)   │  │ (a11y API)   │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │   fb.h       │  │  window.h    │  │              │            │
│  │ (framebuf)   │  │ (window/event)│  │              │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       Core Abstractions                             │
│  (the three pillars — these are what makes Planex Planex)          │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │ relation.c   │  │ estimate.c   │  │ closure.c    │            │
│  │              │  │              │  │              │            │
│  │  Relation    │  │  Estimate    │  │   Closure    │            │
│  │  (graph)     │  │  (state+T)  │  │  (interact)  │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
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
│  │   layout)    │  │  blit, BMP)  │  │  IME, tick)  │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐  ┌──────────────┐                              │
│  │  font.c      │  │ font_ttf.c   │                              │
│  │ (bitmap font)│  │ (FreeType)   │                              │
│  └──────────────┘  └──────────────┘                              │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        Platform Backends                            │
│         (one per OS — auto-detected at build time)                 │
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐            │
│  │   x11.c      │  │   win32.c    │  │   cocoa.c    │            │
│  │  (Linux/BSD)│  │  (Windows)   │  │   (macOS)    │            │
│  └──────────────┘  └──────────────┘  └──────────────┘            │
│  ┌──────────────┐                                                   │
│  │  headless.c  │  (no window — BMP output, for CI/testing)        │
│  └──────────────┘                                                   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Core Abstractions Layer (the three pillars)

### `src/relation.c` — Relation

The graph data structure. Maintains nodes (Estimates and Closures) and edges (Relations of various kinds). Provides a query API (`px_query`, `px_has_relation`) so the runtime can answer questions like "which Estimates does this Closure depend on?"

**Size:** ~3.7 KB  
**External dependencies:** none  
**Theoretical foundation:** Sketchpad constraint graph (Sutherland 1963), Alexander's semilattice (1965)  
**See:** [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md)

### `src/estimate.c` — Estimate

State with time and uncertainty. Holds value, confidence, timestamp, and animation state. Notifies observers (via Relation's DEPENDS_ON edges) when the value changes. Auto-samples continuous animations on each tick.

**Size:** ~12.5 KB  
**External dependencies:** none  
**Theoretical foundation:** Conal Elliott's FRP, Karl Friston's predictive coding  
**Notable:** the largest core abstraction file — handles animation, derivation, observer notification

### `src/closure.c` — Closure

The 7-stage interaction unit. Holds Goal, Intent kind, Action/Perception/Evaluation function pointers, status (IDLE/RUNNING/DONE/FAILED), and feedback text. The trigger function (`px_closure_trigger`) runs the full 7-stage loop.

**Size:** ~7.4 KB  
**External dependencies:** Relation (for TRIGGERS edges)  
**Theoretical foundation:** Don Norman's 7-stage model, Winograd/Flores speech-act theory  
**Notable:** Stage 5 (Perception) is currently a no-op — see [ADR-0001](../../decisions/superseded/ADR-0001-perception-currently-noop.md)

---

## Infrastructure Layer

### `src/layout.c` — Layout helpers

Relation-driven layout. Provides `beside`, `below`, `center` helpers that build spatial Relations (`PX_REL_BESIDE`, `PX_REL_BELOW`). The actual layout computation reads the Relation graph and produces pixel coordinates.

**Size:** ~4.2 KB

### `src/fb.c` — Software framebuffer

CPU framebuffer with primitives: `fill_rect`, `draw_text`, `set_pixel`, `draw_line`. Includes an 8x16 bitmap font (ASCII only) as a fallback when FreeType is not available.

**Size:** ~9 KB  
**External dependencies:** none (the built-in font is compiled in)

### `src/font.c` and `src/font_ttf.c` — Font loading

- `font.c`: built-in bitmap font loader
- `font_ttf.c`: FreeType-based TTF/OTF loader, supports color emoji (FT_LOAD_COLOR), CJK fallback chains

`font_ttf.c` is optional — compiled only when FreeType is detected at build time.

**Size:** ~13 KB (font.c) + ~21 KB (font_ttf.c)

### `src/app.c` — Application loop

The cross-platform app loop: render, click, key, tick, IME. Holds `px_app_run()` which is the main entry point most demos use. Schedules Estimate animation sampling, dispatches events to the active window, calls the user's `on_render` callback each frame.

**Size:** ~5.5 KB

---

## Platform Backends

Each backend implements the same `px_window_*` API declared in `include/planex/window.h`. The build system auto-detects the platform (CMake `if(APPLE)`, `elseif(WIN32)`, `elseif(UNIX)`).

### `src/x11.c` — X11 backend (Linux / BSD)

The most mature backend. Features:
- XShm extension for fast blitting
- XIM for IME support
- HiDPI / Retina scaling
- Window resize handling

**Size:** ~24 KB  
**Status:** ✅ Working

### `src/win32.c` — Win32 / GDI backend (Windows)

Uses GDI for blitting (BitBlt), IMM32 for IME. Tested on Windows 10.

**Size:** ~14.6 KB  
**Status:** ✅ Working (per [PLATFORMS.md](../../../PLATFORMS.md))

### `src/cocoa.c` — Cocoa backend (macOS)

Uses NSWindow + NSBitmapImageRep for blitting, NSTextInputClient for IME.

**Size:** ~15.2 KB  
**Status:** ⚠️ Code complete, untested on hardware

### `src/headless.c` — Headless backend

No window — writes BMP files. Used by CI and tests. Reads input from stdin.

**Size:** ~4.8 KB  
**Status:** ✅ Working

---

## Accessibility

### `src/a11y.c` — Accessibility API stub

Implements the API declared in `include/planex/a11y.h`: roles (button, text, list, etc.), states (focused, disabled), announcements, focus management.

**Currently logging-only** — no bridge to actual screen readers (AT-SPI / UIAutomation / NSAccessibility). See [Limitations L9](limitations.md#l9-accessibility-is-logging-only).

**Size:** ~6.2 KB

---

## Public API Headers (`include/planex/`)

| Header | What it provides |
|---|---|
| `planex.h` | Aggregate header — includes all the others |
| `app.h` | `px_app_desc` struct, `px_app_run()` function |
| `fb.h` | Framebuffer API, font loading, BMP I/O |
| `window.h` | Window creation, event polling |
| `a11y.h` | Accessibility API |

The headers are deliberately small. Planex avoids header bloat — each header is <16 KB and includes only what's necessary.

---

## Tests

### `tests/test_core.c` — Unit tests

Tests for the three core abstractions: Relation graph operations, Estimate derivation, Closure status transitions. Run via `make test` or `ctest`.

**Size:** ~22 KB  
**Coverage:** happy paths only — error paths and edge cases need expansion

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
   to Closures    animations)   user on_render)
        │              │              │
        ▼              ▼              │
   Closure.trigger  Estimate         │
   (closure.c)      .sample          │
        │         (estimate.c)       │
        ▼              │              │
   Action mutates     │              │
   Estimates          │              │
   (estimate.c)       │              │
        │              │              │
        ▼              ▼              ▼
   Relation graph   Relation graph  Framebuffer
   notifies         notifies        (fb.c)
   dependents       dependents         │
   (relation.c)     (relation.c)       │
                                       ▼
                                  Window blit
                                  (x11.c / win32.c / ...)
```

Key insight: **Relation is the communication backbone.** When an Estimate changes, it doesn't directly notify observers — it asks the Relation graph "who depends on me?" and the graph notifies them. This is why Relation is a first-class abstraction, not a hidden implementation detail.

---

## Build dependency order

If you're modifying Planex and need to understand the build dependency order:

1. **Core abstractions** (`relation.c`, `estimate.c`, `closure.c`) depend on each other in a cycle — `closure.c` uses Relation for TRIGGERS, `estimate.c` uses Relation for DEPENDS_ON, `relation.c` is independent.
2. **Infrastructure** (`layout.c`, `fb.c`, `font.c`, `app.c`) depends on core abstractions.
3. **Backends** (`x11.c`, `win32.c`, `cocoa.c`, `headless.c`) depend on infrastructure and core.
4. **Public headers** declare the API surface; the implementation order doesn't matter for users.

The CMake build system handles this automatically. If you're adding a new module, put it in `src/` and add it to `CMakeLists.txt`.

---

## See also

- [Why Four Abstractions](../canonical/why-four-abstractions.md) — design rationale
- [Roadmap Matrix](roadmap-matrix.md) — per-abstraction maturity
- [Limitations](limitations.md) — known gaps
- [ADR index](../../decisions/README.md) — architecturally significant decisions
- [API Reference](../../reference/api.md) — function-level docs
- [PLATFORMS.md](../../../PLATFORMS.md) — per-backend feature matrix
