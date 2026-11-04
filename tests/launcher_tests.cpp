#include <gtest/gtest.h>

#include <vector>

#include "launcher/launch.hpp"

using namespace minesweeper;

namespace {

LaunchOptions parse(std::vector<const char*> args) { return parseArguments(args); }

}  // namespace

TEST(Arguments, DefaultsToABeginnerGame) {
    const auto options = parse({});
    EXPECT_EQ(options.game.size, (Size{9, 9}));
    EXPECT_EQ(options.game.mines, 10);
    EXPECT_FALSE(options.seed);
}

TEST(Arguments, ReadsSizeMinesAndSeed) {
    const auto options = parse({"16", "30", "99", "--seed", "7"});
    EXPECT_EQ(options.game.size, (Size{16, 30}));
    EXPECT_EQ(options.game.mines, 99);
    EXPECT_EQ(options.seed, 7u);
}

TEST(Arguments, RejectsMalformedInput) {
    EXPECT_THROW(parse({"9", "9"}), std::invalid_argument);
    EXPECT_THROW(parse({"9", "x", "10"}), std::invalid_argument);
    EXPECT_THROW(parse({"--seed"}), std::invalid_argument);
}

TEST(Arguments, LeavesGameRulesToTheDomain) {
    EXPECT_NO_THROW(parse({"2", "2", "50"})) << "the parser parses; the Game validates";
}
