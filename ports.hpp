#pragma once

// Ports that define the seams between the game core and any presentation layer
// or input source. By funnelling communication through these simple interfaces,
// we can swap front-ends without touching the application logic.
#include <optional>
#include <string>

#include "types.hpp"

// Read-only view on top of the board. UIs rely on this to render tiles without
// the risk of mutating the underlying game state.
struct IBoardReadable {
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual CellToken at(int x, int y) const = 0;
    virtual ~IBoardReadable() = default;
};

// Describes a user command (mouse click, key press, etc.). The optional
// position lets intents like "restart" exist without coordinates.
struct UserIntent {
    Action action;
    std::optional<Pos> where;
};

// Input port implemented by the game core. Every UI sends translated intents
// through this interface.
struct IIntentSink {
    virtual void handle(const UserIntent& i) = 0;
    virtual ~IIntentSink() = default;
};

// Lightweight struct returned to UIs that need HUD information such as elapsed
// time and remaining bombs.
struct HudSnapshot {
    std::string time_mmss;
    std::string count;
};

// Output port that aggregates the read-only board view, HUD data, and current
// finite state of the game.
struct IOutSnapshot {
    virtual const IBoardReadable& board() const = 0;
    virtual HudSnapshot hud() const = 0;
    virtual GameState state() const = 0;
    virtual ~IOutSnapshot() = 0;
};
inline IOutSnapshot::~IOutSnapshot() = default;
