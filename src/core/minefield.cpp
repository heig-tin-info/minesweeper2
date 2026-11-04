#include "core/minefield.hpp"

#include <stdexcept>
#include <utility>

namespace minesweeper {

// Each mine increments its neighbours: O(mines), instead of counting around
// every square of the grid.
Minefield::Minefield(Size size, std::span<const Pos> mines)
    : squares_(size, Square{}), mineCount_(static_cast<int>(mines.size())) {
    for (Pos mine : mines) {
        if (!size.contains(mine))
            throw std::out_of_range("Minefield: mine outside the grid");
        if (std::exchange(squares_[mine].mine, true))
            throw std::invalid_argument("Minefield: duplicated mine");
        for (Pos n : Neighbors{size, mine}) ++squares_[n].adjacent;
    }
}

}  // namespace minesweeper
