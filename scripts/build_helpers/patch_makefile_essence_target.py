#!/usr/bin/env python3
"""Add `check-essence` target to Planex Makefile.

Writes the Makefile as raw bytes to preserve TAB indentation in
recipe lines. The Edit tool silently converts TABs to 8 spaces and
breaks GNU make — this script avoids that.

Two changes:
1. Append `check-essence` to the .PHONY line.
2. Append a new `check-essence:` target with a TAB-indented recipe
   that runs `scripts/check_essence_admission.sh --check` (the
   real-ADR scan) followed by `--synthetic` (the falsifiability
   demonstration that the lint fires on the synthetic case).
"""
import pathlib

ROOT = pathlib.Path("/home/z/my-project/repos/Planex")
MAKEFILE = ROOT / "Makefile"

text = MAKEFILE.read_bytes().decode("utf-8")

# 1. Append `check-essence` to the .PHONY line.
phony_old = ".PHONY: all clean test test_ortho test_feedback test_v05 check-completeness check-compression check-examples examples backends-info"
phony_new = ".PHONY: all clean test test_ortho test_feedback test_v05 check-completeness check-compression check-examples check-essence examples backends-info"
assert phony_old in text, f"phony line not found verbatim: {phony_old!r}"
text = text.replace(phony_old, phony_new, 1)

# 2. Append the check-essence target. Use literal TAB character `\t`
#    for the recipe line — GNU make REQUIRES TAB indentation.
new_target = (
    "\n"
    "# ============================================================\n"
    "# Essence-justified admission enforcement (Gate 10 from ADR-0014).\n"
    "# Runs scripts/check_essence_admission.sh in two modes:\n"
    "#   --check     -> scans decisions/{proposed,validated}/ for ADRs\n"
    "#                  with `## Essence Check` sections; all must pass.\n"
    "#   --synthetic -> runs on tests/synthetic_adr_0015.md; must exit\n"
    "#                  non-zero (the falsifiability demonstration that\n"
    "#                  the lint fires on the synthetic violation case).\n"
    "# Run: make check-essence\n"
    "# ============================================================\n"
    "\n"
    "check-essence:\n"
    "\t./scripts/check_essence_admission.sh --check\n"
    "\t./scripts/check_essence_admission.sh --synthetic\n"
    "\n"
)

# Append before the `clean:` target (last target in the file).
clean_marker = "\nclean:\n"
assert clean_marker in text, "could not find clean: target marker"
text = text.replace(clean_marker, new_target + "clean:\n", 1)

MAKEFILE.write_bytes(text.encode("utf-8"))
print(f"OK: patched {MAKEFILE}")
