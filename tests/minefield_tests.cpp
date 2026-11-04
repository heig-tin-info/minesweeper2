#include <gtest/gtest.h>

#include <vector>

#include "core/minefield.hpp"

using namespace minesweeper;

TEST(Minefield, CountsAdjacentMines) {
    // * . .
    // . . .
    // . . *
    const std::vector<Pos> mines{{0, 0}, {2, 2}};
    const Minefield field{{3, 3}, mines};

    EXPECT_EQ(field.mineCount(), 2);
    EXPECT_TRUE(field.hasMine({0, 0}));
    EXPECT_FALSE(field.hasMine({1, 1}));
    EXPECT_EQ(field.adjacentMines({1, 1}), 2);
    EXPECT_EQ(field.adjacentMines({0, 1}), 1);
    EXPECT_EQ(field.adjacentMines({0, 2}), 0);
}

TEST(Minefield, RejectsMinesOutsideTheGrid) {
    const std::vector<Pos> mines{{3, 0}};
    EXPECT_THROW((Minefield{{3, 3}, mines}), std::out_of_range);
}

TEST(Minefield, RejectsDuplicatedMines) {
    const std::vector<Pos> mines{{1, 1}, {1, 1}};
    EXPECT_THROW((Minefield{{3, 3}, mines}), std::invalid_argument);
}
