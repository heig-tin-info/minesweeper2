CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Wshadow

.PHONY: test clean

test: run_tests
	./run_tests

run_tests: test_position.cpp position.cpp position.hpp
	$(CXX) $(CXXFLAGS) -o $@ test_position.cpp position.cpp

clean:
	rm -f run_tests
