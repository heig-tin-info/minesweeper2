# Thin wrapper around CMake.
#   make        build
#   make test   build and run the tests
#   make run    play in the terminal (make run ARGS="9 9 10 42")

BUILD ?= build
ARGS ?=

.PHONY: all test run clean

all:
	cmake -S . -B $(BUILD)
	cmake --build $(BUILD) -j

test: all
	./$(BUILD)/tests

run: all
	./$(BUILD)/minesweeper $(ARGS)

clean:
	rm -rf $(BUILD)
