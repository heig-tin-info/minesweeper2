#pragma once

// The console speaks chess notation: columns are letters from the left, rows
// are numbers from the bottom, so "A1" is the bottom-left corner. Translating
// it to and from `Pos` is the console's business; the domain never sees it.
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "app/commands.hpp"
#include "core/position.hpp"

namespace minesweeper::console {

struct Quit {
    bool operator==(const Quit&) const = default;
};

// A line typed by the player: either a command for the application, or a
// request that only concerns the console itself.
using Input = std::variant<Command, Quit>;

std::string columnLabel(int col);                  // 0 -> "A", 26 -> "AA"
std::string rowLabel(int row, Size size);          // the top row is the highest number
std::optional<Pos> parsePos(std::string_view text, Size size);

// "B3" reveals, "B3?" flags, "n" starts a new game, "q" quits.
// Returns nothing for malformed input or a position outside the grid.
std::optional<Input> parseInput(std::string_view line, Size size);

}  // namespace minesweeper::console
