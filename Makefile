# Planex Makefile — Stage 10 (multi-backend: X11 / Win32 / Cocoa / Headless + TTF)
#
# Backends are selected at compile time. The Makefile auto-detects
# the platform; you can override with: make BACKEND=headless

INC_DIR  = include
SRC_DIR  = src
TST_DIR  = tests
EX_DIR   = examples
BUILD    = build

# FreeType for TTF font support (Stage 10) — optional, gracefully
# degrades to built-in 8x16 bitmap font if not available
FREETYPE_CFLAGS = $(shell pkg-config --cflags freetype2 2>/dev/null)
FREETYPE_LIBS   = $(shell pkg-config --libs   freetype2 2>/dev/null)
ifdef FREETYPE_LIBS
  CFLAGS  += -DPLANEX_HAVE_FREETYPE=1
endif

# Fontconfig for font discovery by name (Stage 13) — optional
FONTCONFIG_CFLAGS = $(shell pkg-config --cflags fontconfig 2>/dev/null)
FONTCONFIG_LIBS   = $(shell pkg-config --libs   fontconfig 2>/dev/null)
ifdef FONTCONFIG_LIBS
  CFLAGS  += -DPLANEX_HAVE_FONTCONFIG=1
endif

CC      ?= cc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -g -O0
CFLAGS  += $(CFLAGS_EXTRA)
CFLAGS  += $(FREETYPE_CFLAGS) $(FONTCONFIG_CFLAGS)
LDFLAGS ?= -lm
LDFLAGS += $(FREETYPE_LIBS) $(FONTCONFIG_LIBS)

# Core sources (no backend dependency)
CORE_SRCS = $(SRC_DIR)/relation.c $(SRC_DIR)/estimate.c $(SRC_DIR)/closure.c \
	    $(SRC_DIR)/perception.c $(SRC_DIR)/undo.c $(SRC_DIR)/feedback.c \
            $(SRC_DIR)/fb.c $(SRC_DIR)/font.c $(SRC_DIR)/a11y.c $(SRC_DIR)/layout.c \
            $(SRC_DIR)/interaction.c $(SRC_DIR)/hit.c $(SRC_DIR)/a11y_bridge_atspi.c
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(CORE_SRCS))

# ============================================================
# Optional: FreeType (CJK font rendering)
# ============================================================
FREETYPE_FLAGS := $(shell pkg-config --cflags freetype2 2>/dev/null)
FREETYPE_LIBS := $(shell pkg-config --libs freetype2 2>/dev/null)
ifneq ($(FREETYPE_LIBS),)
  CORE_SRCS += $(SRC_DIR)/font_ttf.c
  CFLAGS += $(FREETYPE_FLAGS) -DPLANEX_HAVE_FREETYPE=1
  LIBS += $(FREETYPE_LIBS)
endif

# ============================================================
# Backend auto-detection
# ============================================================

ifndef BACKEND
  ifeq ($(OS),Windows_NT)
    BACKEND = win32
  else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
      BACKEND = cocoa
    else
      BACKEND = x11
    endif
  endif
endif

# Backend sources and libs
ifeq ($(BACKEND),x11)
  BACKEND_SRCS = $(SRC_DIR)/x11.c $(SRC_DIR)/app.c
  BACKEND_OBJS = $(BUILD)/x11.o $(BUILD)/app.o
  BACKEND_LIBS = -lX11 -lXext
  BACKEND_DEFS = -DPLANEX_BACKEND_X11
else ifeq ($(BACKEND),win32)
  BACKEND_SRCS = $(SRC_DIR)/win32.c $(SRC_DIR)/app.c
  BACKEND_OBJS = $(BUILD)/win32.o $(BUILD)/app.o
  BACKEND_LIBS = -lgdi32 -luser32 -limm32
  BACKEND_DEFS = -DPLANEX_BACKEND_WIN32
  EXE_SUFFIX = .exe
else ifeq ($(BACKEND),cocoa)
  BACKEND_SRCS = $(SRC_DIR)/cocoa.c $(SRC_DIR)/app.c
  BACKEND_OBJS = $(BUILD)/cocoa.o $(BUILD)/app.o
  BACKEND_LIBS = -framework Cocoa -framework Foundation
  BACKEND_DEFS = -DPLANEX_BACKEND_COCOA
  # Note: cocoa.c must be compiled as Objective-C
  CC_OBJC = clang -x objective-c
else ifeq ($(BACKEND),headless)
  BACKEND_SRCS = $(SRC_DIR)/headless.c $(SRC_DIR)/app.c
  BACKEND_OBJS = $(BUILD)/headless.o $(BUILD)/app.o
  BACKEND_LIBS =
  BACKEND_DEFS = -DPLANEX_BACKEND_HEADLESS
else
  $(error Unknown BACKEND '$(BACKEND)'. Use: x11 / win32 / cocoa / headless)
endif

# Backend objects — ensure headless.o / x11.o / win32.o / cocoa.o comes before app.o
BACKEND_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(filter-out $(SRC_DIR)/app.c,$(BACKEND_SRCS))) $(BUILD)/app.o

# ============================================================
# Examples
# ============================================================

EXAMPLES_NO_X11 = counter_4abs multi_perception perception_smoke perception_phase2 \
	          undo_via_graph antipattern_estimate antipattern_closure antipattern_perception \
                  counter_denotative calculator_denotative editor_meaning hover_drag_interaction \
		  palette_afford

# All X11 demos work on all backends (public API is identical)
EXAMPLES_WINDOWED = counter_perception_window counter_interactive hover_drag_4abs

ifeq ($(BACKEND),headless)
  EXAMPLES = $(EXAMPLES_NO_X11)
else
  EXAMPLES = $(EXAMPLES_NO_X11) $(EXAMPLES_WINDOWED)
endif

TEST_SRC       = $(TST_DIR)/test_core.c
TEST_BIN       = $(BUILD)/test_core
TEST_ORTHO     = $(BUILD)/test_orthogonality
TEST_ORTHO_SRC = $(TST_DIR)/test_orthogonality.c
TEST_FEEDBACK  = $(BUILD)/test_feedback
TEST_FEEDBACK_SRC = $(TST_DIR)/test_feedback.c
TEST_V05       = $(BUILD)/test_v05_retire
TEST_V05_SRC   = $(TST_DIR)/test_v05_retire.c
TEST_V06       = $(BUILD)/test_v06_interaction
TEST_V06_SRC   = $(TST_DIR)/test_v06_interaction.c
TEST_V07       = $(BUILD)/test_v07
TEST_V07_SRC   = $(TST_DIR)/test_v07.c
TEST_V08       = $(BUILD)/test_v08
TEST_V08_SRC   = $(TST_DIR)/test_v08.c
TEST_COMP      = $(BUILD)/test_completeness
TEST_COMP_SRC  = $(TST_DIR)/test_completeness.c

.PHONY: all clean test test_ortho test_feedback test_v05 test_v06 test_v07 test_v08 check-completeness check-compression check-examples check-essence examples backends-info

all: examples test

$(BUILD):
	mkdir -p $(BUILD)

# Compile core sources
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(BACKEND_DEFS) -I$(INC_DIR) -c $< -o $@

# Special rule for cocoa.c (Objective-C)
$(BUILD)/cocoa.o: $(SRC_DIR)/cocoa.c | $(BUILD)
	$(CC) $(CFLAGS) $(BACKEND_DEFS) -I$(INC_DIR) -x objective-c -c $< -o $@

# ============================================================
# Examples
# ============================================================

examples: $(addprefix $(BUILD)/,$(EXAMPLES))

# Windowed examples (need backend) — explicit per-target rule
# (GNU Make's stem-selection picks generic % over %_x11)
define WINDOWED_EXAMPLE_RULE
$(BUILD)/$(1): $(EX_DIR)/$(1).c $(CORE_OBJS) $(BACKEND_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $(BACKEND_DEFS) -I$(INC_DIR) $$< $(CORE_OBJS) $(BACKEND_OBJS) -o $$@$(EXE_SUFFIX) $(LDFLAGS) $(BACKEND_LIBS)
endef

$(foreach example,$(EXAMPLES_WINDOWED),$(eval $(call WINDOWED_EXAMPLE_RULE,$(example))))

# Generic rule for non-windowed examples (stdout / BMP only)
$(BUILD)/%: $(EX_DIR)/%.c $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(CORE_OBJS) -o $@$(EXE_SUFFIX) $(LDFLAGS)

# ============================================================
# Tests
# ============================================================

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_ortho: $(TEST_ORTHO)
	./$(TEST_ORTHO)

$(TEST_ORTHO): $(TEST_ORTHO_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_ORTHO_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_feedback: $(TEST_FEEDBACK)
	./$(TEST_FEEDBACK)

$(TEST_FEEDBACK): $(TEST_FEEDBACK_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_FEEDBACK_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_v05: $(TEST_V05)
	./$(TEST_V05)

$(TEST_V05): $(TEST_V05_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_V05_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_v06: $(TEST_V06)
	./$(TEST_V06)

$(TEST_V06): $(TEST_V06_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_V06_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_v07: $(TEST_V07)
	./$(TEST_V07)

$(TEST_V07): $(TEST_V07_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_V07_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

test_v08: $(TEST_V08)
	./$(TEST_V08)

$(TEST_V08): $(TEST_V08_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_V08_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

# ============================================================
# Completeness check — closed-corpus falsifiability test
# Verifies the 68-pattern UI Pattern Corpus
# (docs/reference/ui-pattern-corpus.md) is consistent with the
# implementation examples, limitations, and non-goals.
# Run: make check-completeness
# ============================================================

check-completeness: $(TEST_COMP)
	./$(TEST_COMP)

$(TEST_COMP): $(TEST_COMP_SRC) $(CORE_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_COMP_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

# ============================================================
# Backend info
# ============================================================

backends-info:
	@echo "Available backends: x11, win32, cocoa, headless"
	@echo "Current backend: $(BACKEND)"
	@echo "Backend libs: $(BACKEND_LIBS)"
	@echo "Backend defs: $(BACKEND_DEFS)"

# ============================================================
# Convenience targets (X11 variants)
# ============================================================

run-counter_4abs: $(BUILD)/counter_4abs ; ./$(BUILD)/counter_4abs
run-todo:    $(BUILD)/todo    ; ./$(BUILD)/todo
run-slider:  $(BUILD)/slider  ; ./$(BUILD)/slider
run-radio:   $(BUILD)/radio   ; ./$(BUILD)/radio
run-dropdown: $(BUILD)/dropdown ; ./$(BUILD)/dropdown
run-tabs:    $(BUILD)/tabs    ; ./$(BUILD)/tabs
run-checkbox: $(BUILD)/checkbox ; ./$(BUILD)/checkbox
run-form:    $(BUILD)/form    ; ./$(BUILD)/form
run-wizard:  $(BUILD)/wizard  ; ./$(BUILD)/wizard
run-modal:   $(BUILD)/modal   ; ./$(BUILD)/modal
run-multi_perception: $(BUILD)/multi_perception ; ./$(BUILD)/multi_perception
run-slider_fb:  $(BUILD)/slider_fb  ; ./$(BUILD)/slider_fb
run-counter_perception_window: $(BUILD)/counter_perception_window ; ./$(BUILD)/counter_perception_window
run-slider_x11: $(BUILD)/slider_x11 ; ./$(BUILD)/slider_x11
run-radio_x11: $(BUILD)/radio_x11 ; ./$(BUILD)/radio_x11
run-dropdown_x11: $(BUILD)/dropdown_x11 ; ./$(BUILD)/dropdown_x11
run-checkbox_x11: $(BUILD)/checkbox_x11 ; ./$(BUILD)/checkbox_x11
run-form_x11: $(BUILD)/form_x11 ; ./$(BUILD)/form_x11
run-perf_x11: $(BUILD)/perf_x11 ; ./$(BUILD)/perf_x11
run-resize_x11: $(BUILD)/resize_x11 ; ./$(BUILD)/resize_x11

# ============================================================
# Compression metric - Planex Compression Metric (PCM) v0.1
# Implements docs/concepts/state/compression-metric.md.
# Two sub-metrics:
#   AEL = code_LOC / distinct_px_calls per example
#   LLE = abstraction_layer_LOC / application_layer_LOC aggregate
# Catastrophic thresholds: AEL > 25.0 (non-exempt) OR LLE < 0.3.
# WARN thresholds: AEL > 10.0 OR LLE < 1.0 (tracked, not CI-blocked).
# Run: make check-compression
# ============================================================

check-compression:
	./scripts/compression_metric.sh --check

# ============================================================
# Examples-as-regression-tests (Wave 4.3 from doc-organization.md)
# Each examples/X.c paired with examples/X.expected (seeded from
# current output). CI runs each example, diffs stdout+stderr against
# the expected file, fails on drift. Forces deliberate updates.
# Timestamps (t=<digits>) and addresses (0x<digits>) are normalized
# before diffing so non-deterministic output is comparable.
# Run: make check-examples
# ============================================================

check-examples: examples
	@for ex in $(EXAMPLES_NO_X11); do \
	    if [ -f examples/$$ex.expected ]; then \
	        ./build/$$ex > /tmp/_px_actual_$$ 2>&1; \
	        sed -E 's/t=[0-9]+/t=TS/g; s/0x[0-9a-fA-F]+/0xADDR/g' examples/$$ex.expected > /tmp/_px_exp_$$; \
	        sed -E 's/t=[0-9]+/t=TS/g; s/0x[0-9a-fA-F]+/0xADDR/g' /tmp/_px_actual_$$ > /tmp/_px_act_norm_$$; \
	        if diff -q /tmp/_px_exp_$$ /tmp/_px_act_norm_$$ > /dev/null; then \
	            echo "[PASS] $$ex"; \
	        else \
	            echo "[FAIL] $$ex: output drift detected"; \
	            echo "--- expected (first 20 lines, normalized) ---"; \
	            head -20 /tmp/_px_exp_$$; \
	            echo "--- actual (first 20 lines, normalized) ---"; \
	            head -20 /tmp/_px_act_norm_$$; \
	            rm -f /tmp/_px_actual_$$ /tmp/_px_exp_$$ /tmp/_px_act_norm_$$; \
	            exit 1; \
	        fi; \
	            rm -f /tmp/_px_actual_$$ /tmp/_px_exp_$$ /tmp/_px_act_norm_$$; \
	        else \
	        echo "[SKIP] $$ex: no .expected file"; \
	        fi; \
	done

# ============================================================
# Essence-justified admission enforcement (Gate 10 from ADR-0014).
# Runs scripts/check_essence_admission.sh in two modes:
#   --check     -> scans decisions/{proposed,validated}/ for ADRs
#                  with `## Essence Check` sections; all must pass.
#   --synthetic -> runs on tests/synthetic_adr_0015.md; must exit
#                  non-zero (the falsifiability demonstration that
#                  the lint fires on the synthetic violation case).
# Run: make check-essence
# ============================================================

check-essence:
	./scripts/check_essence_admission.sh --check
	./scripts/check_essence_admission.sh --synthetic

clean:
	rm -rf $(BUILD)
