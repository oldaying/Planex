# ADR-0017: Promote intent compilation (region + AFFORDS + compile step) to the 6th canonical abstraction

## Status

Accepted

Date: 2026-08-30

Promotes the v0.6 afford/region prototype ([ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md)) to canonical status, per the [v0.7 roadmap](../../concepts/history/v0.7-roadmap.md) Line 1 and the admission bar of [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md). This ADR is also a corpus amendment: it re-verdicts three Category D patterns (P24, P25, P32) in the [UI Pattern Corpus](../../reference/ui-pattern-corpus.md); the companion promotion [ADR-0018](ADR-0018-interaction-process-promotion.md) re-verdicts the rest of the v0.7 re-score.

## Context

The v0.5→v0.6 audit's D-A1 finding: from "where the user clicked" to "which Closure fires with what payload" was outsourced to every application. `px_app_run` dispatched raw coordinates to `on_click(x, y)` and every app hand-rolled its own hit-testing — the translation segment of Norman's execution gulf lived outside the framework. v0.6 landed the vocabulary as a prototype: `px_region` (pure data), `PX_REL_AFFORDS` (already in the Relation graph since the 6-tradition survey), and `px_afford_at` (the graph query). But a prototype nothing routes through proves the query works without proving anything depends on it.

v0.7 Line 1 closed that gap with three pieces of evidence, all shipped in this release:

1. **The routing integration.** `px_app_desc.intent_graph` (opt-in): when set, `px_app_run` compiles each pointer-down via `px_afford_compile` before dispatch — the topmost region at `(x, y)` resolves against the graph's AFFORDS edges, and the afforded closure triggers with a `px_pointer_intent` payload. Unresolved clicks fall back to `on_click` unchanged; `NULL` (the default) keeps legacy dispatch byte-for-byte. The compile step is window-free by design — a pure function of (registry, graph, x, y) — so the routing decision is testable without a backend.
2. **The real application.** [`examples/palette_afford.c`](../../../examples/palette_afford.c): a palette painter (three swatches, brightness slider, canvas, reset) whose click handling has **zero raw-coordinate callbacks** — `on_click` is `NULL` by design. Five affordances, one routing rule, no coordinate branches in application code. The canvas closure *uses* `x`/`y` as payload data (where the dot lands) without ever *routing* on them; button-3 compiles to the same intent shape and the closure discriminates in the payload — a context-menu action (P32) with no position branch.
3. **The test suite.** [`tests/test_v07.c`](../../../tests/test_v07.c) section A (7 tests): the compile step's resolution and payload contract, the value-ness of `px_pointer_intent` (the region label is embedded, so the intent survives capture + replay after the region is freed — test a3), the app-level routing semantics (afforded → closure; unresolved → raw fallback; opt-out → legacy), and the pinned multi-edge resolution rule (last-declared AFFORDS edge wins — test a7, a rule discovered by building the real app and specified rather than left implicit).

This is also the industry-wide gap the promotion claims. Mainstream frameworks still route interaction on pixels and callbacks. The strongest prior claim on the alternative — presentation-typed interaction routing (CLIM, 1980s) — died with its host for economic reasons, not because the idea was refuted ([path-C-lineage.md](../../concepts/background/path-C-lineage.md)). The conditions that killed it (an expensive closed ecosystem) no longer hold; the claim is unclaimed. Planex's version has one specific advantage over the nearest mainstream successor: the type drives **interaction routing itself**, not just read-only navigation (LSP-style "click to definition" is the weakest projection of this idea).

## Decision

Intent compilation is promoted from prototype to the **6th canonical abstraction**. Concretely:

- **Constitutive question:** *"What does this input denote?"* — the decode step from physical event to semantic act. The existing five each own a different question (Estimate: what will the world be? Perception: what is the world? Closure: what completed? Relation: what is connected to what? px_loop: when does control yield?); none of them owns input→intent translation. Closure owns the act *after* it is identified; intent compilation owns the identification itself. The companion [ADR-0018](ADR-0018-interaction-process-promotion.md) owns the time-bounded half (*what is the user doing over time?*) — the noun world and the verb world of the same gulf, deliberately separate.
- **Canonical API surface:** `px_region_new/free/rect/label/set_rect/at`, `PX_REL_AFFORDS` edges, `px_afford_at`, `px_pointer_intent`, `px_afford_compile`, and the `intent_graph` routing path in `px_app_run`. `PX_REGION_LABEL_MAX` becomes public (the intent value embeds it).
- **Opt-in is permanent, not transitional.** The abstraction is canonical; the routing path stays opt-in per the Path C posture (v0.7 roadmap Cross-cutting B): zero cost when unset, zero ecosystem demands. `px_afford_at` remains usable standalone (a pure graph query) — the abstraction is the compile step, not a required app skeleton.
- **Resolution rule (specified, was implicit):** a region with multiple AFFORDS edges resolves **last-declared-first** (`px_declare` prepends; `px_query` walks most-recent-first). Pinned by test a7. Multi-act regions should afford a single closure that discriminates by payload (the `palette_afford.c` canvas pattern), not multiple closures racing on declaration order.
- **Scope boundary (recorded, not hidden):** the routing path compiles **discrete acts at pointer-down**. Continuous intents (drags) are the interaction process's domain (ADR-0018); a drag-begin does not afford-route. This boundary is what keeps the two abstractions orthogonal.
- **Corpus amendment (this ADR's amendment, per the closing rule):**
  - P24 Hover highlight: ⚠️→✅ — hover is a region query computed at render time ([`hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c)); no estimate churn.
  - P25 Mouse cursor position: ⚠️→✅ — the pointer is an ambient sample (a plain struct field), derived on demand; the demo retired the 60fps-estimate hack.
  - P32 Context menu (right-click): ⚠️→✅ — button-3 compiles to `px_pointer_intent` with the region label embedded; position context is *in the intent value* ([`palette_afford.c`](../../../examples/palette_afford.c) §3).
  - (P36 Color picker is co-grounded: afford region + interaction drag; its flip is recorded in ADR-0018 with the interaction re-score.)

### Essence Check (admission bar per ADR-0011)

1. **Tradition traceability.** Gibson, *The Ecological Approach to Visual Perception* (1979): affordances are relations between organism and environment, picked up directly — "the affordance of something does not change as the need of the observer changes," and perceiving an affordance is perceiving a *use*, not a geometric property. `PX_REL_AFFORDS` has cited Gibson since the 6-tradition survey; this promotion makes the *routing consequence* of that citation real (the affordance, not the pixel, selects the act). Engineering precedent: CLIM's presentation types (presentation-clicking routes on the presentation's type, 1980s) — the lineage record shows it lost its host, not its argument.
2. **Constitutive orthogonality.** The question "what does this input denote?" is answered by no other abstraction. Precondition for the test: remove intent compilation and the app is back to `on_click(x, y)` hand-rolled hit-testing — the v0.5 state, which the audit scored as the D-A1 leak. The compile step touches no Estimate, no Perception, and no Closure until it triggers one (purity is what test_v07 section A exercises without a backend).
3. **Denotational semantics.** The product is `px_pointer_intent` — a value with the region label **embedded by value** (not pointed to), so it survives `px_closure_last_intent` capture and `px_closure_replay` after the region itself is freed (test a3). It denotates across channels: the audit log records the label, the a11y query side can name the region, and serialization/replay is payload-shaped (no pointer fields). This is exactly the "intent as a value" payoff the project claims.

## Essence Check

> This decision adds a canonical abstraction — the maximal form of an essence-affecting change. The Q1–Q5 checks below apply the ADR-0011 admission bar rather than merely restating it.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface (the criterion by which entries are admitted)
- [x] **Intent space** — the user→machine direction. This is the first canonical abstraction on the decode side of the execution gulf: physical input → semantic act. Estimate/Closure/Relation/Perception/px_loop all live *after* the intent exists; intent compilation is where the intent is identified.
- [ ] State space
- [ ] Presentation space
- [ ] Feedback space (ADR-0008's 5th category — closed-loop coupling; untouched)

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses, measurably: one routing rule replaces N per-app hit-testers. `palette_afford.c` routes five affordances through one compile call with zero coordinate branches; the pre-v0.6 idiom (`counter_interactive.c`) pays one hand-rolled hit-region block per interactive element. The label-driven single-closure pattern (one `select_color` closure serving three swatches) compresses further: the *type* does the dispatch that per-button callbacks would otherwise do. The cost side: an app that opts in must think in region declarations + AFFORDS edges — one concept, declared once, versus N closures wired by coordinates. For apps that never set `intent_graph`, the cognitive cost is exactly zero.

### Q3. Is there a gap between the claim and the implementation?

Yes, and it is recorded rather than hidden:

- **Label capacity.** Region labels cap at 63 bytes (`PX_REGION_LABEL_MAX`, truncation pinned by test a6). Long semantic names need app-side mapping.
- **One closure per resolution.** `px_afford_compile` returns the last-declared affordance, not a candidate set. Multi-act regions use the single-closure-payload-discrimination pattern (canvas in `palette_afford.c`); a candidate-list API would be a new decision, not a patch.
- **Discrete acts only.** Pointer-down compiles; drag-begin does not (ADR-0018's domain). An app whose primary interaction is dragging still wires `on_mouse_move`/`on_mouse_up` raw — the slider in `palette_afford.c` documents this boundary in place.
- **No keyboard affordances yet.** Regions are pointer geometry; keyboard traversal (focus order, activation keys) is not compiled. Future work, recorded in limitations.

### Q4. What is the cost, and who can verify it?

- **Cost:** the routing scan is O(regions) per pointer-down (registry is a declaration-ordered list; z-order is declaration order). For typical UI region counts (<10³) this is noise; it is *not* a spatial index, and a hostile 10⁵-region app would feel it. The API surface grows by 9 symbols + 1 struct + 1 constant.
- **Who can verify:** anyone. `make test_v07` runs the compile-step contract without any backend; `make check-examples` runs the real application end-to-end; `make check-completeness` enforces the amended corpus distribution. The corpus gate is the drift-catcher: if a future change re-breaks P24/P25/P32, the pattern table and the test disagree and CI fails.

### Q5. What are the counterexamples?

- **Plain hit-testing without AFFORDS.** `px_region_at(x, y)` alone is a geometry query — utility use, not the abstraction. The abstraction is the compile step (region + graph + payload). An app using regions purely for hover math is not "using intent compilation" in the canonical sense, exactly as an app using `px_query` for bookkeeping is not "using Relation's undo proof".
- **Canvas painting.** A paint app genuinely needs the raw position — and gets it, as payload *data*. The distinction this ADR formalizes: the routing key is the region; the position is context. `palette_afford.c` demonstrates both in the same closure (paint uses x/y; routing never did).
- **Free-form input (drawing tablets, gesture pads).** Where there is no meaningful region structure, afford routing degenerates to one fullscreen region — which is honest (the whole surface affords the act) but adds nothing over raw dispatch. The abstraction claims nothing for these cases.

### Conditions

Per the v0.7 roadmap's conditions-ledger practice — the external conditions this capability rides on, and their current state:

| Condition | State at promotion | Notes |
|---|---|---|
| Pointer events with position + button (backends) | met (x11/win32/cocoa/headless since v0.6) | PX_EV_WHEEL also landed; touch remains NG-6 |
| A graph-query surface able to type edges (Relation) | met since v0.3 | AFFORDS edges are ordinary graph citizens |
| A value-carrying trigger pipeline (Closure) | met since v0.1 | `px_pointer_intent` rides `px_closure_trigger`'s copy semantics |
| CLIM-lineage precedent available for re-claim | met | unclaimed since the 1980s; conditions that killed it no longer hold |

Proof obligations going forward: keyboard affordances (limitations L-series entry), drag-begin affordance seam (joint with ADR-0018, recorded in both).

## Consequences

### Positive

- The execution gulf's translation segment is framework-owned. "Which Closure fires with what payload" is answered by a graph query an auditor can read, not by N private hit-testers.
- `px_pointer_intent` makes routed clicks replayable and serializable — the same intent-as-value contract as every other abstraction, now on the input side.
- The corpus re-score (with ADR-0018) takes Category D from 0/15 clean to 7/15 clean — the project's largest single falsifiability improvement since the corpus closed.
- The unclaimed 40-year CLIM position is claimed in a zero-dependency C17 form: the type drives routing itself, and the host-die condition that killed the predecessor is structurally answered (the abstractions are host-portable; see Cross-cutting B).
- The opt-in posture means zero migration cost: every existing app compiles and behaves identically.

### Negative

- The abstraction count moves 5→7 in one release (with ADR-0018). Documentation, the README tagline, the leak-budget ledger, the roadmap matrix, and the stale-count gate all re-sync in the same commit — a large, mechanical surface with real drift risk if split.
- Region labels are the semantic type system, and they are strings capped at 63 bytes. Type safety is by convention (label comparison), not by the compiler. A typo in a label fails at runtime, silently (the closure simply doesn't match). This is the honest cost of C17 without a type-description layer — Line 3 (Estimate schema) is the partial answer on the value side.
- The multi-edge resolution rule is declaration-order-sensitive. Correct code doesn't notice; incorrect code gets the wrong closure deterministically, which is better than nondeterministically, but still wrong.

### Neutral

- `px_afford_at` (v0.6 signature) and `px_afford_compile` (v0.7) coexist: the former is the pure query, the latter the routing payload producer. Both canonical; no deprecation.
- The region registry remains process-global (like the perception registry) — single-window apps are the design center; multi-window needs per-window registries (NG-12 territory).

## Alternatives Considered

### Alternative 1: Keep it a prototype until three real apps exist (strict Rule of Three)

Rejected: ADR-0011's essence-justified route exists precisely for this case — the tradition citation (Gibson), the constitutive question, and the denotational value are all documented and machine-checked now. Waiting for usage counts would apply the duplication-justified bar to an essence-justified candidate, which ADR-0011 explicitly names as a category error. The usage evidence (two examples, one real app, two test suites) is a bonus, not the basis.

### Alternative 2: Absorb routing into Closure (on_click receives a resolved payload)

Rejected: Closure owns "what completed?", not "what does this input denote?" A closure that both resolves and executes conflates routing with acting — the conflation the D-A1 audit flagged. It would also give every closure an implicit geometry dependency.

### Alternative 3: Absorb into Relation (AFFORDS edges alone, no compile step)

Rejected: the edges alone were the v0.6 state — vocabulary without routing, a prototype nothing depends on. Relation owns "what is connected?"; the compile step is a *reader* with a denotational product. Promoting the edges alone would promote a graph schema, not an abstraction.

### Alternative 4: Defer promotion, re-score corpus on prototype grounding

Rejected as dishonest: the corpus legend defines ✅ clean as expressible with the *canonical* abstraction set. Grounding clean verdicts on prototype machinery would inflate the corpus — the exact capability-claims drift the freshness gate exists to catch. The re-score and the promotion are one atomic decision.

## CAVEATS

- This ADR does **not** claim touch, gesture recognition, or multi-pointer support. NG-6 stands. Trajectory-derived gestures are ADR-0018's domain and remain *derivable*, not recognized.
- This ADR does **not** make `intent_graph` mandatory, recommended, or default. Apps that never compile an intent are fully supported and pay nothing.
- This ADR does **not** promise the region label mechanism is the final semantic-typing story. Line 3 (Estimate schema) addresses value typing; a richer type layer for regions would be a new ADR.
- This ADR does **not** claim CLIP/CLIM compatibility, CLIM semantics, or any fidelity to presentation types beyond the shared core idea (typed routing). The differences are deliberate: regions are data in *Planex's* graph, not a presentation object system.

## Known issues

- **Label typos fail silently at runtime.** `strcmp`-based matching in closure actions has no compile-time check. Mitigation path: a label-interning or constant-table convention in a future how-to doc; not a core API change.
- **No keyboard affordances.** Focus traversal and activation keys are not compiled — keyboard users cannot be served by this abstraction yet. Recorded in `limitations.md` with the promotion sync.
- **test_orthogonality.c does not yet enumerate the 6th/7th abstractions.** The admission evidence lives in `test_v07.c` A (purity of the compile step) and `test_v06_interaction.c` D (inert hot path). Folding both into the orthogonality suite's per-abstraction anti-pattern structure is follow-up work; the ADR-0011 gap note (criterion documented, enforcement by reviewer vigilance) applies unchanged.
- **The registry is O(n) scan per pointer-down.** Fine at UI scale; a spatial index would be a documented optimization behind the same API, not an API change.

## HISTORY

- 2026-08-30: Accepted — promotion + corpus amendment (P24, P25, P32) + conditions ledger row migrated into this ADR. Evidence: routing integration (`src/app.c`), real app (`examples/palette_afford.c`), 7 tests (`tests/test_v07.c` A).
- 2026-08-29: Prototype landed in v0.6 (ADR-0016, proposed) — vocabulary without routing.
- 2026-08-28: D-A1 audit finding recorded — the translation segment of the execution gulf was outside the framework.

## References

- Code: [`src/hit.c`](../../../src/hit.c) (region registry + affordance queries), [`src/app.c`](../../../src/app.c) (intent routing path), [`include/planex/planex.h`](../../../include/planex/planex.h) (`px_pointer_intent`, `px_afford_compile`, `PX_REGION_LABEL_MAX`), [`include/planex/app.h`](../../../include/planex/app.h) (`intent_graph`)
- Evidence: [`examples/palette_afford.c`](../../../examples/palette_afford.c) (real app, zero raw-coordinate callbacks), [`examples/hover_drag_interaction.c`](../../../examples/hover_drag_interaction.c) (hover as region query), [`tests/test_v07.c`](../../../tests/test_v07.c) section A
- ADRs: [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) (admission bar), [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md) (prototype decision), [ADR-0018](ADR-0018-interaction-process-promotion.md) (companion promotion — the verb world)
- Corpus: [ui-pattern-corpus.md](../../reference/ui-pattern-corpus.md) (amended Category D; closing rule followed)
- Prior art: Gibson, *The Ecological Approach to Visual Perception* (1979), ch. 8; CLIM 2.0 specification, presentation-based input editing; [path-C-lineage.md](../../concepts/background/path-C-lineage.md) (the conditions-led reading of the CLIM lineage)
- Roadmap: [v0.7-roadmap.md](../../concepts/history/v0.7-roadmap.md) Line 1 + Cross-cutting A/B
