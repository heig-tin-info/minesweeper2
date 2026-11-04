#include <gtest/gtest.h>

#include <vector>

#include "core/grid.hpp"
#include "core/position.hpp"

using namespace minesweeper;

TEST(Size, ContainsOnlyPositionsInsideTheGrid) {
    constexpr Size size{2, 3};
    EXPECT_TRUE(size.contains({0, 0}));
    EXPECT_TRUE(size.contains({1, 2}));
    EXPECT_FALSE(size.contains({2, 0}));
    EXPECT_FALSE(size.contains({0, 3}));
    EXPECT_FALSE(size.contains({-1, 0}));
}

TEST(Size, IndexAndPositionAreInverse) {
    constexpr Size size{3, 4};
    for (int i = 0; i < size.area(); ++i) EXPECT_EQ(size.indexOf(size.posOf(i)), i);
    static_assert(Size{3, 4}.indexOf({1, 2}) == 6);  // usable at compile time
}

TEST(Neighbors, InteriorCellHasEight) {
    EXPECT_EQ(Neighbors({3, 3}, {1, 1}).size(), 8);
}

TEST(Neighbors, CornerIsClippedToTheGrid) {
    const Neighbors around{{3, 3}, {0, 0}};
    EXPECT_EQ(std::vector<Pos>(around.begin(), around.end()), (std::vector<Pos>{{0, 1}, {1, 0}, {1, 1}}));
}

TEST(Grid, StoresOneValuePerPosition) {
    Grid<int> grid{{2, 2}, 0};
    grid[{1, 0}] = 7;
    EXPECT_EQ((grid[{1, 0}]), 7);
    EXPECT_EQ((grid[{0, 1}]), 0);
}

TEST(Grid, RejectsNegativeSize) {
    EXPECT_THROW((Grid<int>{{-1, 2}, 0}), std::invalid_argument);
}
