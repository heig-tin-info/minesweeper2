# Default Makefile for the Minesweeper project. Generates console and SDL builds
# while keeping configuration simple and overridable from the command line.

# Toolchain ------------------------------------------------------------------
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
CPPFLAGS ?= -I.
LDFLAGS ?=

# Output directories ---------------------------------------------------------
BUILD_DIR ?= build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)

# Targets --------------------------------------------------------------------
CONSOLE_TARGET := $(BIN_DIR)/minesweeper-console
SDL_TARGET := $(BIN_DIR)/minesweeper-sdl

CONSOLE_OBJS := $(OBJ_DIR)/console_main.o

# SDL configuration via pkg-config -------------------------------------------
SDL2_PKG_CONFIG ?= pkg-config
SDL2_AVAILABLE := $(shell $(SDL2_PKG_CONFIG) --exists sdl2 SDL2_ttf && echo yes)

ifeq ($(SDL2_AVAILABLE),yes)
SDL_OBJS := $(OBJ_DIR)/sdl_main.o
SDL2_CFLAGS := $(shell $(SDL2_PKG_CONFIG) --cflags sdl2 SDL2_ttf)
SDL2_LIBS := $(shell $(SDL2_PKG_CONFIG) --libs sdl2 SDL2_ttf)
else
SDL_OBJS :=
SDL2_CFLAGS :=
SDL2_LIBS :=
endif

# Default target -------------------------------------------------------------
.PHONY: all
all: $(CONSOLE_TARGET)

.PHONY: console
console: $(CONSOLE_TARGET)

.PHONY: sdl
sdl: $(SDL_TARGET)

.PHONY: run
run: $(CONSOLE_TARGET)
	@$(CONSOLE_TARGET)

.PHONY: clean
clean:
	@echo "[clean] Removing $(BUILD_DIR)"
	@rm -rf "$(BUILD_DIR)"

# Build rules ----------------------------------------------------------------
$(CONSOLE_TARGET): $(CONSOLE_OBJS) | $(BIN_DIR)
	@echo "[link] $@"
	@$(CXX) $^ $(LDFLAGS) -o $@

ifeq ($(SDL2_AVAILABLE),yes)
$(SDL_TARGET): $(SDL_OBJS) | $(BIN_DIR)
	@echo "[link] $@"
	@$(CXX) $^ $(LDFLAGS) $(SDL2_LIBS) -o $@

$(OBJ_DIR)/sdl_main.o: main.cpp | $(OBJ_DIR)
	@echo "[cxx ] $< (SDL)"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SDL2_CFLAGS) -DSDL -c $< -o $@
else
$(SDL_TARGET):
	@echo "[error] SDL2 front-end requires pkg-config entries for sdl2 and SDL2_ttf" >&2
	@false
endif

$(OBJ_DIR)/console_main.o: main.cpp | $(OBJ_DIR)
	@echo "[cxx ] $<"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Directory creation ---------------------------------------------------------
$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@
