#include <gtest/gtest.h>

#include <sstream>

#include "adapters/console/console_ui.hpp"
#include "adapters/console/notation.hpp"
#include "adapters/console/renderer.hpp"
#include "app/session.hpp"
#include "test_doubles.hpp"

using namespace minesweeper;
using namespace minesweeper::console;
using minesweeper::testing::FakeClock;
using minesweeper::testing::FixedMineLayout;
using minesweeper::testing::RecordingSink;

TEST(Notation, ColumnsAreLettersRowsCountFromTheBottom) {
    constexpr Size size{8, 30};
    EXPECT_EQ(columnLabel(0), "A");
    EXPECT_EQ(columnLabel(25), "Z");
    EXPECT_EQ(columnLabel(26), "AA");
    EXPECT_EQ(parsePos("A1", size), (Pos{7, 0}));
    EXPECT_EQ(parsePos("b8", size), (Pos{0, 1}));
    EXPECT_EQ(parsePos("AD3", size), (Pos{5, 29}));
}

TEST(Notation, RejectsMalformedOrOutsidePositions) {
    constexpr Size size{8, 8};
    EXPECT_FALSE(parsePos("I1", size));
    EXPECT_FALSE(parsePos("A9", size));
    EXPECT_FALSE(parsePos("A0", size));
    EXPECT_FALSE(parsePos("3A", size));
    EXPECT_FALSE(parsePos("A", size));
    EXPECT_FALSE(parsePos("A1x", size));
}

TEST(Notation, ParsesEveryKindOfInput) {
    constexpr Size size{8, 8};
    EXPECT_EQ(parseInput(" c3 ", size), (Input{Command{Reveal{{5, 2}}}}));
    EXPECT_EQ(parseInput("C3?", size), (Input{Command{ToggleFlag{{5, 2}}}}));
    EXPECT_EQ(parseInput("N", size), Input{Command{NewGame{}}});
    EXPECT_EQ(parseInput("quit", size), Input{Quit{}});
    EXPECT_FALSE(parseInput("hello", size));
}

namespace {

struct ConsoleTest : ::testing::Test {
    FixedMineLayout layout{{{0, 0}}};
    FakeClock clock;
    Session session{{.size = {3, 3}, .mines = 1}, layout, clock};
};

}  // namespace

TEST_F(ConsoleTest, RendersTheGridFromTheReadPort) {
    session.handle(Reveal{{1, 1}});
    std::ostringstream out;
    render(out, session, Style{});
    EXPECT_EQ(out.str(),
              "\n"
              "  Mines: 1   Time: 00:00\n"
              "  ┌───┬───┬───┐\n"
              "3 │ . │ . │ . │\n"
              "  ├───┼───┼───┤\n"
              "2 │ . │ 1 │ . │\n"
              "  ├───┼───┼───┤\n"
              "1 │ . │ . │ . │\n"
              "  └───┴───┴───┘\n"
              "    A   B   C   \n"
              "\n"
              "B3: reveal, B3?: flag, n: new game, q: quit\n");
}

TEST_F(ConsoleTest, TranslatesTypedLinesIntoCommands) {
    std::istringstream in{"B2\nbogus\nA3?\nn\nq\nC1\n"};
    std::ostringstream out;
    RecordingSink sink;
    ConsoleUi{in, out, Style{}}.run(session, sink);

    const std::vector<Command> expected{Reveal{{1, 1}}, ToggleFlag{{0, 0}}, NewGame{}};
    EXPECT_EQ(sink.received, expected) << "stops at q, skips the bogus line";
    EXPECT_NE(out.str().find("Unknown command"), std::string::npos);
}

TEST_F(ConsoleTest, PlaysAWholeGame) {
    std::istringstream in{"C1\n"};
    std::ostringstream out;
    ConsoleUi{in, out, Style{}}.run(session, session);
    EXPECT_EQ(session.status(), Status::Won);
    EXPECT_NE(out.str().find("You win!"), std::string::npos);
}
