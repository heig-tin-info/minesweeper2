// Entry point shared by both the console and SDL front-ends. The file keeps
// the interface-specific logic behind preprocessor switches so that students
// can study a single `main` while understanding how dependency inversion lets
// us swap the presentation layer via a compile flag (`-DSDL`). The code also
// demonstrates minimal argument parsing and delegates all gameplay to
// `GameCore`, which embodies the application rules.
#include <iostream>
#include <string>

#include "minesweeper.hpp"

#ifdef SDL
#include "ui-sdl.hpp"
#else
#include "ui-console.hpp"
#endif

namespace {
// Collects the board configuration requested by the user. The default values
// mirror a classic beginner Minesweeper grid, but exposing the numbers here
// makes it obvious how the dimensions flow into the game domain object.
struct GameConfig {
    int rows;
    int cols;
    int bombs;
};

constexpr GameConfig kDefaultConfig{10, 10, 10};

// Parses optional CLI overrides for the board configuration. We deliberately
// keep the signature simple (no std::span) so the transformation from raw C
// arguments to validated domain values is crystal clear for students. The
// helper ensures that we never create an impossible board (e.g., too many
// bombs) before we hand control to the core game logic.
bool parseArgs(int argc, char* argv[], GameConfig& config) {
    config = kDefaultConfig;
    if (argc == 1) return true;
    if (argc != 4) return false;

    try {
        config.rows = std::stoi(argv[1]);
        config.cols = std::stoi(argv[2]);
        config.bombs = std::stoi(argv[3]);
    } catch (const std::exception&) {
        return false;
    }

    if (config.rows <= 0 || config.cols <= 0 || config.bombs <= 0) return false;
    if (static_cast<long long>(config.rows) * config.cols <= config.bombs)
        return false;
    return true;
}
}  // namespace

// Bootstraps the game: parse arguments, build the core model, instantiate the
// presentation layer, and hand off execution. Everything after configuration
// happens inside the UI loop so that we keep `main` focused on assembly of
// collaborating objects rather than on gameplay itself.
int main(int argc, char* argv[]) {
    GameConfig config{};
    if (!parseArgs(argc, argv, config)) {
        std::cerr << "Usage: " << argv[0] << " [rows cols bombs]\n"
                  << "Example: " << argv[0] << " 16 16 20\n";
        return 1;
    }

    GameCore game(config.rows, config.cols, config.bombs);

#ifdef SDL
    SDLUI ui(game.width(), game.height());
#else
    ConsoleUI ui;
#endif

    if (!ui.ready()) {
        std::cerr << "[x] UI failed to initialise.\n";
        return 1;
    }

#ifdef SDL
    bool running = true;

    while (running) {
        while (auto intent = ui.pollIntent()) {
            if (intent->action == Action::Quit) {
                running = false;
                break;
            }
            game.handle(*intent);
        }

        ui.render(game.board(), game.hud(), game.state());

        if (!running) break;
    }
#else
    ui.render(game.board(), game.hud());

    while (game.state() == GameState::InProgress) {
        auto intent = ui.wait_intent();
        if (intent.action == Action::Quit) break;
        game.handle(intent);
        ui.render(game.board(), game.hud());
    }

    switch (game.state()) {
        case GameState::InProgress:
            std::cout << "Game aborted.\n";
            break;
        case GameState::Won:
            std::cout << "You win!\n";
            break;
        case GameState::Lost:
            std::cout << "Game over!\n";
            break;
    }
#endif

    return 0;
}
