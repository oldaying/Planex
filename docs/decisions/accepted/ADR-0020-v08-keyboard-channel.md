# ADR-0020: The keyboard channel — derived focus ring + key-intent compile (L15a retire)

## Status

Accepted

Date: 2026-08-30

Retires limitations **L15a** (recorded at v0.7 promotion time by ADR-0017/0018's Known-issues sections) and re-verdicts corpus **P61 (Keyboard navigation) ⚠️ → ✅** (distribution 38/24/6 → **39/23/6**, per the corpus closing rule that re-verdicting requires an ADR). This is the v0.8 roadmap Line 1 landing: the type-driven-routing claim stops being pointer-only — the same AFFORDS graph now serves a second input channel.

## Context

ADR-0017 promoted intent compilation on pointer-channel evidence: `palette_afford.c`, zero raw-coordinate callbacks, `px_afford_compile` as the window-free compile step. The promotion honestly recorded two scoped gaps (L15): the keyboard had no compile step (L15a — a keyboard user could not be served by intent compilation at all), and drag-begin bypassed the graph (L15b, still open).

L15a was the larger essence deficit. The channel axiom (A6) claims channels are projections over one ontology; until v0.8, one channel routed through the AFFORDS graph and every other channel bypassed it. The strongest claim in the project — *the type drives interaction routing itself*, not just read-only navigation — was single-channel.

The retire path was already named at promotion time (limitations L15a): "a `px_afford_focus_next`-shaped query over the same AFFORDS graph (the graph already holds what is needed; the missing piece is focus-order data + key event compilation in `app.c`)." This ADR lands exactly that shape.

## Decision

1. **The focus ring is DERIVED, not declared.** A region is focusable iff it affords at least one closure; ring order is region **creation order** (the app-declared order — the registry read forward; the z-order scan is the same registry read backward). One registry, two honest projections. No layout-derived ordering: layout is Perception's business, not Relation's. No separate focus-registry API to forget to update.

2. **`px_afford_focus_first / next / prev(g, from)`** — pure ring queries, wraparound in both directions. A `from` that is NULL, freed, or unfocusable is **nowhere**, and nowhere resolves to the ring's head for either direction (the Tab-from-nowhere semantics: the first Tab focuses the first focusable region). An empty ring returns NULL.

3. **`px_key_intent`** — the keyboard intent value, same construction as `px_pointer_intent`: the region label is **embedded** (replay-safe after the region is freed); the activating key (`'\r'` / `'\n'` / `' '`) rides along as context for the act, never as a routing key. The routing key is the focused region.

4. **`px_afford_compile_focus(g, focused, key, out)`** — the window-free activation compile. Same resolution rule as the pointer channel: a region with several AFFORDS edges resolves **last-declared-first** (pinned by `test_v07.c` a7 for pointer, `test_v08.c` b4 for keyboard — one ring, one rule, two channels). Miss zeroes the payload so stale data cannot leak through the raw-key fallback.

5. **`px_app_run` keyboard routing** (opt-in beside `intent_graph`, same compile-before-dispatch order as pointer-downs): Tab/Shift-Tab walk the ring and report each move through the new optional **`on_focus(region_label, user)`** callback — the label is a value (the region may be freed later); the framework never draws a focus indicator, the app renders one from this notification or ignores focus entirely. Enter/Space compile the focused region's closure with a `px_key_intent` payload. Keys that compile to nothing fall back to `on_key` unchanged; NULL `intent_graph` keeps legacy key dispatch identical (zero cost when unset).

6. **`px_event.modifiers` + `PX_MOD_SHIFT/CTRL/ALT`** — the event model gains a modifier bitmask (the missing piece that made Shift-Tab inexpressible). X11 fills it from the key state; headless stdin gains `t` (Tab), `T` (Shift-Tab), `e` (Enter) named-key injections; win32/cocoa leave it 0 (their untested status is unchanged, L5/L9) — a backend that cannot report a modifier never sets its bit, so the unmodified semantics are the default.

7. **One closure serves both channels; the payload shape carries the channel.** `palette_afford.c`'s actions now sort by payload size (`px_pointer_intent` vs `px_key_intent`) — the closure never branches on the channel, because the channel is not semantics; it is a projection (`test_v08.c` d1 pins this: both compile entry points resolve the same region to the same closure with the same routing key).

## Essence Check

> This decision is the A6 (channel orthogonality) mechanism judge. Q1–Q5 per the ADR-0011 bar's spirit.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface / State space
- [x] **Intent space, input side** — the compile step (physical event → semantic intent) now exists for a second channel over the same ontology.
- [ ] Presentation / Feedback

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses: the app declares affordances once and gets two channels — no parallel keyboard handler table, no focus-order bookkeeping, no second routing ontology. One rule to learn (the ring is derived from what affords). Cost: two payload shapes on dual-channel closures (sorting by payload size is one `if`), and one new callback (`on_focus`).

### Q3. Is there a gap between the claim and the implementation?

No gap on the mechanism: `test_v08.c` (18 tests) pins the ring semantics (creation order, wraparound, unfocusable exclusion, nowhere-normalization, empty ring), the value contract (embedded label survives region free, miss zeroes), the app-level routing decision, and channel orthogonality. The honest residues: arrow-key/shortcut compilation is not modeled (only Tab/Shift-Tab/Enter/Space — the minimal activation set); win32/cocoa do not report modifiers; orca end-to-end verification (the a11y projection of this same focus data) remains the open external condition (L9).

### Q4. What is the cost, and who can verify it?

- **Cost:** `px_event` grows a field (source-compatible, ABI irrelevant pre-v1.0); `px_app_desc` grows `on_focus`; focus ring construction is O(regions × focusable-check) per Tab — bounded by the region count, measured in microseconds at any plausible UI size.
- **Who can verify:** `make test_v08` (sections A–D); `make check-examples` runs the keyboard session in `palette_afford.c` steps [9]–[13] (a keyboard-only walkthrough of the same app: ring walk, Enter activation, Shift-Tab reverse, position-free canvas clear, Space reset).

### Q5. What are the counterexamples?

- **Slider-like regions** (afford no discrete act): they are honestly absent from the ring — the keyboard cannot activate what the app did not declare as an act. `palette_afford.c` documents this in the walk itself (brightness never appears in the focus path). The fix is not a focus API; it is Line 2 (the drag-begin afford).
- **Apps that need layout-order focus** (e.g. column-major grids): creation order is a projection of app declaration order — such an app declares regions in the order it wants focus to follow (or re-declares). If a real consumer needs a graph-declared focus order (a FOCUS_ORDER edge kind), that consumer's evidence can justify it; none exists today.

## Consequences

### Positive

- **L15a retires** — a keyboard user is now served by intent compilation: the ADR-0017 claim holds on two channels.
- **P61 flips ⚠️ → ✅** — "Focus is transient, not state" is answered: the *ring* is derived graph data, the *focus position* is interaction state reported by value (`on_focus` carries the label; the app may write it into an Estimate). Corpus: 39/23/6.
- The a11y bridge gains its focus seam: AT-SPI2 navigation is focus traversal, and focus order is now graph data the bridge can project (Cross-cutting A's precondition).
- The dual-path retirement decision (Line 3) moves closer: keyboard no longer needs the legacy path.

### Negative

- Two payload types share the closure surface; dual-channel actions must sort by payload shape (one `if`, but it is a new micro-rule).
- The activation key set (Enter/Space) is a convention, not a graph decision — a per-region activation-key affordance would be data, this release ships the constant default. Recorded, not hidden.

### Neutral

- Legacy key dispatch (`on_key`) is untouched when `intent_graph` is NULL — the v0.7 opt-in posture carries over exactly.

## Alternatives Considered

### Alternative 1: A declared focus API (px_region_focusable(r, bool) + explicit rank)

Rejected: duplicates what the graph already holds (AFFORDS edges), adds bookkeeping that can drift from the afford declarations, and creates a second ontology for "what is interactive." The v0.6 lesson — hit-testing was already a graph query waiting to be read — applies to focus traversal unchanged.

### Alternative 2: Focus as an Estimate (framework-managed focus state)

Rejected for now: the focus position is interaction state scoped to a run (loop-local in `px_app_run`), not application semantic state. An app that wants focus-as-Estimate writes it from `on_focus` — the seam exists, the framework does not presume. If a real consumer needs framework-owned focus state, that evidence reopens this.

### Alternative 3: Route keys through the region-geometry hit test (focus = pointer position emulation)

Rejected: it would make the keyboard a second-class pointer — exactly the channel-subordination A6 forbids. The keyboard compiles from the *ring* (semantic), not from a synthesized *position* (geometric).

## CAVEATS

- This ADR does **not** claim full keyboard parity: arrow-key navigation, shortcuts, and focus-order-as-data are unmodeled; the activation keys are constants, not affordance data.
- win32/cocoa key events do not carry modifiers yet (their backends are code-complete but untested, L5/L9); the bit is simply never set there — no false positives.
- The orca end-to-end verification (L9) is still open; this ADR delivers the data seam, not the platform proof.

## Known issues

- `px_afford_focus_*` rebuilds the ring per call (collect + reverse + walk). At UI scale this is fine; if a consumer profile shows it hot, a cached ring with registry-change invalidation is the fix — deferred until measured (the compression-metric posture).
- The ring order is creation order; there is no graph API to declare a different focus order (see Q5 counterexample).

## HISTORY

- 2026-08-30: Accepted — focus ring + key compile landed; test_v08.c (18 tests) green; palette_afford.c keyboard session [9]–[13] green; P61 re-scored 39/23/6; L15a retired.
- 2026-08-30: v0.8 roadmap Line 1 named the retire path and its ordering logic (keyboard first — L15b and the dual-path retirement both depend on it).
- 2026-08-30: ADR-0017/0018 Known-issues recorded L15a/L15b at promotion time.

## References

- Code: [`src/hit.c`](../../../src/hit.c) (`px_afford_focus_*`, `px_afford_compile_focus`), [`src/app.c`](../../../src/app.c) (KEY_DOWN compile path), [`src/x11.c`](../../../src/x11.c) + [`src/headless.c`](../../../src/headless.c) (modifiers + named keys), [`include/planex/planex.h`](../../../include/planex/planex.h) (`px_key_intent`), [`include/planex/app.h`](../../../include/planex/app.h) (`on_focus`), [`include/planex/window.h`](../../../include/planex/window.h) (`px_event.modifiers`, `PX_MOD_*`)
- Evidence: [`tests/test_v08.c`](../../../tests/test_v08.c); [`examples/palette_afford.c`](../../../examples/palette_afford.c) steps [9]–[13]
- ADRs: [ADR-0017](ADR-0017-intent-compilation-promotion.md) (the promotion whose gap this retires), [ADR-0018](ADR-0018-interaction-process-promotion.md) (the joint obligation; L15b remains), [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) (the admission bar's spirit, applied in the Essence Check)
- Corpus: [ui-pattern-corpus.md](../../reference/ui-pattern-corpus.md) (P61 re-score, 39/23/6)
- Limitations: [limitations.md](../../concepts/state/limitations.md) (L15a retired; L15b open)
- Roadmap: [v0.8-roadmap.md](../../concepts/state/v0.8-roadmap.md) Line 1
