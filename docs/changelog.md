# Changelog

All notable changes to Planex are documented here.

Format based on [Keep a Changelog](https://keepachangelog.com/).

---

## [0.7.0] — 2026-08-30

### Added — v0.7: intent compilation promoted to the 6th canonical abstraction (ADR-0017)

- `px_pointer_intent`: the compiled-pointer value — region label embedded by value (replay-safe after the region is freed), x/y/button ride along as payload context, not routing keys.
- `px_afford_compile(g, x, y, button, out)`: the window-free compile step (pure function of registry + graph + position); miss zeroes the payload so stale data cannot leak through the fallback path.
- `px_app_desc.intent_graph`: opt-in routing in `px_app_run` — pointer-downs compile before dispatch; unresolved clicks fall back to `on_click`; NULL keeps legacy dispatch identical (zero cost when unset).
- Multi-edge resolution specified and pinned: a region with several AFFORDS edges resolves **last-declared-first** (tests/test_v07.c a7).
- `examples/palette_afford.c`: the real-application evidence — five affordances, one routing rule, zero raw-coordinate callbacks; button-3 context action discriminated in the payload; slider drag as inert trajectory + one committed write; undo through the graph.

### Added — v0.7: px_interaction promoted to the 7th canonical abstraction (ADR-0018)

- The v0.6 prototype is canonical; THE INVARIANT (inert samples, transitions-only seams) is now normative, enforced by test_v06_interaction.c section D. `publish_phase` remains the only sanctioned seam to Estimate. No signature changes.
- Corpus Category D re-scored (same-commit amendment): P24–P28, P32, P36 flip to ✅ CLEAN (EXAMPLE-grounded); P29/P37 downgrade ❌→⚠️. Distribution 31/29/8 → **38/24/6**; test_completeness constants updated (75/75 checks).
- Abstraction count 5 → 7 across all non-exempt docs; `check_stale_abstraction_count.sh` now enforces the 7-count (stale 5/five pattern + stops-at-px_loop enumeration pattern).

### Added — v0.7: Path C adoption posture (Cross-cutting B)

- Host separability is now a **stated property**: `abstraction-form.md` and `intent.md` each gain a section — the 7 canonical abstractions are portable invariants; C17 is the first host, not the ontology; if the embeddability bet fails the ideas move hosts rather than die with one (the falsifiability machinery — leak budgets, corpus, admission bar — is documents and tests, not code). The inverse is recorded with it: no ecosystem-rewrite demands — every v0.7 feature is opt-in and drop-in (the wedge the 60-year Path C failure record demands; the Intentional-Programming demand is structurally refused).

### Changed — v0.7: Closure constructor split retires the last L2 (Line 5, ADR-0019)

- `px_closure_new_with_graph(goal, kind, action, evaluation, user, graph)`: the undo graph arrives with the closure — the bind-before-trigger ordering rule is deleted from the API's grammar (the mistake is unwritable). `px_closure_bind_graph` is **deprecated** (registry entry; removal candidate v1.0), still functional through the window; the v0.6 one-time warning now guards only deliberate unbound use.
- **Aggregate L2 = 0/99 = 0%** — first zero. Retire curve: 17% (v0.4) → 3.8% (v0.5) → 1.7% (v0.6) → 0% (v0.7). The leak-budget gate holds it there.
- Examples migrated to the safe form: `undo_via_graph.c`, `palette_afford.c`, `integration_4abs.c`, `counter_perception_window.c`. Tests: `test_v07.c` section E. `UPGRADING.md` gains the v0.7 migration entries (deprecation + default-budget behavior change + additive structs).

### Added — v0.7: a11y AT-SPI2 bridge behind the query-side contract (Line 4)

- `src/a11y_bridge_atspi.c`: the Linux adapter (ATK + atk-bridge provider path) — a replaceable adapter behind the v0.6 query-side contract, not an ontology commitment. The mirror is minimal (application root + current element + alert): roles/states/names/values sync on flush; the announcement ring drains into the alert object (orca reads alert name changes). Known limits recorded in-source: values ride in the description (no AtkValue on no-op objects), flat tree, identical consecutive announcements fire once.
- `px_a11y_bridge_atspi_attach/flush/detach` (a11y.h): without `-DPX_A11Y_ATSPI` the bridge is an honest stub — attach returns NULL, no-ops are NULL-safe (test_v07 d1); zero dependency, zero regression.
- CI gains the `a11y-atspi-bridge` compile-probe job: installs atk headers and compiles the adapter under the flag — absent headers report "condition unmet" (row stays partial); present headers make the compile blocking (API drift is caught).
- `PLATFORMS.md`: Linux accessibility flips from "API + logging" to "AT-SPI2 bridge behind PX_A11Y_ATSPI + query side". `limitations.md` L9 partial-resolution: orca end-to-end verification remains the open external condition.

### Fixed — v0.7: edge lifecycle — px_undeclare (the CI-found dangling edge)

- The v0.7 push's first CI run exposed two real defects; both are fixed here.
- **Dangling AFFORDS edges**: `examples/hover_drag_interaction.c` aborted on its final affordance assertion on the Ubuntu runner while passing on Debian — the example freed and recreated the drag process but left its five AFFORDS edges pointing at the dead object; `px_afford_at` returned a freed pointer. The local pass was allocator layout luck (Debian glibc reused the freed address; the runner's glibc did not). No Planex API could express the honest fix: the graph had create/read without delete.
- `px_undeclare(g, a, kind, b)`: retires the first edge matching (a, kind, b) — the edge-lifecycle counterpart of `px_has_relation`, symmetric predicate, propagation-accounted. **Edge-lifecycle contract** stated at the declaration site: edges name endpoints by pointer, nothing cascades, the declarer retires before freeing the endpoint.
- `hover_drag_interaction.c`: the re-arm sequence now retires → frees → recreates → re-declares; the assertion tightens to the live process pointer (no address-reuse luck involved) — its second arm (`commit_reorder`) was never an AFFORDS target.
- `tests/test_v07.c` section F (3 tests): retirement semantics, sibling-edge sparing, and the dangling-edge regression pinned as the discipline.
- **CI `a11y-atspi-bridge`**: `at-spi2-atk` no longer exists on Ubuntu 24.04 (folded into at-spi2-core); the probe installs the split dev packages `libatk1.0-dev` (atk headers) + `libatk-bridge2.0-dev` (the bridge header) and gates on the `atk` / `atk-bridge-2.0` pkg-config entries — the header's on-disk layout is distro-dependent (jammy: `atk-bridge-2.0/`, noble: `at-spi2-atk/2.0/`), so pkg-config is the only reliable resolver.
- **The probe's first real compile found two portability defects in the adapter** (exactly the API-drift class it exists to catch): the source hard-coded `#include <atk-bridge-2.0/atk-bridge.h>` (jammy layout; unwritable on noble) and omitted `_POSIX_C_SOURCE` (strict c17 hides `nanosleep` from planex.h's `px_sleep_ms`). Fixed: the source includes `<atk-bridge.h>` bare — the build's pkg-config cflags resolve the layout — and defines `_POSIX_C_SOURCE 200809L` like every other source. Verified against real atk 2.52 + glib 2.80 headers: clean `-Wall -Wextra` compile; the ATK API calls themselves had zero drift.

### Added — v0.7: Estimate schema — the describable value contract (Line 3)

- `px_estimate_schema` (kind + name + optional print/equal) beside the value: opt-in via `px_estimate_set_schema` (borrowed pointer, app-owned static const; zero cost when unset). Not a type system — a describable contract: tests assert "this estimate is INT and equals 3" (`px_estimate_schema_of` + `px_value_kind_name`), values denotate kind-aware (`px_estimate_describe` — INT/DOUBLE/PERCENT/BOOL defaults or custom print), equality is kind-aware (`px_estimate_value_equal` — exact for discrete kinds, 1e-9 for DOUBLE).
- `px_a11y_set_value_estimate`: the a11y query side reads the schema for value naming — the seam the Line 4 platform bridges adapt.
- Leak-budget: the void* L1 entry gains its retirement path — the **contract half** closes (the schema declares what the value means); the **pointer half** (void* user in callbacks) stays as documented permanent C17 host cost. Estimate section re-enumerated: 27 ops, L1 4, L2 0.

### Added — v0.7: budget as contract (Line 2)

- `PX_LOOP_DEFAULT_BUDGET_MS` (16ms): every `px_loop` ships with a deadline — the feedback axiom's "instantly visible" given a number. `px_loop_set_budget(loop, 0)` is the explicit opt-out. Overruns are loud: warn-once on stderr in all builds, abort under `-DPX_DEBUG_BUDGET` strict mode; `px_loop_budget_overruns()` counts.
- Propagation accounting in every audit entry: `propagation_edges` (per-step delta of the new `px_relation_edges_walked()` monotonic counter) + `propagation_depth` (`px_derive_depth_peak()` / `px_derive_depth_reset()` — read and reset are separate ops so no query carries a side effect).
- `tests/test_v07.c` section B (4 tests); `test_v06_interaction.c` j1/j2 updated for the default (j2 renamed `budget_explicit_opt_out`).

### Changed — v0.7.0 release cut

- `PLANEX_VERSION` bumped `0.6.0` → `0.7.0`. **No git tag** — per the release convention, a release is the version constant + this changelog cut (the v0.6.0 tag was deleted at maintainer direction; tags are not part of the release process).
- `docs/concepts/state/v0.7-roadmap.md` → `docs/concepts/history/v0.7-roadmap.md` (superseded banner added; all 8 referencing docs re-pointed). New `docs/concepts/state/v0.8-roadmap.md` takes its place: keyboard affordances (L15a) → drag-begin afford (L15b) → dual-path retirement decision, with the conditions ledger carried forward at its v0.7.0 state.

## [0.6.0] — 2026-08-30

### Added — v0.6: interaction prototype (ADR-0016, proposed)

- New `px_interaction` process abstraction (prototype, not canonical): begin → sample* → commit|cancel phase machine with a trajectory ring and pure-metric accessors (position / velocity / distance / duration), built on the design invariant **sample laziness** — zero observer notifications and zero perception invocations while no one samples. Three bridges: `on_phase` hook (BEGAN / MOVED / ENDED / COMMITTED / CANCELLED transitions), Closure `on_commit` / `on_cancel` triggers, `publish_phase` converting transitions to Estimate writes. Implemented in `src/interaction.c` (~370 lines); 27 tests in `tests/test_v06_interaction.c` (sections A–F prototype, G–J v0.6 retire verification), wired into all 4 CI jobs (linux-cmake, linux-make, windows, strict-warnings). Auto-begin semantics: the first sample enters the trajectory *before* the BEGAN transition fires (the starting event belongs to the trajectory).
- New `px_region` + `px_afford_at` (prototype): global region registry with z-order scan and AFFORDS-graph affordance query — intent compilation as a graph query instead of raw-coordinate hit-testing. Implemented in `src/hit.c` (~160 lines).
- New example `examples/hover_drag_interaction.c` + `.expected` snapshot: the boundary-closure demo against `hover_drag_4abs.c` (130 mouse events → 2 estimate writes; 5 of 7 HACKs retired; cancel as a first-class outcome; swipe derivable from metrics).
- ADR-0016 (`docs/decisions/proposed/ADR-0016-interaction-prototype-option-b.md`, proposed): follows the ADR-0009 v3-prototype precedent — evidence-gathering code without touching the 5 canonical abstractions. Promotion to a 6th canonical abstraction requires its own ADR against the ADR-0011 admission bar, real-application evidence, and a Category D re-scoring.

### Added — v0.6: six audit-fix retires (aggregate L2 3.8% → 1.7%)

- `PX_EV_WHEEL` added to `window.h` + x11 (Button4/5) / win32 (WM_MOUSEWHEEL) / cocoa (scrollWheel) backends + `on_wheel` app callback.
- Closure `bind_graph` omission is now loud: one-time stderr warning when undo is enabled with no graph bound (the ordering dependency itself remains; the constructor split is the v0.7 retire path).
- Estimate: `px_estimate_predict` / `px_estimate_surprise` — Friston predictive-loop seeds; confidence gains a framework-side consumer.
- Perception: `px_perception_set_free_fn` — opt-in representamen destructor; the cache leak is retired across clear_cache / fire / free paths.
- px_loop: view-only and replay branches now invoke only the loop's bound perception (scope leak retired — feedback section L2 → 0%, was 9%); audit entries gain `iteration_ms` / `budget_ms` / `budget_exceeded`; `px_loop_set_budget` / `px_loop_budget`.
- a11y: query-side API (getters, 16-entry announcement ring, `set_verbose`) — the stable contract that platform bridges will adapt to; render fast path (row-level memcpy) in `app.c`.

### Changed — v0.6 documentation sync

- `limitations.md` L12: "not abstracted" → "prototype landed in v0.6 (canonical promotion pending)", with the open items (multi-touch / NG-6, intent gradient, Category D re-scoring, promotion ADR).
- `leak-budgets.md`: v0.6 retire summary — aggregate L2 3.8% → 1.7% (remaining: Closure `bind_graph` ordering dependency, loud since v0.6).

### Added — Abstraction-form comparative study (research report)

- New `docs/research/2025-08-28-abstraction-as-form-comparative-study.md` (289 lines) — survey of 8 alternative organizational forms (DSL, component library, pattern language, ECS, FRP, data-driven config, Kay-OOP, tagless final), 6 critique traditions (Worse-Is-Better, Simple Made Easy, Leaky Abstractions, Abstraction Inversion, Rule of Three, Over-Abstraction/End-of-Civ), 5 philosophy-driven design precedents (Winograd/Flores Coordinator, Conal Elliott Fran/Pan, Dourish/Ishii embodied UI, Friston free-energy, Kay Smalltalk), and 9 production C/UI libraries for calibration (SDL, GLib/GObject, Cairo, Wayland, libuv, GTK, Qt, Dear ImGui, Redis). **Verdict**: abstraction is the correct primary form for Planex's stated goals (intent-as-value, multi-channel denotation, cognitive-bandwidth constraint, C17 feasibility, no-AI-as-driver per ADR-0003), with three caveats — Planex must (1) explicitly distinguish "abstraction-as-typed-value" (Planex's form) from "abstraction-as-encapsulation" (rejected), (2) explicitly rebut the Rule of Three via an essence-justified vs duplication-justified ADR, (3) quantify per-abstraction leak budgets. Seven gaps identified with three tiers of recommendations (T1.1–T1.3, T2.1–T2.4, T3.1–T3.3). The 32 web searches and 7 page_reader fetches are not committed to the repo; their artifacts are kept under `/home/z/my-project/research-task14/` and `/home/z/my-project/scripts/extract_page_text.py`. Status: reference research output, not a decision.

### Added — v4 essence derivation clean-room (not yet an API; design artifact)

- New `docs/concepts/history/essence-derivation-v4-clean.md` (767 lines) — clean-room re-derivation of Planex's abstraction surface, with methodological self-audit (Part VII) against 10 constitutive demands of first-principles derivation across 6 methodological traditions (Aristotelian, Cartesian, Husserlian, Popperian, Quinean, Wittgensteinian + Brooks/Lakatos for software-specific demands). **Result: v4 meets 0 of 10 demands**; the framing is downgraded from "essence-derived" to "tradition-grounded design rationale".
- New `v4/` prototype: 8 abstractions (Intent / Relation / Closure / Interpretant / Perlocution / Estimate / Actor / Perception + Loop binding) + 9 test binaries (~133 assertions, all green, zero warnings). v4 is a verification artifact, **not** a shipping API (v0.4 unchanged).
- Audit sources (60 Wikipedia articles): `/home/z/my-project/research/firstprinciples/body/*.txt` (not committed to repo; single-source Wikipedia for this audit, deeper work would consult primary texts).

### Changed — Framing downgrade (ADR-0010)

- **ADR-0010** (`docs/decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md`) accepted: v1–v4 essence-derivation lineage is now framed as design rationale, not essence discovery. Code and API unchanged. Future ADRs/docs use "8-abstraction design proposal" not "8 essence categories"; the word "deferred" is replaced by three-state labeling (essence-but-unimplemented / not-essence / undecided). ADR-0009 remains Proposed; its implementation decisions stand, its essence-claim language is downgraded. The 10 constitutive demands of first-principles derivation (see ADR-0010 Context) become the project's institutional bar for any future re-claim of "first-principles derivation". v5 path (genuine re-derivation per Part VII.11 of v4 doc) is left open but not committed.

### Added — Documentation versioning industry survey (research report)

- New `docs/research/2025-08-28-doc-versioning-top-solutions.md` — survey of 8 foundational documentation traditions (Diátaxis, Docs as Code, SemVerDoc, ADRs, C4 + arc42, Literate Programming, RFC/PEP/TC39, SSoT/DRY), 6 platforms (MkDocs Material, Docusaurus, GitBook, Sphinx, Backstage TechDocs, Docsy), and 5 large-project case studies (Linux Kernel, Kubernetes, Python, Rust, arXiv). Confirms Planex's three-system numbering (doc version / section number / software version) is industry-standard, not a defect — arXiv, PEPs, and Rust RFCs all use the same pattern. Identifies gaps: (1) no `Stability` field on concept docs (rust-lang-style experimental/unstable/stable/legacy), (2) no machine-readable `Superseded by` cross-link on ADRs, (3) no CI link-checker, (4) no RFC track separate from ADRs, (5) no generated API reference. Three-tier recommendations provided. Status: reference research output, not a decision.

### Changed — Document versioning consistency

- Renamed `docs/concepts/essence-derivation.md` → `docs/concepts/history/essence-derivation-v1.md` (filename now matches the v1/v2/v3/v4 convention used by its successors; the doc was already referred to as "v1" everywhere but its filename and title didn't show it).
- Updated H1 title to "Essence Derivation v1" (was just "Essence Derivation").
- Updated H1 title of `docs/concepts/history/essence-derivation-v3.md` to "Planex Essence Derivation v3 — A First-Principles Audit" (was "Planex Essence Re-derivation — A First-Principles Audit"; filename had v3 but title didn't).
- Updated all cross-references to the renamed v1 file across 5 docs (ADR-0007, ADR-0010, changelog, why-four-abstractions — 2 occurrences).
- Added `docs/concepts/state/versioning.md` — authoritative reference for the three independent number systems used in Planex docs: (1) document version (`v1`, `v2`, `v3`, `v4` — Arabic, no dot, for derivation-lineage docs); (2) section number within a long-form doc (`Part I`, `Part VII` — Roman, for sections, not versions); (3) software release (`v0.4`, `v0.4.0`, `v1.0+` — Arabic, with dot, for code releases per SemVer).
- Added `> **Applies to**: v0.4` status lines to 10 concept docs that previously had no version marker (architecture, glossary, limitations, non-goals, ui-essence-layers, why-four-abstractions, alternative-perspectives, path-C-lineage, roadmap-matrix, ui-pattern-coverage). Docs without a marker are now either tied to v0.4 explicitly or noted as release-independent.

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

- New `docs/concepts/history/essence-derivation-v2.md` — based on 6-tradition literature survey (2835 lines of research reports in `research/reports/`)
- v1 (`essence-derivation-v1.md`) marked SUPERSEDED with summary of v2 corrections
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
