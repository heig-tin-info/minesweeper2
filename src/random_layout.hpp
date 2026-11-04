#pragma once

#include "ports.hpp"

// Adapter for the MineLayout port, built on std::rand(). The seed is given
// from outside: the same seed always gives the same game, which makes games
// reproducible and tests deterministic.
class RandomLayout : public MineLayout {
public:
    explicit RandomLayout(unsigned seed);
    std::vector<Pos> place(int rows, int cols, int count, Pos firstClick) override;
};
