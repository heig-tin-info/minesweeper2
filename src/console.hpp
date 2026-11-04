#pragma once

#include <iosfwd>
#include <optional>
#include <string>

#include "game.hpp"
#include "ports.hpp"

// Turns one line typed by the player into a command:
//   "r 2 3" reveals row 2, column 3;  "f 2 3" flags it;  "q" quits.
// Returns nothing if the line cannot be understood.
std::optional<Command> parse(const std::string& line);

// Adapter for the UserInterface port: a raw text grid and a prompt.
// It takes streams rather than std::cin/std::cout, so tests can drive it.
class Console : public UserInterface {
public:
    Console(std::istream& in, std::ostream& out) : in_(in), out_(out) {}
    void show(const Game& game) override;
    Command ask() override;

private:
    std::istream& in_;
    std::ostream& out_;
};
