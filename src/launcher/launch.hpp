#pragma once

// Composition root shared by every executable: it parses the command line,
// builds the object graph, and hands the session to a user interface.
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>

#include "app/session.hpp"

namespace minesweeper {

struct LaunchOptions {
    GameConfig game{.size = {9, 9}, .mines = 10};
    std::optional<std::uint64_t> seed;
    bool help = false;
};

// Parses `[rows cols mines] [--seed N] [--help]` (program name excluded).
// Throws std::invalid_argument on malformed input. Whether the numbers make a
// playable game is the domain's decision, not the parser's.
LaunchOptions parseArguments(std::span<const char* const> args);

std::string usage(const std::string& program);

// Builds the session and runs `play` on it. Returns the process exit code.
int launch(int argc, const char* const argv[], const std::function<void(Session&)>& play);

}  // namespace minesweeper
