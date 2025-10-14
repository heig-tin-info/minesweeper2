#pragma once

// Thin adapter that turns `GameCore` into the `IBoardReadable` interface
// expected by both the console and SDL front-ends. The goal is to keep the core
// decoupled from presentation details while still offering a pleasant API to
// rendering code.
#include "ports.hpp"

class GameCore;  // fwd

class BoardPresenter : public IBoardReadable {
   public:
    // Bind the presenter to a specific game instance. Only const access is
    // provided so UIs cannot mutate state through this pathway.
    explicit BoardPresenter(const GameCore& g) : game(g) {}
    int width() const override;
    int height() const override;
    CellToken at(int x, int y) const override;

   private:
    // Reference back into the owning `GameCore`; lifetime is managed by the
    // core, not by the presenter.
    const GameCore& game;
};
