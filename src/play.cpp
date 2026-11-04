#include "play.hpp"

void play(Game& game, UserInterface& ui) {
    ui.show(game);
    while (game.status() == Status::Playing) {
        const Command cmd = ui.ask();
        if (cmd.type == Command::Quit) return;
        if (!game.contains(cmd.pos)) continue;  // the UI may not know the size

        if (cmd.type == Command::Reveal) game.reveal(cmd.pos);
        if (cmd.type == Command::Flag) game.toggleFlag(cmd.pos);
        ui.show(game);
    }
}
