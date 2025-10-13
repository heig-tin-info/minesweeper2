#pragma once
#include <string>

struct Pos {
    int x;
    int y;
};

enum class Action { Reveal, ToggleFlag, NewGame, Quit, Redraw };

enum class GameState { InProgress, Won, Lost };

using CellToken = std::string;
