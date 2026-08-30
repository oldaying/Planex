# Planex Intent — the one-page design sheet

> **Status:** Canonical orientation page. Date: 2026-08-28.
>
> **Source format**: Stone Librande's "one-page design" GDC 2010 talk
> (GDD tradition). Every Planex subsystem gets a single readable page
> distilling: vision statement (1 sentence), 3 design pillars, core
> loop, non-goals, ASCII diagram. The page is what gets hung on the
> wall.
>
> **Purpose**: `docs/concepts/canonical/` already runs ~10,000+ words
> across [`abstraction-form.md`](abstraction-form.md),
> [`why-four-abstractions.md`](why-four-abstractions.md),
> [`leak-budgets.md`](leak-budgets.md), and
> [`non-goals.md`](non-goals.md). This page is the *one-page entry
> point* a new contributor reads before any of those. If a reader cannot
> reconstruct Planex's intent from this page alone, the page has failed.
>
> **What this page is NOT**: not a manifesto (that's
> `why-four-abstractions.md`), not a conditional defense (that's
> `abstraction-form.md`), not a leak audit (that's `leak-budgets.md`),
> not a scope statement (that's `non-goals.md`). This page is the
> orientation sheet — one page, no asides, no digressions.

---

## Vision statement (one sentence)

**Planex is a C-language UI library that treats UI as typed values flowing through a denotation pipeline, where every abstraction is admitted by essence (tradition + orthogonality + denotational semantics) rather than by duplication.**

That single sentence is the contract. If a contribution cannot be made consistent with this sentence, the contribution is in the wrong project — not the sentence in the wrong project.

---

## Three design pillars

| # | Pillar | One-line elaboration | Falsifiable test |
|---|---|---|---|
| 1 | **Intent-as-value** | Every user interaction is a serializable typed value (a Closure), not a callback. | `px_closure_replay(buf, len)` round-trips any closure through `px_closure_serialize(buf, len)` without loss; undo/redo and replay are mechanical, not bolted-on. |
| 2 | **Multi-channel denotation** | The same Estimate drives pixels, accessibility tree, and audit log through pure functions, not parallel trees. | Adding a new perception channel (e.g., test-snapshot JSON) does not require touching existing channels; remove one channel, the others still work. |
| 3 | **Essence-justified admission** | Abstractions are admitted by tradition-cite + orthogonality-test + denotational-semantics, NOT by Rule-of-Three duplication. | ADR-0011's three criteria must be documented in any new abstraction's admission ADR; `tests/test_orthogonality.c` (v0.4) and `tests/test_v4_orthogonality.c` (v4 proposals) enforce the orthogonality half in code. |

A contribution that breaks any of these three pillars is not a Planex contribution — it is a contribution to a different project wearing Planex's name.

---

## The seven abstractions (v0.7 shipping set)

| # | Abstraction | Constitutive question | Essence tradition | Evidence |
|---|---|---|---|---|
| 1 | **[Estimate](../../reference/glossary.md#estimate)** | What will the world be? | Elliott's denotative FRP; Friston's predictive coding | `px_estimate_new/set/animate/confidence` — time + uncertainty first-class |
| 2 | **[Closure](../../reference/glossary.md#closure)** | What completed? | Searle's illocutionary act; Winograd/Flores' speech acts | 5-stage closure (Goal/Intent/Action/Execution/Evaluation) + replay |
| 3 | **[Perception](../../reference/glossary.md#perception)** | What is the world? | Peirce's percept; Norman's stages 5-6 | pure-function denotations, multi-channel, parallel (visual / a11y / JSON / log) |
| 4 | **[Relation](../../reference/glossary.md#relation)** | What is connected to what? | Heidegger's Mitsein; Simmel; Alexander's "not a tree" | `px_declare/px_query` — TRIGGERS / DEPENDS_ON / BESIDE / AFFORDS, undo-via-graph |
| 5 | **`px_loop`** | When does control yield? | Hoare's CSP; Harel's statecharts | `px_loop_new(closure, perception) / px_loop_step` — first-class loop, auditable + replayable |
| 6 | **Intent compilation** | What does this input denote? | Gibson's affordances; CLIM presentation types | `px_region` + AFFORDS edges + `px_afford_compile` — `px_pointer_intent` as a replayable value; app-level `intent_graph` routing ([ADR-0017](../../decisions/accepted/ADR-0017-intent-compilation-promotion.md)) |
| 7 | **`px_interaction`** | What is the user doing over time? | Hoare's CSP (prefix + choice); Harel's statecharts | begin → sample* → commit\|cancel with inert hot path; transitions-only bridges ([ADR-0018](../../decisions/accepted/ADR-0018-interaction-process-promotion.md)) |

v4 design proposal (clean-room, not shipping): adds **Interpretant** (Peirce's interpretant), **Perlocution** (Searle's perlocutionary act), **Breakdown** (Heidegger's Zuhandenheit, Winograd/Flores). Pressure-tested by ADR-0012; not yet promoted.

**Host separability (stated property, v0.7):** the abstractions are portable invariants; C17 is the first host, not the ontology. Each abstraction's core is host-independent data + a question (see [abstraction-form.md](abstraction-form.md) § Host separability); if the embeddability bet fails, the ideas move hosts rather than die with one — the falsifiability machinery (leak budgets, corpus, admission bar) is documents and tests, not C code. The inverse holds: no ecosystem-rewrite demands — every feature is opt-in and drop-in (the adoption wedge the Path C record demands).

---

## The core loop (ASCII diagram)

```
                     Human user
                         |
                         | intent (typed value: Closure{Goal, Intent, Action, ...})
                         v
              +-----------------------+
              |       Closure          |     <---- Closure (Searle illocution)
              |   5-stage execution   |              speech-act-as-value
              +-----------+-----------+
                          |
                          | mutates
                          v
              +-----------------------+
              |       Estimate        |     <---- Estimate (Elliott/Friston)
              |   state + time +      |              state-as-prediction
              |   uncertainty         |
              +-----------+-----------+
                          |
                          | read by (pure function)
                          v
              +-----------------------+
              |      Perception        |     <---- Perception (Peirce percept)
              |  multi-channel:        |              denotation-as-function
              |  pixel / a11y /       |
              |  audit log / JSON     |
              +-----------+-----------+
                          |
                          | denotation channels fan out to
                          v
            +-----------------------------+
            |  Pixel | a11y tree | log |  |   (multiple simultaneous outputs)
            +-----------------------------+
                          ^
                          | driven by
                          |
              +-----------------------+
              |       px_loop          |     <---- px_loop (Hoare CSP, Harel)
              |   closed-loop coupling |              loop-as-first-class-value
              |   Closure <---> Perception |
              +-----------------------+
                          |
                          | optional: Relation graph
                          |             (undo-via-graph, dependency, layout)
                          v
              +-----------------------+
              |       Relation        |     <---- Relation (Heidegger Mitsein)
              |   queryable graph     |              structure-as-graph
              +-----------------------+
```

The loop is *first-class*: `px_loop_step` is auditable, replayable, and interruptible. This is the dividing-line from React-style frameworks, where the loop is implicit and unobservable.

---

## Routing surfaces (v0.8 doctrine)

Event routing has two surfaces, decided **per event class** ([ADR-0022](../../decisions/accepted/ADR-0022-v08-dual-path-adjudication.md) — the dual-path adjudication):

| Event class | The path | The raw callback's role |
|---|---|---|
| Pointer-down, discrete act | `px_afford_compile` — the closure form ([ADR-0017](../../decisions/accepted/ADR-0017-intent-compilation-promotion.md)) | `on_click` = fallback for unresolved presses |
| Pointer gesture (drag) | `px_afford_compile_process` — the process form ([ADR-0021](../../decisions/accepted/ADR-0021-v08-drag-begin-afford.md)) | `on_mouse_move` / `on_mouse_up` suppressed — the process owns the stream |
| Keyboard focus + activation | focus-ring walk + `px_afford_compile_focus` ([ADR-0020](../../decisions/accepted/ADR-0020-v08-keyboard-channel.md)) | `on_key` = fallback for keys that compile to nothing |
| Wheel, non-activation keys, IME | none yet | `on_wheel` / `on_key` / `on_ime_commit` = the only surface |

The afford path is canonical for every class it serves; the raw callbacks are a **declared transition state** with per-callback retirement conditions (ADR-0022), and **no new event class may ship raw-only** — the drift guard that keeps "type drives interaction routing" from degrading into an optional convenience.

---

## Non-goals (full list in `non-goals.md`)

The five categories below are NOT things Planex will ever do — they are scope boundaries that protect the three pillars above. Each non-goal is paired with the pillar it would break if adopted.

| # | Non-goal | Why rejected | Pillar protected |
|---|---|---|---|
| 1 | AI integration (LLM in the loop) | Couples UI to a non-deterministic reasoner; intent ceases to be a verifiable value | Intent-as-value |
| 2 | Mobile / touch first-class support | Trajectories make single-pointer gestures derivable, but simultaneous-pointer arbitration is unmodeled (NG-6); the channel axiom is proven one channel at a time — pointer, then keyboard (v0.8) | Essence-justified admission |
| 3 | Native styling (look-native-on-each-OS) | Multi-channel denotation already serves a11y + audit; "native look" is a Perception channel, not a new abstraction | Multi-channel denotation |
| 4 | Backwards-compatible ABI across major versions | Planex is single-maintainer, no shipped-product ABI consumers; v0.4→v4 ABI break is documented in UPGRADING.md | Essence-justified admission (allows retiring wrong abstractions) |
| 5 | DSL form (Tcl/Lua-style embedded language) | DSLs leak syntactically; abstraction leaks semantically — the two forms are not interchangeable (see `abstraction-form.md` Prerequisite 2) | Intent-as-value (specifically: typed value, not parse tree) |

---

## The three prerequisites (the conditional contract)

Planex's claim that *abstraction is the right form* is **conditional on three prerequisites**. If any one breaks, the fallback form is named. This page lists them; `abstraction-form.md` defends them at length.

| Prerequisite | Current standing | Failure fallback |
|---|---|---|
| 1. **Ontological stability** — the essences survive scrutiny | Partial (tradition-grounded; ADR-0010 admits v4 satisfies 0/10 constitutive demands) | Pattern language |
| 2. **Orthogonal separability** — clean abstraction boundaries | 5/5 shipping pass; v0.5 leak budget 2 L2 / 54 ops (3.8%); v4 pressure-tested (ADR-0012) | DSL |
| 3. **Falsifiability** — mechanisms that detect a wrong abstraction | Satisfied (epistemic + 5 of 5 engineering mechanisms in place: leak-budget + completeness corpus + first migration cycle ADR-0013 + compression metric v0.1 + essence-justified admission enforcement ADR-0014) | Component library |

A future audit that finds one of these prerequisites broken flips Planex's form choice to the named fallback. The fallback is named in advance; the project does not get to bluff its way out of a broken prerequisite.

---

## The CI contract (12 gates)

The doc-organization contract is enforced by 12 CI gates. If any one fails, the PR cannot merge. The gates are listed in `.github/workflows/docs.yml`.

| Gate | What it falsifies |
|---|---|
| 1. `check_doc_sections.sh` | Every ADR has all 9 mandatory sections (Mathlib docBlame analogue) |
| 2. `check_adr_lifecycle.sh` | ADR's lifecycle subdirectory matches its Status field (seL4 lifecycle-as-directory, now including the `validated/` stage from ADR-0014) |
| 3. `gen_adr_index.sh` | The hand-maintained `decisions/README.md` matches the auto-generated index (Nygard auto-ADR-1) |
| 4. `check_links.sh` | Every internal markdown link resolves (no silent dead links) |
| 5. `find_orphans.sh` | No unreferenced doc files (no orphan drift) |
| 6. `check_terms.sh` | Every glossary term used in `canonical/` is linked to `glossary.md` (Lean module docstring rule) |
| 7. `check_stale_abstraction_count.sh` | No stale 4-abstraction references in v0.5-current docs (closes the gap surfaced by commit aa752e7; makes CONTRIBUTING.md rule 5's manual grep self-enforcing) |
| 8. `check_limitations_freshness.sh` | No stale capability-claims in `limitations.md` / `canonical/` (e.g., the script catches a line claiming `undo-via-graph has not been implemented` while `examples/undo_via_graph.c` exists; a line claiming `No anti-pattern tests` while `examples/antipattern_*.c` exist; a line claiming `no px_undo() API` while `src/undo.c` declares it) — closes the gap surfaced when L2/L3/L4 were all found stale in the post-v0.5 audit |
| 9. `make check-completeness` | 68-pattern UI corpus invariants hold (75 checks: count + verdict distribution + grounding) |
| 10. `make check-compression` | Planex Compression Metric v0.1: no example AEL > 25.0 (catastrophic) and aggregate LLE > 0.3 |
| 11. `make check-examples` | Every `examples/X.expected` matches the actual output of `examples/X.c` (timestamps normalized) — closes Wave 4.3 |
| 12. `make check-essence` | Every ADR with `## Essence Check` section passes ADR-0011's three-criterion admission gate (tradition-cite + real alternatives + negative consequences); the synthetic case `tests/synthetic_adr_0015.md` correctly triggers the lint — closes ADR-0014's `Validated` stage |

Plus the 4 code jobs in `.github/workflows/ci.yml`: 3 build jobs (`linux-cmake`, `linux-make`, `windows`) running `test_core` + `test_orthogonality` + `test_feedback` + `test_v05_retire`, plus a `strict-warnings` job re-running the same test suite under `-Werror`. The `test_completeness` corpus runs in `docs.yml` Gate 9 (not in `ci.yml`) because the 68-pattern corpus is a doc-org concern, not a code-build concern. `test_v3_prototype` and `test_v4_orthogonality` are CMake-built but **not** CI-enforced — they pressure-test v3/v4 proposal-stage designs that are not yet promoted to shipping (per ADR-0009 Proposed + ADR-0012 accepted-but-not-promoted); running them locally requires `cc -std=c17 -I v4/include tests/test_v4_orthogonality.c v4/src/*.c -lm`.

---

## How to read Planex (in order)

1. **This page.** (Orientation, ~1,000 words.)
2. [`why-four-abstractions.md`](why-four-abstractions.md) — argues *which* abstractions. (~5,000 words.)
3. [`abstraction-form.md`](abstraction-form.md) — argues *that* abstraction is the right form, conditional on three prerequisites. (~3,000 words.)
4. [`leak-budgets.md`](leak-budgets.md) — quantitative leak audit per abstraction, with retire targets. (~1,500 words.)
5. [`non-goals.md`](non-goals.md) — scope boundaries. (~1,000 words.)
6. [`../state/limitations.md`](../state/limitations.md) — current implementation gaps. (~3,000 words.)
7. [`../state/roadmap-matrix.md`](../state/roadmap-matrix.md) — what's shipping vs deferred vs not pursued. (~1,500 words.)
8. [`../state/compression-metric.md`](../state/compression-metric.md) — Planex Compression Metric v0.1 (AEL + LLE), the 4th engineering mechanism of Prerequisite 3.
9. [`../../decisions/`](../../decisions/) — ADRs. Start with [ADR-0010](../../decisions/accepted/ADR-0010-v4-design-rationale-not-essence-discovery.md) (the honesty downgrade), then [ADR-0011](../../decisions/accepted/ADR-0011-essence-justified-abstraction-exempts-rule-of-three.md) (the Rule-of-Three rebuttal), then the rest in numeric order.
10. [`../../reference/ui-pattern-corpus.md`](../../reference/ui-pattern-corpus.md) — the 68-pattern completeness corpus, paired with `tests/test_completeness.c` as the falsifier.

A reader who finishes this sequence can constructively dissent with any Planex decision — which is the whole point of the falsifiability posture.

---

## See also

- [`abstraction-form.md`](abstraction-form.md) — the conditional thesis (3 prerequisites + fallback forms).
- [`why-four-abstractions.md`](why-four-abstractions.md) — which 5 (+3 v4) abstractions and why.
- [`leak-budgets.md`](leak-budgets.md) — quantitative Spolsky-leak metric per abstraction.
- [`non-goals.md`](non-goals.md) — what Planex is NOT, with reasons.
- [`../../reference/glossary.md`](../../reference/glossary.md) — defined terms.
- [`../../decisions/`](../../decisions/) — ADRs.
- [`../../doc-organization.md`](../../doc-organization.md) — the doc-tree organization proposal (Part IX Principle 18 source for this page).
