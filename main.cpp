#include <iostream>
#include <string>

#include "minesweeper.hpp"

#ifdef SDL
#include "ui-sdl.hpp"
#else
#include "ui-console.hpp"
#endif

namespace {
struct GameConfig {
    int rows;
    int cols;
    int bombs;
};

constexpr GameConfig kDefaultConfig{10, 10, 10};

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
