#include <gtest/gtest.h>

#include "app/session.hpp"
#include "test_doubles.hpp"

using namespace minesweeper;
using namespace std::chrono_literals;
using minesweeper::testing::FakeClock;
using minesweeper::testing::FixedMineLayout;

TEST(Stopwatch, MeasuresFromStartToStop) {
    Stopwatch watch;
    const Clock::TimePoint t0{};
    EXPECT_EQ(watch.elapsed(t0 + 5s), 0s) << "not started";
    watch.start(t0);
    EXPECT_EQ(watch.elapsed(t0 + 5s), 5s);
    watch.stop(t0 + 7s);
    EXPECT_EQ(watch.elapsed(t0 + 60s), 7s);
}

namespace {

// Two mines in the top row of a 3x3 grid:  * . *
struct SessionTest : ::testing::Test {
    FixedMineLayout layout{{{0, 0}, {0, 2}}};
    FakeClock clock;
    Session session{{.size = {3, 3}, .mines = 2}, layout, clock};
};

}  // namespace

TEST_F(SessionTest, ForwardsCommandsToTheGame) {
    session.handle(ToggleFlag{{0, 0}});
    EXPECT_EQ(session.cell({0, 0}), CellView{cell::Flagged{}});
    EXPECT_EQ(session.minesLeft(), 1);
}

TEST_F(SessionTest, ClockStartsOnTheFirstReveal) {
    clock.advance(30s);
    EXPECT_EQ(session.elapsed(), 0s);
    session.handle(ToggleFlag{{0, 0}});
    clock.advance(10s);
    EXPECT_EQ(session.elapsed(), 0s) << "flagging does not start the game";

    session.handle(Reveal{{2, 1}});
    clock.advance(4s);
    EXPECT_EQ(session.elapsed(), 4s);
}

TEST_F(SessionTest, ClockStopsWhenTheGameEnds) {
    session.handle(Reveal{{2, 1}});
    clock.advance(3s);
    session.handle(Reveal{{0, 0}});
    ASSERT_EQ(session.status(), Status::Lost);
    clock.advance(100s);
    EXPECT_EQ(session.elapsed(), 3s);
}

TEST_F(SessionTest, NewGameStartsAFreshGameAndClock) {
    session.handle(Reveal{{2, 1}});
    clock.advance(3s);
    session.handle(NewGame{});
    EXPECT_EQ(session.status(), Status::Ready);
    EXPECT_EQ(session.cell({2, 1}), CellView{cell::Hidden{}});
    EXPECT_EQ(session.elapsed(), 0s);
}

TEST(Session, RejectsAnImpossibleConfiguration) {
    FixedMineLayout layout{{}};
    FakeClock clock;
    EXPECT_THROW((Session{{.size = {2, 2}, .mines = 4}, layout, clock}), std::invalid_argument);
}
