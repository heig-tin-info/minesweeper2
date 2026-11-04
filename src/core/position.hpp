#pragma once

// Value objects that describe the geometry of the grid. They carry no game
// rule, only the arithmetic every other class would otherwise repeat.
#include <array>

namespace minesweeper {

// A cell coordinate. Row 0 is the top row, column 0 the leftmost column.
struct Pos {
    int row;
    int col;

    friend constexpr bool operator==(Pos, Pos) = default;
};

// Dimensions of a rectangular grid.
struct Size {
    int rows;
    int cols;

    constexpr int area() const { return rows * cols; }

    constexpr bool contains(Pos p) const {
        return 0 <= p.row && p.row < rows && 0 <= p.col && p.col < cols;
    }

    // Row-major mapping between a position and a flat index.
    constexpr int indexOf(Pos p) const { return p.row * cols + p.col; }
    constexpr Pos posOf(int index) const { return {index / cols, index % cols}; }

    friend constexpr bool operator==(Size, Size) = default;
};

// The (up to eight) cells surrounding a position, clipped to the grid. It is
// a small iterable value, so neighbours can be walked without any allocation:
//
//     for (Pos n : Neighbors{size, p}) ...
class Neighbors {
public:
    constexpr Neighbors(Size size, Pos center) {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                const Pos n{center.row + dr, center.col + dc};
                if ((dr != 0 || dc != 0) && size.contains(n)) items_[count_++] = n;
            }
    }

    constexpr auto begin() const { return items_.begin(); }
    constexpr auto end() const { return items_.begin() + count_; }
    constexpr int size() const { return count_; }

private:
    std::array<Pos, 8> items_{};
    int count_ = 0;
};

}  // namespace minesweeper
