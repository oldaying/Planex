#!/usr/bin/env python3
"""Insert the NDEBUG test guard before the first #include of each
assert-based test source. The guard keeps assert() alive in Release
builds (MSVC defines NDEBUG via CMake's default Release flags, which
compiles the suites' assertions to no-ops — vacuous passes).

Run from repo root:  python3 scripts/add_ndebug_guard.py
Idempotent: skips files that already carry the marker.
"""
import re
import sys

FILES = [
    "tests/test_core.c",
    "tests/test_orthogonality.c",
    "tests/test_feedback.c",
    "tests/test_v05_retire.c",
    "tests/test_v06_interaction.c",
    "tests/test_v07.c",
    "tests/test_v08.c",
    "tests/test_v4_orthogonality.c",
    "examples/perception_smoke.c",
    "examples/perception_phase2.c",
    "examples/undo_via_graph.c",
]

GUARD = """\
/* Keep assert() alive in Release builds: MSVC Release defines NDEBUG
 * (CMake default flags), which compiles this suite's assertions to
 * no-ops — a vacuous pass. Found on the first real Windows run
 * (C4700 on the out-param copy was the tell). */
#ifdef NDEBUG
#undef NDEBUG
#endif
"""

MARKER = "#undef NDEBUG"


def main() -> int:
    for path in FILES:
        with open(path, encoding="utf-8") as fh:
            text = fh.read()
        if MARKER in text:
            print(f"skip (already guarded): {path}")
            continue
        lines = text.split("\n")
        idx = next((i for i, ln in enumerate(lines) if ln.startswith("#include")), None)
        if idx is None:
            print(f"ERROR: no #include found in {path}", file=sys.stderr)
            return 1
        lines.insert(idx, GUARD.rstrip("\n"))
        with open(path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(lines))
        print(f"guarded: {path} (inserted before line {idx + 1})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
