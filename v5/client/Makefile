# Planex v5 client Makefile — the presentation zone (ADR-0028)
#
# Self-contained like the core's: builds the if5c client library
# (framebuffer, font, layout, hit, ease, annotate) on top of the
# v5 core, its test suites, and the calculator demo. The client
# LINKS the core; the core never links the client — the rebound
# gate's dependency-direction ban (ADR-0028 Decision 6) is the
# static half of that sentence, this Makefile is the build half.
#
# CFLAGS_EXTRA passes through (the root Makefile's -Werror
# discipline applies here unchanged).

INC_DIR  = include
SRC_DIR  = src
TST_DIR  = tests
BUILD    = build

CC      ?= cc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -g -O0
CFLAGS  += $(CFLAGS_EXTRA)
LDFLAGS ?= -lm

# The client modules (the purity ban's scope: these files may
# not call if5_submit / if5__append / if5__post_debit — the
# client renders, it does not speak).
IF5C_SRCS = $(SRC_DIR)/if5c_fb.c $(SRC_DIR)/if5c_font.c \
            $(SRC_DIR)/if5c_layout.c $(SRC_DIR)/if5c_hit.c \
            $(SRC_DIR)/if5c_ease.c $(SRC_DIR)/if5c_annotate.c
IF5C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(IF5C_SRCS))

# The v5 core (relative — the client builds on its own tree)
CORE_SRC = ../src/if5_core.c ../src/if5_merge.c ../src/if5_budget.c \
           ../src/if5_cut.c ../src/if5_tree.c ../src/if5_schedule.c \
           ../src/if5_estimate.c ../src/if5_cause.c ../src/if5_judge.c \
           ../src/if5_projection.c ../src/if5_process.c \
           ../src/if5_route.c ../src/if5_snapshot.c
CORE_OBJ = $(patsubst ../src/%.c,$(BUILD)/core_%.o,$(CORE_SRC))

TESTS = test_v5c_render test_v5c_hit test_v5c_persist
APPS  = if5c_calc

.PHONY: all test apps golden-check clean $(TESTS)

all: test apps

$(BUILD):
	mkdir -p $(BUILD)

HDRS   = $(SRC_DIR)/if5c_internal.h $(INC_DIR)/planex/if5c.h
COREHD = ../src/if5_internal.h ../include/planex/if5.h

$(BUILD)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(SRC_DIR) -I../include -c $< -o $@

$(BUILD)/core_%.o: ../src/%.c $(COREHD) | $(BUILD)
	$(CC) $(CFLAGS) -I../include -I../src -c $< -o $@

# --- tests ------------------------------------------------------

$(BUILD)/test_v5c_render: $(TST_DIR)/test_v5c_render.c $(IF5C_OBJS) $(CORE_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I../include $< $(IF5C_OBJS) $(CORE_OBJ) -o $@ $(LDFLAGS)

$(BUILD)/test_v5c_hit: $(TST_DIR)/test_v5c_hit.c $(IF5C_OBJS) $(CORE_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I../include $< $(IF5C_OBJS) $(CORE_OBJ) -o $@ $(LDFLAGS)

$(BUILD)/test_v5c_persist: $(TST_DIR)/test_v5c_persist.c $(IF5C_OBJS) $(CORE_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I../include $< $(IF5C_OBJS) $(CORE_OBJ) -o $@ $(LDFLAGS)

# --- the demo app (an APPLICATION: it may submit; the client
#     modules it links may not) -----------------------------------

$(BUILD)/if5c_calc: $(TST_DIR)/../apps/if5c_calc.c $(IF5C_OBJS) $(CORE_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I../include apps/if5c_calc.c $(IF5C_OBJS) $(CORE_OBJ) -o $@ $(LDFLAGS)

test: $(addprefix $(BUILD)/,$(TESTS))
	./$(BUILD)/test_v5c_render
	./$(BUILD)/test_v5c_hit
	./$(BUILD)/test_v5c_persist

apps: $(addprefix $(BUILD)/,$(APPS))

# The golden check: the demo renders the registered session (and
# its one-undo variant) and the committed goldens answer
# byte-for-byte (F2's public form; the .golden files are 24-bit
# BMPs by content — the extension keeps them out of the
# transient-screenshot ignore rule, because they are fixtures,
# not artifacts).
golden-check: $(BUILD)/if5c_calc
	./$(BUILD)/if5c_calc --render $(BUILD)/calc_session.golden
	cmp $(BUILD)/calc_session.golden tests/golden/if5c_calc.golden
	./$(BUILD)/if5c_calc --render $(BUILD)/calc_undo.golden --undo 1
	cmp $(BUILD)/calc_undo.golden tests/golden/if5c_calc_undo.golden
	@echo "golden: byte-identical (session + undo frames)"

clean:
	rm -rf $(BUILD)
