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
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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
    async_demo            # PROMISE/DECLARE/FAIL lifecycle
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
