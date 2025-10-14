#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "minesweeper.hpp"
#include "types.hpp"

namespace {

UserIntent make_intent(Action action, std::optional<Pos> where = std::nullopt) {
    return UserIntent{action, where};
}

std::vector<std::pair<int, int>> find_bomb_positions(const GameCore& core) {
    std::vector<std::pair<int, int>> bombs;
    for (int y = 0; y < core.height(); ++y) {
        for (int x = 0; x < core.width(); ++x) {
            if (core.cell(x, y).bomb()) bombs.emplace_back(x, y);
        }
    }
    return bombs;
}

}  // namespace

TEST(GameCoreTest, ConstructorRejectsInvalidParameters) {
    EXPECT_THROW(GameCore(0, 5, 1), std::invalid_argument);
    EXPECT_THROW(GameCore(5, 0, 1), std::invalid_argument);
    EXPECT_THROW(GameCore(3, 3, 9), std::invalid_argument);
}

TEST(GameCoreTest, ToggleFlagReflectsInProjectionAndHud) {
    GameCore core(6, 6, 6);

    const Pos p{2, 3};
    core.handle(make_intent(Action::ToggleFlag, p));
    EXPECT_EQ(core.board().at(p.x, p.y), "?");
    EXPECT_EQ(core.hud().count, "5");

    core.handle(make_intent(Action::ToggleFlag, p));
    EXPECT_EQ(core.board().at(p.x, p.y), ".");
    EXPECT_EQ(core.hud().count, "6");
}

TEST(GameCoreTest, IgnoreRevealWithoutCoordinates) {
    GameCore core(5, 5, 3);
    core.handle(make_intent(Action::Reveal));

    // Board must remain uninitialised and no cells revealed.
    EXPECT_FALSE(core.cell(0, 0).revealed());
    EXPECT_EQ(core.state(), GameState::InProgress);
}

TEST(GameCoreTest, RevealingBombEndsGameAsLoss) {
    GameCore core(6, 6, 6);

    // First reveal initialises the board safely.
    core.handle(make_intent(Action::Reveal, Pos{0, 0}));

    const auto bombs = find_bomb_positions(core);
    ASSERT_FALSE(bombs.empty());

    // Pick the first bomb that wasn't automatically revealed.
    std::pair<int, int> bomb{-1, -1};
    for (const auto& b : bombs) {
        if (!core.cell(b.first, b.second).revealed()) {
            bomb = b;
            break;
        }
    }
    ASSERT_NE(bomb.first, -1);

    core.handle(make_intent(Action::Reveal, Pos{bomb.first, bomb.second}));
    EXPECT_EQ(core.state(), GameState::Lost);
    EXPECT_EQ(core.board().at(bomb.first, bomb.second), "*");
}

TEST(GameCoreTest, RevealingAllSafeCellsLeadsToVictory) {
    GameCore core(6, 5, 5);
    core.handle(make_intent(Action::Reveal, Pos{0, 0}));

    for (int y = 0; y < core.height(); ++y) {
        for (int x = 0; x < core.width(); ++x) {
            if (core.cell(x, y).bomb()) continue;
            if (core.cell(x, y).revealed()) continue;
            core.handle(make_intent(Action::Reveal, Pos{x, y}));
        }
    }

    EXPECT_EQ(core.state(), GameState::Won);
    EXPECT_EQ(core.hud().count, "5");

    // After victory the board is fully revealed.
    for (int y = 0; y < core.height(); ++y) {
        for (int x = 0; x < core.width(); ++x) {
            EXPECT_TRUE(core.cell(x, y).revealed());
        }
    }
}

TEST(GameCoreTest, NewGameResetsBoardAndState) {
    GameCore core(6, 6, 6);
    core.handle(make_intent(Action::Reveal, Pos{0, 0}));

    auto bombs = find_bomb_positions(core);
    ASSERT_FALSE(bombs.empty());
    const auto bomb = bombs.front();
    core.handle(make_intent(Action::Reveal, Pos{bomb.first, bomb.second}));
    ASSERT_EQ(core.state(), GameState::Lost);

    core.handle(make_intent(Action::NewGame));
    EXPECT_EQ(core.state(), GameState::InProgress);
    EXPECT_FALSE(core.cell(0, 0).revealed());
    EXPECT_EQ(core.board().at(0, 0), ".");
    EXPECT_EQ(core.hud().count, "6");
}
