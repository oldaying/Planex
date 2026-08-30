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

| Demo | X11 | Headless | Win32 | Cocoa |
|---|---|---|---|---|
| counter, todo, slider, radio, dropdown, tabs, checkbox, form, wizard, modal | stdout only | ✅ | ✅ | ✅ |
| counter_fb, slider_fb | BMP output | ✅ | ❓ | ❓ |
| counter_x11 ... resize_x11 (9 windowed) | ✅ | ✅ headless mode | ❓ | ❓ |
| animate_x11 | ✅ | ✅ | ❓ | ❓ |
| perf_x11 | ✅ 55fps@800x600 | ✅ | ❓ | ❓ |
| text_input_ime (Stage 9) | ⚠️ XIM only | ❌ | ❌ | ❌ |

## What's needed for full coverage

- [ ] Win32 backend runtime tested on real Windows
- [ ] Cocoa backend runtime tested on real macOS
- [ ] IME on Win32 (TSF) and Cocoa (NSTextInputClient)
- [ ] Cross-platform clipboard
- [ ] HiDPI/Retina support
- [ ] Accessibility (AT-SPI on Linux, UIA on Windows, AXUIElement on macOS)

## How to help

If you have Windows or macOS, please:

1. Clone: `git clone https://github.com/oldaying/Planex.git`
2. Build: `make BACKEND=win32` (or `cocoa CC=clang`)
3. Run: `./build/counter_x11` (or `.exe`)
4. Report: open an issue with "Backend verification report" template

Even "works for me on Windows 11" comments help us mark the matrix.
