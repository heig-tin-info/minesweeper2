#pragma once

#include <array>
#include <optional>
#include <string>

#include "adapters/sdl/resources.hpp"
#include "app/ports.hpp"

#ifndef MINESWEEPER_FONT  // set by the build to the font shipped in assets/
#define MINESWEEPER_FONT "assets/collegiate.ttf"
#endif

namespace minesweeper::sdl {

struct Theme {
    int tile = 32;      // px
    int hudHeight = 48; // px
    std::string font = MINESWEEPER_FONT;
};

// Graphical adapter. Unlike the console, SDL owns an event loop: the adapter
// waits for events, translates the relevant ones into commands, and redraws
// the frame from the read port. It holds no game state at all.
class SdlUi {
public:
    // Throws std::runtime_error if a window, renderer or font is unavailable.
    SdlUi(Size grid, Theme theme = {});

    // Returns when the window is closed or Escape is pressed.
    void run(const GameView& view, CommandSink& commands);

private:
    // Textures rendered once and reused for every frame.
    struct Glyphs {
        std::array<TexturePtr, 9> digits;  // [0] unused
        TexturePtr flag, wrongFlag;
    };

    std::optional<Command> translate(const SDL_Event& event) const;
    std::optional<Pos> cellAt(int x, int y) const;

    void draw(const GameView& view);
    void drawHud(const GameView& view);
    void drawCell(Pos p, const CellView& cell);
    void drawMine(int cx, int cy);
    void drawBanner(Status status);

    TexturePtr text(TTF_Font* font, const std::string& s, SDL_Color colour) const;
    void fill(const SDL_Rect& rect, SDL_Color colour);
    enum class Align { Left, Center, Right };
    void blit(SDL_Texture* texture, int x, int y, Align align = Align::Center);

    // Declaration order is construction order, and the reverse of destruction
    // order: the library must outlive every resource created with it.
    Library library_;
    Size grid_;
    Theme theme_;
    WindowPtr window_;
    RendererPtr renderer_;
    FontPtr cellFont_;
    FontPtr hudFont_;
    Glyphs glyphs_;
};

}  // namespace minesweeper::sdl
