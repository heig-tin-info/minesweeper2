#pragma once

// Everything an adapter can ask the application to do. Each command is its
// own type carrying exactly the data it needs: a `Reveal` without a position,
// or a `NewGame` with one, cannot even be written.
#include <variant>

#include "core/position.hpp"

namespace minesweeper {

struct Reveal {
    Pos pos;
    bool operator==(const Reveal&) const = default;
};

struct ToggleFlag {
    Pos pos;
    bool operator==(const ToggleFlag&) const = default;
};

struct NewGame {
    bool operator==(const NewGame&) const = default;
};

using Command = std::variant<Reveal, ToggleFlag, NewGame>;

}  // namespace minesweeper
