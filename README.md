# Planex

> **Plane + X** — what if a UI library's core abstractions were Relation + Estimate + Closure + Perception, directly mapping UI essence's four axes?
>
> Per [ADR-0005](docs/decisions/ADR-0005-promote-perception-to-fourth-abstraction.md): Perception was promoted to a 4th first-class abstraction. Closure restructured from 7 stages to 5 stages (execution side). See [UI Essence Layers](docs/concepts/ui-essence-layers.md) for the four-axis essence model.

[![CI](https://github.com/oldaying/Planex/actions/workflows/ci.yml/badge.svg)](https://github.com/oldaying/Planex/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C Standard](https://img.shields.io/badge/C-17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))

---

## Quick Start

```bash
git clone https://github.com/oldaying/Planex.git
cd Planex
cmake -B build
cmake --build build --config Release
./build/counter_4abs                # Linux/macOS — minimal 4-abstraction demo
./build/multi_perception             # shows why Perception is a 4th abstraction
./build/perception_smoke             # 9 API surface tests
```

That's it. Zero external dependencies. All three demos run stdout-only — no window required.

---

## What is Planex?

Planex is a UI library built on **four abstractions**, each mapping directly to an axis of UI essence (see [ui-essence-layers.md](docs/concepts/ui-essence-layers.md)):

1. **Estimate** — state space (state with time + uncertainty)
2. **Relation** — state-state relationships (queryable graph, not a tree)
3. **Closure** — intent space, execution side (5 stages: Goal → Intent → Action → Execution → Evaluation)
4. **Perception** — machine → user direction (pure function denoting state — pixels, a11y, log, etc.)

Every widget emerges from these four. No pre-defined component types. No inheritance. No callbacks.

```c
#include "planex/planex.h"

px_estimate* count = px_estimate_new(0, 1.0);

/* Closure (5-stage, no perception parameter — new API per ADR-0005) */
px_closure* inc = px_closure_new(
    "increment counter", PX_INTENT_REQUEST,
    on_inc, eval_nonneg, &app);

/* Perception — pure function denoting state */
px_estimate* inputs[] = { count };
px_perception* p = px_perception_new(
    "counter_text", render_to_string, inputs, 1, NULL);

px_declare(graph, inc, PX_REL_TRIGGERS, count);
px_closure_trigger(inc, NULL, 0);
```

See [examples/counter_4abs.c](examples/counter_4abs.c) for the canonical hello world, and [examples/multi_perception.c](examples/multi_perception.c) for why Perception is a 4th abstraction.

---

## Documentation

### For users (Diátaxis framework)

| Section | What it covers |
|---|---|
| [Getting Started](docs/tutorials/getting-started.md) | From zero to your first Planex window in 10 minutes |
| [API Reference](docs/reference/api.md) | Every function: parameters, return values, usage |
| [How-to Guides](docs/how-to/) | Common tasks: buttons, inputs, lists, animations |
| [FAQ](docs/faq.md) | Frequently asked questions |
| [Changelog](docs/changelog.md) | What changed in each version |

### For researchers and contributors

| Section | What it covers |
|---|---|
| [UI Essence Layers](docs/concepts/ui-essence-layers.md) | Six-layer nested structure of UI essence; Planex implements layers 1-3 |
| [Path C Lineage](docs/concepts/path-C-lineage.md) | Planex's place in 60-year Path C history (Sketchpad → Eve → Planex) |
| [Why Four Abstractions](docs/concepts/why-four-abstractions.md) | Manifesto — why 4 abstractions map to 4 essence axes  |
| [Alternative Perspectives](docs/concepts/alternative-perspectives.md) | Four academic schools; Planex adopts Cognitive + Mathematical/Linguistic |
| [Roadmap Matrix](docs/concepts/roadmap-matrix.md) | Per-abstraction maturity matrix — tracks what's done vs. what's missing |
| [Non-Goals](docs/concepts/non-goals.md) | What Planex deliberately does NOT aim to do |
| [Limitations & Known Gaps](docs/concepts/limitations.md) | Where the README's claims exceed current implementation |
| [Decision Records (ADR)](docs/decisions/README.md) | Architecturally significant decisions, with context and alternatives |

---

## Build

```bash
cmake -B build
cmake --build build --config Release
make test    # run unit tests
```

Backends (auto-detected):

| Backend | Platform | Status |
|---|---|---|
| `x11` | Linux/BSD | ✅ Working |
| `win32` | Windows (GDI) | ✅ Working (tested on Windows 10) |
| `cocoa` | macOS | ⚠️ Code complete, untested on hardware |
| `headless` | Any (no window) | ✅ Working (BMP output) |

Optional: FreeType + fontconfig for CJK font support. Without them, Planex uses a built-in 8x16 bitmap font (ASCII only).

---

## Roadmap

The flat stage list has been replaced by a **per-abstraction maturity matrix** — a more honest view that tracks each abstraction (Relation / Estimate / Closure / Perception) across five maturity dimensions (Theory / Proof-of-concept / Engineering / Docs / Anti-pattern test).

| Abstraction | Theory | Proof | Eng | Docs | Anti-pattern |
|---|---|---|---|---|---|
| **Relation** | ✅ | 🔴 | ✅ | ✅ | 🔴 |
| **Estimate** | ✅ | ✅ | ✅ | ✅ | 🔴 |
| **Closure** | ✅ | ✅ | ✅ | ✅ | 🔴 |
| **Perception** | ✅ | ⚠️ | 🔴 Phase 2 | ✅ | 🔴 |

**Per ADR-0005 Phase 1:** Perception is now a 4th abstraction (API + registry implemented). Phase 2 will add runtime perception-driven rendering (replacing `on_render` callback).

➡️ **Full matrix with explanations, demo mapping, and next-priority decisions:** [docs/concepts/roadmap-matrix.md](docs/concepts/roadmap-matrix.md)

---

## License

MIT
