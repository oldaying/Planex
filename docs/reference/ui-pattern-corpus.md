# UI Pattern Corpus — Closed Falsifiability Set

> **Status:** Canonical reference. Versioned at v0.5; re-scored at v0.7 by [ADR-0017](../decisions/accepted/ADR-0017-intent-compilation-promotion.md) + [ADR-0018](../decisions/accepted/ADR-0018-interaction-process-promotion.md). Date: 2026-08-30 (re-scored again at v0.8 by [ADR-0020](../decisions/accepted/ADR-0020-v08-keyboard-channel.md) — P61).
>
> **Purpose:** Prerequisite 3 (falsifiability) Layer 3c — the closed UI-pattern corpus used to verify completeness of the abstraction set. Each pattern is either (a) cleanly expressible (✅, exemplified in `examples/`), (b) forced (⚠️, documented as a limitation in [`limitations.md`](../concepts/state/limitations.md)), or (c) impossible (❌, documented as a non-goal in [`non-goals.md`](../concepts/canonical/non-goals.md) or as a structural limitation).
>
> **Closing rule:** Adding, removing, or re-verdicting a pattern requires an ADR. The corpus is closed at **68 patterns**; the v0.7 re-score (Category D, per ADR-0017/0018) changed the distribution to **38 ✅ + 24 ⚠️ + 6 ❌ = 68**; the v0.8 re-score (P61, per ADR-0020) changed it to **39 ✅ + 23 ⚠️ + 6 ❌ = 68**. The count is an invariant — drift from this distribution is a CI failure surfaced by [`tests/test_completeness.c`](../../tests/test_completeness.c).
>
> **Companion documents:**
> - [`../concepts/state/ui-pattern-coverage.md`](../concepts/state/ui-pattern-coverage.md) — the qualitative coverage matrix this corpus distills (one row per pattern with per-abstraction verdict + notes).
> - [`../concepts/canonical/abstraction-form.md`](../concepts/canonical/abstraction-form.md) — Prerequisite 3 (falsifiability) Layer 3c that this corpus closes.
> - [`../concepts/state/limitations.md`](../concepts/state/limitations.md) — where ⚠️ and ❌ patterns must be grounded.
> - [`../concepts/canonical/non-goals.md`](../concepts/canonical/non-goals.md) — where some ❌ patterns are explicitly rejected.
>
> **Verification:** [`tests/test_completeness.c`](../../tests/test_completeness.c) exercises the corpus as a CI-ready check (`make check-completeness`). The test hardcodes the 68-pattern table below and verifies that (a) the count and verdict distribution match, (b) every ✅ pattern with a `EXAMPLE` grounding has its named file in `examples/`, (c) every ⚠️/❌ pattern with a `LIMITATION` grounding names a real L# in `limitations.md`, (d) every ❌ pattern with a `NONGOAL` grounding names a real NG-# in `non-goals.md`. Drift between this corpus and the test is a CI failure.

---

## Closing rule and invariant

The corpus is **closed** at 68 patterns. This means:

1. **Count invariant** — `tests/test_completeness.c` asserts the count is exactly 68. If a pattern is added (e.g. P69: "Undo via voice command") or removed (e.g. P4 dropdown subsumed by P7 tabs), the test fails until the corpus and the test agree.
2. **Verdict-distribution invariant** — `39 ✅ + 23 ⚠️ + 6 ❌ = 68` (v0.8 re-score; was `38 + 24 + 6` at v0.7, `31 + 29 + 8` at v0.5). If a re-verdict happens (e.g. P20 Redo moves from ⚠️ to ✅ when L4 closes), both this corpus and the test must be updated in the same commit. A one-sided update is a CI failure.
3. **Grounding invariant** — every ✅ pattern grounds to either (a) a file in `examples/` or (b) the `CLAIM_ONLY` sentinel (for trivially expressible patterns where a separate demo would be redundant). Every ⚠️/❌ pattern grounds to either (a) a limitation L# in `limitations.md` or (b) a non-goal NG-# in `non-goals.md`. Patterns without grounding are CI failures.

These invariants compose with the leak-budget mechanism ([`leak-budgets.md`](../concepts/canonical/leak-budgets.md)) to form the falsifiability contract: the leak-budget catches *quantitative* leaks in implemented abstractions, the corpus catches *completeness* gaps in the abstraction set as a whole.

---

## How to read this corpus

Each row in the per-category tables below has these columns:

| Column | Meaning |
|--------|---------|
| **ID** | Stable pattern ID `P1`–`P68`. Never renumbered; if a pattern is removed its ID is retired, not reused. |
| **Pattern** | Short name, matching the row in `ui-pattern-coverage.md`. |
| **Verdict** | `✅ clean` (cleanly expressible) / `⚠️ forced` (expressible but semantically wrong) / `❌ cannot` (cannot express without a new abstraction). |
| **Grounding** | Either `EXAMPLE <file>` (a file in `examples/` that demonstrates the pattern), `CLAIM_ONLY` (no example needed — the pattern is trivially expressible), `LIMITATION <L#>` (a section in `limitations.md` documenting why it's forced/impossible), or `NONGOAL <NG-#>` (a section in `non-goals.md` explicitly rejecting the pattern). |
| **Coverage ref** | Link to the corresponding row in `ui-pattern-coverage.md`. |

---

## Category A: Discrete State Manipulation (P1–P12)

Planex's design center. All 12 patterns are ✅ — discrete state is what the original abstraction set (Relation + Estimate + Closure + Perception + px_loop) was designed for; the first four carry the discrete-state load directly, `px_loop` provides the feedback loop the others run inside, and the v0.7 additions (intent compilation, `px_interaction`) sit on the input side. Most patterns are `CLAIM_ONLY` because the implementation is a few lines of `px_estimate_set` + `px_closure_trigger` + a pure-function perception; demos that exercise multiple abstractions together are in `examples/`.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P1 | Counter (inc/dec) | ✅ clean | EXAMPLE counter_4abs.c | Canonical 4-abstractions demo |
| P2 | Checkbox toggle | ✅ clean | CLAIM_ONLY | Boolean Estimate + REQUEST Closure |
| P3 | Radio button group | ✅ clean | CLAIM_ONLY | Mutual exclusion via single source |
| P4 | Dropdown / select | ✅ clean | CLAIM_ONLY | Same as radio group with visible state |
| P5 | Text input (basic) | ✅ clean | CLAIM_ONLY | String Estimate; IME for CJK is out of scope (NG-9) |
| P6 | Form with validation | ✅ clean | CLAIM_ONLY | Derived Estimate (all_valid) via DEPENDS_ON |
| P7 | Tabs | ✅ clean | CLAIM_ONLY | State-switch via Relation as state machine |
| P8 | Modal dialog | ✅ clean | CLAIM_ONLY | Async lifecycle via PROMISE/DECLARE |
| P9 | Wizard (multi-step) | ✅ clean | CLAIM_ONLY | DEPENDS_ON chain across steps |
| P10 | Todo list | ✅ clean | CLAIM_ONLY | Items + derived count via DEPENDS_ON |
| P11 | Slider (value in range) | ✅ clean | CLAIM_ONLY | Value + constraints via DEPENDS_ON |
| P12 | Button with loading state | ✅ clean | CLAIM_ONLY | Async lifecycle (PROMISE→DECLARE) |

**Category verdict: 12/12 ✅.** Discrete state manipulation is Planex's strength.

---

## Category B: Animation & Time (P13–P18)

Estimate's time dimension. All 6 patterns are ✅ — `px_estimate_animate` was the first animation mechanism and remains the canonical one.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P13 | Fade in/out | ✅ clean | EXAMPLE animation_demo.c | Animate opacity |
| P14 | Slide transition | ✅ clean | EXAMPLE animation_demo.c | Animate position |
| P15 | Progress bar | ✅ clean | EXAMPLE async_demo.c | Async + animation compose |
| P16 | Typing animation | ✅ clean | CLAIM_ONLY | Animate char index |
| P17 | Continuous data stream (sensor) | ✅ clean | EXAMPLE confidence_demo.c | Value + confidence (Friston predictive coding) |
| P18 | Countdown timer | ✅ clean | CLAIM_ONLY | Animate to 0 |

**Category verdict: 6/6 ✅.** Animation is natural in Estimate.

---

## Category C: Undo / History (P19–P23)

Relation graph's strength. Undo works cleanly; redo/time-travel are forced (need extra stacks); fork/collaboration are impossible (no fork abstraction, no multi-writer).

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P19 | Undo last action | ✅ clean | EXAMPLE undo_via_graph.c | undo_via_graph demo — Closure trigger + auto-snapshot |
| P20 | Redo | ⚠️ forced | LIMITATION L4 | Planex has undo but NOT redo — needs reverse stack |
| P21 | Time travel (jump to any point) | ⚠️ forced | LIMITATION L4 | Intent is serializable (value) but no time-travel API |
| P22 | Branching history (fork) | ❌ cannot | LIMITATION L4 | Trace has `branch_point` marker but no real fork |
| P23 | Collaborative editing (multi-user) | ❌ cannot | NONGOAL NG-12 | Multi-process UI explicitly out of scope |

**Category verdict: 1/5 ✅, 2/5 ⚠️, 2/5 ❌.**

---

## Category D: Continuous / Transient Interaction (P24–P38)

The boundary zone, re-scored at v0.7. **7/15 patterns are clean** after the promotion of intent compilation (ADR-0017) and the interaction process (ADR-0018) to canonical abstractions. The remaining 8 are forced (⚠️) or impossible (❌) — dominated by multi-touch absence (NG-6), timing-delay patterns, and scroll-position transients.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P24 | Hover highlight | ✅ clean | EXAMPLE hover_drag_interaction.c | Hover is a region query computed at render time — no estimate churn (ADR-0017) |
| P25 | Mouse cursor position | ✅ clean | EXAMPLE hover_drag_interaction.c | Ambient sample stream (plain struct field), derived on demand — the 60fps-estimate hack retired (ADR-0017) |
| P26 | Pressed button visual | ✅ clean | EXAMPLE hover_drag_interaction.c | Press-release = BEGAN→COMMITTED arc; publish_phase at transitions only (ADR-0018) |
| P27 | Drag preview (ghost image) | ✅ clean | EXAMPLE hover_drag_interaction.c | Preview derived per frame from the trajectory — zero writes while dragging (ADR-0018) |
| P28 | Drag-drop reorder | ✅ clean | EXAMPLE hover_drag_interaction.c | Trajectory + commit Closure + undo through the graph (ADR-0018) |
| P29 | Swipe gesture (touch) | ⚠️ forced | LIMITATION L12 | Derivable from trajectory measures (velocity + displacement); touch input channel is NG-6 (ADR-0018) |
| P30 | Pinch-to-zoom | ❌ cannot | LIMITATION L12 | Multi-touch continuous — needs simultaneous trajectories + arbitration (NG-6) |
| P31 | Tooltip on hover (delayed) | ⚠️ forced | LIMITATION L11 | Hover is clean now; the time-delay dimension still needs on_tick timing hacks |
| P32 | Context menu (right-click) | ✅ clean | EXAMPLE palette_afford.c | Button-3 compiles to px_pointer_intent with region label embedded — position context is in the intent value (ADR-0017) |
| P33 | Autocomplete suggestions | ⚠️ forced | CLAIM_ONLY | Async list + temporary selection — doable but forced |
| P34 | Infinite scroll | ⚠️ forced | LIMITATION L12 | Scroll position is transient + continuous; wheel events landed but no scroll abstraction |
| P35 | Resizable panel (drag handle) | ⚠️ forced | LIMITATION L11 | Drag mechanism exists (ADR-0018) but drag-begin affordance seam + no demo = forced |
| P36 | Color picker (drag slider) | ✅ clean | EXAMPLE palette_afford.c | Live preview derived from trajectory, one committed estimate write (ADR-0017 + ADR-0018) |
| P37 | Knob / rotary control | ⚠️ forced | CLAIM_ONLY | Rotary = drag process + app-side angle math — derivable but undemonstrated (ADR-0018) |
| P38 | Scroll position | ⚠️ forced | LIMITATION L12 | High-frequency transient; wheel events land (v0.6) but position-as-state remains forced |

**Category verdict: 7/15 ✅, 7/15 ⚠️, 1/15 ❌.**

The v0.7 re-score (ADRs [0017](../decisions/accepted/ADR-0017-intent-compilation-promotion.md) and [0018](../decisions/accepted/ADR-0018-interaction-process-promotion.md)) flipped P24–P28, P32, P36 from ⚠️/❌ to ✅ and downgraded P29/P37 from ❌ to ⚠️. What remains: multi-touch (NG-6), timing-delay composites, and scroll-position transients. ADR-0006's deferral protocol completed: the boundary-exposing demo measured the pain, the prototypes landed in v0.6, the promotions landed in v0.7 on real-application evidence.

The v0.8 Line 2 landing ([ADR-0021](../decisions/accepted/ADR-0021-v08-drag-begin-afford.md)) changed no verdicts: P28 and P36 were already ✅ on hand-wired-begin evidence — their *begin* evidence upgraded to graph-routed (`designer_tools.c`: drags compile through `px_afford_compile_process`), which strengthens the verdicts without re-scoring them (the closing rule requires an ADR to change a verdict; this is a recorded evidence upgrade, not a change).

---

## Category E: Layout & Spatial (P39–P44)

Layout is partially covered. `PX_REL_BESIDE` / `PX_REL_BELOW` cover fixed layout; complex layout (flexbox, grid, clipping) needs work.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P39 | Fixed layout | ✅ clean | CLAIM_ONLY | `px_layout` helpers cover fixed positioning |
| P40 | Responsive layout (resize) | ⚠️ forced | LIMITATION L11 | Window size changes continuously |
| P41 | Flexbox / grid | ⚠️ forced | CLAIM_ONLY | No constraint solver in Planex |
| P42 | Absolute positioning | ✅ clean | CLAIM_ONLY | Just use coordinates directly |
| P43 | Z-order / stacking | ⚠️ forced | CLAIM_ONLY | Stacking order is implicit, not relational |
| P44 | Clipping / overflow | ⚠️ forced | CLAIM_ONLY | No clip abstraction in fb backend |

**Category verdict: 2/6 ✅, 4/6 ⚠️.**

---

## Category F: Async & External Data (P45–P52)

Async is mostly covered — Closure's PROMISE/DECLARE/FAIL lifecycle is built for this. Streaming/polling need work.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P45 | Fetch data (async) | ✅ clean | EXAMPLE async_demo.c | Built-in PROMISE→DECLARE lifecycle |
| P46 | Polling (interval) | ⚠️ forced | CLAIM_ONLY | Needs on_tick + manual repeat |
| P47 | WebSocket (real-time) | ⚠️ forced | CLAIM_ONLY | Incoming events need stream abstraction |
| P48 | File upload progress | ✅ clean | CLAIM_ONLY | Progress Estimate + PROMISE |
| P49 | Search debouncing | ⚠️ forced | CLAIM_ONLY | Debounce is a timer hack |
| P50 | Optimistic update | ✅ clean | EXAMPLE confidence_demo.c | Confidence + undo = natural fit |
| P51 | Error retry | ✅ clean | CLAIM_ONLY | PROMISE→FAIL→retry via graph |
| P52 | Offline mode | ⚠️ forced | CLAIM_ONLY | Network status is external |

**Category verdict: 4/8 ✅, 4/8 ⚠️.**

---

## Category G: Multi-window / Multi-context (P53–P57)

Multi-window is a gap. Planex has no cross-window sync (NG-12 explicit); split pane and popup are forced because the single Perception renders the whole framebuffer.

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P53 | Multi-window sync | ❌ cannot | NONGOAL NG-12 | Each window has own Perception; no cross-window sync |
| P54 | Split pane | ⚠️ forced | CLAIM_ONLY | Single Perception renders whole fb |
| P55 | Popup window | ⚠️ forced | CLAIM_ONLY | No multi-window Perception |
| P56 | Notification toast | ✅ clean | CLAIM_ONLY | Transient list + auto-dismiss |
| P57 | Global state (Redux-like) | ✅ clean | CLAIM_ONLY | Estimate is already global by design |

**Category verdict: 2/5 ✅, 2/5 ⚠️, 1/5 ❌.**

---

## Category H: Accessibility & Multi-denotation (P58–P63)

Multi-denotation is Planex's differentiator. Three patterns are clean (separate Perception for a11y / test / live-region); three are forced (theme, focus, reduced-motion are system preferences, not app state).

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P58 | Screen reader (a11y) | ✅ clean | EXAMPLE multi_perception.c | Separate Perception = separate denotation |
| P59 | Test snapshot | ✅ clean | EXAMPLE counter_denotative.c | Pure fn = testable; denotative mode |
| P60 | High contrast mode | ⚠️ forced | CLAIM_ONLY | Theme shouldn't be Estimate (NG-8 styling) |
| P61 | Keyboard navigation | ✅ clean | EXAMPLE palette_afford.c (v0.8, ADR-0020) | Focus ring derived from AFFORDS; key intents compile like pointer intents |
| P62 | Reduced motion | ⚠️ forced | CLAIM_ONLY | System preference, not app state |
| P63 | ARIA live region | ✅ clean | CLAIM_ONLY | Closure feedback → a11y Perception |

**Category verdict: 4/6 ✅, 2/6 ⚠️.**

---

## Category I: Extension & Programmability (P64–P68)

Extension is a gap (Layer 6 — medium). Planex is a library, not a platform; plugin/scripting are explicitly out of scope (NG-5 component library, NG-9 i18n).

| ID | Pattern | Verdict | Grounding | Notes |
|----|---------|---------|-----------|-------|
| P64 | Plugin / extension | ❌ cannot | NONGOAL NG-5 | Planex is a library, not a platform |
| P65 | User scripting | ❌ cannot | NONGOAL NG-5 | Layer 6 (medium) — not in scope |
| P66 | Theme system | ⚠️ forced | NONGOAL NG-8 | Styling/theming is explicitly out of scope |
| P67 | Internationalization | ⚠️ forced | NONGOAL NG-9 | i18n is explicitly out of scope |
| P68 | Custom widgets | ✅ clean | EXAMPLE integration_4abs.c | This is the thesis — widgets emerge from the abstraction set (7 since v0.7) |

**Category verdict: 1/5 ✅, 2/5 ⚠️, 2/5 ❌.**

---

## Grand summary

| Category | ✅ clean | ⚠️ forced | ❌ cannot | Total |
|----------|----------|-----------|-----------|-------|
| A: Discrete state | 12 | 0 | 0 | 12 |
| B: Animation & time | 6 | 0 | 0 | 6 |
| C: Undo & history | 1 | 2 | 2 | 5 |
| D: Continuous/transient interaction | 7 | 7 | 1 | 15 |
| E: Layout & spatial | 2 | 4 | 0 | 6 |
| F: Async & external data | 4 | 4 | 0 | 8 |
| G: Multi-window | 2 | 2 | 1 | 5 |
| H: Accessibility | 3 | 3 | 0 | 6 |
| I: Extension | 1 | 2 | 2 | 5 |
| **Total** | **39** | **23** | **6** | **68** |

**Closed-corpus invariant:** `39 + 23 + 6 = 68`. The test `tests/test_completeness.c` recomputes these counts from its hardcoded pattern table and asserts the equality; any drift is a CI failure. Re-scored at v0.7 by ADR-0017 + ADR-0018 (was `31 + 29 + 8` at v0.5); at v0.8 by ADR-0020 (P61).

---

## What this corpus falsifies

The corpus exists to make Prerequisite 3 (falsifiability) Layer 3c falsifiable:

1. **Forward direction** — if a UI pattern *cannot* be expressed in the 7 abstractions (i.e. verdict is `❌ cannot`), the abstraction set is *incomplete*. Currently 6/68 patterns are ❌ — these are the gaps the corpus names honestly:
   - P22 Branching history → fork abstraction missing (L4)
   - P23 Collaborative editing → multi-writer / CRDT missing (NG-12)
   - P30 Pinch-to-zoom → multi-touch trajectories + arbitration missing (L12, NG-6)
   - P53 Multi-window sync → cross-window Relation missing (NG-12)
   - P64 Plugin / extension → extension API missing (NG-5)
   - P65 User scripting → scripting API missing (NG-5)

2. **Backward direction** — if an abstraction is *not exercised* by any ✅ pattern, the abstraction is *redundant*. Currently each of the 7 abstractions is exercised by at least one ✅ pattern:
   - **Estimate** — P1, P13, P17, P50, … (state + animation + confidence)
   - **Closure** — P1, P8, P12, P19, P45, P63, … (discrete intent + async lifecycle)
   - **Relation** — P1 (TRIGGERS), P3 (DEPENDS_ON), P10, P19 (graph), P39 (BESIDE/BELOW), …
   - **Perception** — P1, P58 (a11y), P59 (test snapshot), P63 (live region), …
   - **px_loop** — P19 (trigger→snapshot→re-render), P50 (optimistic + undo), … (closed-loop coupling)
   - **Intent compilation** (ADR-0017) — P24, P25, P32, P36 (afford-routed clicks, region queries, context intents)
   - **px_interaction** (ADR-0018) — P26, P27, P28, P36, P29⚠️ (phases, derived previews, drag-drop commits, gesture measures)

The backward-direction check is qualitative (not enforced by `test_completeness.c` yet) — it lives in `ui-pattern-coverage.md`'s "What this reveals" section. A future Wave 4.3 follow-up could encode it as a CI assertion: "for each of the 7 abstractions, ≥1 ✅ pattern names it in the Coverage column."

---

## Closing rule: how to amend the corpus

To add, remove, or re-verdict a pattern:

1. **Open an ADR** with title "Amend UI Pattern Corpus: <change>". The ADR must name:
   - The pattern(s) affected (P# IDs).
   - The change (add/remove/re-verdict).
   - The rationale (what new abstraction? what new limitation? what new demo?).
   - The new count and verdict distribution (must still sum to 68, or the new total if extending).
2. **Update this corpus** in the same commit — the table row(s) change here, the count in the Grand Summary changes, and the test's expected count is updated.
3. **Update `tests/test_completeness.c`** — the hardcoded pattern table must match this corpus exactly. The test's `EXPECTED_TOTAL = 68`, `EXPECTED_CLEAN = 38`, `EXPECTED_FORCED = 24`, `EXPECTED_CANNOT = 6` constants must be updated.
4. **Update `ui-pattern-coverage.md`** — the per-abstraction matrix row(s) for the affected pattern(s) must change too. The two documents are paired; one-sided updates are drift.
5. **Run `make check-completeness`** — the test must pass after the update. If it fails, the corpus, the test, or `ui-pattern-coverage.md` is still inconsistent.

The closing rule is itself a falsifiability mechanism: amendments are not silent. Every change to the corpus is a public, versioned ADR — the corpus cannot drift without a recorded decision.

---

## See also

- [`../concepts/state/ui-pattern-coverage.md`](../concepts/state/ui-pattern-coverage.md) — the qualitative coverage matrix (per-abstraction verdicts for each pattern).
- [`../concepts/canonical/abstraction-form.md`](../concepts/canonical/abstraction-form.md) § Prerequisite 3 Layer 3c — the place this corpus is referenced as the falsifiability mechanism.
- [`../concepts/canonical/leak-budgets.md`](../concepts/canonical/leak-budgets.md) — the *quantitative* falsifiability mechanism (per-abstraction L1/L2 leak counts). This corpus is the *completeness* falsifiability mechanism (per-pattern expressibility verdicts).
- [`../concepts/state/limitations.md`](../concepts/state/limitations.md) — where ⚠️ and ❌ patterns ground.
- [`../concepts/canonical/non-goals.md`](../concepts/canonical/non-goals.md) — where some ❌ patterns are explicitly rejected as out of scope.
- [`../../tests/test_completeness.c`](../../tests/test_completeness.c) — the executable CI check that verifies the corpus's invariants.
