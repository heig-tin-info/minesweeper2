#pragma once

// Shared, UI-agnostic types that allow both front-ends and the core to speak a
// common language without introducing heavy dependencies.
#include <string>

// Logical board position expressed in UI-friendly coordinates (x -> column,
// y -> row). UIs produce these from user interactions.
struct Pos {
    int x;
    int y;
};

// All high-level actions a user can trigger. These map directly to
// `UserIntent::action`.
enum class Action { Reveal, ToggleFlag, NewGame, Quit, Redraw };

// Coarse state of the game used to drive HUD messaging and overlays.
enum class GameState { InProgress, Won, Lost };

// Single-character token exposed by the presenter and consumed by UIs.
using CellToken = std::string;
