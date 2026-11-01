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
| **Total** | **82** |

All tests must pass on both Linux (gcc) and Windows (MSVC).
