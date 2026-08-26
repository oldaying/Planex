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
| **Total** | **68** |

All tests must pass on both Linux (gcc) and Windows (MSVC).
