# Platform Support Status

> Last updated: Stage 9 (2026-08-22)
> This document tracks which features work on which backend.

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
| Accessibility (Stage 16) | ⚠️ API + logging | ⚠️ | ⚠️ API stub | ⚠️ API stub |

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
