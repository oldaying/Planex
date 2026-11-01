# Testing

## Quick Start

```bash
cmake -B build
cmake --build build --config Release
```

## Run Tests

### Core tests (33 tests)

```bash
./build/test_core           # Linux/macOS
.\build\Release\test_core.exe   # Windows
```

### API tests

```bash
./build/perception_smoke     # 9 Phase 1 API tests
./build/perception_phase2    # 7 Phase 2 runtime tests
./build/undo_via_graph      # 7 undo-via-graph tests
```

### Anti-pattern tests

```bash
./build/antipattern_estimate   # 3 anti-patterns
./build/antipattern_closure    # 5 anti-patterns
./build/antipattern_perception # 4 anti-patterns
```

### Demos

```bash
./build/counter_4abs              # 4-abstraction hello world
./build/multi_perception         # 4 perceptions of same state
./build/counter_denotative       # (c) route single-state prototype
./build/calculator_denotative   # (c) route multi-state prototype
./build/editor_meaning           # phenomenological school prototype
```

### Windowed demos (need X11/Win32)

```bash
./build/counter_perception_window  # perception-driven window + undo (Z)
./build/counter_interactive        # (c) route real window
./build/hover_drag_4abs           # hover+drag boundary demo
```

## Make (alternative)

```bash
make BACKEND=headless
make test
```

## The intent-flow vertical slice (ADR-0025)

The slice is a parallel ontology tree (`slice/`, namespace `ifl_`)
following the `v4/` precedent — Program I's falsification
instrument, not a shipping direction. It carries its own suites:

```bash
make -C slice test            # calibration (C3/C4/C5/C7/C9,
                              #   I1/I2/I3/I5/I6/I7) + property
                              #   (C6: 1,000,000 random groups per
                              #   round, shrinking, synthetic bite)
                              #   + both arms (self-checking) + the
                              #   E2 engineering leg (self-judging:
                              #   C8's registered boundaries
                              #   re-verify on every checkout)
make -C slice rebound-check   # the rebound signal-1 static gate
                              #   (kind dispatch / quiet-mode /
                              #   single-source special-casing
                              #   forbidden in slice/src) + its
                              #   synthetic-bite demonstration
```

The control arm links the frozen shipping body (`classic_arm` ->
the px_* core, frozen at the slice's branch point) — same task
script as the treatment arm, honestly-optimally encoded; the two
cost tables it prints are C9's comparison surface. The E2 raw data
lives in `measurements/e2-engineering-leg/` and recomputes via
`scripts/analyze_e2_residency.sh`.

## The v5 derivation tree (ADR-0026 + ADR-0027)

The full rewrite's stages R1 and R2: a parallel tree (`v5/`,
namespace `if5_`) that PROMOTES the slice's core to library grade
(R1: the global cut algebra plus the S15/S17/S24 seats) and
re-derives the MECHANISM LAYER (R2: the seven shipping
abstractions as projections, queries, and judgments over the
durable core, plus the two Known-issue discharges). A derivation,
not a shipping direction — the shipping API stays frozen as the
E-series control arm. It carries seven suites:

```bash
make -C v5 test              # calibration (the slice's battery
                              #   re-derived on if5: C3/C4/C5/C7/C9,
                              #   I1/I2/I3/I5/I6/I7, down-closure
                              #   via the promoted public verifier)
                              # + property (1M groups, shrinking,
                              #   synthetic bite)
                              # + cut (the algebra's laws; the
                              #   independent membership fold)
                              # + seats (S15 tree totality/
                              #   determinism, S17 PROV-backed
                              #   timetable, S24 projection purity)
                              # + the DIFFERENTIAL: both cores in
                              #   one binary, TRIP-17 + 100 seeded
                              #   random streams + the E2 shape,
                              #   byte-compared on the registered
                              #   observation surface, with v5's
                              #   residency re-verified against
                              #   the C8 boundary directly
make -C v5 rebound-check     # the extended rebound gate: the same
                              #   four bans over v5/src + v5/include
                              #   PLUS the S15 visual-field ban, each
                              #   with its own synthetic bite, PLUS
                              #   the mechanism-layer ban (ADR-0027):
                              #   if5_process_sample's body may not
                              #   submit/append/debit — samples fan
                              #   out to nothing, with its own bite
                              # + mechanism (M1–M7: estimate-fold
                              #   agreement + evidence ordinals,
                              #   cause-graph exactness/reversal/
                              #   concurrency/reachability, judgment
                              #   totality/verdict precedence,
                              #   projection registry byte-equality,
                              #   process discipline, routing
                              #   equivalence, budget refusal)
                              # + structure (M8–M9: reparent folds/
                              #   undoes/values-invariant + the R1
                              #   no-reparent regression; kcut
                              #   down-closure/degeneration/frontier/
                              #   two-source discrimination)
                              # + snapshot (ADR-0028: F6–F9 + refusals —
                              #   named-address round trip, save/load
                              #   fidelity over the full op alphabet,
                              #   format canonicity, the one-pass
                              #   rebuild's exactness)
                              # + client render (F1/F2/F3/F5 + the
                              #   annotation table — purity, determinism,
                              #   layout totality, easing endpoints)
                              # + client hit (F4: 200 randomized trees
                              #   against a brute-force inverse)
                              # + client persistence (F2 rt + F6 client
                              #   half — render across save/load)
```

## Test Summary

| Category | Tests |
|---|---|
| Core (Relation/Estimate/Closure/derived/animation/font) | 33 |
| Perception Phase 1 API | 9 |
| Perception Phase 2 runtime | 7 |
| Undo-via-graph | 7 |
| Anti-pattern Estimate | 3 |
| Anti-pattern Closure | 5 |
| Anti-pattern Perception | 4 |
| Slice calibration sections (C3/C4/C5/C7/C9, I1–I7, down-closure) | 8 |
| Slice property sections (C6/I4, 1M groups/round) | 6 |
| v5 calibration sections (the battery re-derived on if5) | 8 |
| v5 property sections (1M groups/round, on if5) | 6 |
| v5 cut sections (V2/V3: algebra laws, address checks) | 6 |
| v5 seats sections (V4/V5/V6: S15/S17/S24) | 6 |
| v5 mechanism sections (M1–M7: the seven ADR-0027 mappings) | 9 |
| v5 structure sections (M8–M9: the two discharges) | 4 |
| v5 snapshot sections (ADR-0028: F6–F9 + refusals) | 5 |
| v5 client render sections (F1/F2/F3/F5 + annotations) | 5 |
| v5 client hit sections (F4: randomized inversion) | 1 |
| v5 client persistence sections (F2 rt + F6) | 2 |
| v5 demo golden checks (session + undo frames) | 2 |
| **Total** | **136** |

All tests must pass on both Linux (gcc) and Windows (MSVC).
