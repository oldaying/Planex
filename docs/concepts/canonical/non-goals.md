# Non-Goals

> **Applies to**: v0.4 (and likely all future v0.x / v1.x). Things Planex deliberately does **not** aim to do. Documented here so they can be referenced when community pressure pushes the project to add them.

Non-goals are not weaknesses. They are the **negative space** that defines what Planex is. A project without non-goals is a project without identity — it eventually tries to be everything and ends up being nothing. (See: Eve, Subtext, and other research projects that died from scope creep.)

Each non-goal here has an ADR recording the decision.

---

## NG-1: AI integration

Planex does not integrate AI in any form — not as a runtime component, not as a built-in feature, not as a recommended pattern.

Intent-as-value is justified by **non-AI** benefits: serialization, replay, undo, audit. These are independently valuable.

**ADR:** [ADR-0003](../../decisions/accepted/ADR-0003-no-ai-integration.md)

---

## NG-2: Replace React / SwiftUI / Flutter as a general-purpose UI library

Planex is **not** a drop-in replacement for React in web applications. It does not target the browser DOM, does not aim for feature parity with React's ecosystem, and does not compete for the same use cases.

Planex is a research-grade exploration of whether UI can be built on different abstractions. It may inform future libraries; it does not aim to displace current ones.

This is the hardest non-goal to hold. The temptation to broaden scope is constant. We resist it because broadening scope = death for research projects (see Eve's postmortem).

---

## NG-3: Web / browser backend

There is no Planex backend that renders to HTML/CSS/DOM. The current backends are X11, Win32 GDI, Cocoa, and headless BMP output. A WebGPU or canvas backend may be added later, but not a DOM backend.

DOM is a layout model, not a rendering target. Planex renders directly to pixel buffers, which is architecturally incompatible with DOM's box model.

---

## NG-4: Backward compatibility before v1.0

Planex is pre-1.0. The API **will break** between minor versions as the abstractions are refined (see ADR-0001 on [Perception](../../reference/glossary.md#perception), ADR-0002 on [Relation](../../reference/glossary.md#relation) necessity).

Do not build production systems against Planex v0.x. The library is for:
- Researchers studying UI abstractions
- Embedded developers who can pin a specific commit
- UI library authors looking for prior art

This will change at v1.0, which will not happen until the matrix in `roadmap-matrix.md` is entirely green.

---

## NG-5: Component library / widget set

Planex does not ship a "components" directory with `Button`, `Input`, `Select`, `Modal`, etc. The thesis is that widgets **emerge** from the four abstractions — see `docs/concepts/canonical/why-four-abstractions.md`.

The `examples/` directory contains reference implementations of common widgets. These are demos, not a shipped component library. Copy them into your project if you want; they're MIT.

---

## NG-6: Mobile (iOS / Android) first-class support

iOS and Android are not first-class targets. Planex targets desktop (X11 / Win32 / Cocoa) and embedded (headless / framebuffer). Mobile would require:
- Touch-first input model (Planex currently assumes pointer + keyboard)
- Mobile GPU backends (Metal / Vulkan ES)
- Mobile lifecycle (backgrounding, restoration)

These are real engineering projects, not in Planex's scope. The Cocoa backend compiles on iOS but is untested.

---

## NG-7: GPU-accelerated rendering (for now)

Current rendering is software rasterization (CPU framebuffer → blit). A GPU backend (Vulkan / Metal / D3D12 / WebGPU) is on the roadmap but **not before** the abstraction questions (ADR-0001, ADR-0002) are resolved.

GPU work before abstractions are settled would be premature optimization. We follow Zig's "refuse to ship 1.0" discipline here.

---

## NG-8: Styling / theming system

Planex has no CSS-like styling layer, no theme engine, no design token system. Colors, fonts, sizes are set programmatically. A styling layer may be added as a separate library on top of Planex, but is not part of core.

---

## NG-9: Internationalization beyond rendering

Planex renders CJK text, color emoji, and supports IME input. It does not:
- Translate strings
- Format dates / numbers / currencies per locale
- Handle bidirectional text layout (RTL languages)
- Manage plural forms

Use a dedicated i18n library for these.

---

## NG-10: Animation / physics engine

Planex's `px_estimate_animate()` provides linear interpolation between values. It does not provide:
- Spring physics
- Keyframe animation curves
- Choreography / sequencing primitives
- SVG path animation

These can be built on top of [Estimate](../../reference/glossary.md#estimate). They are not in core.

---

## NG-11: Networking / async I/O

Planex has no built-in networking, no async I/O, no event loop integration with libuv / Tokio / asyncio. Closures that need to perform network I/O do so through user-supplied code in the `action` function pointer; Planex provides the `promise / declare / fail` lifecycle for tracking async state but does not provide the async runtime.

---

## NG-12: Multi-window / multi-process UI

Planex assumes one process, one main window. Multi-window applications are supported (you can create multiple `px_window`s) but there is no IPC, no process-per-window model, no Wayland-style compositor abstraction.

---

## What this means for contributors

If you're considering a contribution, ask:

1. **Does it touch the four abstractions?** If yes, it probably belongs in core — but write an ADR first.
2. **Is it a new widget?** Add it to `examples/`, not core. Document how it emerges from the four abstractions.
3. **Is it a new backend?** Yes, please — but only if it doesn't compromise abstraction questions.
4. **Is it AI / mobile / styling / networking / animation?** Out of scope. Fork the project, don't pull-request.

See [CONTRIBUTING.md](../../../CONTRIBUTING.md) for the practical process.
