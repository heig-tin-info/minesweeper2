#include <iostream>

#include "minesweeper.hpp"
#include "ui-sdl.hpp"

int main() {
    GameCore game(8, 8, 10);
    SDLUI ui(game.width(), game.height());

    if (!ui.ready()) {
        std::cerr << "[x] SDL UI failed to initialise.\n";
        return 1;
    }

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

        SDL_Delay(16);  // ~60 FPS cap
    }

    return 0;
}
