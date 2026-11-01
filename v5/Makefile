# Planex v5 Makefile — the intent-flow derivation tree (ADR-0026)
#
# Self-contained: builds the if5_ core (the promoted slice core +
# the cut algebra + the two seats + the R2 mechanism layer), the
# seven test suites, and the differential against the frozen
# slice. CFLAGS_EXTRA passes through (the root Makefile's -Werror
# discipline applies here unchanged).
#
# The DIFFERENTIAL links BOTH cores (slice/ifl_* + v5/if5_*) — the
# frozen instrument is the reference arm, never modified. The
# shipping API is not linked: v5 is a derivation, not a shipping
# direction (ADR-0026 Decision 1).

INC_DIR  = include
SRC_DIR  = src
TST_DIR  = tests
BUILD    = build

CC      ?= cc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -g -O0
CFLAGS  += $(CFLAGS_EXTRA)
LDFLAGS ?= -lm

# The v5 core: the promoted slice sources + the R1 modules + the
# R2 mechanism layer (ADR-0027: estimate, cause, judgment,
# projection, process, route)
IF5_SRCS = $(SRC_DIR)/if5_core.c $(SRC_DIR)/if5_merge.c \
           $(SRC_DIR)/if5_budget.c $(SRC_DIR)/if5_cut.c \
           $(SRC_DIR)/if5_tree.c $(SRC_DIR)/if5_schedule.c \
           $(SRC_DIR)/if5_estimate.c $(SRC_DIR)/if5_cause.c \
           $(SRC_DIR)/if5_judge.c $(SRC_DIR)/if5_projection.c \
           $(SRC_DIR)/if5_process.c $(SRC_DIR)/if5_route.c
IF5_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(IF5_SRCS))

# The frozen slice core — the differential's reference arm
IFL_SRCS = ../slice/src/ifl_core.c ../slice/src/ifl_merge.c \
           ../slice/src/ifl_budget.c
IFL_OBJS = $(patsubst ../slice/src/%.c,$(BUILD)/ifl_%.o,$(IFL_SRCS))

TESTS = test_v5_calibration test_v5_property test_v5_cut \
        test_v5_seats test_v5_differential \
        test_v5_mech test_v5_structure

.PHONY: all test rebound-check clean $(TESTS)

all: test

$(BUILD):
	mkdir -p $(BUILD)

# Header dependency (the R2 lesson, learned the hard way: a stale
# if5_merge.o kept writing stats at the pre-ADR-0027 struct offset
# and the calibration suite caught it — headers are prerequisites,
# not suggestions; clean-build parity is now structural).
HDRS = $(SRC_DIR)/if5_internal.h $(INC_DIR)/planex/if5.h

$(BUILD)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) -c $< -o $@

$(BUILD)/ifl_%.o: ../slice/src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I../slice/include -c $< -o $@

# --- tests ------------------------------------------------------

$(BUILD)/test_v5_calibration: $(TST_DIR)/test_v5_calibration.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/test_v5_property: $(TST_DIR)/test_v5_property.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/test_v5_cut: $(TST_DIR)/test_v5_cut.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/test_v5_seats: $(TST_DIR)/test_v5_seats.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

# The differential: BOTH cores, one binary (ADR-0026 Decision 4)
$(BUILD)/test_v5_differential: $(TST_DIR)/test_v5_differential.c $(IF5_OBJS) $(IFL_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I../slice/include -I../slice/app $< $(IF5_OBJS) $(IFL_OBJS) -o $@ $(LDFLAGS)

# The R2 mechanism suites (ADR-0027: M1-M7 mappings, M8-M9 the
# two Known-issue discharges)
$(BUILD)/test_v5_mech: $(TST_DIR)/test_v5_mech.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

$(BUILD)/test_v5_structure: $(TST_DIR)/test_v5_structure.c $(IF5_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(IF5_OBJS) -o $@ $(LDFLAGS)

test: $(addprefix $(BUILD)/,$(TESTS))
	@echo ""
	@echo "=== v5 suites (ADR-0026 core/seats/differential + ADR-0027 mechanism/structure) ==="
	@echo ""
	@fail=0; \
	for t in $(TESTS); do \
			echo "--- $$t ---"; \
			./$(BUILD)/$$t || fail=1; \
			echo ""; \
	done; \
	exit $$fail

# --- the rebound gate (extended: the same script covers v5/) ----

rebound-check:
	@../scripts/check_slice_rebound.sh

clean:
	rm -rf $(BUILD)
