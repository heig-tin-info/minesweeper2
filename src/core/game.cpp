#include "core/game.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace minesweeper {

Game::Game(Size size, int mines, MineLayout& layout)
    : layout_(&layout),
      mines_(mines),
      marks_(size, Mark::Hidden),
      hiddenSafe_(size.area() - mines) {
    if (mines < 1 || mines >= size.area())
        throw std::invalid_argument("Game: mines must be between 1 and rows * cols - 1");
}

void Game::reveal(Pos p) {
    require(p);
    if (isOver(status()) || marks_[p] != Mark::Hidden) return;

    const Minefield& field = field_ ? *field_ : plant(p);
    if (field.hasMine(p)) {
        marks_[p] = Mark::Revealed;
        exploded_ = p;
        return;
    }
    open(p);
}

void Game::toggleFlag(Pos p) {
    require(p);
    Mark& mark = marks_[p];
    if (isOver(status()) || mark == Mark::Revealed) return;

    mark = (mark == Mark::Flagged) ? Mark::Hidden : Mark::Flagged;
    flags_ += (mark == Mark::Flagged) ? 1 : -1;
}

// Derived, never stored: the status cannot disagree with the marks.
Status Game::status() const {
    if (exploded_) return Status::Lost;
    if (!field_) return Status::Ready;
    return hiddenSafe_ == 0 ? Status::Won : Status::Playing;
}

CellView Game::cell(Pos p) const {
    require(p);
    const Mark mark = marks_[p];
    if (mark == Mark::Revealed) {
        if (field_->hasMine(p)) return cell::Mine{.exploded = true};
        return cell::Revealed{field_->adjacentMines(p)};
    }

    const Status s = status();
    if (!isOver(s)) {
        if (mark == Mark::Flagged) return cell::Flagged{};
        return cell::Hidden{};
    }

    // Game over: the projection now tells the whole truth. Nothing is
    // mutated to show it.
    const bool mine = field_->hasMine(p);
    if (mark == Mark::Flagged) {
        if (mine) return cell::Flagged{};
        return cell::WrongFlag{};
    }
    if (!mine) return cell::Hidden{};
    if (s == Status::Won) return cell::Flagged{};
    return cell::Mine{.exploded = false};
}

// The layout is an outside collaborator: trust, but verify its contract.
const Minefield& Game::plant(Pos firstClick) {
    const std::vector<Pos> mines = layout_->place(size(), mines_, firstClick);
    if (std::ssize(mines) != mines_ || std::ranges::find(mines, firstClick) != mines.end())
        throw std::logic_error("Game: the mine layout broke its contract");
    return field_.emplace(size(), mines);
}

// Reveals `start` and, when it has no adjacent mine, its whole empty region.
// An explicit stack instead of recursion: a huge empty grid cannot overflow
// the call stack.
void Game::open(Pos start) {
    std::vector<Pos> pending{start};
    while (!pending.empty()) {
        const Pos p = pending.back();
        pending.pop_back();
        if (marks_[p] != Mark::Hidden) continue;

        marks_[p] = Mark::Revealed;
        --hiddenSafe_;
        if (field_->adjacentMines(p) != 0) continue;
        for (Pos n : Neighbors{size(), p})
            if (marks_[n] == Mark::Hidden) pending.push_back(n);
    }
}

void Game::require(Pos p) const {
    if (!size().contains(p)) throw std::out_of_range("Game: position outside the grid");
}

}  // namespace minesweeper
