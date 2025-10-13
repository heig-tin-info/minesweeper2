#include <iostream>

#include "minesweeper.hpp"

#ifdef SDL
#include "ui-sdl.hpp"
#else
#include "ui-console.hpp"
#endif

namespace {
#ifdef SDL
constexpr int kRows = 16;
constexpr int kCols = 16;
constexpr int kBombs = 20;
#else
constexpr int kRows = 8;
constexpr int kCols = 8;
constexpr int kBombs = 5;
#endif
}  // namespace

int main() {
    GameCore game(kRows, kCols, kBombs);

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
