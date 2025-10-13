#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

class Cell {
   public:
    bool bomb = false;
    bool revealed = false;
    bool flagged = false;
    int neighborBombCount = 0;

    Cell() = default;
    explicit Cell(bool hasBomb) : bomb(hasBomb) {}

    char getSymbol(bool debug = false) const {
        auto symbol = [this, debug]() -> char {
            if (bomb) return '*';
            if (neighborBombCount > 0)
                return static_cast<char>('0' + neighborBombCount);
            return debug ? ':' : ' ';
        };
        if (debug || revealed) return symbol();
        return flagged ? '?' : '.';
    }
};

class Board {
   public:
    using coord_t = std::pair<size_t, size_t>;

    Board(size_t rows = 7, size_t cols = 7, size_t bombs = 5)
        : cells(rows * cols), rows(rows), cols(cols), bombs(bombs), g(rd()) {
        if (rows == 0 || cols == 0) {
            throw std::invalid_argument("Board dimensions must be > 0");
        }
        if (bombs >= rows * cols) {
            throw std::invalid_argument(
                "Bomb count must be less than total cells");
        }
        leftToReveal = rows * cols - bombs;
        initialized = false;
    }

    size_t width() const { return cols; }
    size_t height() const { return rows; }

    Cell& operator[](coord_t const& p) {
        const size_t row = p.first;
        const size_t col = p.second;
        if (row >= rows || col >= cols) throw std::out_of_range("Invalid cell");
        return cells[index(p)];
    }

    const Cell& operator[](coord_t const& p) const {
        const size_t row = p.first;
        const size_t col = p.second;
        if (row >= rows || col >= cols) throw std::out_of_range("Invalid cell");
        return cells[index(p)];
    }

    bool isInside(coord_t const& p) const {
        return p.first < rows && p.second < cols;
    }

    void reset() {
        for (auto& cell : cells) cell = Cell{};
        flagged = 0;
        leftToReveal = rows * cols - bombs;
        initialized = false;
    }

    bool reveal(coord_t coord) {
        if (!isInside(coord)) throw std::out_of_range("Reveal outside board");

        auto& cell = (*this)[coord];
        if (cell.revealed) return true;
        if (cell.flagged) return true;

        if (!initialized) initialize(coord);

        if (cell.bomb) return false;

        expandBlanks(coord);
        return true;
    }

    void toggleFlag(coord_t coord) {
        if (!isInside(coord)) throw std::out_of_range("Flag outside board");

        auto& cell = (*this)[coord];
        if (cell.revealed) return;

        if (cell.flagged) {
            cell.flagged = false;
            if (flagged > 0) --flagged;
        } else {
            cell.flagged = true;
            ++flagged;
        }
    }

    int bombsRemaining() const {
        const size_t remaining = bombs > flagged ? bombs - flagged : 0;
        return static_cast<int>(remaining);
    }

    size_t flaggedCount() const { return flagged; }
    size_t unrevealedSafeCount() const { return leftToReveal; }

    bool isInitialized() const { return initialized; }

    void revealAll() {
        flagged = 0;
        for (auto& cell : cells) {
            if (!cell.revealed && !cell.bomb && leftToReveal > 0)
                --leftToReveal;
            cell.revealed = true;
            cell.flagged = false;
        }
    }

    const Cell& cellAt(size_t col, size_t row) const {
        return (*this)[{row, col}];
    }

   private:
    std::vector<Cell> cells;

    size_t flagged = 0;
    size_t leftToReveal = 0;
    bool initialized = false;

    const size_t rows;
    const size_t cols;
    const size_t bombs;

    std::random_device rd;
    std::mt19937 g;

    static constexpr std::array<std::pair<int, int>, 8> deltas = {
        std::pair{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1},
        {1, -1},           {1, 0},  {1, 1}};

    size_t index(coord_t coord) const {
        return coord.first * cols + coord.second;
    }

    int countAdjacentBombs(coord_t coord) const {
        int count = 0;
        for (auto [dy, dx] : deltas) {
            const int nr = static_cast<int>(coord.first) + dy;
            const int nc = static_cast<int>(coord.second) + dx;
            if (nr < 0 || nc < 0) continue;
            if (nr >= static_cast<int>(rows) || nc >= static_cast<int>(cols))
                continue;
            const auto neighbor =
                coord_t{static_cast<size_t>(nr), static_cast<size_t>(nc)};
            if ((*this)[neighbor].bomb) ++count;
        }
        return count;
    }

    void updateAdjacentBombs() {
        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col) {
                auto& cell = (*this)[{row, col}];
                if (!cell.bomb) {
                    cell.neighborBombCount = countAdjacentBombs({row, col});
                }
            }
        }
    }

    void initialize(coord_t first) {
        if (initialized) return;
        for (auto& cell : cells) {
            cell.bomb = false;
            cell.neighborBombCount = 0;
        }

        const size_t forbidden = index(first);
        std::vector<size_t> indices(cells.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), g);

        size_t placed = 0;
        for (size_t idx : indices) {
            if (idx == forbidden) continue;
            cells[idx].bomb = true;
            if (++placed == bombs) break;
        }
        if (placed != bombs) {
            throw std::runtime_error(
                "Unable to place all bombs (board too small for safe first "
                "move)");
        }

        updateAdjacentBombs();
        initialized = true;
    }

    void expandBlanks(coord_t coord) {
        auto& cell = (*this)[coord];
        if (cell.revealed || cell.flagged) return;
        if (cell.bomb) return;
        cell.revealed = true;
        if (leftToReveal > 0) --leftToReveal;
        if (cell.neighborBombCount != 0) return;

        for (auto [dy, dx] : deltas) {
            const int nr = static_cast<int>(coord.first) + dy;
            const int nc = static_cast<int>(coord.second) + dx;
            if (nr < 0 || nc < 0) continue;
            if (nr >= static_cast<int>(rows) || nc >= static_cast<int>(cols))
                continue;
            expandBlanks({static_cast<size_t>(nr), static_cast<size_t>(nc)});
        }
    }
};
