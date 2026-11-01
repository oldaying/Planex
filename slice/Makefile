# Planex slice Makefile — the intent-flow vertical slice (ADR-0025)
#
# Self-contained: builds the ifl_ core, the two arms, the two test
# suites, and the E2 harness. CFLAGS_EXTRA passes through (the root
# Makefile's -Werror discipline applies here unchanged).
#
# The CONTROL arm links the frozen shipping body's core sources
# (classic_arm -> the px_* library code) — the frozen body IS the
# control (vol. 6 §6.3). No platform backend is linked: the arms
# are headless by design.

INC_DIR  = include
SRC_DIR  = src
APP_DIR  = app
TST_DIR  = tests
BUILD    = build

CC      ?= cc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -g -O0
CFLAGS  += $(CFLAGS_EXTRA)
LDFLAGS ?= -lm

# The intent-flow core
IFL_SRCS = $(SRC_DIR)/ifl_core.c $(SRC_DIR)/ifl_merge.c $(SRC_DIR)/ifl_budget.c
IFL_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(IFL_SRCS))

# The frozen shipping body's core subset the control arm links.
# No backend (x11/win32/cocoa/headless): the arms are headless.
CLASSIC_SRCS = ../src/relation.c ../src/estimate.c ../src/closure.c \
               ../src/undo.c ../src/perception.c ../src/feedback.c
CLASSIC_OBJS = $(patsubst ../src/%.c,$(BUILD)/classic_%.o,$(CLASSIC_SRCS))

APPS = slice_app classic_arm e2_engineering_leg
TESTS = test_slice_calibration test_slice_property

.PHONY: all apps test rebound-check clean $(APPS) $(TESTS)

all: apps test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) -c $< -o $@

$(BUILD)/classic_%.o: ../src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I../include -c $< -o $@

# --- apps ------------------------------------------------------

$(BUILD)/slice_app: $(APP_DIR)/slice_app.c $(IFL_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) -I$(APP_DIR) $< $(IFL_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/classic_arm: $(APP_DIR)/classic_arm.c $(CLASSIC_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(APP_DIR) -I../include $< $(CLASSIC_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/e2_engineering_leg: $(APP_DIR)/e2_engineering_leg.c $(IFL_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) -I$(APP_DIR) $< $(IFL_OBJS) -o $@ $(LDFLAGS)

apps: $(addprefix $(BUILD)/,$(APPS))

# --- tests -----------------------------------------------------

$(BUILD)/test_slice_calibration: $(TST_DIR)/test_slice_calibration.c $(IFL_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IFL_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/test_slice_property: $(TST_DIR)/test_slice_property.c $(IFL_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IFL_OBJS) -o $@ $(LDFLAGS)

test: $(addprefix $(BUILD)/,$(TESTS)) apps
	@echo ""
	@echo "=== slice calibration + property suites (ADR-0025) ==="
	@echo ""
	@fail=0; \
	for t in $(TESTS); do \
	    echo "--- $$t ---"; \
	    ./$(BUILD)/$$t || fail=1; \
	    echo ""; \
	done; \
	echo "--- slice_app (scenario, self-checking) ---"; \
	./$(BUILD)/slice_app > /dev/null || fail=1; \
	echo "--- classic_arm (control, self-checking) ---"; \
	./$(BUILD)/classic_arm > /dev/null || fail=1; \
	echo "--- e2_engineering_leg (self-judging) ---"; \
	./$(BUILD)/e2_engineering_leg > /dev/null || fail=1; \
	exit $$fail

# --- the rebound gate -------------------------------------------

rebound-check:
	@../scripts/check_slice_rebound.sh

clean:
	rm -rf $(BUILD)
