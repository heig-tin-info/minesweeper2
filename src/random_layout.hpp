#pragma once

#include <algorithm>
#include <cstdlib>

#include "ports.hpp"

// Adapter for the MineLayout port, built on std::rand(). The seed is given
// from outside: the same seed always gives the same game, which makes games
// reproducible and tests deterministic.
class RandomLayout : public MineLayout {
public:
    explicit RandomLayout(unsigned seed) { std::srand(seed); }

    // Draws random cells until there are enough, skipping the first click
    // and the cells already taken.
    std::vector<Pos> place(int rows, int cols, int count, Pos firstClick) override {
        std::vector<Pos> mines;
        while (static_cast<int>(mines.size()) < count) {
            const Pos p{std::rand() % rows, std::rand() % cols};
            if (p != firstClick && std::find(mines.begin(), mines.end(), p) == mines.end())
                mines.push_back(p);
        }
        return mines;
    }
};
