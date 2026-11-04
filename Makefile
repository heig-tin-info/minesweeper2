# Build the game and its tests. Intermediate files (.o, .d) go to build/.
#   make        build everything
#   make test   build and run the tests
#   make run    play in the terminal (make run ARGS="10 9 9 42")
#   make clean  remove the build products

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -g
CXXFLAGS += -MMD -MP   # also track header dependencies (.d files)

BUILD = build
LIB   = $(BUILD)/src/game.o   # the other modules are header-only

all: minesweeper $(BUILD)/unit_tests

minesweeper: $(BUILD)/src/main.o $(LIB)
	$(CXX) $^ -o $@

$(BUILD)/unit_tests: $(BUILD)/tests/tests.o $(LIB)
	$(CXX) $^ -o $@

# build/src/game.o is compiled from src/game.cpp, and so on.
$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(BUILD)/unit_tests
	./$(BUILD)/unit_tests

run: minesweeper
	./minesweeper $(ARGS)

clean:
	rm -rf $(BUILD) minesweeper

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all test run clean
