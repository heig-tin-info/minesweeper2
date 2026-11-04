#pragma once

#include <iosfwd>

#include "app/ports.hpp"

namespace minesweeper::console {

struct Style {
    bool colour = false;  // ANSI escape codes
};

// Draws the whole game (HUD, grid and status line) from the read port only.
void render(std::ostream& out, const GameView& view, Style style);

}  // namespace minesweeper::console
