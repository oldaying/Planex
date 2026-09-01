# Planex CMakeLists.txt — works with MSVC, GCC, Clang
#
# Zero external dependencies required to build.
# FreeType + fontconfig are optional (auto-detected via find_package).
# If not found, Planex uses built-in 8x16 bitmap font (ASCII only).
#
# Usage:
#   cmake -B build
#   cmake --build build --config Release
#   build\Release\counter_x11.exe       (Windows)
#   ./build/counter_x11                  (Linux/macOS)
#
# To get CJK font support (optional):
#   vcpkg install freetype fontconfig
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

cmake_minimum_required(VERSION 3.16)
project(planex C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /utf-8)
    # _CRT_SECURE_NO_WARNINGS: strcpy/sscanf etc. (MSVC security nag)
    # _CRT_NONSTDC_NO_WARNINGS: strdup etc. (MSVC POSIX-name nag, C4996
    # — seen on tests/test_v3_prototype.c in the first real Windows build)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================
# Core sources — zero external dependencies
# ============================================================

set(CORE_SOURCES
    src/relation.c
    src/estimate.c
    src/closure.c
    src/perception.c
    src/undo.c
    src/feedback.c
    src/fb.c
    src/font.c
    src/a11y.c
    src/layout.c
    src/actor.c        # v3 prototype (essence derivation v3 / Path B)
    src/breakdown.c    # v3 prototype (essence derivation v3 / Path B)
    src/interaction.c  # 7th canonical abstraction (ADR-0018; landed v0.6 as prototype per ADR-0016)
    src/hit.c          # 6th canonical abstraction (ADR-0017; landed v0.6 as prototype per ADR-0016)
    src/a11y_bridge_atspi.c  # v0.7 L4: AT-SPI2 adapter behind the a11y query-side contract (PX_A11Y_ATSPI)
)

# ============================================================
# Optional: FreeType (CJK font rendering)
# ============================================================

find_package(Freetype QUIET)
if(Freetype_FOUND)
    message(STATUS "FreeType found — CJK font support enabled")
    list(APPEND CORE_SOURCES src/font_ttf.c)
    add_compile_definitions(PLANEX_HAVE_FREETYPE=1)
else()
    message(STATUS "FreeType NOT found — using built-in 8x16 bitmap font (ASCII only)")
endif()

# ============================================================
# Optional: fontconfig (font discovery by name)
# ============================================================

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(FONTCONFIG QUIET fontconfig)
endif()
if(FONTCONFIG_FOUND)
    message(STATUS "fontconfig found — font discovery by name enabled")
    add_compile_definitions(PLANEX_HAVE_FONTCONFIG=1)
endif()

# ============================================================
# Backend selection (auto-detect)
# ============================================================

if(WIN32)
    set(BACKEND_SOURCES src/win32.c src/app.c)
    set(BACKEND_LIBS gdi32 user32 imm32)
    set(BACKEND_DEF PLANEX_BACKEND_WIN32)
    message(STATUS "Backend: Win32 (GDI)")
elseif(APPLE)
    set(BACKEND_SOURCES src/cocoa.c src/app.c)
    set(BACKEND_LIBS "-framework Cocoa")
    set(BACKEND_DEF PLANEX_BACKEND_COCOA)
    message(STATUS "Backend: Cocoa")
else()
    find_package(X11 QUIET)
    if(X11_FOUND)
        set(BACKEND_SOURCES src/x11.c src/app.c)
        find_library(X11_LIB_PATH X11)
        find_library(XEXT_LIB_PATH Xext)
        set(BACKEND_LIBS ${X11_LIB_PATH} ${XEXT_LIB_PATH})
        set(BACKEND_DEF PLANEX_BACKEND_X11)
        message(STATUS "Backend: X11 (libs: ${X11_LIB_PATH} ${XEXT_LIB_PATH})")
    else()
        set(BACKEND_SOURCES src/headless.c src/app.c)
        set(BACKEND_DEF PLANEX_BACKEND_HEADLESS)
        message(STATUS "Backend: headless (no X11 found)")
    endif()
endif()

add_compile_definitions(${BACKEND_DEF})

# ============================================================
# Library
# ============================================================

add_library(planex_lib STATIC ${CORE_SOURCES} ${BACKEND_SOURCES})
target_include_directories(planex_lib PUBLIC include)

if(Freetype_FOUND)
    target_link_libraries(planex_lib PUBLIC Freetype::Freetype)
endif()
if(FONTCONFIG_FOUND)
    target_link_libraries(planex_lib PUBLIC ${FONTCONFIG_LIBRARIES})
    target_include_directories(planex_lib PUBLIC ${FONTCONFIG_INCLUDE_DIRS})
endif()
if(BACKEND_LIBS)
    target_link_libraries(planex_lib PUBLIC ${BACKEND_LIBS})
endif()

# Link math library
if(NOT WIN32)
    target_link_libraries(planex_lib PUBLIC m)
endif()

# ============================================================
# Demos — post-ADR-0005 (4-abstraction era)
#
# Only demos using the new 4-abstraction API (Relation + Estimate +
# Closure + Perception) are listed. Old 3-abstraction demos were
# removed in commit c24bbcab57.
#
# See docs/concepts/why-four-abstractions.md (to be written) and
# ADR-0005 for the API change.
# ============================================================

# Demos that build + run on the new API
set(STDOUT_DEMOS
    counter_4abs         # canonical 4-abstraction hello world
    multi_perception     # why Perception is a 4th abstraction
    perception_smoke     # Phase 1 API validation (9 tests)
    perception_phase2    # Phase 2 runtime validation (7 tests)
    undo_via_graph       # ADR-0002 Relation necessity proof (7 tests)
    antipattern_estimate  # why Estimate is necessary (3 anti-patterns)
    antipattern_closure   # why Closure is necessary (5 anti-patterns)
    antipattern_perception # why Perception is necessary (4 anti-patterns)
    counter_denotative    # (c) route single-state prototype (4 tests)
    calculator_denotative # (c) route multi-state prototype (4 tests)
    editor_meaning        # phenomenological school prototype
    integration_4abs      # 4 abstractions working simultaneously
    confidence_demo       # confidence field in real use
    animation_demo        # time as first-class dimension
    palette_afford        # v0.7 L1: afford-routed app, zero raw-coordinate clicks
    designer_tools        # v0.8 L2: drag-begin afford — drags decided by the graph
    async_demo            # PROMISE/DECLARE/FAIL lifecycle
    v3_prototype_actor          # v3: 3-place Relation (actor parameter)
    v3_prototype_perlocution    # v3: Searle level 3 effect-of-utterance
    v3_prototype_interpretant  # v3: Peirce interpretant gap
    v3_prototype_breakdown     # v3: Zuhandenheit/breakdown (prototype, not admitted)
)

foreach(demo ${STDOUT_DEMOS})
    add_executable(${demo} examples/${demo}.c)
    target_link_libraries(${demo} planex_lib)
endforeach()

# ============================================================
# Windowed demos (need backend)
# ============================================================

set(WINDOWED_DEMOS
    counter_perception_window  # Phase 2 capstone — perception-driven window
    counter_interactive       # (c) route prototype with real window (2 tests)
    hover_drag_4abs          # boundary demo: hover+drag with 7 hacks (ADR-0006)
    a11y_orca_demo           # v0.8 Cross-cutting A: orca end-to-end evidence app
)

foreach(demo ${WINDOWED_DEMOS})
    add_executable(${demo} examples/${demo}.c)
    target_link_libraries(${demo} planex_lib)
endforeach()

# ============================================================
# All demos now use the post-ADR-0005 4-abstraction API.
# (Prototype demos above were migrated in commits 46a94386, f0c81fd3,
#  915996d8, 22605ec0.)

# ============================================================
# Tests
# ============================================================

enable_testing()
add_executable(test_core tests/test_core.c)
target_link_libraries(test_core planex_lib)
add_test(NAME core_tests COMMAND test_core)

# Essence orthogonality test suite — proves the 4 abstractions are
# truly orthogonal (not implicitly coupled). See tests/test_orthogonality.c
# for the full design and documented essence gaps.
add_executable(test_orthogonality tests/test_orthogonality.c)
target_link_libraries(test_orthogonality planex_lib)
add_test(NAME orthogonality_tests COMMAND test_orthogonality)

# Feedback (v0.4) test suite — validates the 5th essence category.
# Per essence-derivation-v2.md and ADR-0008, Feedback is closed-loop
# coupling: intent → action → state → perception → next intent.
add_executable(test_feedback tests/test_feedback.c)
target_link_libraries(test_feedback planex_lib)
add_test(NAME feedback_tests COMMAND test_feedback)

# v0.5 leak-budget retire test suite — validates that the 7 L2 leaks
# retired in v0.5 (4 Estimate + 3 Perception) are actually closed.
# Per ADR-0013 (v0.5 leak-budget retire) + canonical/leak-budgets.md
# §2 (Estimate) + §4 (Perception). Without this test in CI, the
# ADR-0013 falsifiability contract is silent — the test existed in
# the Makefile as `make test_v05` but no CI job invoked it, so a
# regression that re-introduced an L2 leak would not break the build.
# Wired into all 4 ci.yml jobs in the same commit.
add_executable(test_v05_retire tests/test_v05_retire.c)
target_link_libraries(test_v05_retire planex_lib)
add_test(NAME v05_retire_tests COMMAND test_v05_retire)

# Interaction + leak-retire test suite — validates the 7th canonical
# abstraction (px_interaction: inert hot path, transitions-only bridges;
# section D is THE INVARIANT's normative enforcement per ADR-0018) + the
# v0.6 retires (confidence predictive loop, representamen free_fn,
# a11y query side, loop feedback budget).
# Per ADR-0016 (stays proposed as protocol record) + canonical/leak-budgets.md v0.6 retire summary.
add_executable(test_v06_interaction tests/test_v06_interaction.c)
target_link_libraries(test_v06_interaction planex_lib)
add_test(NAME v06_interaction_tests COMMAND test_v06_interaction)

# v0.7 line-by-line verification suite — one section per v0.7-roadmap
# line as it ships. Section A (Line 1): intent compilation routing —
# px_afford_compile as the window-free compile step behind the app's
# opt-in intent_graph path, the px_pointer_intent value contract
# (embedded region label survives capture + replay), and the routing
# semantics (afforded → closure trigger, unresolved → raw fallback).
add_executable(test_v07 tests/test_v07.c)
target_link_libraries(test_v07 planex_lib)
add_test(NAME v07_line_tests COMMAND test_v07)

# v0.8 line verification suite — validates the keyboard channel:
# the derived focus ring (regions that afford closures, in creation
# order), px_afford_compile_focus and the px_key_intent value
# contract, the app-level key routing decision, and channel
# orthogonality (one graph, two channels, one closure).
add_executable(test_v08 tests/test_v08.c)
target_link_libraries(test_v08 planex_lib)
add_test(NAME v08_line_tests COMMAND test_v08)

# v3 prototype test suite — validates that the 4 new essence
# categories (Interpretant, Perlocution, Breakdown, 3-place Relation)
# are expressible in Planex's C17 zero-dependency API surface, and
# that the v3 prototype does not break v0.4's existing API.
# Per essence-derivation-v3.md Part V and ADR-0009 (Proposed).
add_executable(test_v3_prototype tests/test_v3_prototype.c)
target_link_libraries(test_v3_prototype planex_lib)
add_test(NAME v3_prototype_tests COMMAND test_v3_prototype)

# v4 orthogonality test suite — pressure-tests the 3 NEW first-class
# abstractions (Interpretant, Perlocution, Breakdown) introduced by
# essence-derivation-v4-clean.md against their v0.4 siblings
# (Perception, Closure, Relation). v4 is a clean-room verification
# artifact — ABI breaks from shipping Planex are intentional. The v4
# sources are NOT linked into planex_lib; they are a separate library
# to verify essence claims without polluting the shipping API.
# Per abstraction-form.md Prerequisite 2 (orthogonal separability):
# the v4 proposal seams (Interpretant/Perception, Perlocution/Closure,
# Breakdown/Relation) had not been pressure-tested prior to this suite.
add_library(planex_v4_lib STATIC
    v4/src/actor.c
    v4/src/intent.c
    v4/src/estimate.c
    v4/src/perception.c
    v4/src/interpretant.c
    v4/src/closure.c
    v4/src/perlocution.c
    v4/src/relation.c
    v4/src/loop.c
    v4/src/breakdown.c
)
target_include_directories(planex_v4_lib PUBLIC v4/include)

# Link math library — v4/src/estimate.c:157 uses pow() for cubic ease-out.
# Without this, test_v4_orthogonality fails to link on Linux/GCC with
# "undefined reference to `pow'" (libm is not auto-linked outside MSVC).
# Mirrors the planex_lib m-link above (lines 126-128).
if(NOT WIN32)
    target_link_libraries(planex_v4_lib PUBLIC m)
endif()

add_executable(test_v4_orthogonality tests/test_v4_orthogonality.c)
target_link_libraries(test_v4_orthogonality planex_v4_lib)
add_test(NAME v4_orthogonality_tests COMMAND test_v4_orthogonality)
