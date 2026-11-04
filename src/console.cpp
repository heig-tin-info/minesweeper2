#include "console.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

std::optional<Command> parse(const std::string& line) {
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

// One character per cell. Mines are shown only once the game is lost.
static char symbol(const Cell& c, Status status) {
    if (c.mine && (c.revealed || status == Status::Lost)) return '*';
    if (c.flagged) return 'F';
    if (!c.revealed) return '#';
    return c.adjacent == 0 ? '.' : static_cast<char>('0' + c.adjacent);
}

void Console::show(const Game& game) {
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

Command Console::ask() {
    std::string line;
    for (;;) {
        out_ << "> " << std::flush;
        if (!std::getline(in_, line)) return {Command::Quit};  // end of input
        if (auto cmd = parse(line)) return *cmd;
        out_ << "Commands: r <row> <col> (reveal), f <row> <col> (flag), q (quit)\n";
    }
}
