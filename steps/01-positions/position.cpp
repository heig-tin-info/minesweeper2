#include "position.hpp"

int Size::area() const { return rows * cols; }

bool Size::contains(Pos p) const {
    return 0 <= p.row && p.row < rows && 0 <= p.col && p.col < cols;
}

int Size::indexOf(Pos p) const { return p.row * cols + p.col; }

Pos Size::posOf(int index) const { return {index / cols, index % cols}; }

std::vector<Pos> neighbors(Size size, Pos p) {
    std::vector<Pos> result;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            const Pos n{p.row + dr, p.col + dc};
            if ((dr != 0 || dc != 0) && size.contains(n)) result.push_back(n);
        }
    return result;
}
