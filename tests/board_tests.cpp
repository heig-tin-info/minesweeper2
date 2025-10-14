#include <array>
#include <gtest/gtest.h>
#include <vector>

#include "board.hpp"

namespace {

constexpr Coord make_coord(index_t r, index_t c) { return Coord{r, c}; }

std::vector<Coord> neighbors_of(const Board& board, Coord origin) {
    static constexpr std::array<std::pair<int, int>, 8> deltas{
        std::pair{-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
        {0, 1},            {1, -1}, {1, 0},  {1, 1}};

    std::vector<Coord> result;
    for (auto [dr, dc] : deltas) {
        Coord n{origin.r + dr, origin.c + dc};
        if (board.in_bounds(n)) result.push_back(n);
    }
    return result;
}

}  // namespace

TEST(BoardTest, ConstructorRejectsInvalidDimensions) {
    EXPECT_THROW(Board(0, 5, 1), std::invalid_argument);
    EXPECT_THROW(Board(5, 0, 1), std::invalid_argument);
    EXPECT_THROW(Board(3, 3, 9), std::invalid_argument);
    EXPECT_NO_THROW(Board(3, 3, 8));
}

TEST(BoardTest, FirstRevealKeepsOpeningSafe) {
    Board board(5, 5, 5, /*seed=*/42);
    const Coord start = make_coord(2, 2);

    const auto result = board.reveal(start);
    EXPECT_EQ(result, Board::RevealResult::Ok);
    EXPECT_TRUE(board.isInitialized());
    EXPECT_FALSE(board(start).bomb());
    EXPECT_EQ(board(start).neighborBombCount, 0);

    for (const auto neighbor : neighbors_of(board, start)) {
        EXPECT_FALSE(board(neighbor).bomb()) << "Neighbor should be safe";
    }
}

TEST(BoardTest, ToggleFlagUpdatesCounters) {
    Board board(4, 4, 3, /*seed=*/123);
    const Coord pos = make_coord(1, 1);

    EXPECT_EQ(board.flaggedCount(), 0u);
    board.toggleFlag(pos);
    EXPECT_TRUE(board(pos).flagged());
    EXPECT_EQ(board.flaggedCount(), 1u);
    EXPECT_EQ(board.bombsRemaining(), 2);

    board.toggleFlag(pos);
    EXPECT_FALSE(board(pos).flagged());
    EXPECT_EQ(board.flaggedCount(), 0u);
    EXPECT_EQ(board.bombsRemaining(), 3);
}

TEST(BoardTest, RevealRespectsFlagsAndExistingOpens) {
    Board board(5, 5, 5, /*seed=*/99);
    const Coord start = make_coord(0, 0);
    ASSERT_EQ(board.reveal(start), Board::RevealResult::Ok);

    Coord target{-1, -1};
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            const Coord pos = make_coord(r, c);
            if (board(pos).bomb()) continue;
            if (!board(pos).revealed()) {
                target = pos;
                break;
            }
        }
        if (target.r != -1) break;
    }
    ASSERT_NE(target.r, -1) << "Expected to find a hidden safe cell";

    board.toggleFlag(target);
    EXPECT_EQ(board.reveal(target), Board::RevealResult::Noop);
    board.toggleFlag(target);
    ASSERT_FALSE(board(target).revealed());
    EXPECT_EQ(board.reveal(target), Board::RevealResult::Ok);
    EXPECT_EQ(board.reveal(target), Board::RevealResult::Noop);
}

TEST(BoardTest, FloodFillRevealsContiguousSafeArea) {
    Board board(6, 6, 4, /*seed=*/2023);
    const Coord start = make_coord(0, 0);
    ASSERT_EQ(board.reveal(start), Board::RevealResult::Ok);

    // The opening move and its neighbours should be revealed because they are
    // enforced to be bomb-free.
    std::size_t revealed = 0;
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            if (board(make_coord(r, c)).revealed()) ++revealed;
        }
    }
    EXPECT_GT(revealed, 1u);
    EXPECT_LT(board.unrevealedSafeCount(),
              board.height() * board.width() - board.flaggedCount());
}

TEST(BoardTest, HittingBombTriggersLossAndRevealsAll) {
    Board board(6, 6, 8, /*seed=*/517);
    const Coord start = make_coord(0, 0);
    ASSERT_NE(board.reveal(start), Board::RevealResult::HitBomb);

    Coord bomb{-1, -1};
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            const Coord pos = make_coord(r, c);
            if (!board(pos).bomb()) continue;
            if (!board(pos).revealed()) {
                bomb = pos;
                break;
            }
        }
        if (bomb.r != -1) break;
    }
    ASSERT_NE(bomb.r, -1);

    const auto result = board.reveal(bomb);
    EXPECT_EQ(result, Board::RevealResult::HitBomb);
    EXPECT_TRUE(board.lost());
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            EXPECT_TRUE(board(make_coord(r, c)).revealed());
        }
    }
}

TEST(BoardTest, RevealingAllSafeCellsYieldsVictory) {
    Board board(6, 5, 4, /*seed=*/321);
    const Coord start = make_coord(0, 0);
    ASSERT_EQ(board.reveal(start), Board::RevealResult::Ok);

    const std::size_t initialUnrevealed = board.unrevealedSafeCount();
    bool sawVictory = false;
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            const Coord pos = make_coord(r, c);
            if (board(pos).bomb()) continue;
            if (board(pos).revealed()) continue;
            const auto result = board.reveal(pos);
            if (result == Board::RevealResult::Victory) sawVictory = true;
        }
    }

    EXPECT_TRUE(board.won());
    EXPECT_EQ(board.unrevealedSafeCount(), 0u);
    EXPECT_TRUE(sawVictory || initialUnrevealed == 0u);
}

TEST(BoardTest, RevealAllExposesEveryCellAndClearsFlags) {
    Board board(5, 5, 5, /*seed=*/777);
    const Coord start = make_coord(0, 0);
    ASSERT_EQ(board.reveal(start), Board::RevealResult::Ok);

    const Coord flagged = make_coord(1, 3);
    board.toggleFlag(flagged);
    board.revealAll();

    EXPECT_EQ(board.flaggedCount(), 0u);
    EXPECT_EQ(board.unrevealedSafeCount(), 0u);
    for (index_t r = 0; r < static_cast<index_t>(board.height()); ++r) {
        for (index_t c = 0; c < static_cast<index_t>(board.width()); ++c) {
            const auto& cell = board(make_coord(r, c));
            EXPECT_TRUE(cell.revealed());
            EXPECT_FALSE(cell.flagged());
        }
    }
}
