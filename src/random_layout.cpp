#include "random_layout.hpp"

#include <algorithm>
#include <cstdlib>

RandomLayout::RandomLayout(unsigned seed) { std::srand(seed); }

// Draws random cells until there are enough, skipping the first click and
// the cells already taken.
std::vector<Pos> RandomLayout::place(int rows, int cols, int count, Pos firstClick) {
    std::vector<Pos> mines;
    while (static_cast<int>(mines.size()) < count) {
        const Pos p{std::rand() % rows, std::rand() % cols};
        if (p != firstClick && std::find(mines.begin(), mines.end(), p) == mines.end())
            mines.push_back(p);
    }
    return mines;
}
