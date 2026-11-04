/**
 * @file console.hpp
 * Adapter for the UserInterface port: a raw text grid and a prompt.
 */
#pragma once

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "game.hpp"
#include "ports.hpp"

/**
 * Turns one line typed by the player into a command:
 * `r 2 3` reveals row 2, column 3; `f 2 3` flags it; `q` quits.
 * @return the command, or nothing if the line cannot be understood.
 */
inline std::optional<Command> parse(const std::string& line) {
    std::istringstream words(line);
    char letter = 0;
    words >> letter;
    if (letter == 'q') return Command{Command::Quit};

    Pos p{};
    if (!(words >> p.row >> p.col)) return std::nullopt;
    if (letter == 'r') return Command{Command::Reveal, p};
    if (letter == 'f') return Command{Command::Flag, p};
    return std::nullopt;
}

/**
 * Plays in a text terminal.
 *
 * It takes streams rather than std::cin and std::cout, so tests can drive
 * it with strings.
 */
class Console : public UserInterface {
public:
    Console(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

    /**
     * Prints the grid with row and column numbers, then the result if the
     * game is over.
     */
    void show(const Game& game) override {
        out_ << "\n   ";
        for (int c = 0; c < game.cols(); ++c) out_ << std::setw(2) << c;
        out_ << '\n';
        for (int r = 0; r < game.rows(); ++r) {
            out_ << std::setw(3) << r;
            for (int c = 0; c < game.cols(); ++c)
                out_ << std::setw(2) << symbol(game.at({r, c}), game.status());
            out_ << '\n';
        }
        if (game.status() == Status::Won) out_ << "You won!\n";
        if (game.status() == Status::Lost) out_ << "Boom! You lost.\n";
    }

    /**
     * Prompts until the player types a valid command. The end of the input
     * (Ctrl+D) counts as quitting.
     */
    Command ask() override {
        std::string line;
        for (;;) {
            out_ << "> " << std::flush;
            if (!std::getline(in_, line)) return {Command::Quit};
            if (auto cmd = parse(line)) return *cmd;
            out_ << "Commands: r <row> <col> (reveal), f <row> <col> (flag), q (quit)\n";
        }
    }

private:
    /**
     * One character per cell. Mines are shown only once the game is lost.
     */
    static char symbol(const Cell& c, Status status) {
        if (c.mine && (c.revealed || status == Status::Lost)) return '*';
        if (c.flagged) return 'F';
        if (!c.revealed) return '#';
        return c.adjacent == 0 ? '.' : static_cast<char>('0' + c.adjacent);
    }

    std::istream& in_;
    std::ostream& out_;
};
