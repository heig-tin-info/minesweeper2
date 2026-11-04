#include "core/mine_layout.hpp"

#include <algorithm>
#include <iterator>

namespace minesweeper {

std::vector<Pos> RandomMineLayout::place(Size size, int count, Pos firstClick) {
    // When the grid has room for it, the whole 3x3 opening stays free, so the
    // first click uncovers an area rather than a lone number.
    const Neighbors around{size, firstClick};
    const bool roomy = size.area() - 1 - around.size() >= count;
    auto isSafe = [&](Pos p) {
        return p == firstClick || (roomy && std::ranges::find(around, p) != around.end());
    };

    std::vector<Pos> candidates;
    for (int i = 0; i < size.area(); ++i)
        if (const Pos p = size.posOf(i); !isSafe(p)) candidates.push_back(p);

    std::vector<Pos> mines;
    std::ranges::sample(candidates, std::back_inserter(mines), count, rng_);
    return mines;
}

}  // namespace minesweeper
