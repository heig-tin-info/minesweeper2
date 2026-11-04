/**
 * @file main.cpp
 * Composition root: the only place that knows every concrete class.
 *
 * It chooses the adapters, plugs them into the ports and starts the game.
 * Usage: `minesweeper [mines [rows cols [seed]]]`
 */
#include <exception>
#include <iostream>
#include <string>

#include "console.hpp"
#include "game.hpp"
#include "play.hpp"
#include "random_layout.hpp"

int main(int argc, char* argv[]) {
    try {
        const int mines = argc > 1 ? std::stoi(argv[1]) : 10;
        const int rows = argc > 3 ? std::stoi(argv[2]) : 9;
        const int cols = argc > 3 ? std::stoi(argv[3]) : 9;

        RandomLayout layout = argc > 4 ? RandomLayout{static_cast<unsigned>(std::stoul(argv[4]))}
                                       : RandomLayout{};
        Game game{rows, cols, mines, layout};
        Console console{std::cin, std::cout};
        play(game, console);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\nusage: " << argv[0]
                  << " [mines [rows cols [seed]]]\n";
        return 1;
    }
}
