/**
 * @file ports.hpp
 * The ports: the only interfaces between the game and the outside world.
 *
 * The game depends on these abstractions, never on a terminal, a window or
 * a particular random generator. Adapters implement them.
 */
#pragma once

#include <vector>

/**
 * A cell position in the grid, counted from 0 at the top-left corner.
 */
struct Pos {
    int row;
    int col;
    bool operator==(const Pos&) const = default;
};

/**
 * What the player asks for.
 *
 * A plain struct: easy to read, easy to extend (add a type, handle it in
 * play()).
 */
struct Command {
    enum Type { Reveal, Flag, Quit };
    Type type;
    Pos pos{0, 0};  // unused for Quit
};

/**
 * Driven port: decides where the mines go.
 *
 * The game calls it once, on the first reveal, so that the first click is
 * never a mine.
 */
class MineLayout {
public:
    virtual ~MineLayout() = default;

    /**
     * Chooses the mine positions.
     * @return `count` distinct positions inside the grid, none equal to
     *         `firstClick`.
     */
    virtual std::vector<Pos> place(int rows, int cols, int count, Pos firstClick) = 0;
};

class Game;

/**
 * Driving port: whatever lets a player see the game and act on it
 * (a terminal today, a window or a network client tomorrow).
 */
class UserInterface {
public:
    virtual ~UserInterface() = default;

    /**
     * Displays the current state of the game.
     */
    virtual void show(const Game& game) = 0;

    /**
     * Waits for the player's next command.
     */
    virtual Command ask() = 0;
};
