#include "game.hpp"

#include <stdexcept>

Game::Game(int rows, int cols, int mines, MineLayout& layout)
    : rows_(rows),
      cols_(cols),
      mines_(mines),
      layout_(layout),
      cells_(rows * cols),
      hiddenSafe_(rows * cols - mines) {
    if (rows < 1 || cols < 1 || mines < 1 || mines >= rows * cols)
        throw std::invalid_argument("Game: need 1 <= mines < rows * cols");
}

bool Game::contains(Pos p) const {
    return p.row >= 0 && p.row < rows_ && p.col >= 0 && p.col < cols_;
}

const Cell& Game::at(Pos p) const {
    if (!contains(p)) throw std::out_of_range("Game: position outside the grid");
    return cells_[p.row * cols_ + p.col];
}

void Game::reveal(Pos p) {
    const Cell& c = at(p);
    if (status_ != Status::Playing || c.revealed || c.flagged) return;
    if (!planted_) plant(p);

    if (c.mine) {
        cell(p).revealed = true;
        status_ = Status::Lost;
        return;
    }
    open(p);
    if (hiddenSafe_ == 0) status_ = Status::Won;
}

void Game::toggleFlag(Pos p) {
    const Cell& c = at(p);
    if (status_ != Status::Playing || c.revealed) return;
    cell(p).flagged = !c.flagged;
}

std::vector<Pos> Game::neighbours(Pos p) const {
    std::vector<Pos> result;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            const Pos n{p.row + dr, p.col + dc};
            if ((dr != 0 || dc != 0) && contains(n)) result.push_back(n);
        }
    return result;
}

// Asks the layout where the mines go, then counts each cell's neighbours.
void Game::plant(Pos firstClick) {
    for (Pos m : layout_.place(rows_, cols_, mines_, firstClick)) cell(m).mine = true;
    for (int r = 0; r < rows_; ++r)
        for (int c = 0; c < cols_; ++c)
            for (Pos n : neighbours({r, c}))
                if (at(n).mine) ++cell({r, c}).adjacent;
    planted_ = true;
}

// Reveals `p`; if it has no adjacent mine, opens its neighbours too.
void Game::open(Pos p) {
    Cell& c = cell(p);
    if (c.revealed || c.flagged || c.mine) return;
    c.revealed = true;
    --hiddenSafe_;
    if (c.adjacent == 0)
        for (Pos n : neighbours(p)) open(n);
}
