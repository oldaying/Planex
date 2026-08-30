# UI Pattern Coverage Matrix

> **Applies to**: v0.4. **Purpose**: Before writing real application demos, systematically check which UI patterns the 4 abstractions (Relation + Estimate + Closure + Perception) can express cleanly, and which expose boundaries.
>
> **Method**: For each pattern, ask:
> 1. Can Estimate express the state?
> 2. Can Closure express the intent?
> 3. Can Relation express the dependencies?
> 4. Can Perception express the rendering?
> 5. Is the expression CLEAN (no hacks) or FORCED (needs workarounds)?
>
> **Legend**: ✅ clean | ⚠️ forced (hack/workaround) | ❌ cannot express

---

## Category A: Discrete State Manipulation (4 abstractions' home turf)

These patterns are what Planex was designed for. All should be ✅.

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Counter (inc/dec) | ✅ value | ✅ REQUEST | ✅ TRIGGERS | ✅ pure fn | ✅ | counter_4abs demo |
| 2 | Checkbox toggle | ✅ bool | ✅ REQUEST | ✅ TRIGGERS | ✅ pure fn | ✅ | trivial |
| 3 | Radio button group | ✅ selection | ✅ REQUEST | ✅ DEPENDS_ON | ✅ pure fn | ✅ | mutual exclusion via single source |
| 4 | Dropdown / select | ✅ selection | ✅ REQUEST | — | ✅ pure fn | ✅ | old demo worked |
| 5 | Text input (basic) | ✅ string value | ✅ REQUEST | — | ✅ pure fn | ✅ | needs IME for CJK |
| 6 | Form with validation | ✅ derived (all_valid) | ✅ REQUEST (submit) | ✅ DEPENDS_ON | ✅ pure fn | ✅ | old form demo worked |
| 7 | Tabs | ✅ active tab | ✅ REQUEST | ✅ state-switch | ✅ pure fn | ✅ | Relation as state machine |
| 8 | Modal dialog | ✅ visibility | ✅ PROMISE/DECLARE | — | ✅ pure fn | ✅ | async lifecycle |
| 9 | Wizard (multi-step) | ✅ step | ✅ REQUEST | ✅ DEPENDS_ON | ✅ pure fn | ✅ | old wizard demo worked |
| 10 | Todo list | ✅ items + derived count | ✅ REQUEST (add/toggle/delete) | ✅ DEPENDS_ON | ✅ pure fn | ✅ | old todo_app worked |
| 11 | Slider (value in range) | ✅ value + constraints | ✅ REQUEST | ✅ DEPENDS_ON | ✅ pure fn | ✅ | old slider demo worked |
| 12 | Button with loading state | ✅ status | ✅ PROMISE→DECLARE | — | ✅ pure fn | ✅ | async lifecycle built-in |

**Summary: 12/12 ✅.** Discrete state is Planex's strength.

---

## Category B: Animation & Time (Estimate's time dimension)

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 13 | Fade in/out | ✅ animate opacity | — | — | ✅ pure fn samples | ✅ | px_estimate_animate |
| 14 | Slide transition | ✅ animate position | — | — | ✅ pure fn | ✅ | same mechanism |
| 15 | Progress bar | ✅ animate 0→100 | ✅ PROMISE | — | ✅ pure fn | ✅ | async + animation |
| 16 | Typing animation | ✅ animate char index | — | — | ✅ pure fn | ✅ | |
| 17 | Continuous data stream (sensor) | ✅ value + confidence | — | ✅ DEPENDS_ON | ✅ pure fn | ✅ | Friston predictive coding |
| 18 | Countdown timer | ✅ animate to 0 | — | — | ✅ pure fn | ✅ | |

**Summary: 6/6 ✅.** Animation is natural in Estimate.

---

## Category C: Undo / History (Relation graph's strength)

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 19 | Undo last action | ✅ snapshot via graph | ✅ trigger + auto-snapshot | ✅ TRIGGERS query | ✅ re-render | ✅ | undo_via_graph demo |
| 20 | Redo | ⚠️ needs redo stack | ✅ trigger | ✅ TRIGGERS | ✅ re-render | ⚠️ | Planex has undo but NOT redo yet — needs reverse stack |
| 21 | Time travel (jump to any point) | ⚠️ needs full history | ✅ serialized intent | ✅ intent log | ✅ re-render | ⚠️ | Intent is value (serializable) but no time-travel API |
| 22 | Branching history (fork) | ❌ no fork abstraction | ✅ serialized intent | ❌ no fork relation | ✅ re-render | ❌ | Trace has branch_point marker but no real fork |
| 23 | Collaborative editing (multi-user) | ❌ no multi-writer | ❌ no conflict resolution | ❌ no CRDT | ❌ | ❌ | completely out of scope |

**Summary: 1/5 ✅, 2 ⚠️, 2 ❌.** Undo works; redo/time-travel/fork/collaboration need more.

---

## Category D: Continuous / Transient Interaction (re-scored at v0.7)

These patterns involve state that is temporary, continuous, or contextual. **Re-scored at v0.7** after the promotion of intent compilation (ADR-0017) and the interaction process (ADR-0018): 7/15 are now clean — the transient/continuous patterns that the 4/5-abstraction set forced into Estimate are expressible as region queries, compiled pointer intents, and inert-trajectory processes. This table is the v0.4-era snapshot updated per the corpus amendment; the corpus (`reference/ui-pattern-corpus.md`) is the canonical verdict set.

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 24 | Hover highlight | — (no estimate needed) | — | ✅ region query | ✅ computed at render time | ✅ | hover = region query at render time — no estimate churn (ADR-0017; hover_drag_interaction.c) |
| 25 | Mouse cursor position | — (ambient sample field) | — | — | ✅ derived on demand | ✅ | pointer is a plain sample stream, read at render time (ADR-0017; hover_drag_interaction.c) |
| 26 | Pressed button visual | ✅ phase via publish_phase | ✅ commit/cancel triggers | — | ✅ renders from phase | ✅ | press-release = BEGAN→COMMITTED arc, transitions only (ADR-0018; hover_drag_interaction.c) |
| 27 | Drag preview (ghost image) | — (derived from trajectory) | — | — | ✅ derived per frame | ✅ | preview computed from trajectory, zero writes while dragging (ADR-0018) |
| 28 | Drag-drop reorder | ✅ committed state | ✅ drag end = commit | ✅ TRIGGERS + AFFORDS | ✅ | ✅ | trajectory + commit closure + undo through the graph (ADR-0018) |
| 29 | Swipe gesture (touch) | — | ✅ commit payload | — | — | ⚠️ | derivable from velocity/displacement measures; touch input is NG-6 (ADR-0018) |
| 30 | Pinch-to-zoom | ❌ multi-touch | ❌ | — | ❌ | ❌ | simultaneous trajectories + arbitration missing (NG-6) |
| 31 | Tooltip on hover (delayed) | ⚠️ timer hack | — | — | ⚠️ | ⚠️ | hover is clean now; the time-delay dimension still needs on_tick hacks |
| 32 | Context menu (right-click) | ✅ menu state | ✅ compiled REQUEST | ✅ AFFORDS | ✅ | ✅ | button-3 compiles to px_pointer_intent; region label embedded (ADR-0017; palette_afford.c) |
| 33 | Autocomplete suggestions | ⚠️ suggestions as Estimate | ✅ PROMISE (async fetch) | ✅ DEPENDS_ON | ⚠️ needs selection state | ⚠️ | async list + temporary selection — doable but forced |
| 34 | Infinite scroll | ⚠️ page as Estimate | ✅ PROMISE (fetch next) | — | ⚠️ needs scroll position | ⚠️ | scroll position is transient + continuous |
| 35 | Resizable panel (drag handle) | ✅ size | ✅ drag end = commit | ✅ AFFORDS possible | ✅ | ⚠️ | drag mechanism exists (ADR-0018) but drag-begin affordance seam + no demo = forced |
| 36 | Color picker (drag slider) | ✅ committed value | ✅ commit closure | ✅ AFFORDS region | ✅ live preview derived | ✅ | live preview from trajectory, one committed write (ADR-0017 + 0018; palette_afford.c) |
| 37 | Knob / rotary control | ✅ value | ✅ commit | ✅ AFFORDS possible | ✅ | ⚠️ | rotary = drag process + app-side angle math — derivable, undemonstrated (ADR-0018) |
| 38 | Scroll position | ⚠️ can be Estimate but updates per-frame | — | — | ⚠️ needs scroll | ⚠️ | high-frequency, transient; wheel events landed (v0.6), no scroll abstraction |

**Summary: 7/15 ✅, 7 ⚠️, 1 ❌ (v0.7 re-score; was 0/12/3).** The boundary zone moved: what the 4/5-abstraction set forced into Estimate is now split between region queries (hover/position), compiled intents (context menus, slider commits), and inert-trajectory processes (drag previews, reorder, pressed states). What remains is multi-touch (NG-6), timing-delay composites, and scroll-position transients.

---

## Category E: Layout & Spatial

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 39 | Fixed layout | — | — | ✅ BESIDE/BELOW | ✅ pure fn | ✅ | px_layout helpers |
| 40 | Responsive layout (resize) | ⚠️ window size as Estimate | — | — | ✅ pure fn | ⚠️ | window size changes continuously |
| 41 | Flexbox / grid | — | — | ⚠️ needs constraint solver | ✅ pure fn | ⚠️ | Planex has simple layout, not full solver |
| 42 | Absolute positioning | — | — | — | ✅ pure fn | ✅ | just use coordinates |
| 43 | Z-order / stacking | ⚠️ z-index as Estimate | — | — | ⚠️ needs sort | ⚠️ | stacking order is implicit, not relational |
| 44 | Clipping / overflow | — | — | — | ⚠️ needs clip rect | ⚠️ | no clip abstraction in fb |

**Summary: 2/6 ✅, 4 ⚠️.** Layout is partially covered; complex layout needs work.

---

## Category F: Async & External Data

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 45 | Fetch data (async) | ✅ status (PROMISE) | ✅ PROMISE→DECLARE | — | ✅ pure fn | ✅ | built-in lifecycle |
| 46 | Polling (interval) | ✅ status | ✅ PROMISE (repeated) | — | ✅ pure fn | ⚠️ | needs on_tick + manual repeat |
| 47 | WebSocket (real-time) | ⚠️ can update Estimate | ⚠️ no event stream | — | ✅ pure fn | ⚠️ | incoming events need stream abstraction |
| 48 | File upload progress | ✅ progress Estimate | ✅ PROMISE | — | ✅ pure fn | ✅ | |
| 49 | Search debouncing | ⚠️ timer as Estimate | ✅ REQUEST (debounced) | — | ✅ pure fn | ⚠️ | debounce is a timer hack |
| 50 | Optimistic update | ✅ value + confidence | ✅ DECLARE then rollback | ✅ undo-via-graph | ✅ pure fn | ✅ | confidence + undo = natural fit |
| 51 | Error retry | ✅ retry_count Estimate | ✅ PROMISE→FAIL→retry | — | ✅ pure fn | ✅ | |
| 52 | Offline mode | ⚠️ online status as Estimate | — | — | ⚠️ needs status indicator | ⚠️ | network status is external |

**Summary: 4/8 ✅, 4 ⚠️.** Async is mostly covered; streaming/polling need work.

---

## Category G: Multi-window / Multi-context

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 53 | Multi-window sync | ❌ no cross-window | — | ❌ no cross-window | ❌ | ❌ | each window has own Perception, no sync |
| 54 | Split pane | ⚠️ split ratio as Estimate | — | — | ⚠️ needs two render areas | ⚠️ | single Perception renders whole fb |
| 55 | Popup window | ⚠️ popup as Estimate | ✅ REQUEST | — | ⚠️ needs second window | ⚠️ | no multi-window Perception |
| 56 | Notification toast | ✅ toast_list Estimate | ✅ EXPRESS | ✅ DEPENDS_ON | ✅ pure fn | ✅ | transient list + auto-dismiss |
| 57 | Global state (Redux-like) | ✅ Estimate = global store | — | ✅ DEPENDS_ON | ✅ pure fn | ✅ | Estimate is already global |

**Summary: 2/5 ✅, 2 ⚠️, 1 ❌.** Multi-window is a gap.

---

## Category H: Accessibility & Multi-denotation

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 58 | Screen reader (a11y) | — | — | — | ✅ separate Perception | ✅ | multi_perception demo |
| 59 | Test snapshot | — | — | — | ✅ separate Perception | ✅ | pure fn = testable |
| 60 | High contrast mode | ⚠️ theme as Estimate | — | — | ✅ conditional Perception | ⚠️ | theme shouldn't be Estimate |
| 61 | Keyboard navigation | ⚠️ focus as Estimate | ✅ key handler | — | ⚠️ needs focus visual | ⚠️ | focus is transient, not state |
| 62 | Reduced motion | ⚠️ preference as Estimate | — | — | ⚠️ conditional animate | ⚠️ | system preference, not app state |
| 63 | ARIA live region | — | ✅ EXPRESS | — | ✅ a11y Perception | ✅ | Closure feedback → a11y Perception |

**Summary: 3/6 ✅, 3 ⚠️.** Multi-denotation is Planex's differentiator.

---

## Category I: Extension & Programmability

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 64 | Plugin / extension | ❌ no extension API | ❌ no plugin Closure | ❌ no dynamic graph | ❌ | ❌ | Planex is a library, not a platform |
| 65 | User scripting | ❌ no scripting | ❌ no user Closure | ❌ | ❌ | ❌ | Layer 6 (medium) — not in scope |
| 66 | Theme system | ⚠️ theme as Estimate | — | — | ⚠️ conditional Perception | ⚠️ | theme is config, not state |
| 67 | Internationalization | ⚠️ locale as Estimate | — | — | ⚠️ localized Perception | ⚠️ | i18n is config, not state |
| 68 | Custom widgets | ✅ emerge from 4 abs | ✅ | ✅ | ✅ | ✅ | this is the thesis |

**Summary: 1/5 ✅, 2 ⚠️, 2 ❌.** Extension/programmability is a gap (Layer 6).

---

## Grand Summary

| Category | ✅ Clean | ⚠️ Forced | ❌ Cannot | Total |
|---|---|---|---|---|
| A: Discrete state | 12 | 0 | 0 | 12 |
| B: Animation & time | 6 | 0 | 0 | 6 |
| C: Undo & history | 1 | 2 | 2 | 5 |
| D: Continuous/transient interaction | 0 | 12 | 3 | 15 |
| E: Layout & spatial | 2 | 4 | 0 | 6 |
| F: Async & external data | 4 | 4 | 0 | 8 |
| G: Multi-window | 2 | 2 | 1 | 5 |
| H: Accessibility | 3 | 3 | 0 | 6 |
| I: Extension | 1 | 2 | 2 | 5 |
| **Total** | **31** | **29** | **8** | **68** |

---

## What this reveals

### 1. Planex's strength: discrete state (Category A) + animation (B) + a11y (H)

38/68 patterns are cleanly expressible (v0.7 re-score; was 31/68 at v0.5). These are all in Planex's design center — discrete state manipulation, animation, and multi-denotation.

### 2. Planex's boundary: continuous/transient interaction (Category D)

**7/15 patterns in Category D are clean (v0.7 re-score; was 0/15).** The 7 clean patterns are the ones the two v0.7 abstractions own: hover/position as region queries (ADR-0017), pressed/drag-preview/reorder/slider as inert-trajectory processes (ADR-0018), context menus and compiled intents as afford-routed values (both). The remaining 8 are still forced or impossible — multi-touch (NG-6), timing-delay composites, scroll-position transients.

The historical root cause (v0.4-era): **hover, drag, gesture, scroll are continuous processes, not discrete states.** Forcing them into Estimate was semantically wrong — Estimate is "state with time + uncertainty", not "high-frequency transient input stream." The v0.7 promotions are the structural fix: process is now an abstraction (trajectory + outcome), so the forcing is no longer needed where a bounded process exists.

This validates the `continuous-intent-speculation.md` observation: **intent is modeled as discrete events, but real interaction is continuous** — the continuous half now has canonical machinery; the *gradient* half (intent strength, decay) remains speculation.

### 3. Planex's secondary gaps

- **Undo redo** (Category C): undo works, redo/time-travel/fork don't
- **Multi-window** (Category G): no cross-window sync
- **Extension** (Category I): no plugin/scripting (Layer 6)

### 4. What doesn't need fixing

- **Layout** (Category E): mostly forced but workable — v0.4 can improve incrementally
- **Async** (Category F): mostly clean — Closure's PROMISE/DECLARE handles it
- **a11y** (Category H): Perception's multi-denotation is the right answer

---

## Implications for v0.4

### Priority 1: Acknowledge the continuous interaction boundary

The 15 forced/impossible patterns in Category D are Planex's biggest limitation. Two options:

1. **Document as known limitation** (like L11 multi-frame process) — honest, defer to v1.0+
2. **Add a 5th abstraction for continuous processes** — ambitious, but this is the "intent is continuous" question from continuous-intent-speculation.md

### Priority 2: Add redo to undo-via-graph

Category C shows redo is ⚠️ — undo works but redo doesn't. This is a small addition (reverse stack) but completes the undo story.

### Priority 3: Write a demo that exposes the biggest boundary

Based on this matrix, the most revealing demo would be **hover + drag reorder** (patterns 24+28) — it would expose the continuous interaction boundary most clearly.

### What NOT to prioritize

- Multi-window sync (❌) — too complex for v0.4, defer to v1.0+
- Extension/scripting (❌) — Layer 6, explicitly out of scope
- Complex layout (⚠️) — workable with hacks, improve incrementally

---

## See also

- [continuous-intent-speculation.md](../speculation/continuous-intent-speculation.md) — the theoretical root of Category D's gap
- [Limitations L11](limitations.md) — multi-frame interaction not abstracted
- [Limitations L12](limitations.md) — continuous interaction processes not abstracted (this matrix confirmed it)
- [UI Pattern Corpus](../../reference/ui-pattern-corpus.md) — the closed, versioned corpus distilled from this matrix; `tests/test_completeness.c` enforces its invariants in CI
- [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md) — undo works (Category C pattern 19)
- [ADR-0005](../../decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md) — Perception handles Category H
- [v0.4 Roadmap](../history/v0.4-roadmap.md) — superseded (v0.4–v0.6 shipped); retained in `history/`
