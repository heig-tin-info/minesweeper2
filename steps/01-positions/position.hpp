#pragma once

// Geometry of the grid: where a cell is, how big the grid is, and which cells
// touch each other. No game rule lives here.
#include <vector>

// A cell coordinate. Row 0 is the top row, column 0 the leftmost column.
struct Pos {
    int row;
    int col;

    bool operator==(const Pos&) const = default;
};

// Dimensions of a rectangular grid.
struct Size {
    int rows;
    int cols;

    int area() const;
    bool contains(Pos p) const;

    // Row-major mapping between a position and an index in a flat array.
    int indexOf(Pos p) const;
    Pos posOf(int index) const;
};

// The cells surrounding p (at most eight), clipped to the grid.
std::vector<Pos> neighbors(Size size, Pos p);
