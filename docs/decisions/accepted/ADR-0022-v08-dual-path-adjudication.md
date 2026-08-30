# ADR-0022: The dual-path adjudication — the raw-coordinate surface is a declared transition state

## Status

Accepted

Date: 2026-08-30

Decides **v0.8 roadmap Line 3** — the last open item on the intent-compilation ledger row. The outcome is the third of the three the roadmap named at planning time ("default-flip vs deprecation vs explicit keep — all three outcomes are acceptable; the admission bar decides, not enthusiasm"): **explicit keep, declared**. The raw-coordinate routing surface is not deprecated and the default is not flipped; it is split by role per event class, and each callback carries a named retirement condition. The claim that holds afterwards is precise: **the afford graph is the canonical routing surface for every event class it serves — pointer discrete, pointer continuous, keyboard focus + activation — and the raw callbacks are a declared transition state whose boundary is this document's census.**

## Context

ADR-0016 adopted the opt-in posture: `px_app_desc.intent_graph` NULL keeps raw-coordinate dispatch byte-for-byte identical, because in the v0.6 prototype phase the afford path had zero shipped consumers and forcing it would have been enthusiasm. ADR-0017 (pointer discrete), ADR-0020 (keyboard focus + activation), and ADR-0021 (pointer continuous, the process form) each shipped a real application routing through the graph with `on_click` NULL by design. The v0.8 roadmap named the residual risk at planning time: *"If it stays forever, the essence claim degrades to 'an optional convenience' — the exact drift pattern the conditions ledger exists to prevent"* — and deliberately scheduled this decision last, so it would have an evidence base instead of an enthusiasm base. Lines 1–2 shipped; the evidence base is now complete. This ADR is the adjudication.

### The evidence census (Task 1, recomputed 2026-08-30)

**Event classes dispatched by `px_app_run`**, and which surface serves each:

| Event class | Graph form | Raw callback's role |
|---|---|---|
| Pointer-down on a process-afforded region | `px_afford_compile_process` (ADR-0021) | — the process owns the down |
| Pointer-down on a closure-afforded region | `px_afford_compile` (ADR-0017) | `on_click` = fallback on compile-miss |
| Pointer-down on unafforded space | — | `on_click` = the catch-all |
| Pointer move/up during a compiled gesture | sample / commit (ADR-0021) | `on_mouse_move` / `on_mouse_up` suppressed — the process owns the stream |
| Pointer move/up otherwise | — | `on_mouse_move` / `on_mouse_up` = the surface (ambient streams; hover-as-query is the graph-side alternative, ADR-0017) |
| Keyboard Tab / Shift-Tab | focus-ring walk (ADR-0020) | `on_key` = fallback when the ring is empty |
| Keyboard Enter / Space | `px_afford_compile_focus` (ADR-0020) | `on_key` = fallback when focus affords nothing |
| Keyboard — everything else | **none** | `on_key` = **the only surface** (shortcuts, arrows — ADR-0020 CAVEATS) |
| Wheel | **none** | `on_wheel` = **the only surface** (L12 owns the abstraction question) |
| IME commit | **none** | `on_ime_commit` = **the only surface** (text commit, not a routing concern) |

**Examples** (25 total; 6 do event routing at all):

| Example | Routing | Recorded reason |
|---|---|---|
| `palette_afford.c` | graph (headless; replicates the `px_app_run` decision verbatim) | the v0.7/v0.8 pointer + keyboard evidence app; `on_click` NULL by design |
| `designer_tools.c` | graph (headless; replicates the decision verbatim) | the Line 2 process-form evidence app; `on_click` NULL by design |
| `hover_drag_interaction.c` | hand-wired process begin (headless) | the ADR-0018 evidence app, written before intent compilation existed; its begin idiom is the documented pre-graph form, and its affordance check reads the process form (ADR-0021's honest evolution) |
| `hover_drag_4abs.c` | raw (windowed, `px_app_run`) | the boundary-exposing demo per ADR-0006's protocol — its raison d'être is to measure what the raw world costs; migrating it would erase the measurement instrument |
| `counter_perception_window.c` | raw (windowed, `px_app_run`) | the Perception demo (the probe/a11y vehicle); routing is not its subject |
| `perception_phase2.c` | none (headless) | no event routing |

**Corpus Category D metric** (the metric Line 3 named): 7/15 clean since v0.7 (P24–P28, P32, P36 ✅). The remaining D verdicts are bounded by **channel coverage, not by the dual-path question**: P29/P30 are touch (NG-6), P34/P38 are scroll (L12 — the wheel channel has no compile form, which is exactly this census's "only surface" row), P31 is timing (L11), P33 async, P37 undemonstrated. The v0.7 D re-score took the category from 0/15 to 7/15 with the raw surface fully in place — the afford path earned its canonicity on evidence, not on the raw path's retirement.

## Decision

1. **Explicit keep.** The raw-coordinate surface is not deprecated, and the opt-in mechanics do not change: `intent_graph` set → compile-before-dispatch (process form, then closure form, then the raw callback as fallback); `intent_graph` NULL → raw dispatch identical to v0.6. No API changes, no behavior changes, no default changes. What changes is doctrine: the posture matures from ADR-0016's *evidence-gathering* opt-in to a **declared transition state** with named boundaries.

2. **The surface is split by role, per event class** (the census table above is normative). For graph-served classes the raw callback is the *fallback* — the graph answers what it knows about declared regions, the app stays sovereign over the undeclared (a press on empty space, a key that compiles to nothing). For unserved classes (wheel, non-activation keys, IME) the raw callback is *the only surface* — not a legacy residue, the designed routing surface until a compile form exists. Both roles are declared in `app.h` at each callback and in `intent.md`'s routing-surface table.

3. **Per-callback retirement conditions.** A raw callback moves to the [deprecation registry](../../reference/deprecation-registry.md) — per the ADR-0019 process — only when all three hold: **(a)** a compile form exists for its event class; **(b)** at least one real example routes that class through the graph; **(c)** the corpus re-score supports it. The nearest candidates are `on_mouse_up` and the drag-begin use of `on_mouse_move` (condition (a) has held since ADR-0021); the census records that neither retires today because the ambient-stream role of `on_mouse_move` (P25) has no graph form by design — hover is a query (ADR-0017), so `on_mouse_move` may be permanent-as-query-surface, which the census records honestly rather than promising a retirement.

4. **The drift guard.** *No new event class may ship raw-only.* A future event class (touch when NG-6 opens, scroll if L12 abstracts) must land with either a compile form or a conditions-ledger row naming the compile-form gap. This is the enforceable form of "a kept transition state must at least be a declared one" — the wheel channel is the standing counter-example, having landed raw-only in v0.6 before this doctrine existed, and its row in this census is the debt made visible.

5. **Doctrine homes.** This ADR is the canonical record; `limitations.md` L16 carries the declared state in the gap registry; `app.h` labels each callback's role at the point of use; `intent.md` carries the one-page routing-surface table. The census is a snapshot dated 2026-08-30 and must be recomputed when a new example or event class lands.

## Essence Check

> This is a doctrine decision, not an abstraction admission — no essence axis changes, no new abstraction, no mechanism. The Essence Check is included voluntarily, per the ADR-0017 spirit: this decision names the boundary of the project's strongest claim, and a boundary unnamed is a claim unfalsifiable.

### Q1. Which essence axis does this decision affect?

- [ ] Semantic interface / State space / none — no axis changes.
- [x] **Intent space, doctrine side** — the claim "the type drives interaction routing itself" (ADR-0017) acquires its precise coverage statement: canonical for pointer discrete + continuous and keyboard focus + activation; conditional elsewhere; the conditions ledger tracks the rest.

### Q2. Does it compress or increase human cognitive bandwidth?

Compresses: "which routing surface do I use for which event" goes from folklore (read `app.c`'s switch, infer the history) to one lookup table in `intent.md` and role labels at each callback in `app.h`. The census also compresses the dual-path question itself — "is the afford path real or a convenience?" — into recomputable evidence (two graph-routed evidence apps with `on_click` NULL by design; three unserved classes, each with an owning limitation).

### Q3. Is there a gap between the claim and the implementation?

No. Every mechanism claim in this ADR is already pinned by tests: the three-way fallback contract (graph + afforded → closure, not `on_click`; graph + unresolved → `on_click` fallback; NULL graph → `on_click` always) is pinned in `tests/test_v07.c`; the process-owns-the-gesture routing (down = reset + begin + press sample, moves sample, release commits, raw callbacks suppressed, supersede, app-cancel release) is pinned verbatim in `tests/test_v08.c` section G. The census is recomputable by re-running the searches this ADR's Context records. The honest residue: the drift guard (Decision 4) is review discipline, not a CI gate — no script today can answer "is this new event class raw-only."

### Q4. What is the cost, and who can verify it?

- **Cost:** zero runtime cost (no code change); doc surface grows by this ADR, L16, the `intent.md` table, and `app.h` role labels; review discipline gains one rule (the guard).
- **Who can verify:** re-run the census (the table's searches are named); read the pinned tests above; the doctrine claims are checkable against `src/app.c`'s switch in one sitting.

### Q5. What are the counterexamples?

- **A new app copies `counter_perception_window.c`'s raw shape because it is the smaller example** — real; the mitigation is the census row recording why each raw user is raw, and the `intent.md` table declaring the afford path canonical for interactive routing. The convenience gradient is documented, not hidden.
- **A future event class lands raw-only anyway** — the exact drift this ADR guards; the mitigation is Decision 4 (compile form or ledger row, in the same PR) plus the precedent that this census exists at all: the wheel's raw-only landing is now a named debt (L12, P34/P38), not an invisible default.

## Consequences

### Positive

- The strongest claim in the project ("type drives interaction routing") is now unconditional **for every event class the graph serves**, with the boundary named and the boundary's owner named (L12 wheel; ADR-0020 CAVEATS keys; IME by design).
- The transition state has teeth: per-callback retirement conditions (the three-part test) and the no-new-raw-only-classes guard convert "kept" from silent default into declared posture.
- The census is reusable: the next retirement decision (or the next drift audit) starts from a dated, recomputable table instead of re-deriving history.

### Negative

- The raw surface persists, and with it the residual "optional convenience" risk — managed (declared, bounded, guarded) rather than eliminated. A reader who wants the raw path gone will read this ADR as the project choosing honesty over ambition; that is the correct reading.
- The drift guard is discipline, not machinery — it binds maintainers and reviewers, not CI. If it is ignored, nothing red fails. The mitigation is that the ledger and this census make the ignoring visible.

### Neutral

- `hover_drag_4abs.c` keeps raw routing by design (the boundary-exposing instrument); `counter_perception_window.c` keeps raw routing because routing is not its subject. Neither is migration debt.

## Alternatives Considered

### Alternative 1: Default-flip — a raw-routing opt-out flag

Rejected. `intent_graph` is a graph pointer, not a boolean: there is no default graph to flip on — a graph is application data the framework cannot invent. The only available flip is a negative flag ("I acknowledge raw routing"), which adds a field and a concept while changing no mechanism, no behavior, and no claim. Friction is not doctrine.

### Alternative 2: Default-flip — the graph authoritative on miss

Rejected. The behavioral variant: when `intent_graph` is set, unresolved presses stop falling to `on_click`. Today this changes zero apps (both graph users set `on_click` NULL by design), so it would be a semantic break purchased for no consumer. It would also remove the honest catch-all (the app sovereign over the undeclared) and push apps toward full-canvas affordance declarations whose only purpose is to recover click-anywhere semantics — mechanism churn that the evidence does not request. If a future consumer needs authoritative-mode semantics, that is a new decision with a real consumer, per the admission bar.

### Alternative 3: Deprecation of the raw-coordinate surface

Rejected on the registry's own standard: the deprecation registry (ADR-0019's process) retires an API **with its replacement shipped** — `bind_graph` deprecated only after `px_closure_new_with_graph` existed. The raw surface's replacement does not exist for wheel, non-activation keys, or IME: no compile form, no example, no corpus support. Deprecating an API whose replacement does not exist is the enthusiasm the admission bar (ADR-0011) exists to refuse — it would trade an honest transition state for a dishonest deadline. Partial deprecation of just the covered callbacks fails the same test at smaller scale: the covered callbacks are exactly the ones already pinned as fallbacks, where they are doing declared work.

## CAVEATS

- This decision promises **no retirement by any date**. It promises the conditions are named, the census is dated and recomputable, and the guard binds future event classes. A reader must not read "retirement conditions" as "retirement scheduled."
- The drift guard is process discipline (ledger + review), not a CI gate. No script today can answer "is this new event class raw-only"; the guard is enforced by the reviewer asking.
- The census is a snapshot at v0.8 (2026-08-30). It goes stale silently unless recomputed when examples or event classes land — Decision 5 makes recomputation the named obligation of the PR that lands such a change.

## Known issues

- The wheel channel (`PX_EV_WHEEL` → `on_wheel`) is the standing raw-only debt; L12 owns its abstraction question, P34/P38 carry the corpus cost.
- Keyboard shortcuts, arrow-key traversal, and per-region activation keys remain `on_key`-only (ADR-0020 CAVEATS); `on_mouse_move`'s ambient-stream role may never meet its retirement condition (hover is a query by design — ADR-0017), making it permanent-as-query-surface; both facts are recorded rather than deferred with a date.

## HISTORY

- 2026-08-30: Accepted — decides v0.8 Line 3; census recorded; L16 opened in `limitations.md` as the declared transition state; `intent.md` routing-surface table + `app.h` role labels landed in the same commit.

## References

- [ADR-0016](../proposed/ADR-0016-interaction-prototype-option-b.md) — the opt-in posture this decision matures (the prototype ADR stays proposed, historically)
- [ADR-0017](ADR-0017-intent-compilation-promotion.md) — the promotion whose claim this adjudication bounds; the pointer discrete channel
- [ADR-0018](ADR-0018-interaction-process-promotion.md) — the process promotion; the L15b joint obligation ADR-0021 retired
- [ADR-0020](ADR-0020-v08-keyboard-channel.md) — the keyboard channel (Line 1); CAVEATS names the keys that stay raw
- [ADR-0021](ADR-0021-v08-drag-begin-afford.md) — the process form (Line 2); the continuous channel
- [ADR-0019](ADR-0019-v07-leak-budget-retire.md) — the deprecation-process precedent Decision 3 follows
- [ADR-0011](ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) — the admission bar that rejected Alternative 3
- [ADR-0006](ADR-0006-continuous-interaction-deferred.md) — the boundary-exposing protocol that keeps `hover_drag_4abs.c` raw by design
- [deprecation-registry.md](../../reference/deprecation-registry.md) — where a retired callback will land, when it retires
- [limitations.md L16](../../concepts/state/limitations.md) — the declared transition state in the gap registry
- [intent.md](../../concepts/canonical/intent.md) — the one-page routing-surface table
- `src/app.c` — the switch the census reads; `tests/test_v07.c` (three-way fallback contract), `tests/test_v08.c` G (process routing verbatim)
- [v0.8-roadmap.md](../../concepts/state/v0.8-roadmap.md) — Line 3, which this decides
