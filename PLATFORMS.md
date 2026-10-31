# Platform Support Status

> Last updated: v0.8 (2026-08-30)
> This document tracks which features work on which backend.

## Accessibility (v0.7 Line 4 — verified end-to-end in v0.8 Cross-cutting A)

The v0.6 query-side contract (getters, announcement ring, `set_verbose`, `px_a11y_set_value_estimate`) is stable and platform-free. On top of it, v0.7 ships `src/a11y_bridge_atspi.c` — an AT-SPI2 adapter for Linux:

- **Build:** `make CFLAGS_EXTRA="-DPX_A11Y_ATSPI $(pkg-config --cflags atk atk-bridge-2.0)"` (needs `libatk1.0-dev` + `libatk-bridge2.0-dev`; the source includes `<atk-bridge.h>` bare — pkg-config resolves the distro-dependent header layout, found by the CI probe's first real compile). The CI `a11y-atspi-bridge` job compile-probes this path on every push. **The full compile line — every src translation unit — is owned by `scripts/verify_orca_e2e.sh`.**
- **Without the flag:** the bridge is a stub — `attach()` returns NULL, everything else is unchanged. No dependency, no regression.
- **Verified (v0.8 Cross-cutting A, 2026-08-30): the observed orca pass on x11** — `scripts/verify_orca_e2e.sh` drives XTEST keys through Xvfb into a Planex app (`examples/a11y_orca_demo.c`) whose focus ring is derived from the AFFORDS graph, and asserts orca's speech log (window, focus moves with values, activation announcements) against the app's stdout ground truth. Observed with orca 48.1 / at-spi2-core 2.56 / Debian 13. Known limits (documented in the bridge source, not hidden): values ride in the accessible description; region objects materialize on focus; the app must flush() regularly; detach leaves the app registered on the accessibility bus until process exit (the GTK exit semantic — atk-bridge's cleanup path is unsafe for non-GTK embedders with a live client).
- **Windows (UIA) / macOS (NSAccessibility):** follow the same adapter pattern — the AT-SPI2 shape is now proven end-to-end; the deferral is capacity, not shape. Currently stubs.

## Backend Maturity

| Backend | Compiles | Runs | CI Status | User Verified |
|---|---|---|---|---|
| **X11** | ✅ | ✅ | [![CI](https://github.com/oldaying/Planex/actions/workflows/ci.yml/badge.svg)](https://github.com/oldaying/Planex/actions/workflows/ci.yml) | ✅ Maintainer (Xvfb) |
| **Headless** | ✅ | ✅ | ✅ CI | ✅ Maintainer |
| **Win32** | ✅ CI | ❓ | ✅ CI compiles | ❓ Need user |
| **Cocoa** | ❓ CI runs | ❓ | ⚠️ CI unstable | ❓ Need user |

## Feature Matrix per Backend

| Feature | X11 | Headless | Win32 | Cocoa |
|---|---|---|---|---|
| Window creation | ✅ | ✅ (BMP only) | ✅ code | ✅ code |
| Mouse events | ✅ | stdin | ✅ code | ✅ code |
| Keyboard (ASCII) | ✅ | stdin | ✅ code | ✅ code |
| **IME (CJK input)** | ✅ Stage 9 (XIM) | ❌ | ✅ Stage 14 (IMM32) | ✅ Stage 14 (NSTextInputClient) |
| **CJK font rendering** | ✅ Stage 10 (FreeType + TTF) | ✅ | ✅ code | ✅ code |
| **Color emoji** | ✅ Stage 12 (FT_LOAD_COLOR + BGRA) | ✅ | ✅ code | ✅ code |
| **Fallback font chain** | ✅ Stage 11 (4 fonts max) | ✅ | ✅ code | ✅ code |
| **Font discovery (fontconfig)** | ✅ Stage 13 | ✅ | ⚠️ needs mingw fontconfig | ⚠️ needs brew fontconfig |
| Resize | ✅ | N/A | ✅ code | ✅ code |
| Animation (60fps) | ✅ | ✅ | ✅ code | ✅ code |
| Derived state | ✅ all backends | ✅ | ✅ | ✅ |
| BMP output | ✅ | ✅ | ❓ untested | ❓ untested |
| XShm optimization | ✅ | N/A | N/A | N/A |
| Clipboard | ❌ | ❌ | ❌ | ❌ |
| Drag & drop | ❌ | ❌ | ❌ | ❌ |
| HiDPI / Retina | ✅ Stage 15 | N/A | ✅ Stage 15 code | ✅ Stage 15 code |
| Accessibility (Stage 16) | ✅ AT-SPI2 bridge behind `PX_A11Y_ATSPI` (v0.7) + query side | ⚠️ query side | ⚠️ API stub | ⚠️ API stub |

## Demos by Backend

### Stdout demos (23) — backend-free by construction

These exercise the abstraction API without a window (text on stdout), so they build and run identically on every backend and in every CI job. All 23 are in BOTH build systems (enforced by Gate 13, `make check-build-sync`).

| Demo | What it exercises |
|---|---|
| `counter_4abs` | Canonical multi-abstraction hello world |
| `multi_perception` | Why Perception is a separate abstraction |
| `perception_smoke` | Perception Phase 1 API validation (9 tests) |
| `perception_phase2` | Perception Phase 2 runtime validation (7 tests) |
| `undo_via_graph` | ADR-0002 Relation-necessity proof (7 tests) |
| `antipattern_estimate` | Why Estimate is necessary (3 anti-patterns) |
| `antipattern_closure` | Why Closure is necessary (5 anti-patterns) |
| `antipattern_perception` | Why Perception is necessary (4 anti-patterns) |
| `counter_denotative` | Denotative-route single-state prototype (4 tests) |
| `calculator_denotative` | Denotative-route multi-state prototype (4 tests) |
| `editor_meaning` | Phenomenological-school prototype |
| `hover_drag_interaction` | `px_interaction` regression demo (ADR-0018) |
| `semantic_drift_probe` | ADR-0023: mid-flight semantic reinterpretation (the B→C stress test) |
| `palette_afford` | v0.7 L1: afford-routed app, zero raw-coordinate clicks |
| `designer_tools` | v0.8 L2: drag-begin afford — drags decided by the graph |
| `animation_demo` | Time as a first-class dimension |
| `async_demo` | PROMISE/DECLARE/FAIL intent lifecycle |
| `confidence_demo` | The confidence field in real use |
| `integration_4abs` | All abstractions working simultaneously |
| `v3_prototype_actor` | v3: 3-place Relation (actor parameter) |
| `v3_prototype_perlocution` | v3: Searle level-3 effect-of-utterance |
| `v3_prototype_interpretant` | v3: Peirce interpretant gap |
| `v3_prototype_breakdown` | v3: Zuhandenheit/breakdown (prototype, not admitted) |

### Windowed demos (4) — need a real backend

| Demo | X11 | Headless | Win32 | Cocoa | Notes |
|---|---|---|---|---|---|
| `counter_perception_window` | ✅ | ✅ headless mode | ✅ code | ✅ code | Phase 2 capstone — perception-driven window |
| `counter_interactive` | ✅ | ✅ headless mode | ✅ code | ✅ code | Denotative route with a real window (2 tests) |
| `hover_drag_4abs` | ✅ | ✅ headless mode | ✅ code | ✅ code | Boundary demo: hover+drag (ADR-0006) |
| `a11y_orca_demo` | ✅ (needs AT-SPI2 + orca) | ⚠️ query side only | ⚠️ API stub | ⚠️ API stub | v0.8 Cross-cutting A evidence app; driven end-to-end by `scripts/verify_orca_e2e.sh` |

("✅ code" = compiles against that backend, runtime verification wanted — see [How to help](#how-to-help) below.)

The demo table previously listed counter/todo/slider/radio/dropdown/tabs/checkbox/form/wizard/modal, counter_fb/slider_fb, nine `*_x11` windowed demos, animate_x11, perf_x11 and text_input_ime — the v0.3 demo set, deleted in the v0.3 refactor. None of those files exist; the changelog records that a slider demo never existed in this repository at all.

## What's needed for full coverage

- [ ] Win32 backend runtime tested on real Windows (compiles clean under CI `/WX`; only the runtime is unverified)
- [ ] Cocoa backend runtime tested on real macOS
- [ ] IME runtime verification on Win32 (IMM32, Stage 14 — code-complete) and Cocoa (NSTextInputClient, Stage 14 — code-complete); X11 XIM is verified (Stage 9)
- [ ] Cross-platform clipboard
- [ ] Drag & drop
- [ ] UIA adapter (Windows) and NSAccessibility adapter (macOS) — the AT-SPI2 shape is proven end-to-end; the deferral is capacity, not design

Delivered since this list was last curated (do not re-add): HiDPI/Retina (Stage 15, all backends), AT-SPI2 on Linux (v0.7 Line 4, orca-verified v0.8).

## How to help

If you have Windows or macOS, please:

1. Clone: `git clone https://github.com/oldaying/Planex.git`
2. Build: `make BACKEND=win32` (or `cocoa CC=clang`)
3. Run: `./build/counter_4abs` (stdout, backend-free) or `./build/counter_perception_window` (windowed)
4. Report: open an issue with "Backend verification report" template

Even "works for me on Windows 11" comments help us mark the matrix.
