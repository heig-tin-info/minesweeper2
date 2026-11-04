# Thin convenience wrapper around CMake, which is the actual build system.
#   make          build everything (the SDL front-end only if SDL2 is found)
#   make test     build and run the unit and architecture tests
#   make run      play in the terminal
#   make run-sdl  play in a window
#   make doc      build the printable PDF of docs/ (needs uv for TeXSmith)

BUILD ?= build
ARGS ?=
TEXSMITH ?= uvx texsmith

.PHONY: all configure test run run-sdl doc clean

all: configure
	cmake --build $(BUILD) -j

configure:
	cmake -S . -B $(BUILD)

test: all
	ctest --test-dir $(BUILD) --output-on-failure

run: all
	./$(BUILD)/minesweeper-console $(ARGS)

run-sdl: all
	./$(BUILD)/minesweeper-sdl $(ARGS)

doc:
	$(TEXSMITH) docs/livre.md -t book -o $(BUILD)/doc --build
	@echo "PDF: $(BUILD)/doc/livre.pdf"

clean:
	rm -rf $(BUILD)
