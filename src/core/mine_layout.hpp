#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "core/position.hpp"

namespace minesweeper {

// Port: decides where the mines go. The game asks only once, on the first
// reveal, so that the first click can never hit a mine.
//
// Contract: return exactly `count` distinct positions inside `size`, none of
// them equal to `firstClick`.
class MineLayout {
public:
    virtual ~MineLayout() = default;
    virtual std::vector<Pos> place(Size size, int count, Pos firstClick) = 0;
};

// Uniformly random layout. The seed is injected, so a game is reproducible.
class RandomMineLayout final : public MineLayout {
public:
    explicit RandomMineLayout(std::uint64_t seed) : rng_(seed) {}

    std::vector<Pos> place(Size size, int count, Pos firstClick) override;

private:
    std::mt19937_64 rng_;
};

}  // namespace minesweeper
