# Makefile for SimpleFS Project

# Compiler and Flags
CC = gcc
# -Iinclude tells GCC to look for headers in the include/ directory
# -Wall enables all common warnings, which is good practice
# -g adds debugging symbols
CFLAGS = -Iinclude -Wall -g

# Core source files
SRCS = src/simplefs-disk.c src/simplefs-ops.c

# Find all testcase source files
TEST_SRCS = $(wildcard testcases/testcase*.c)

# Create a list of target executables in the bin/ directory
# e.g., turns "testcases/testcase0.c" into "bin/testcase0"
TARGETS = $(patsubst testcases/%.c, bin/%, $(TEST_SRCS))

# The default command when you just type "make"
# This will build all the testcase executables
all: $(TARGETS)

# This is a pattern rule that tells make how to build any target
# in bin/ from a corresponding source file in testcases/
# $@ is the name of the target (e.g., bin/testcase0)
# $< is the name of the first prerequisite (e.g., testcases/testcase0.c)
bin/%: testcases/%.c $(SRCS)
	@echo "Compiling $@..."
	@# Create the bin directory if it doesn't exist
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

# A rule to run all tests, save their output, and check against expected output
test: all
	@echo "Running all tests..."
	@for i in $(shell seq 0 7); do \
		echo "--- Running testcase$$i ---"; \
		./bin/testcase$$i > output/my_output$$i.txt; \
		if diff -q output/my_output$$i.txt expected_output/testcase$$i.out > /dev/null; then \
			echo "✅ PASSED"; \
		else \
			echo "❌ FAILED: Output differs. Run 'diff output/my_output$$i.txt expected_output/testcase$$i.out'"; \
		fi \
	done

# A rule to clean up all generated files
clean:
	@echo "Cleaning up..."
	@rm -rf bin output simplefs
	@echo "Done."

# Phony targets are not actual files
.PHONY: all test clean