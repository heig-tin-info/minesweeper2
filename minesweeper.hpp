#pragma once

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

class Cell {
   public:
    bool bomb;
    bool revealed = false;
    bool flagged = false;
    int neighborBombCount = 0;

    Cell(bool bomb = false) : bomb{bomb} {}

    char getSymbol(bool debug = false) const {
        auto symbol = [this, debug]() -> char {
            if (bomb) return '*';
            if (neighborBombCount > 0) return '0' + neighborBombCount;
            return debug ? ':' : ' ';
        };
        if (debug || revealed) return symbol();
        return flagged ? '?' : '.';
    }
};

class Board {
   private:
    std::vector<Cell> cells;

    size_t flagged;
    size_t leftToReveal;
    bool initialized;

    using coord_t = std::pair<size_t, size_t>;

    std::random_device rd;
    std::mt19937 g;

    static constexpr std::array<std::pair<int, int>, 8> deltas = {
        std::pair{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1},
        {1, -1},           {1, 0},  {1, 1}};

   public:
    const size_t bombs;
    const size_t rows, cols;

    Board(size_t rows = 7, size_t cols = 7, size_t bombs = 5)
        : rows(rows),
          cols(cols),
          bombs(bombs),
          flagged(0),
          cells(rows * cols),
          initialized(false),
          leftToReveal(rows * cols - bombs),
          g(42 /*rd()*/) {}

    auto operator[](coord_t const& p) -> Cell& {
        const size_t row = p.first, col = p.second;
        if (row >= rows || col >= cols) throw std::out_of_range("Invalid cell");
        return cells[row * cols + col];
    }

    auto operator[](coord_t const& p) const -> const Cell& {
        const size_t row = p.first, col = p.second;
        if (row >= rows || col >= cols) throw std::out_of_range("Invalid cell");
        return cells[row * cols + col];
    }

    auto getNeighborsCoords(coord_t coord) const {
        return deltas | std::views::filter([=, this](auto delta) {
                   return coord.first + delta.first < rows &&
                          coord.second + delta.second < cols;
               }) |
               std::views::transform([=, this](auto delta) {
                   return coord_t{coord.first + delta.first,
                                  coord.second + delta.second};
               });
    }
    auto getNeighborsIterator(coord_t coord) {
        return getNeighborsCoords(coord) |
               std::views::transform(
                   [=, this](auto coord) -> Cell& { return (*this)[coord]; });
    }

    auto getNeighborsIterator(coord_t coord) const {
        return getNeighborsCoords(coord) |
               std::views::transform(
                   [=, this](auto coord) { return (*this)[coord]; });
    }

    int countAdjacentBombs(coord_t coord) const {
        return std::ranges::count_if(
            getNeighborsIterator(coord),
            [](const Cell& neighbor) { return neighbor.bomb; });
    }

    void updateAdjacentBombs() {
        for (size_t row = 0; row < rows; ++row)
            for (size_t col = 0; col < cols; ++col)
                if (!(*this)[{row, col}].bomb)
                    (*this)[{row, col}].neighborBombCount =
                        countAdjacentBombs({row, col});
    }

    void expandBlanks(coord_t coord) {
        if ((*this)[coord].revealed || (*this)[coord].flagged) return;
        (*this)[coord].revealed = true;
        (*this)[coord].flagged = false;
        --leftToReveal;
        if ((*this)[coord].neighborBombCount == 0)
            for (auto coords : getNeighborsCoords(coord)) expandBlanks(coords);
    }

    void initialize() {
        if (initialized) return;
        std::vector<size_t> indices(rows * cols);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), g);
        std::ranges::for_each(
            indices | std::views::take(bombs),
            [this](size_t index) { cells[index].bomb = true; });

        updateAdjacentBombs();
        initialized = true;
    }

    bool reveal(coord_t coord) {
        if (!initialized) {
            initialize();
        }
        if ((*this)[coord].bomb) return false;
        expandBlanks(coord);
        return true;
    }

    void toggleFlag(coord_t coord) {
        auto& cell = (*this)[coord];
        if (cell.revealed) return;
        cell.flagged = !cell.flagged;
        flagged += cell.flagged ? 1 : -1;
    }

    int getBombs() const { return std::max<int>(bombs - flagged, 0); }
    int getFlagged() const { return flagged; }
    int getLeftToReveal() const { return leftToReveal; }
    auto begin() const { return cells.begin(); }
    auto end() const { return cells.end(); }
};

class GameCore : public IIntentSink, public IOutSnapshot {
    Stopwatch stopwatch;
};
