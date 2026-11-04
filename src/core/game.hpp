#pragma once

#include <cstdint>
#include <optional>

#include "core/cell_view.hpp"
#include "core/grid.hpp"
#include "core/mine_layout.hpp"
#include "core/minefield.hpp"
#include "core/position.hpp"

namespace minesweeper {

enum class Status { Ready, Playing, Won, Lost };

constexpr bool isOver(Status s) { return s == Status::Won || s == Status::Lost; }

// Aggregate root: one game of Minesweeper. Every rule lives here, and the only
// way to change a game is through its two commands. Nothing else can touch
// the marks or the minefield, so the invariants below always hold:
//
//  - mines are planted on the first reveal, never under that first click;
//  - `hiddenSafe_` is the number of safe squares not yet revealed;
//  - once the game is over, commands are ignored.
class Game {
public:
    // Throws std::invalid_argument unless 1 <= mines < size.area().
    Game(Size size, int mines, MineLayout& layout);

    // Commands. Throw std::out_of_range for a position outside the grid.
    void reveal(Pos p);
    void toggleFlag(Pos p);

    // Queries.
    Size size() const { return marks_.size(); }
    int mines() const { return mines_; }
    int minesLeft() const { return mines_ - flags_; }  // negative if over-flagged
    Status status() const;
    CellView cell(Pos p) const;

private:
    enum class Mark : std::uint8_t { Hidden, Flagged, Revealed };

    const Minefield& plant(Pos firstClick);
    void open(Pos start);
    void require(Pos p) const;

    MineLayout* layout_;  // not owned
    int mines_;
    Grid<Mark> marks_;
    std::optional<Minefield> field_;  // empty until the first reveal
    std::optional<Pos> exploded_;
    int flags_ = 0;
    int hiddenSafe_;
};

}  // namespace minesweeper
