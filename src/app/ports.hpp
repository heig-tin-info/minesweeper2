#pragma once

// The application's ports. Adapters depend on these interfaces, never on the
// classes that implement them.
#include <chrono>

#include "app/commands.hpp"
#include "core/cell_view.hpp"
#include "core/game.hpp"

namespace minesweeper {

// Driving port, read side: everything a user interface may display.
class GameView {
public:
    virtual ~GameView() = default;
    virtual Size size() const = 0;
    virtual CellView cell(Pos p) const = 0;
    virtual Status status() const = 0;
    virtual int minesLeft() const = 0;
    virtual std::chrono::seconds elapsed() const = 0;
};

// Driving port, write side: the only way a user interface changes anything.
class CommandSink {
public:
    virtual ~CommandSink() = default;
    virtual void handle(const Command& command) = 0;
};

// Driven port: the application needs the time, not a particular clock.
class Clock {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    virtual ~Clock() = default;
    virtual TimePoint now() const = 0;
};

}  // namespace minesweeper
