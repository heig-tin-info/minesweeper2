#pragma once

// What a player is allowed to know about a cell. This projection is the only
// cell representation that leaves the domain: during play it never tells
// where the hidden mines are, and it says nothing about how to draw a cell.
#include <variant>

namespace minesweeper {

namespace cell {

struct Hidden {
    bool operator==(const Hidden&) const = default;
};

struct Flagged {
    bool operator==(const Flagged&) const = default;
};

struct Revealed {  // a safe square the player opened
    int adjacentMines;
    bool operator==(const Revealed&) const = default;
};

struct Mine {  // shown once the game is lost
    bool exploded;
    bool operator==(const Mine&) const = default;
};

struct WrongFlag {  // a flag on a safe square, shown once the game is lost
    bool operator==(const WrongFlag&) const = default;
};

}  // namespace cell

using CellView = std::variant<cell::Hidden, cell::Flagged, cell::Revealed,
                              cell::Mine, cell::WrongFlag>;

}  // namespace minesweeper
