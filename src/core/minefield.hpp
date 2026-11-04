#pragma once

#include <cstdint>
#include <span>

#include "core/grid.hpp"
#include "core/position.hpp"

namespace minesweeper {

// Where the mines are, and how many surround each square. Immutable once
// built: a game discovers its minefield, it never changes it.
class Minefield {
public:
    // Throws std::out_of_range for a mine outside the grid and
    // std::invalid_argument for a duplicated mine.
    Minefield(Size size, std::span<const Pos> mines);

    Size size() const { return squares_.size(); }
    int mineCount() const { return mineCount_; }
    bool hasMine(Pos p) const { return squares_[p].mine; }
    int adjacentMines(Pos p) const { return squares_[p].adjacent; }

private:
    struct Square {
        bool mine = false;
        std::uint8_t adjacent = 0;
    };

    Grid<Square> squares_;
    int mineCount_;
};

}  // namespace minesweeper
