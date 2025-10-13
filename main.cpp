#include "minesweeper.hpp"
#include "ui-console.hpp"

int main() {
    GameCore game(8, 8, 5);
    ConsoleUI ui;

    ui.render(game.board(), game.hud());

    while (game.state() == GameState::InProgress) {
        auto intent = ui.wait_intent();
        if (intent.action == Action::Quit) break;
        game.handle(intent);
        ui.render(game.board(), game.hud());
    }

    switch (game.state()) {
      case GameState::InProgress: std::cout << "Game aborted.\n"; break;
      case GameState::Won:        std::cout << "You win!\n"; break;
      case GameState::Lost:       std::cout << "Game over!\n"; break;
    }
}
