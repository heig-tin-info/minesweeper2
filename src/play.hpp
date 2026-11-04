#pragma once

#include "game.hpp"
#include "ports.hpp"

// The application: the game loop. It only knows the ports, so the same loop
// drives a terminal, a window or a scripted test.
void play(Game& game, UserInterface& ui);
