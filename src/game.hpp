#pragma once

// The domain: the rules of Minesweeper, and nothing else. No input, no
// output, no randomness: everything external goes through a port.
#include <vector>

#include "ports.hpp"

enum class Status { Playing, Won, Lost };

struct Cell {
    bool mine = false;
    bool revealed = false;
    bool flagged = false;
    int adjacent = 0;  // number of mines among the 8 neighbours
};

class Game {
public:
    // Throws std::invalid_argument unless 1 <= mines < rows * cols.
    Game(int rows, int cols, int mines, MineLayout& layout);

    // Commands. Throw std::out_of_range for a position outside the grid.
    // Once the game is over, they do nothing.
    void reveal(Pos p);
    void toggleFlag(Pos p);

    // Queries.
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int mines() const { return mines_; }
    Status status() const { return status_; }
    bool contains(Pos p) const;
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
