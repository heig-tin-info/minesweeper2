#pragma once
#include <optional>
#include <string>

#include "types.hpp"

struct IBoardReadable {
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual CellToken at(int x, int y) const = 0;
    virtual ~IBoardReadable() = default;
};

struct UserIntent {
    Action action;
    std::optional<Pos> where;
};

struct IIntentSink {
    virtual void handle(const UserIntent& i) = 0;
    virtual ~IIntentSink() = default;
};

struct HudSnapshot {
    std::string time_mmss;
    std::string count;
};

struct IOutSnapshot {
    virtual const IBoardReadable& board() const = 0;
    virtual HudSnapshot hud() const = 0;
    virtual GameState state() const = 0;
    virtual ~IOutSnapshot() = 0;
};
inline IOutSnapshot::~IOutSnapshot() = default;
