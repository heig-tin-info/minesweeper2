// Composition root: the only place that knows every concrete class. It
// chooses the adapters, plugs them into the ports and starts the game.
//
//   minesweeper [rows cols mines [seed]]
#include <ctime>
#include <exception>
#include <iostream>
#include <string>

#include "console.hpp"
#include "game.hpp"
#include "play.hpp"
#include "random_layout.hpp"

int main(int argc, char* argv[]) {
    try {
        const int rows = argc > 3 ? std::stoi(argv[1]) : 9;
        const int cols = argc > 3 ? std::stoi(argv[2]) : 9;
        const int mines = argc > 3 ? std::stoi(argv[3]) : 10;
        const unsigned seed = argc > 4 ? std::stoul(argv[4]) : std::time(nullptr);

        RandomLayout layout{seed};
        Game game{rows, cols, mines, layout};
        Console console{std::cin, std::cout};
        play(game, console);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\nusage: " << argv[0]
                  << " [rows cols mines [seed]]\n";
        return 1;
    }
}
