/**
 * @file play.hpp
 * The application: the game loop that connects a user interface to a game.
 */
#pragma once

#include "game.hpp"
#include "ports.hpp"

/**
 * Runs a game until it is won, lost or the player quits.
 *
 * It only knows the ports, so the same loop drives a terminal, a window or
 * a scripted test.
 */
inline void play(Game& game, UserInterface& ui) {
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
