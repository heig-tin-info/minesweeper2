#pragma once

// A rectangular container indexed by `Pos`. Both the minefield and the
// player's marks are grids; writing the index arithmetic once keeps them DRY.
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "core/position.hpp"

namespace minesweeper {

template <class T>
class Grid {
    // std::vector<bool> is a bit-packed proxy container: `grid[p]` would not
    // return a real `bool&`. Store an enum or a small struct instead.
    static_assert(!std::is_same_v<T, bool>, "use an enum or a struct, not bool");

public:
    Grid(Size size, const T& fill)
        : size_(size), cells_(checkedArea(size), fill) {}

    Size size() const { return size_; }

    // Unchecked in release builds: callers validate positions at the public
    // boundary of their own class, once.
    T& operator[](Pos p) { return cells_[index(p)]; }
    const T& operator[](Pos p) const { return cells_[index(p)]; }

private:
    static std::size_t checkedArea(Size size) {
        if (size.rows < 0 || size.cols < 0)
            throw std::invalid_argument("Grid: negative size");
        return static_cast<std::size_t>(size.area());
    }

    std::size_t index(Pos p) const {
        assert(size_.contains(p));
        return static_cast<std::size_t>(size_.indexOf(p));
    }

    Size size_;
    std::vector<T> cells_;
};

}  // namespace minesweeper
