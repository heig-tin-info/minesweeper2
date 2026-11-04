#include <gtest/gtest.h>

#include "core/game.hpp"
#include "test_doubles.hpp"

using namespace minesweeper;
using minesweeper::testing::FixedMineLayout;

namespace {

// A 3x3 game with one mine in the top-left corner:
//
//     * 1 .
//     1 1 .
//     . . .
struct OneMineGame : ::testing::Test {
    FixedMineLayout layout{{{0, 0}}};
    Game game{{3, 3}, 1, layout};
};

}  // namespace

TEST(Game, RejectsImpossibleConfigurations) {
    FixedMineLayout layout{{}};
    EXPECT_THROW((Game{{3, 3}, 0, layout}), std::invalid_argument);
    EXPECT_THROW((Game{{3, 3}, 9, layout}), std::invalid_argument);
    EXPECT_THROW((Game{{0, 3}, 1, layout}), std::invalid_argument);
}

TEST_F(OneMineGame, StartsReadyWithEverythingHidden) {
    EXPECT_EQ(game.status(), Status::Ready);
    EXPECT_EQ(game.cell({0, 0}), CellView{cell::Hidden{}});
    EXPECT_EQ(layout.calls, 0) << "mines are planted lazily";
}

TEST_F(OneMineGame, PlantsMinesOnTheFirstRevealOnly) {
    game.reveal({2, 2});
    game.reveal({0, 1});
    EXPECT_EQ(layout.calls, 1);
    EXPECT_EQ(layout.lastFirstClick, (Pos{2, 2}));
}

TEST_F(OneMineGame, RevealingAnEmptyCellOpensItsRegion) {
    game.reveal({2, 2});
    EXPECT_EQ(game.status(), Status::Won) << "every safe cell is connected here";
    EXPECT_EQ(game.cell({1, 1}), CellView{cell::Revealed{1}});
    EXPECT_EQ(game.cell({2, 2}), CellView{cell::Revealed{0}});
}

TEST_F(OneMineGame, RevealingANumberOpensOnlyThatCell) {
    game.reveal({1, 1});
    EXPECT_EQ(game.status(), Status::Playing);
    EXPECT_EQ(game.cell({1, 1}), CellView{cell::Revealed{1}});
    EXPECT_EQ(game.cell({2, 2}), CellView{cell::Hidden{}});
}

TEST_F(OneMineGame, FlagsStopTheFloodFill) {
    game.toggleFlag({2, 1});
    game.reveal({2, 2});
    EXPECT_EQ(game.cell({2, 1}), CellView{cell::Flagged{}});
    EXPECT_EQ(game.status(), Status::Playing) << "the flagged safe cell is still to reveal";
}

TEST_F(OneMineGame, FlagCanBeToggledBeforeTheFirstReveal) {
    game.toggleFlag({0, 0});
    EXPECT_EQ(game.minesLeft(), 0);
    game.toggleFlag({0, 0});
    EXPECT_EQ(game.minesLeft(), 1);
    EXPECT_EQ(game.status(), Status::Ready);
}

TEST_F(OneMineGame, RevealedCellsCannotBeFlagged) {
    game.reveal({1, 1});
    game.toggleFlag({1, 1});
    EXPECT_EQ(game.cell({1, 1}), CellView{cell::Revealed{1}});
    EXPECT_EQ(game.minesLeft(), 1);
}

TEST(Game, HittingAMineLosesAndShowsTheTruth) {
    // . * .
    // . . .     first click on (1,1), then the player flags (2,2) wrongly
    // . . .     and steps on the mine.
    FixedMineLayout layout{{{0, 1}}};
    Game game{{3, 3}, 1, layout};
    game.reveal({1, 1});
    game.toggleFlag({2, 2});
    game.reveal({0, 1});

    EXPECT_EQ(game.status(), Status::Lost);
    EXPECT_EQ(game.cell({0, 1}), CellView{cell::Mine{.exploded = true}});
    EXPECT_EQ(game.cell({2, 2}), CellView{cell::WrongFlag{}});
    EXPECT_EQ(game.cell({0, 0}), CellView{cell::Hidden{}});
}

TEST(Game, LosingShowsTheOtherMinesUnexploded) {
    FixedMineLayout layout{{{0, 0}, {0, 2}}};
    Game game{{3, 3}, 2, layout};
    game.reveal({2, 1});
    game.reveal({0, 0});
    EXPECT_EQ(game.cell({0, 2}), CellView{cell::Mine{.exploded = false}});
}

TEST_F(OneMineGame, WinningFlagsTheRemainingMines) {
    game.reveal({2, 2});
    EXPECT_EQ(game.cell({0, 0}), CellView{cell::Flagged{}});
}

TEST_F(OneMineGame, CommandsAreIgnoredOnceTheGameIsOver) {
    game.reveal({2, 2});
    game.reveal({0, 0});
    game.toggleFlag({0, 0});
    EXPECT_EQ(game.status(), Status::Won);
}

TEST_F(OneMineGame, RejectsPositionsOutsideTheGrid) {
    EXPECT_THROW(game.reveal({3, 0}), std::out_of_range);
    EXPECT_THROW(game.toggleFlag({0, -1}), std::out_of_range);
    EXPECT_THROW((void)game.cell({9, 9}), std::out_of_range);
}

TEST(Game, RejectsALayoutThatBreaksItsContract) {
    FixedMineLayout underFirstClick{{{1, 1}}};
    Game game{{3, 3}, 1, underFirstClick};
    EXPECT_THROW(game.reveal({1, 1}), std::logic_error);

    FixedMineLayout tooFew{{}};
    Game other{{3, 3}, 1, tooFew};
    EXPECT_THROW(other.reveal({1, 1}), std::logic_error);
}

TEST(Game, FloodFillHandlesHugeEmptyGrids) {
    FixedMineLayout layout{{{0, 0}}};
    Game game{{1000, 1000}, 1, layout};
    game.reveal({999, 999});  // a recursive fill would overflow the stack here
    EXPECT_EQ(game.status(), Status::Won);
}
