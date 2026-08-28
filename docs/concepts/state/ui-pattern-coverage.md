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

## Category D: Continuous / Transient Interaction (the boundary zone)

These patterns involve state that is temporary, continuous, or contextual — not cleanly expressible as discrete Estimates.

| # | UI Pattern | Estimate | Closure | Relation | Perception | Verdict | Notes |
|---|---|---|---|---|---|---|---|
| 24 | Hover highlight | ⚠️ can use Estimate but semantically wrong | — | — | ⚠️ needs mouse pos | ⚠️ | hover is transient, not "state" — forcing it into Estimate is semantic stretch |
| 25 | Mouse cursor position | ⚠️ could be Estimate but updates 60fps = expensive | — | — | ⚠️ needs pos input | ⚠️ | high-frequency transient state — Estimate overkill |
| 26 | Pressed button visual | ⚠️ can use Estimate but it's transient | ✅ mouse_down/up | — | ⚠️ needs pressed flag | ⚠️ | press-release is a process, not a state |
| 27 | Drag preview (ghost image) | ⚠️ drag_state as Estimate | ✅ drag start/end | — | ⚠️ needs drag offset | ⚠️ | drag is multi-frame process, not discrete state |
| 28 | Drag-drop reorder | ⚠️ reorder in progress as Estimate | ✅ drag end = commit | ✅ TRIGGERS | ⚠️ needs drag visual | ⚠️ | commit goes through Closure (good), preview is hacky |
| 29 | Swipe gesture (touch) | ❌ gesture is continuous trajectory | ❌ no gesture Closure | — | ❌ | ❌ | gesture = continuous intent, no abstraction |
| 30 | Pinch-to-zoom | ❌ multi-touch continuous | ❌ no gesture | — | ❌ | ❌ | same as swipe |
| 31 | Tooltip on hover (delayed) | ⚠️ timer as Estimate | — | — | ⚠️ needs hover + delay | ⚠️ | hover + time delay — two transient dimensions |
| 32 | Context menu (right-click) | ⚠️ menu_visible as Estimate | ✅ REQUEST | — | ⚠️ needs click position | ⚠️ | position context not in Estimate |
| 33 | Autocomplete suggestions | ⚠️ suggestions as Estimate | ✅ PROMISE (async fetch) | ✅ DEPENDS_ON | ⚠️ needs selection state | ⚠️ | async list + temporary selection — doable but forced |
| 34 | Infinite scroll | ⚠️ page as Estimate | ✅ PROMISE (fetch next) | — | ⚠️ needs scroll position | ⚠️ | scroll position is transient + continuous |
| 35 | Resizable panel (drag handle) | ⚠️ size as Estimate | ✅ drag start/end | — | ⚠️ needs drag visual | ⚠️ | same as drag-drop preview |
| 36 | Color picker (drag slider) | ⚠️ color as Estimate | ✅ REQUEST | — | ⚠️ needs drag | ⚠️ | continuous value during drag |
| 37 | Knob / rotary control | ❌ rotation is continuous gesture | ❌ no gesture | — | ❌ | ❌ | same as swipe |
| 38 | Scroll position | ⚠️ can be Estimate but updates per-frame | — | — | ⚠️ needs scroll | ⚠️ | high-frequency, transient |

**Summary: 0/15 ✅, 11 ⚠️, 4 ❌.** This is the boundary zone. All transient/continuous interactions are FORCED — they can be hacked into Estimate but it's semantically wrong.

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

**Summary: 1/5 ✅, 3 ⚠️, 1 ❌.** Extension/programmability is a gap (Layer 6).

---

## Grand Summary

| Category | ✅ Clean | ⚠️ Forced | ❌ Cannot | Total |
|---|---|---|---|---|
| A: Discrete state | 12 | 0 | 0 | 12 |
| B: Animation & time | 6 | 0 | 0 | 6 |
| C: Undo & history | 1 | 2 | 2 | 5 |
| D: Continuous/transient interaction | 0 | 11 | 4 | 15 |
| E: Layout & spatial | 2 | 4 | 0 | 6 |
| F: Async & external data | 4 | 4 | 0 | 8 |
| G: Multi-window | 2 | 2 | 1 | 5 |
| H: Accessibility | 3 | 3 | 0 | 6 |
| I: Extension | 1 | 3 | 1 | 5 |
| **Total** | **31** | **29** | **8** | **68** |

---

## What this reveals

### 1. Planex's strength: discrete state (Category A) + animation (B) + a11y (H)

31/68 patterns are cleanly expressible. These are all in Planex's design center — discrete state manipulation, animation, and multi-denotation.

### 2. Planex's boundary: continuous/transient interaction (Category D)

**0/15 patterns in Category D are clean.** All 15 are forced (⚠️) or impossible (❌). This is the single biggest gap.

The root cause: **hover, drag, gesture, scroll are continuous processes, not discrete states.** Forcing them into Estimate is semantically wrong — Estimate is "state with time + uncertainty", not "high-frequency transient input stream."

This validates the `continuous-intent-speculation.md` observation: **intent is modeled as discrete events, but real interaction is continuous.**

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
- [ADR-0002](../../decisions/accepted/ADR-0002-relation-necessity-pending-undo.md) — undo works (Category C pattern 19)
- [ADR-0005](../../decisions/accepted/ADR-0005-promote-perception-to-fourth-abstraction.md) — Perception handles Category H
- [v0.4 Roadmap](v0.4-roadmap.md) — should be updated with these findings
