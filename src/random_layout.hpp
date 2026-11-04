/**
 * @file random_layout.hpp
 * Adapter for the MineLayout port: random mines with std::rand().
 */
#pragma once

#include <algorithm>
#include <cstdlib>
#include <ctime>

#include "ports.hpp"

/**
 * Places the mines at random, with std::rand().
 *
 * The seed can be given from outside: the same seed always gives the same
 * game, which makes games reproducible and tests deterministic.
 */
class RandomLayout : public MineLayout {
public:
    /**
     * Uses a fixed seed: always the same game.
     */
    explicit RandomLayout(unsigned seed) { std::srand(seed); }

    /**
     * Uses the current time as seed: a different game at each launch.
     */
    RandomLayout() : RandomLayout(static_cast<unsigned>(std::time(nullptr))) {}

    /**
     * Draws random cells until there are enough, skipping the first click
     * and the cells already taken.
     */
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
