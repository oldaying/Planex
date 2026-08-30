# ADR-0021: The drag-begin afford — the process form of intent compilation (L15b retire)

## Status

Accepted

Date: 2026-08-30

Retires limitation **L15b** (the drag-begin seam, recorded at v0.7 promotion time by ADR-0017/0018 as a joint obligation) and completes the v0.8 roadmap Line 2 evidence. No corpus verdicts flip: P28 (drag-drop) and P36 (drag slider) were already ✅ on hand-wired-begin evidence — this ADR upgrades their *begin* evidence to graph-routed (the verdicts were honest, the mechanism is now stronger). P29/P37 are untouched (touch is NG-6; the knob demo does not exist). The claim that changes shape is the ADR-0017 claim itself: *"the type drives interaction routing"* is now unconditionally held across every channel Planex serves — discrete and continuous, pointer and keyboard.

## Context

ADR-0017 promoted intent compilation on pointer-channel, discrete-act evidence: `px_afford_compile` resolves a pointer-down to a closure, and `palette_afford.c` showed an app with zero raw-coordinate callbacks. ADR-0018 promoted `px_interaction` — the inert-trajectory process machine — on `hover_drag_interaction.c` evidence. But the two promotions did not meet: a drag was still begun by the app hand-wiring `on_mouse_move`/`on_mouse_up` into a `px_interaction`. The begin step did not resolve through the afford graph, so **a region's drag-ability was not graph data**. `palette_afford.c` documented the boundary in place: its slider affords no closure, a press on it is an unresolved click in the discrete route, and the drag enters the process by a hand call to `px_interaction_sample`.

This is the seam ADR-0017/0018 jointly recorded as L15b, and the v0.8 roadmap Line 2 named the retire shape at planning time: "an afford variant that resolves a *process* rather than a closure." ADR-0020 (Line 1, the keyboard channel) retired L15a and left this as the last gap; while it is open, the strongest claim in the project — type-driven routing — has an asterisk ("except continuous intent").

## Decision

1. **One relation, two resolution forms.** An AFFORDS edge whose target is a `px_interaction` resolves a pointer-down to a PROCESS, not to a closure:

   ```
   px_declare(g, slider_region, PX_REL_AFFORDS, slider_drag);
   ```

   There is no second affordance vocabulary (`PX_REL_AFFORDS_PROCESS` was rejected — see Alternatives). Gibson's affordance is one concept; whether the afforded action is a discrete act or a bounded process is a property of the *target*, not of the *relation*. The reader selects the form by asking the target's kind.

2. **Kind discrimination is registry-backed, not type-punned.** Interactions and closures are process-global registered objects (the regions/perceptions precedent for named graph-participating objects). `px_is_interaction(const void*)` / `px_is_closure(const void*)` answer by pointer identity against the registries — they never dereference the node. This also fixes a latent defect the process form would have tripped over: the pre-v0.8 `px_afford_at` cast the *first non-NULL* AFFORDS target to `px_closure*` (its comment claimed "first closure wins"; the code did not check). The closure form now skips non-closure targets, the process form skips non-interaction targets, and odd declarations (an estimate on an AFFORDS edge) resolve nothing — safer than v0.7 in the same stroke. One pinned behavior evolved honestly: `test_v07.c` f3 (the dangling-edge regression) verified through the blind cast; its vehicle is now the process-form reader, its spirit (the edge names the LIVE process across a rebuild) unchanged.

3. **`px_afford_compile_process(g, x, y, button, &out)`** — the process form of the compile step, window-free and backend-free. Same resolution rule as the closure form (last-declared-first among edges of the asked kind); miss returns NULL and zeroes the payload. The payload is `px_drag_intent` — same value contract as `px_pointer_intent`/`px_key_intent`: the region label is EMBEDDED (replay-safe after the region dies), the press position and button ride along as context (they seed the first trajectory sample), never as routing keys.

4. **`px_region_affords_process(g, r)`** — drag-ability as a pure graph query. This is the "drag-ability is graph data" claim made testable, and the reader the a11y projection uses: `PX_A11Y_STATE_DRAGGABLE` (new query-side state bit) is derived from this query, never hand-set from app bookkeeping. The AT-SPI2 bridge surfaces the bit in the element description — the atk `AtkStateType` enum has no draggable state (verified; the D-Bus StateType does, but the bridge mirror is built from AtkStateTypes), so the bit rides in text beside values, the bridge's documented posture for state it cannot express natively.

5. **The dual-form rule: the process owns the down.** When a region affords BOTH closure(s) and a process, `px_app_run` compiles the down to the process. The press is genuinely ambiguous (tap vs drag); only the trajectory resolves it — CSP's shape: the tap is a small-displacement COMMIT, and the process's own bridges reach the discrete act. `designer_tools.c` demonstrates the arbitration: the dual chip's commit hook measures displacement; a tap re-compiles the press position through the *closure* form (the graph decides again), a drag drops a dot at the release. Regions affording only closures keep the v0.7 immediate-trigger semantics byte-for-byte — no v0.7 app can hold a process edge, so nothing breaks.

6. **`px_interaction_reset(it)` — the rearm.** `begin()` on a terminal process is a no-op by design (the outcome is final), but an AFFORDS edge points at a *stable* target: the slider must survive its second drag. Reset returns the machine to IDLE (trajectory and cancel reason cleared) and KEEPS everything bound — name, capacity, phase hook, commit/cancel closures and payload, phase estimate. No transition fires: rearm is not an outcome. `px_app_run` resets before each compiled begin; `designer_tools.c` [7] pins the second drag on the same object.

7. **`px_app_run` pointer routing (opt-in `intent_graph`, as before):** the down tries the process form first, then the v0.7 closure form, then the raw fallback. While a compiled process is active, moves SAMPLE it (the inert hot path — preview is derived per frame from the trajectory, not per event from a callback; `on_mouse_move` does not fire) and the release COMMITS it (`on_mouse_up` does not fire — the process owns the gesture). A new press while a process is active cancels it first ("superseded by a new press" — one pointer, one gesture). An app-side cancel (its hook, its on_key) is honored: a move/up that finds the process terminal drops it and falls through to normal routing — no zombie capture. The button policy is the app's, not the framework's: any button compiles, and the begin hook may cancel on `button != 1` (the button is payload context).

## Essence Check

> This decision is the A3/A6 mechanism judge for continuous intent. Q1–Q5 per the ADR-0011 bar's spirit.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface / State space
- [x] **Intent space, input side** — the compile step (physical event → semantic intent) now resolves both forms of action: discrete acts (closures) and bounded processes (interactions), over one ontology.
- [x] **Channel orthogonality (A6), jointly** — the pointer channel now serves both forms; combined with ADR-0020, every channel Planex serves routes through the same AFFORDS graph.

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses: the app declares drag-ability as an edge and gets routing, process lifecycle (rearm), tap-vs-drag arbitration through the graph, and the a11y projection — no hand-wired begin, no per-region drag flags, no mode state. `designer_tools.c`'s router is one rule over five regions, two processes, and one closure, with zero region branches. Cost: two resolution forms to understand (the kind filter), and dual-form regions require the app's commit hook to arbitrate by measure (one displacement check — the same tap/drag split apps already owned, now with the closure path re-entering through the graph).

### Q3. Is there a gap between the claim and the implementation?

No gap on the mechanism: `test_v08.c` sections E–G (13 tests; the suite is 31) pin the process compile (resolution, value contract, last-declared-first, zeroed miss), form orthogonality (the closure form skips process targets and vice versa; the pre-v0.8 blind-cast layout is a test fixture), the dual-form rule at the app-routing level, the kind predicates, the Line 1 focus-ring pins holding, the reset/rearm contract (bindings survive), and the supersede/app-cancel contracts. The honest residues: keyboard process-activation (arrow-key slider adjustment) is unmodeled — a process-only region is honestly absent from the focus ring; drop semantics (what a committed drag *does*) are the app's commit-hook business by design — the framework delivers the process and the trajectory, not the interpretation; win32/cocoa remain untested (L5/L9).

### Q4. What is the cost, and who can verify it?

- **Cost:** two process-global registries (a few bytes per live closure/interaction, freed with the object — the px_region contract); kind predicates are O(live objects) per compile call — pointer compares against dozens of objects per click, microseconds; the closure compile gained a filter it always claimed to have.
- **Who can verify:** `make test_v08` (sections E–G); `make check-examples` runs `designer_tools.c` (a ten-step script: three drags, the dual-form tap AND drag on the same region, the closure-only control, two slider drags on one process object, the empty-space no-ops, a mid-drag cancel).

### Q5. What are the counterexamples?

- **A region that should tap AND drag with different semantics per button** (e.g. middle-drag vs left-tap): the process form compiles on any button; the begin hook must cancel on the wrong one. Honest, because button policy is channel context, not affordance — but it is a real micro-rule for such apps.
- **Multi-pointer drag** (two simultaneous gestures): one active process per `px_app_run` loop — a second press supersedes the first. Single-pointer is a framework-wide posture (NG-6 touch deferral), not a process-form limit.
- **Hover-only processes** (dwell-to-activate): a process that should begin on *move* has no compile step here — the down is the only begin trigger this ADR routes. The seam (a process form over another event class) is the same shape; no consumer has asked for it yet.

## Consequences

### Positive

- **L15b retires** — the ADR-0017/0018 joint obligation closes: drags begin through the afford graph, and "type-driven routing" holds unconditionally across every channel Planex serves.
- Drag-ability is queryable data (`px_region_affords_process`) — the a11y projection (`PX_A11Y_STATE_DRAGGABLE`) derives from the graph, and the corpus evidence base (P28, P36) upgrades from hand-wired begins to graph-routed ones.
- The latent `px_afford_at` type confusion is fixed (kind filtering), making the pre-v0.8 comment true in code.
- The dual-path retirement decision (v0.8 Line 3) now has its full evidence base: pointer-discrete, pointer-continuous, and keyboard all route through the graph in real examples.

### Negative

- Dual-form regions no longer fire their closure on the down — the app opted into process arbitration by declaring the process edge, and must reach the discrete act through the commit path. A new composition rule to learn (this ADR, Decision 5).
- Two payload shapes join the closure surface's sorting (`px_pointer_intent`, `px_key_intent`, `px_drag_intent`, app shapes) — the act sorts by payload size, one `if` per shape.
- The begin hook that needs the pressed REGION re-derives it from the first sample (`px_region_at`) — the idiom `hover_drag_interaction.c` already established, now the documented pattern for compiled processes.

### Neutral

- The raw move/up callbacks simply do not fire during a compiled gesture — apps that need per-move behavior read the trajectory (derived preview), which is the ADR-0018 design anyway.

## Alternatives Considered

### Alternative 1: A second relation kind (PX_REL_AFFORDS_PROCESS)

Rejected: a second affordance vocabulary for one Gibson concept. "The slider affords dragging" and "the button affords clicking" are the same relation; discrete-vs-process is a property of the target. Two kinds would also split the resolution rule book (which kind wins a down? a new arbitration), and the a11y reader would query two relations to answer "is this element interactive."

### Alternative 2: Fire the closure on the down AND begin the process (dual-fire)

Rejected: the down would trigger the discrete act before the gesture reveals whether the user meant it — a tap-drag intent would paint a dot AND drop one. The ambiguity is temporal; resolving it at the down is exactly the mode-flag thinking the process machine exists to replace.

### Alternative 3: Allocate a process per gesture (no reset; re-declare the edge)

Rejected: the AFFORDS edge would point at a dead object after the first gesture — the dangling-edge discipline (px_undeclare) exists to prevent precisely this, and per-gesture allocation churns the graph. A stable target plus an explicit rearm is the honest lifecycle: terminal outcomes stay final (replay-safe), the machine simply becomes available again.

### Alternative 4: Kind tags in the structs (first-member discriminators)

Rejected: reading the first bytes of a void* node is type punning by convention — safe for closures and interactions, dishonest for the estimate someone declares on an odd edge. Registry membership is pointer identity, never dereferences, and follows the regions/perceptions precedent for graph-participating objects.

## CAVEATS

- This ADR does **not** claim keyboard parity for processes: arrow-key adjustment (the keyboard's continuous-intent form) is unmodeled, and process-only regions are honestly absent from the focus ring until it exists.
- The framework does not interpret committed drags: drop targets, drop effects, and drag-over feedback are the commit hook's business. This is the ADR-0018 division (the process is the trajectory + outcome; the act is a closure), not a gap.
- The AT-SPI2 projection rides in the element description (atk has no draggable state constant); orca reads it as text until a state-carrying AtkObject subclass lands — the same posture as values.
- Button policy is app-side: the framework compiles any button's press to the process; a middle-click-drag-only region cancels the wrong buttons in its begin hook.

## Known issues

- Kind predicates walk the live registries per compile call — O(closures) / O(interactions) pointer compares per down. Fine at UI scale; a hash set is the fix if a profile ever shows it hot (deferred until measured, the compression-metric posture).
- App-side cancels are detected lazily (on the next move/up event) — an app that cancels and never sends another pointer event keeps the loop's `active` reference until the next press supersedes it. Harmless (the process is terminal; nothing observes it), but stated.
- `test_v07.c` f3's verification vehicle changed (see Decision 2) — the dangling-edge pin itself is intact and stronger (it now also asserts the kind-honest miss).

## HISTORY

- 2026-08-30: Accepted — process form landed; test_v08.c sections E–G (suite 31) green; designer_tools.c ten-step script green; L15b retired; no corpus flips (P28/P36 evidence upgraded in place).
- 2026-08-30: v0.8 roadmap Line 2 named the retire shape ("an afford variant that resolves a process, not a closure") and the evidence obligation (a designer-tool palette whose drags must be data-driven).
- 2026-08-30: ADR-0020 (Line 1) retired L15a, leaving this the last compile-step gap.
- 2026-08-30: ADR-0017/0018 Known-issues recorded L15a/L15b at promotion time.

## References

- Code: [`src/hit.c`](../../../src/hit.c) (`px_afford_compile_process`, `px_region_affords_process`, kind filtering), [`src/app.c`](../../../src/app.c) (pointer routing: process-first down, sampled moves, committing ups), [`src/interaction.c`](../../../src/interaction.c) (registry + `px_interaction_reset`), [`src/closure.c`](../../../src/closure.c) (registry + `px_is_closure`), [`include/planex/planex.h`](../../../include/planex/planex.h) (`px_drag_intent`, the Line 2 section), [`include/planex/a11y.h`](../../../include/planex/a11y.h) (`PX_A11Y_STATE_DRAGGABLE`), [`src/a11y_bridge_atspi.c`](../../../src/a11y_bridge_atspi.c) (description ride-along)
- Evidence: [`tests/test_v08.c`](../../../tests/test_v08.c) sections E–G; [`examples/designer_tools.c`](../../../examples/designer_tools.c)
- ADRs: [ADR-0017](ADR-0017-intent-compilation-promotion.md) and [ADR-0018](ADR-0018-interaction-process-promotion.md) (the joint obligation this retires), [ADR-0020](ADR-0020-v08-keyboard-channel.md) (Line 1 — the ordering dependency), [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md) (the process machine's own admission bar)
- Limitations: [limitations.md](../../concepts/state/limitations.md) (L15b retired — L15 closes entire)
- Roadmap: [v0.8-roadmap.md](../../concepts/state/v0.8-roadmap.md) Line 2
