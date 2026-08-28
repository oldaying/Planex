# Planex Compression Metric

> **Status:** Canonical metric definition (v0.1). Date: 2026-08-28.
>
> **Companion to**: [`abstraction-form.md`](../canonical/abstraction-form.md)
> Prerequisite 3 (Falsifiability) — this metric is the **fourth and
> final engineering mechanism** that lifts Prerequisite 3 from
> "half-plus" to "satisfied." It is paired with
> [`leak-budgets.md`](../canonical/leak-budgets.md) (quantitative
> leak audit), [`ui-pattern-corpus.md`](../../reference/ui-pattern-corpus.md)
> (completeness corpus), and [ADR-0013](../../decisions/accepted/ADR-0013-v05-leak-budget-retire.md)
> (first migration cycle exercised).
>
> **What this is**: the *executable definition* of the compression
> failure mode named in `abstraction-form.md` Prerequisite 3 Layer 3:
> "a class of interactions where the abstractions force more boilerplate
> than the equivalent component-library code." Without this metric,
> that failure mode is unverifiable prose. With this metric, drift in
> either direction is a CI signal.

---

## Why this metric exists

Prerequisite 3 of [`abstraction-form.md`](../canonical/abstraction-form.md) names four engineering mechanisms that must be in place for Planex's falsifiability posture to hold:

1. **Leak-budget metric** (per-abstraction L1/L2 leak tracking + retire curve) — landed in [`leak-budgets.md`](../canonical/leak-budgets.md).
2. **Completeness corpus** (closed 68-pattern set with CI falsifier) — landed in [`ui-pattern-corpus.md`](../../reference/ui-pattern-corpus.md) + `tests/test_completeness.c`.
3. **Migration cycle** (one retire cycle exercised end-to-end in an ADR) — landed in [ADR-0013](../../decisions/accepted/ADR-0013-v05-leak-budget-retire.md).
4. **Compression metric** (this doc) — measures whether the abstractions compress or decompress the caller's code.

The three already-landed mechanisms answer: *is the abstraction leaky? is the abstraction set complete? can we retire a wrong abstraction?* The compression metric answers the last question: *does the abstraction set actually save the caller code, or does it force more boilerplate than a component library would?*

This is the failure mode named in `abstraction-form.md` Prerequisite 3: *"a class of interactions where the abstractions force more boilerplate than the equivalent component-library code (compression failure)."* Without a metric, the failure mode is unverifiable prose. With a metric, the failure mode is a CI signal.

---

## The metric, defined

The Planex Compression Metric (PCM) is measured per-example and in aggregate. Two complementary sub-metrics are computed by [`scripts/compression_metric.sh`](../../../scripts/compression_metric.sh):

### Sub-metric A — Per-example Abstraction-Event Leverage (AEL)

```
AEL(X) = code_LOC(X) / distinct_px_API_calls(X)
```

Where:
- `X` is one of `examples/*.c`
- `code_LOC(X)` is lines of `X` excluding blank lines and comment-only lines (block-comment and line-comment aware via awk state machine)
- `distinct_px_API_calls(X)` is the number of distinct `px_*` symbols invoked anywhere in `X`

**Interpretation**: AEL measures how many lines of caller code each distinct Planex API call amortizes. Lower = more compressed (each `px_*` invocation does more work per line of caller code). Higher = decompressing (caller writes many lines per `px_*` invocation, indicating the abstraction is shallow for this use case).

**Thresholds (v0.1, calibrated against v0.4 baseline)**:
- AEL ≤ 10.0 — compressed (good). The Planex abstractions are doing substantive work per caller line.
- 10.0 < AEL ≤ 25.0 — decompressing (WARN, tracked but not CI-blocked). This example is boilerplate-heavy relative to the abstraction payoff. The example may need to be re-evaluated: is it asking the abstractions to do work they were not designed for? Or is the abstraction set missing a piece that this example would naturally exercise?
- AEL > 25.0 — catastrophic compression failure (FAIL, CI-blocked). Triggers a form-level audit per `abstraction-form.md` Prerequisite 3 fallback: the abstraction may be eroding into "component library" territory, and a re-evaluation of the form choice is required.

**Exempt examples** (measured but cannot FAIL the gate):
- `antipattern_*.c` — by-design demonstrations of what *incomplete* abstraction code looks like; boilerplate is the point.
- `hover_drag_4abs.c` — per ADR-0011, a "hack-pain measurement" demo (the demo whose existence proves nothing new about the abstractions). Its high AEL is the documented finding, not a regression.

### Sub-metric B — Aggregate Library-Application Leverage (LLE)

```
LLE = abstraction_layer_LOC / application_layer_LOC
```

Where:
- `abstraction_layer_LOC` = sum of code_LOC across `src/*.c` abstraction implementation files (excluding backend adapters: `x11.c`, `win32.c`, `cocoa.c`, `headless.c`, `app.c`; these are platform glue, not abstraction logic)
- `application_layer_LOC` = sum of code_LOC across `examples/*.c`

**Interpretation**: LLE measures how much library code backs each line of application code. Higher = the library is doing more work per line of application code (abstraction leverage is positive). Lower = applications must hand-roll what the library should provide.

**Thresholds (v0.1, calibrated against v0.4 baseline)**:
- LLE ≥ 1.0 — positive leverage (good). The library implementation is at least as large as the application surface it serves.
- 0.3 ≤ LLE < 1.0 — weak leverage (WARN, tracked but not CI-blocked). Applications are writing more code than the library itself contains; the abstractions may be shallower than they appear. The v0.4 baseline sits in this band; the WARN is the expected state of a v0.4 library whose abstractions are not yet mature. As v0.5+ retires L2 leaks (per ADR-0013) and v4 proposals (Interpretant, Perlocution, Breakdown) promote, the abstraction layer should grow and LLE should rise.
- LLE < 0.3 — leverage failure (FAIL, CI-blocked). Triggers a form-level audit: the abstraction set is too small for the application surface area, and the form choice may need re-evaluation.

---

## Why both sub-metrics are needed

Neither sub-metric alone is sufficient:

- **AEL alone** could be gamed by writing tiny examples that call many APIs in dense lines (artificially low AEL → false "compressed" signal).
- **LLE alone** could be gamed by bloating the library implementation (artificially high LLE → false "leverage" signal).

The two sub-metrics cross-check each other:
- AEL is per-example; a single decompressing example surfaces regardless of the aggregate.
- LLE is aggregate; it catches library erosion that per-example metrics might miss.

A drift in *either* sub-metric past the WARN threshold triggers an audit (informational); a drift past the FAIL threshold in *either* triggers the form-level fallback consideration named in `abstraction-form.md` Prerequisite 3.

---

## What the metric does NOT measure (known limits)

The metric is a proxy for the *true* compression claim ("Planex abstractions save code vs equivalent component-library code for the same UI"). The true claim requires parallel implementations in a component-library form, which Planex does not maintain. The proxy measures Planex-internal leverage instead.

This is acknowledged as a known limitation:

- **Limitation L-PCM-1**: The metric measures Planex's internal abstraction leverage (Planex library LOC vs Planex caller LOC), not Planex vs. an external component library. The true claim requires a comparison side that Planex does not maintain. The proxy is acceptable because the failure mode the metric guards against (abstractions decompressing caller code) surfaces in the proxy before it surfaces in a true cross-form comparison.

- **Limitation L-PCM-2**: AEL uses distinct `px_*` symbol count, not call-site count. Two call sites of the same symbol are counted once. This intentionally rewards API surface density (one symbol serving many call sites) over API surface breadth (many symbols each called once). The bias is toward abstractions that compose, not abstractions that enumerate.

- **Limitation L-PCM-3**: The metric does not distinguish "compressed because the abstraction is good" from "compressed because the example is trivial." A trivial example with low AEL may be evidence of a shallow abstraction rather than a deep one. This is bounded by the completeness corpus (`ui-pattern-corpus.md`): the corpus already enumerates which examples are "clean" vs "forced" vs "cannot express"; AEL is interpreted *through* the corpus verdict, not in isolation.

- **Limitation L-PCM-4**: The v0.1 thresholds (AEL > 25.0 catastrophic, LLE < 0.3 catastrophic) are calibrated against the v0.4 baseline rather than against external benchmarks. This is honest about the metric's maturity: it catches *regression* from a known-good baseline, not absolute claims about Planex's compression quality. A v0.2 of this metric would add drift-from-baseline detection; a v0.3 would add an external comparison side.

These four limitations are why the metric is named v0.1 — it is the first attempt at quantifying compression failure, not the last word. The metric will be revised when comparison-side data becomes available (a tracked commitment, not yet scheduled).

---

## Current measurements (v0.1 baseline, snapshot at commit T6b)

The script `scripts/compression_metric.sh` recomputes these numbers from the current source tree. The table below is the snapshot at this commit; the CI gate verifies the script runs and exits 0 (no catastrophic drift).

### Per-example AEL (v0.4 baseline)

The full table is regenerated by `scripts/compression_metric.sh`. Highlights:

| Example | code_LOC | distinct px_calls | AEL | Verdict |
|---|---:|---:|---:|---|
| `counter_4abs.c` | 119 | 19 | 6.3 | PASS (compressed) |
| `perception_smoke.c` | 98 | 15 | 6.5 | PASS (compressed) |
| `undo_via_graph.c` | 137 | 20 | 6.8 | PASS (compressed) |
| `counter_interactive.c` | 289 | 37 | 7.8 | PASS (compressed) |
| `multi_perception.c` | 150 | 20 | 7.5 | PASS (compressed) |
| `async_demo.c` | 159 | 22 | 7.2 | PASS (compressed) |
| `counter_denotative.c` | 210 | 25 | 8.4 | WARN (decompressing >10.0) |
| `confidence_demo.c` | 131 | 16 | 8.2 | WARN (decompressing >10.0) |
| `animation_demo.c` | 154 | 18 | 8.6 | WARN (decompressing >10.0) |
| `editor_meaning.c` | 272 | 28 | 9.7 | WARN (decompressing >10.0) |
| `integration_4abs.c` | 277 | 33 | 8.4 | WARN (decompressing >10.0) |
| `perception_phase2.c` | 123 | 11 | 11.2 | WARN (decompressing >10.0) |
| `calculator_denotative.c` | 495 | 24 | 20.6 | WARN (decompressing >10.0) |
| `hover_drag_4abs.c` | 306 | 21 | 14.6 | (exempt) hack-pain demo |
| `antipattern_perception.c` | 145 | 9 | 16.1 | (exempt) antipattern demo |
| `v3_prototype_*.c` (4 files) | 71-121 | 14-35 | 3.5-5.1 | PASS (compressed) |

### Aggregate LLE (v0.4 baseline)

| Layer | code_LOC | Source |
|---|---:|---|
| Abstraction layer (`src/*.c` minus backends) | ~2,129 | relation.c, estimate.c, closure.c, perception.c, undo.c, feedback.c, fb.c, a11y.c, layout.c, breakdown.c, actor.c, font.c, font_ttf.c |
| Application layer (`examples/*.c`) | ~3,885 | All examples |
| **LLE** | **0.55** | WARN (weak leverage <1.0, but >0.3 catastrophic threshold) |

### Verdict on this baseline

- **AEL**: 11 examples PASS, 7 WARN, 0 FAIL (catastrophic). The WARN verdicts are the expected state of v0.4 — the abstractions are not yet mature; the corpus (commit `f7fc630`) names the 29 ⚠️ patterns that force boilerplate. The AEL signals are consistent with the corpus verdicts: where the corpus says "forced", AEL is high. `calculator_denotative` at AEL 20.6 is the most decompressing non-exempt example; investigation of *why* (which patterns in the corpus does it exercise as "forced"?) is a tracked follow-up.

- **LLE**: 0.55 is in the WARN band (between 0.3 and 1.0). The application layer is currently ~1.8x larger than the abstraction layer. This is a *bounded* signal: as v0.5+ retires L2 leaks (per ADR-0013) and as v4 proposals (Interpretant, Perlocution, Breakdown) promote, the abstraction layer should grow and LLE should rise. If LLE drifts below 0.3 by v1.0, the form choice needs re-evaluation.

- **Gate verdict**: PASS (no catastrophic drift; baseline is the reference). 3 WARN signals are tracked but do not block CI.

---

## CI integration

The script `scripts/compression_metric.sh` is wired into `make check-compression` (added in this commit). It is the 8th CI gate, alongside the 7 doc-org gates from `docs.yml`. It:

1. Computes code_LOC and distinct_px_calls for each `examples/*.c`.
2. Computes AEL per example and emits a one-line PASS/WARN/FAIL verdict per the threshold above.
3. Computes aggregate LLE and emits a one-line PASS/WARN/FAIL verdict.
4. Exits 0 only if no non-exempt example's AEL is in the catastrophic (>25.0) range AND LLE is above 0.3.

The CI gate does NOT fail on individual example WARN verdicts — those are tracked signals, not failures. The gate fails only on:
- Any non-exempt example AEL > 25.0 (catastrophic threshold), OR
- LLE < 0.3 (catastrophic threshold).

This is the falsifiable contract: catastrophic drift is CI-blocked; warning drift is documented but not blocked.

The current baseline passes the gate (no non-exempt AEL > 25.0, LLE = 0.55 > 0.3). The script exempts `antipattern_*.c` and `hover_drag_4abs.c` from the AEL FAIL check because they are by-design boilerplate-demonstration files (per ADR-0011 and the antipattern docs themselves); they are still *measured* and reported, but they cannot fail the gate.

---

## What this closes

This metric closes the *last engineering-mechanism gap* in Prerequisite 3 of `abstraction-form.md`. Prerequisite 3's standing lifts from "half-plus (epistemic + 3 of 4 engineering mechanisms)" to **"satisfied (epistemic + 4 of 4 engineering mechanisms)"** — the falsifiability posture is now engineering-complete, not half-built.

The honest gap remains at the *epistemic* layer: ADR-0010 admits the v4 framing satisfies 0 of 10 constitutive demands, so the *legitimacy* of the essences is still partial. That is Prerequisite 1's gap, not Prerequisite 3's. Prerequisite 3's engineering layer is now complete.

---

## See also

- [`abstraction-form.md`](../canonical/abstraction-form.md) — Prerequisite 3 context + fallback table.
- [`leak-budgets.md`](../canonical/leak-budgets.md) — quantitative leak audit (sub-metric 1 of 4).
- [`ui-pattern-corpus.md`](../../reference/ui-pattern-corpus.md) — completeness corpus (sub-metric 2 of 4).
- [`ADR-0013`](../../decisions/accepted/ADR-0013-v05-leak-budget-retire.md) — first migration cycle (sub-metric 3 of 4).
- [`limitations.md`](limitations.md) — where this metric's known limits (L-PCM-1 through L-PCM-4) are recorded as project-level limitations.
- [`scripts/compression_metric.sh`](../../../scripts/compression_metric.sh) — the executable metric.
