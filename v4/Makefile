# Planex v4 Makefile — clean-room essence verification
#
# Builds only the v4 abstractions (8 of them) and runs their tests.
# No platform backends (X11/Win32/Cocoa) — pure essence validation,
# fully headless. This proves the v4 design compiles and passes tests
# in C17 with zero external dependencies.

INC_DIR  = include
SRC_DIR  = src
TST_DIR  = tests
BUILD    = build

CC      ?= cc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wpedantic -g -O0
LDFLAGS ?= -lm

# All v4 sources (one per essence abstraction + actor + intent symbols)
V4_SRCS = $(SRC_DIR)/actor.c \
	  $(SRC_DIR)/intent.c \
	  $(SRC_DIR)/estimate.c \
	  $(SRC_DIR)/perception.c \
	  $(SRC_DIR)/interpretant.c \
	  $(SRC_DIR)/closure.c \
	  $(SRC_DIR)/perlocution.c \
	  $(SRC_DIR)/relation.c \
	  $(SRC_DIR)/loop.c \
	  $(SRC_DIR)/breakdown.c

V4_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(V4_SRCS))

# One test binary per essence category + orthogonality integration
TESTS = test_estimate test_perception test_interpretant \
	test_closure test_perlocution test_relation \
	test_loop test_breakdown test_orthogonality

.PHONY: all clean test $(TESTS)

all: test

$(BUILD):
	mkdir -p $(BUILD)

# Compile v4 sources
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Each test binary
$(BUILD)/test_%: $(TST_DIR)/test_%.c $(V4_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(INC_DIR) $< $(V4_OBJS) -o $@ $(LDFLAGS)

test: $(addprefix $(BUILD)/,$(TESTS))
	@echo ""
	@echo "=== Running v4 essence verification tests ==="
	@echo ""
	@fail=0; \
	for t in $(TESTS); do \
	    echo "--- $$t ---"; \
	    ./$(BUILD)/$$t || fail=1; \
	    echo ""; \
	done; \
	if [ $$fail -eq 0 ]; then \
	    echo "=== ALL v4 TESTS PASS ==="; \
	else \
	    echo "=== v4 TESTS FAILED ==="; \
	    exit 1; \
	fi

clean:
	rm -rf $(BUILD)
