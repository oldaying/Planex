# Planex Roadmap — Matrix View

> Replaces the flat Stage 0-19 list. Each row = one abstraction. Each column = one maturity dimension.  
> **Updated for v0.3: matrix is now ALL GREEN.**

This view is borrowed from research-grade projects that survived (seL4, Lean, Raph Levien's Xilem, Zig): track **per-abstraction completeness**, not stage count.

---

## The Matrix (v0.3 — all green)

| Abstraction | Theory | Proof-of-concept | Engineering | Docs | Anti-pattern test |
|---|---|---|---|---|---|
| **Relation** | ✅ Sketchpad + Alexander | ✅ undo-via-graph (7 tests) | ✅ auto-dep tracking + TRIGGERS query | ✅ written | ✅ `antipattern_estimate` references Relation |
| **Estimate** | ✅ Conal FRP + Friston | ✅ animation + derived (auto-update) | ✅ auto-sampling + dynamic derived | ✅ written | ✅ `antipattern_estimate` (3 anti-patterns) |
| **Closure** | ✅ Norman + Winograd/Flores | ✅ 5-stage + promise/declare/fail | ✅ status machine + undo binding | ✅ written | ✅ `antipattern_closure` (5 anti-patterns) |
| **Perception** | ✅ Conal denotative + Norman stages 5-6 | ✅ multi-denotation + pure fn + selective invoke | ✅ Phase 2 runtime + perception-driven window | ✅ written | ✅ `antipattern_perception` (4 anti-patterns) |

**All 20 cells green.** This is the v1.0 minimum bar — every abstraction is proven necessary and implemented.

---

## What changed since v0.1

### Perception: entirely red → entirely green

v0.1 had Perception as a no-op placeholder in `closure.c:131`. ADR-0001 recorded this gap. ADR-0005 (v0.2) promoted Perception to the 4th first-class abstraction. Phase 1 (v0.2) added the API. Phase 2 (v0.3) implemented the runtime + perception-driven window.

### Relation: Proof + Anti-pattern green

v0.1 had Relation's necessity unproven. v0.3 implemented undo-via-graph (`examples/undo_via_graph.c`, 7 tests pass), proving Relation is necessary — Solid.js cannot do scoped undo because it tracks dependencies per-effect, not as a globally queryable graph.

### Anti-pattern column: all green

3 new demos (`antipattern_estimate`, `antipattern_closure`, `antipattern_perception`) provide 12 anti-pattern arguments proving each abstraction is necessary, not just elegant.

---

## Demo → Abstraction Mapping (v0.3)

### Tier 1 — Canonical demos (in build)

| Demo | Abstraction | Capability proven |
|---|---|---|
| `counter_4abs` | All four | 4-abstraction hello world |
| `multi_perception` | Perception | 4 denotations of same Estimate |
| `perception_smoke` | Perception | Phase 1 API (9 tests) |
| `perception_phase2` | Perception | Phase 2 runtime (7 tests) |
| `undo_via_graph` | Relation | undo-via-graph (7 tests, ADR-0002) |
| `antipattern_estimate` | Estimate | 3 anti-patterns |
| `antipattern_closure` | Closure | 5 anti-patterns |
| `antipattern_perception` | Perception | 4 anti-patterns |

### Tier 2 — Prototype demos (in build, migrated to new API)

| Demo | Abstraction | Capability proven |
|---|---|---|
| `counter_denotative` | Perception | (c) route single-state (4 tests) |
| `calculator_denotative` | Perception | (c) route multi-state (4 tests + 3 scenarios) |
| `counter_interactive` | Perception | (c) route real window (2 tests) |
| `editor_meaning` | All (phenomenological) | Context/Trace/Affordance on Planex |

### Tier 3 — Windowed demos

| Demo | Abstraction | Capability proven |
|---|---|---|
| `counter_perception_window` | Perception + Closure + Estimate + Relation | Phase 2 capstone: perception-driven window + undo (Z key) |

### Tier 4 — Core tests

| Demo | Coverage |
|---|---|
| `test_core` | 33/33 tests (Relation, Estimate, Closure, derived, animation, font) |

**Total: 78 tests, all passing.**

---

## v0.4 Roadmap

With the matrix all green, v0.4 focuses on **breadth and robustness**, not new abstractions.

### Phase 1: Real application demo (highest priority)

The matrix proves each abstraction is necessary. But it hasn't proven they work together in a **real application** (not just counter/calculator). 

**Goal**: write a demo that exercises all 4 abstractions in a production-like scenario.

Candidate: a simple text editor or calculator GUI with:
- Multiple Closure types (edit, save, undo, redo)
- Derived Estimates (word count, modified flag)
- Multiple Perceptions (screen + a11y + save format)
- Undo-via-graph in real window

**Estimated effort**: 1-2 weeks.

### Phase 2: HiDPI / font support

Current bitmap font (8×16 ASCII) doesn't scale on HiDPI displays (125%/150%). Text appears too small or truncated.

**Goal**: integrate FreeType TTF font rendering as default (not optional), with scale-aware sizing.

**Estimated effort**: 1 week.

### Phase 3: More widget demos (new API)

Recreate slider, form, tabs, modal, todo_app using the new 4-abstraction API. These were deleted in the v0.3 redesign.

**Goal**: prove the 4 abstractions can express all common widget types.

**Estimated effort**: 1 week (each widget ~1 day).

### Phase 4: v1.0 preparation

When Phase 1-3 are done:
- Write v1.0 release notes
- Public announcement (HN / Reddit / Twitter)
- API freeze (no more breaking changes after v1.0)

---

## Cross-domain patterns applied

| Source | Pattern applied |
|---|---|
| seL4 | Proof-as-spine (each abstraction has existence proof + anti-pattern) |
| Lean | Theory-ecosystem dual-axis tracking |
| Raph Levien (Xilem) | Theory → Principle → Architecture → Implementation layers |
| Zig | Refuse to ship 1.0 until core is proven (matrix all green = bar met) |
| Yjs/Automerge | Narrow scope = survival |
| Conal Elliott | Denotative path — Perception as pure function |

---

## How to use this matrix

- Every commit should answer: **which cell did I just turn green?**
- If a commit doesn't turn any cell green, it's engineering polish — fine, but don't pretend it's progress on the abstractions.
- A research-grade project is "done" when the matrix is full. **v0.3 reached this bar.**
