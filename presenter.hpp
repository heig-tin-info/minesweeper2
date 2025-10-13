#pragma once

#include "ports.hpp"

class GameCore;  // fwd

class BoardPresenter : public IBoardReadable {
   public:
    explicit BoardPresenter(const GameCore& g) : game(g) {}
    int width() const override;
    int height() const override;
    CellToken at(int x, int y) const override;

   private:
    const GameCore& game;
};
