#pragma once

#include "app/ports.hpp"
#include "app/stopwatch.hpp"
#include "core/game.hpp"

namespace minesweeper {

struct GameConfig {
    Size size;
    int mines;
};

// Application service: what one player does in front of the program. It
// orchestrates the domain (one `Game` at a time) with the outside world (the
// clock), and exposes both driving ports to the adapters. It holds no game
// rule of its own.
class Session final : public GameView, public CommandSink {
public:
    // Throws std::invalid_argument if `config` describes an impossible game.
    Session(GameConfig config, MineLayout& layout, const Clock& clock);

    // A session has an identity; copying it would silently fork the game.
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void handle(const Command& command) override;

    Size size() const override { return game_.size(); }
    CellView cell(Pos p) const override { return game_.cell(p); }
    Status status() const override { return game_.status(); }
    int minesLeft() const override { return game_.minesLeft(); }
    std::chrono::seconds elapsed() const override { return stopwatch_.elapsed(clock_.now()); }

private:
    void updateStopwatch(Status before, Status after);

    GameConfig config_;
    MineLayout& layout_;
    const Clock& clock_;
    Game game_;
    Stopwatch stopwatch_;
};

}  // namespace minesweeper
