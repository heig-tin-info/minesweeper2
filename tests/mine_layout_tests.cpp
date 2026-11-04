#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <utility>

#include "core/mine_layout.hpp"

using namespace minesweeper;

namespace {

std::set<std::pair<int, int>> asSet(const std::vector<Pos>& mines) {
    std::set<std::pair<int, int>> out;
    for (Pos p : mines) out.emplace(p.row, p.col);
    return out;
}

}  // namespace

TEST(RandomMineLayout, PlacesTheRequestedNumberOfDistinctMines) {
    RandomMineLayout layout{1};
    const auto mines = layout.place({8, 8}, 20, {4, 4});
    EXPECT_EQ(mines.size(), 20u);
    EXPECT_EQ(asSet(mines).size(), 20u);
    for (Pos p : mines) EXPECT_TRUE(Size(8, 8).contains(p));
}

TEST(RandomMineLayout, KeepsTheOpeningFreeWhenThereIsRoom) {
    RandomMineLayout layout{2};
    const Pos first{4, 4};
    const auto mines = layout.place({8, 8}, 55, first);  // 64 - 9 = 55: exactly room
    EXPECT_EQ(std::ranges::count(mines, first), 0);
    for (Pos n : Neighbors{{8, 8}, first}) EXPECT_EQ(std::ranges::count(mines, n), 0);
}

TEST(RandomMineLayout, OnlySparesTheFirstClickOnACrowdedGrid) {
    RandomMineLayout layout{3};
    const auto mines = layout.place({3, 3}, 8, {1, 1});
    EXPECT_EQ(mines.size(), 8u);
    EXPECT_EQ(std::ranges::count(mines, Pos{1, 1}), 0);
}

TEST(RandomMineLayout, IsReproducibleWithTheSameSeed) {
    RandomMineLayout a{42}, b{42};
    EXPECT_EQ(a.place({9, 9}, 10, {0, 0}), b.place({9, 9}, 10, {0, 0}));
}
