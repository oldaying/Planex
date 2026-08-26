# Changelog

All notable changes to Planex are documented here.

Format based on [Keep a Changelog](https://keepachangelog.com/).

---

## [Unreleased]

---

## [0.4.0] — 2026-08-27

### Added — Feedback as 5th essence category (ADR-0008)

- **`px_loop` abstraction** — closed-loop coupling as first-class (per essence-derivation-v2.md)
- New `src/feedback.c` (~200 lines) — binds Closure (intent side) to Perception (view side)
- New `px_loop` API: `px_loop_new`, `px_loop_free`, `px_loop_step`, `px_loop_step_view_only`
- Audit log: `px_loop_audit_count`, `px_loop_audit_get`, `px_loop_audit_clear` — each iteration records (triggered? perceived? timestamp)
- Pause/resume: `px_loop_pause`, `px_loop_resume`, `px_loop_is_paused` — interrupt the loop for batch updates / modal blocking
- Replay: `px_loop_replay(loop, n)` — re-run last n iterations for testing/debugging
- 13 new tests in `tests/test_feedback.c` — all pass
- **Closes the essence gap acknowledged in ADR-0007**. Planex now implements 5 of 5 essence categories (was 4 of 5 in v0.3).

### Added — Essence derivation v2 (ADR-0007)

- New `docs/concepts/essence-derivation-v2.md` — based on 6-tradition literature survey (2835 lines of research reports in `research/reports/`)
- v1 (`essence-derivation.md`) marked SUPERSEDED with summary of v2 corrections
- `why-four-abstractions.md` rewritten: framing changed from "4 = 4 essence axes" to "5 essence categories: 4 implemented + Feedback (now first-class in v0.4) + 4 deferred (Embodiment, Situatedness, Affordance-as-relation, Breakdown)"
- `limitations.md` updated: L13 (Feedback gap) added then resolved by ADR-0008; L14 (deferred essence candidates) added
- ADR-0007 records the revision with full Essence Check (Q1-Q5)
- ADR-0008 records the Feedback design with full Essence Check

### Changed — Version bump

- `PLANEX_VERSION` updated from `0.1.0` to `0.4.0` (the constant was stale; actual functionality has been at 0.3 since 2026-08-26)

### v0.4 essence claim (per ADR-0007 + ADR-0008)

```
Planex implements 5 of 5 essence categories:
  ✅ State                          → Estimate
  ✅ Communication (human→machine)  → Closure
  ✅ Presentation (machine→human)    → Perception
  ✅ Relational ontology             → Relation
  ✅ Feedback / closed-loop coupling → px_loop  (NEW in v0.4)

Deferred essence candidates (acknowledged, not implemented):
  - Embodiment (Dourish)
  - Situatedness (Suchman)
  - Affordance-as-relation (Gibson)
  - Breakdown (Heidegger-Winograd/Flores)
```

---

## [0.3.0] — 2026-08-26

### Added — Perception abstraction (ADR-0005)
- **Perception promoted to 4th first-class abstraction** (ADR-0005)
- New `px_perception` API: `px_perception_new`, `px_perception_free`, `px_perception_count`
- Phase 2 runtime: `px_perception_invoke_all`, `px_perception_invoke_for_estimate`
- `px_perceptions_for_estimate` — query which perceptions depend on an Estimate
- `px_app_desc.perception` field — replaces `on_render` callback (backward compatible)
- Perception-driven real window: `counter_perception_window` demo
- `multi_perception` demo — 4 denotations of same Estimate (visual + a11y + json + log)

### Added — Undo-via-graph (ADR-0002 closed)
- New `src/undo.c` — undo stack with Relation graph-driven scoping
- `px_undo_record`, `px_undo`, `px_undo_count`, `px_undo_clear`
- `px_undo_set_enabled` / `px_undo_is_enabled` — global on/off
- `px_closure_bind_graph` — bind graph to closure for auto-snapshot
- `undo_via_graph` demo — 7 tests, proves Relation is necessary
- Undo in real window: `counter_perception_window` supports Z/Ctrl+Z

### Added — Anti-pattern tests (matrix all green)
- `antipattern_estimate` — 3 anti-patterns (time + confidence + derived)
- `antipattern_closure` — 5 anti-patterns (intent + goal + eval + lifecycle + speech acts)
- `antipattern_perception` — 4 anti-patterns (multi-denotation + pure fn + lifecycle + selective)
- **Total: 12 anti-pattern arguments proving each abstraction is necessary**

### Changed — API migration (breaking, pre-v1.0)
- `px_closure_new` signature: 6-arg to 5-arg (removed `perception` parameter)
- Closure restructured: 7 stages to 5 stages (execution side only)
- `px_perception_fn` typedef removed from Closure (moved to Perception API)
- 25 legacy 3-abstraction-era demos deleted (commit c24bbcab)
- 4 prototype demos migrated to new API (counter_denotative, calculator_denotative, counter_interactive, editor_meaning)

### Changed — Documentation
- README rewritten: "3 abstractions" to "4 abstractions mapping UI essence four axes"
- New manifesto: `why-four-abstractions.md` (replaces why-three-abstractions.md)
- `ui-essence-layers.md` — six-layer nested structure of UI essence
- `path-C-lineage.md` — Planex place in 60-year Path C history
- `alternative-perspectives.md` — four academic schools (Cognitive + Mathematical/Linguistic adopted)
- `continuous-intent-speculation.md` — future-research marker
- ADR-0001 superseded by ADR-0005
- Limitations L1 updated: "Phase 1 done, Phase 2 pending"
- Roadmap matrix: all 20 cells green
- v0.4 roadmap added

### Fixed
- `layout.c` — 13 C4244 warnings eliminated (int to float conversion)
- `closure.c` — C4244 sign-compare warning eliminated
- `counter_perception_window` — text truncation on HiDPI (window widened 256 to 320)

### Stats
- 78 tests, all passing (was 42 in v0.2.0, was 25 in v0.1.0)
- 13 build targets, all passing (was 3 in v0.2.0, was 24 with failures in v0.1.0)
- Zero warnings on MSVC /W3 + gcc -Wall -Wextra
- 5 ADRs (was 5 in v0.2.0, was 0 in v0.1.0)
- 27+ documentation files (was 26+ in v0.2.0, was 11 in v0.1.0)

---

## [0.2.0] — 2026-08-25

### Added — ADR-0005: Perception promoted to 4th abstraction
- **ADR-0005 accepted**: Perception promoted from no-op placeholder to 4th first-class abstraction
- ADR-0001 superseded by ADR-0005
- New `px_perception` struct + API (`px_perception_new`, `px_perception_free`, `px_perception_count`)
- `px_closure_new` signature changed: 6-arg to 5-arg (removed `perception` parameter)
- Closure restructured: 7 stages to 5 stages (execution side only, Norman stages 1-4 + 7)
- `px_perception_fn` typedef removed from Closure, moved to Perception API
- Phase 1: API surface complete, `px_perceptions_for_estimate` is stub

### Added — Documentation overhaul
- README rewritten: "3 abstractions" to "4 abstractions mapping UI essence four axes"
- New manifesto: `why-four-abstractions.md`
- `ui-essence-layers.md` — six-layer nested structure of UI essence
- `path-C-lineage.md` — Planex place in 60-year Path C history
- `alternative-perspectives.md` — four academic schools (Cognitive + Mathematical/Linguistic adopted)
- `continuous-intent-speculation.md` — future-research marker
- ADR template updated with mandatory Essence Check (5 questions)
- Limitations L1 updated: "Phase 1 done, Phase 2 pending"
- Non-goals updated: "three abstractions" to "four abstractions"

### Changed — Demo redesign
- 25 legacy 3-abstraction-era demos deleted (commit c24bbcab)
- New canonical demos: `counter_4abs` (4-abstraction hello world), `multi_perception` (why Perception is 4th)
- `perception_smoke` — 9 Phase 1 API tests
- `examples/README.md` catalog rewritten for post-ADR-0005 era

### Changed — Test migration
- `tests/test_core.c` migrated: 5 `px_closure_new` calls updated to 5-arg signature
- 33/33 tests still passing after migration

### Fixed
- `layout.c` — 13 C4244 warnings eliminated (int to float conversion)
- `closure.c` — C4244 sign-compare warning eliminated

### Stats
- 4 abstractions (was 3 with no-op)
- 42 tests (9 Phase 1 + 33 migrated), all passing
- 3 build targets in STDOUT_DEMOS (counter_4abs, multi_perception, perception_smoke)
- Zero warnings on MSVC /W3 + gcc -Wall -Wextra
- 5 ADRs (was 0)

---

## [0.1.0] — 2026-08-22

### Added
- Three core abstractions: Relation, Estimate, Closure
- 10 widget demos proving emergence (counter, slider, radio, dropdown, checkbox, form, wizard, modal, tabs, todo)
- Framebuffer renderer with BMP output
- 8x16 bitmap font (ASCII)
- X11 window backend with XShm optimization
- Win32 (GDI) window backend — tested on Windows 10
- Cocoa (NSWindow) window backend — code complete, untested
- Headless backend (BMP output, stdin input)
- IME support: X11 (XIM), Win32 (IMM32), Cocoa (NSTextInputClient)
- TTF font rendering via FreeType (optional)
- Color emoji support (FT_LOAD_COLOR + BGRA)
- Fallback font chain (up to 4 fonts)
- Fontconfig integration (find fonts by name)
- HiDPI / Retina support (DPI scale detection)
- Accessibility API (roles, states, announce)
- Window resize handling
- 60fps animation with auto-sampling (Behavior = Time to a)
- Derived estimates with automatic dependency tracking
- Dynamic derived sources (add/remove at runtime)
- Closure auto-evaluation (failure to FAILED + feedback)
- Closure Promise/Declare/Fail (machine-initiated status)
- Relation-driven layout helpers (beside, below, center)
- CMake build system (MSVC, GCC, Clang)
- GitHub Actions CI (Linux, macOS, Windows)
- Todo app (first real application)
- Professional documentation (Diataxis structure)

### Known Limitations
- No GPU rendering (software rasterization only)
- No clipboard / drag-drop
- No preedit rendering (IME intermediate text)
- Accessibility is API + logging (no real screen reader integration)
- Cocoa backend untested on real macOS
- No mobile (iOS/Android) support
- px_estimate_observe does not support unobserve (dangling pointers on remove)
