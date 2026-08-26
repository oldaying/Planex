# CONTRIBUTING.md — Updated for v0.3 (4-abstraction era)

> This document records the rules learned from this session's mistakes.
> Each rule has a "LESSON" — a concrete failure that caused it.

---

## 1. Build system sync rule

**LESSON**: CI failed for 20+ commits because Makefile still listed deleted demos. CMakeLists.txt was updated but Makefile was not.

**Rule**: When adding, renaming, or deleting any source file:
1. Update `CMakeLists.txt` (STDOUT_DEMOS or WINDOWED_DEMOS)
2. Update `Makefile` (EXAMPLES_NO_X11 or EXAMPLES_WINDOWED)
3. Update `examples/README.md` (catalog)
4. Verify both build systems compile: `cmake --build build && make`

**Checklist before commit**:
- [ ] CMakeLists.txt updated?
- [ ] Makefile updated?
- [ ] examples/README.md updated?
- [ ] Both build systems pass?

---

## 2. API change sweep rule

**LESSON**: ADR-0005 changed `px_closure_new` from 6-arg to 5-arg. `examples/` was migrated but `tests/test_core.c` was missed, causing build failure on Windows.

**Rule**: When changing any function signature in `include/planex/*.h`:
1. `grep -rn "old_function_name" src/ examples/ tests/`
2. Update ALL matching files — not just the ones you remember
3. Build on BOTH platforms (Linux + Windows) before pushing
4. Run ALL tests: `make test && cmake --build build && ctest`

**Checklist before commit**:
- [ ] grep found all call sites?
- [ ] All call sites updated?
- [ ] Linux build passes?
- [ ] Windows build passes (or at minimum, CI passes)?
- [ ] All tests pass?

---

## 3. const correctness rule

**LESSON**: Added `const` to `px_estimate_value` and `px_estimate_is_animating`, but these functions have side effects (auto-sample + finalize animation). MSVC correctly rejected with C2166.

**Rule**: Before adding `const` to a function parameter:
1. Read the function body — does it modify the parameter?
2. Check for hidden side effects (lazy initialization, caching, finalization)
3. If the function name looks like a getter but has side effects, document why const is NOT used

**Checklist**:
- [ ] Function body checked for writes to the parameter?
- [ ] Hidden side effects checked?
- [ ] If const is NOT used, is there a comment explaining why?

---


## 4. Demo design rule

**LESSON**: hover_drag_4abs.c was written to "prove 4 abstractions can do hover/drag" — but all UI libraries can do hover/drag. The demo's purpose was wrong. It should have been "measure how painful the hack is" (which it did, but the framing was misleading).

**Rule**: Before writing a demo, ask:
1. What question does this demo answer?
2. Can mainstream libraries (React/Qt/ImGui) already do this?
3. If yes — what is Planex's DIFFERENTIATION in this scenario?
4. If no differentiation — the demo proves nothing. Don't write it.

**Good demo purposes**:
- "Prove abstraction X is necessary" (anti-pattern test)
- "Measure hack pain for a boundary" (hover_drag_4abs)
- "Show 4 abstractions working together" (future integration demo)
- "Validate an ADR decision" (counter_denotative validated ADR-0005)

**Bad demo purposes**:
- "Show Planex can do X" (if everyone can do X, this proves nothing)
- "Add another widget" (unless it proves a new capability)

---

## 5. Documentation sync rule

**LESSON**: Multiple docs still said "3 abstractions" after ADR-0005 promoted Perception to 4th. README was updated but non-goals.md, limitations.md, and roadmap-matrix.md were not.

**Rule**: When making a decision that changes the project's claims:
1. Update README.md (the front door)
2. Update the manifesto (why-four-abstractions.md)
3. Update limitations.md (if a limitation is resolved or added)
4. Update non-goals.md (if scope changes)
5. Update roadmap-matrix.md (if matrix cells change)
6. Update changelog.md (always)
7. Update glossary.md (if terminology changes)
8. grep for old terminology: `grep -rn "three abstractions" docs/ README.md`

**Checklist before commit**:
- [ ] All docs updated?
- [ ] grep found no stale references?
- [ ] changelog.md has an entry?

---

## 6. Commit message rule

**LESSON**: Commit messages sometimes used non-ASCII characters (em-dash —, emoji) that caused Python string parsing errors when programmatically pushing via API.

**Rule**:
1. Use only ASCII in commit messages
2. Use `-` (hyphen) instead of `—` (em-dash)
3. Use `->` instead of `→` (arrow)
4. Use `*` instead of `✓` or `✗`
5. Keep first line under 72 characters
6. Prefix with type: `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `build:`

---

## 7. Platform testing rule

**LESSON**: Win32 backend only handled WM_LBUTTONDOWN, not WM_LBUTTONUP or WM_MOUSEMOVE. Hover and drag were impossible on Windows. The bug was invisible because all development was on Linux.

**Rule**: When adding event handling:
1. Check ALL platform backends (win32.c, x11.c, cocoa.c, headless.c)
2. If you add a new event type, add it to ALL backends
3. Test on at least 2 platforms before pushing
4. CI tests all platforms — but CI only catches compile errors, not runtime behavior

**Checklist**:
- [ ] win32.c handles the new event?
- [ ] x11.c handles the new event?
- [ ] cocoa.c handles the new event?
- [ ] headless.c handles the new event (or explicitly doesn't need to)?

---

## 8. "Paper before code" rule

**LESSON**: Writing a text editor demo would have taken 2 weeks and proven nothing new. 1 hour of systematic pattern analysis (ui-pattern-coverage.md) revealed more boundaries than 2 weeks of coding would have.

**Rule**: Before writing code to "prove" something:
1. Can you analyze it on paper first?
2. List the scenarios systematically (like ui-pattern-coverage.md)
3. Identify which scenarios are likely to expose boundaries
4. Only write code for the scenario MOST likely to expose a boundary
5. If paper analysis says "all clean" — don't write the demo, trust the analysis

---

## 9. Essence check rule

**LESSON**: The hover_drag demo was framed as "proving 4 abstractions can do hover/drag" — but that's not an essence question. It's a capability question. Essence questions are about what UI IS, not what it CAN DO.

**Rule**: Before starting work, ask the 5 essence questions (from ADR template):
1. Which essence axis does this affect?
2. Does it compress or increase cognitive bandwidth?
3. Is there a gap between claim and implementation?
4. What is the cost, and who can verify it?
5. What are the counterexamples?

If the answer to Q1 is "none" — this is engineering, not essence. Fine, but don't pretend it's essence-driven.

---

## Summary

| Rule | Lesson source | One-liner |
|---|---|---|
| 1. Build sync | Makefile not updated | Update ALL build systems when deleting files |
| 2. API sweep | test_core.c missed | grep ALL call sites when changing signatures |
| 3. const | px_estimate_value had side effects | Check function body before adding const |
| 4. Demo design | hover_drag proved nothing new | Ask "what question does this demo answer?" |
| 5. Doc sync | "3 abstractions" stale in multiple docs | grep for old terminology |
| 6. Commit msg | Non-ASCII caused Python errors | ASCII only in commit messages |
| 7. Platform test | Win32 missing mouse-up/move | Check ALL backends for new events |
| 8. Paper first | Text editor demo would prove nothing | Analyze before coding |
| 9. Essence check | hover_drag framed wrong | Ask "which essence axis?" before starting |
