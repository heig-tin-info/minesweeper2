# Build the game and its tests.
#   make        build everything
#   make test   build and run the tests
#   make run    play in the terminal (make run ARGS="10 9 9 42")
#   make clean  remove the build products

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -g
CXXFLAGS += -MMD -MP   # also track header dependencies (.d files)

LIB = src/game.o src/play.o src/random_layout.o src/console.o

all: minesweeper tests/tests

minesweeper: src/main.o $(LIB)
	$(CXX) $^ -o $@

tests/tests: tests/tests.o $(LIB)
	$(CXX) $^ -o $@

test: tests/tests
	./tests/tests

run: minesweeper
	./minesweeper $(ARGS)

clean:
	rm -f minesweeper tests/tests src/*.o src/*.d tests/*.o tests/*.d

-include $(wildcard src/*.d tests/*.d)

.PHONY: all test run clean
