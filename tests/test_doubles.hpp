#pragma once

// Test doubles: small stand-ins for the ports, so that every test controls
// exactly where the mines are and what time it is.
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

#include "app/ports.hpp"
#include "core/mine_layout.hpp"

namespace minesweeper::testing {

// Stub (returns the mines it was given) and spy (records how it was called).
class FixedMineLayout final : public MineLayout {
public:
    explicit FixedMineLayout(std::vector<Pos> mines) : mines_(std::move(mines)) {}

    std::vector<Pos> place(Size, int, Pos firstClick) override {
        ++calls;
        lastFirstClick = firstClick;
        return mines_;
    }

    int calls = 0;
    std::optional<Pos> lastFirstClick;

private:
    std::vector<Pos> mines_;
};

// Fake: a working clock whose time only moves when the test says so.
class FakeClock final : public Clock {
public:
    TimePoint now() const override { return now_; }
    void advance(std::chrono::seconds d) { now_ += d; }

private:
    TimePoint now_{};
};

// Spy on the write port.
class RecordingSink final : public CommandSink {
public:
    void handle(const Command& command) override { received.push_back(command); }
    std::vector<Command> received;
};

}  // namespace minesweeper::testing
