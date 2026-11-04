#include "app/session.hpp"

#include <variant>

#include "core/overloaded.hpp"

namespace minesweeper {

Session::Session(GameConfig config, MineLayout& layout, const Clock& clock)
    : config_(config),
      layout_(layout),
      clock_(clock),
      game_(config.size, config.mines, layout) {}

void Session::handle(const Command& command) {
    const Status before = game_.status();
    std::visit(overloaded{
                   [&](const Reveal& c) { game_.reveal(c.pos); },
                   [&](const ToggleFlag& c) { game_.toggleFlag(c.pos); },
                   [&](const NewGame&) {
                       game_ = Game{config_.size, config_.mines, layout_};
                       stopwatch_ = {};
                   },
               },
               command);
    updateStopwatch(before, game_.status());
}

// The clock runs from the first reveal to the end of the game.
void Session::updateStopwatch(Status before, Status after) {
    const auto now = clock_.now();
    if (before == Status::Ready && after != Status::Ready) stopwatch_.start(now);
    if (!isOver(before) && isOver(after)) stopwatch_.stop(now);
}

}  // namespace minesweeper
