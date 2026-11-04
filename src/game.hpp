/**
 * @file game.hpp
 * The domain: the rules of Minesweeper, and nothing else.
 *
 * No input, no output, no randomness: everything external goes through a
 * port (see ports.hpp).
 */
#pragma once

#include <vector>

#include "ports.hpp"

enum class Status { Playing, Won, Lost };

/**
 * Everything the game knows about one cell.
 */
struct Cell {
    bool mine = false;
    bool revealed = false;
    bool flagged = false;
    int adjacent = 0;  // number of mines among the 8 neighbours
};

/**
 * One game of Minesweeper: a grid of cells and the rules that change it.
 */
class Game {
public:
    /**
     * Creates a grid where every cell is hidden. The mines are placed by
     * `layout` on the first reveal.
     * @throws std::invalid_argument unless 1 <= mines < rows * cols.
     */
    Game(int rows, int cols, int mines, MineLayout& layout);

    /**
     * Reveals a cell; if it has no adjacent mine, opens its neighbours too.
     * Does nothing on a flagged cell or once the game is over.
     * @throws std::out_of_range for a position outside the grid.
     */
    void reveal(Pos p);

    /**
     * Puts or removes a flag on a hidden cell.
     * @throws std::out_of_range for a position outside the grid.
     */
    void toggleFlag(Pos p);

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int mines() const { return mines_; }
    Status status() const { return status_; }

    /**
     * Tells whether `p` is inside the grid.
     */
    bool contains(Pos p) const;

    /**
     * Read-only access to a cell.
     * @throws std::out_of_range for a position outside the grid.
     */
    const Cell& at(Pos p) const;

private:
    Cell& cell(Pos p) { return cells_[p.row * cols_ + p.col]; }
    std::vector<Pos> neighbours(Pos p) const;
    void plant(Pos firstClick);
    void open(Pos p);

    int rows_;
    int cols_;
    int mines_;
    MineLayout& layout_;
    std::vector<Cell> cells_;  // row by row
    bool planted_ = false;
    int hiddenSafe_;  // safe cells still to reveal: 0 means won
    Status status_ = Status::Playing;
};
