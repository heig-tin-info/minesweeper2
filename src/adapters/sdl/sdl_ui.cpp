#include "adapters/sdl/sdl_ui.hpp"

#include <format>
#include <string>
#include <utility>

#include "core/overloaded.hpp"

namespace minesweeper::sdl {

namespace {

constexpr SDL_Color kBackground{34, 40, 49, 255};
constexpr SDL_Color kHud{45, 52, 54, 255};
constexpr SDL_Color kText{236, 240, 241, 255};
constexpr SDL_Color kGrass[2]{{164, 190, 123, 255}, {141, 169, 113, 255}};
constexpr SDL_Color kSand[2]{{238, 239, 213, 255}, {215, 212, 200, 255}};
constexpr SDL_Color kExploded{231, 76, 60, 255};
constexpr SDL_Color kFlag{192, 57, 43, 255};
constexpr SDL_Color kMine{44, 62, 80, 255};
constexpr SDL_Color kVeil{0, 0, 0, 150};
constexpr SDL_Color kDigit[9]{
    {}, {52, 152, 219, 255}, {39, 174, 96, 255}, {231, 76, 60, 255}, {142, 68, 173, 255},
    {211, 84, 0, 255}, {22, 160, 133, 255}, {44, 62, 80, 255}, {127, 140, 141, 255}};

constexpr Uint32 kFrameMs = 200;  // wake up regularly so the HUD clock ticks

bool isQuit(const SDL_Event& event) {
    return event.type == SDL_QUIT ||
           (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
}

}  // namespace

SdlUi::SdlUi(Size grid, Theme theme) : grid_(grid), theme_(std::move(theme)) {
    window_.reset(check(SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         grid_.cols * theme_.tile,
                                         grid_.rows * theme_.tile + theme_.hudHeight, SDL_WINDOW_SHOWN),
                        "SDL_CreateWindow"));
    renderer_.reset(check(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_PRESENTVSYNC),
                          "SDL_CreateRenderer"));
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);

    cellFont_.reset(check(TTF_OpenFont(theme_.font.c_str(), theme_.tile * 2 / 3), "TTF_OpenFont"));
    hudFont_.reset(check(TTF_OpenFont(theme_.font.c_str(), theme_.hudHeight / 2), "TTF_OpenFont"));

    for (int n = 1; n <= 8; ++n) glyphs_.digits[n] = text(cellFont_.get(), std::to_string(n), kDigit[n]);
    glyphs_.flag = text(cellFont_.get(), "F", kFlag);
    glyphs_.wrongFlag = text(cellFont_.get(), "X", kFlag);
}

void SdlUi::run(const GameView& view, CommandSink& commands) {
    for (;;) {
        draw(view);
        SDL_Event event;
        if (!SDL_WaitEventTimeout(&event, kFrameMs)) continue;
        do {
            if (isQuit(event)) return;
            if (const auto command = translate(event)) commands.handle(*command);
        } while (SDL_PollEvent(&event));
    }
}

std::optional<Command> SdlUi::translate(const SDL_Event& event) const {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_n || event.key.keysym.sym == SDLK_r) return NewGame{};
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (const auto p = cellAt(event.button.x, event.button.y)) {
                if (event.button.button == SDL_BUTTON_LEFT) return Reveal{*p};
                if (event.button.button == SDL_BUTTON_RIGHT) return ToggleFlag{*p};
            }
            break;
        default:
            break;
    }
    return std::nullopt;
}

// Pixels to grid coordinates: a concern of this adapter only.
std::optional<Pos> SdlUi::cellAt(int x, int y) const {
    if (y < theme_.hudHeight) return std::nullopt;
    const Pos p{(y - theme_.hudHeight) / theme_.tile, x / theme_.tile};
    if (!grid_.contains(p)) return std::nullopt;
    return p;
}

void SdlUi::draw(const GameView& view) {
    SDL_SetRenderDrawColor(renderer_.get(), kBackground.r, kBackground.g, kBackground.b, 255);
    SDL_RenderClear(renderer_.get());
    drawHud(view);
    for (int row = 0; row < grid_.rows; ++row)
        for (int col = 0; col < grid_.cols; ++col) drawCell({row, col}, view.cell({row, col}));
    drawBanner(view.status());
    SDL_RenderPresent(renderer_.get());
}

void SdlUi::drawHud(const GameView& view) {
    const int width = grid_.cols * theme_.tile;
    const int middle = theme_.hudHeight / 2;
    const auto seconds = view.elapsed().count();
    fill({0, 0, width, theme_.hudHeight}, kHud);
    blit(text(hudFont_.get(), std::format("Mines {}", view.minesLeft()), kText).get(), 12, middle, Align::Left);
    blit(text(hudFont_.get(), std::format("{:02}:{:02}", seconds / 60, seconds % 60), kText).get(),
         width - 12, middle, Align::Right);
}

void SdlUi::drawCell(Pos p, const CellView& cell) {
    const SDL_Rect tile{p.col * theme_.tile, theme_.hudHeight + p.row * theme_.tile, theme_.tile, theme_.tile};
    const int cx = tile.x + tile.w / 2, cy = tile.y + tile.h / 2;
    const int shade = (p.row + p.col) % 2;  // checkerboard

    std::visit(overloaded{
                   [&](cell::Hidden) { fill(tile, kGrass[shade]); },
                   [&](cell::Flagged) {
                       fill(tile, kGrass[shade]);
                       blit(glyphs_.flag.get(), cx, cy);
                   },
                   [&](cell::Revealed r) {
                       fill(tile, kSand[shade]);
                       if (r.adjacentMines > 0) blit(glyphs_.digits[r.adjacentMines].get(), cx, cy);
                   },
                   [&](cell::Mine m) {
                       fill(tile, m.exploded ? kExploded : kSand[shade]);
                       drawMine(cx, cy);
                   },
                   [&](cell::WrongFlag) {
                       fill(tile, kSand[shade]);
                       blit(glyphs_.wrongFlag.get(), cx, cy);
                   },
               },
               cell);
}

// A body and two spikes: the bundled font has no glyph that looks like a mine.
void SdlUi::drawMine(int cx, int cy) {
    const int r = theme_.tile / 5;
    fill({cx - r, cy - r, 2 * r, 2 * r}, kMine);
    fill({cx - 2 * r, cy - 1, 4 * r, 3}, kMine);
    fill({cx - 1, cy - 2 * r, 3, 4 * r}, kMine);
}

void SdlUi::drawBanner(Status status) {
    if (!isOver(status)) return;
    const int width = grid_.cols * theme_.tile;
    const int height = grid_.rows * theme_.tile;
    fill({0, theme_.hudHeight, width, height}, kVeil);
    const int middle = theme_.hudHeight + height / 2;
    blit(text(hudFont_.get(), status == Status::Won ? "You win!" : "Game over", kText).get(), width / 2,
         middle - theme_.hudHeight / 2);
    blit(text(cellFont_.get(), "Press N for a new game", kText).get(), width / 2, middle + theme_.hudHeight / 2);
}

TexturePtr SdlUi::text(TTF_Font* font, const std::string& s, SDL_Color colour) const {
    const SurfacePtr surface{check(TTF_RenderUTF8_Blended(font, s.c_str(), colour), "TTF_RenderUTF8_Blended")};
    return TexturePtr{check(SDL_CreateTextureFromSurface(renderer_.get(), surface.get()),
                            "SDL_CreateTextureFromSurface")};
}

void SdlUi::fill(const SDL_Rect& rect, SDL_Color colour) {
    SDL_SetRenderDrawColor(renderer_.get(), colour.r, colour.g, colour.b, colour.a);
    SDL_RenderFillRect(renderer_.get(), &rect);
}

// Draws a texture vertically centred on y, and anchored horizontally on x.
void SdlUi::blit(SDL_Texture* texture, int x, int y, Align align) {
    int w = 0, h = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    if (align == Align::Center) x -= w / 2;
    if (align == Align::Right) x -= w;
    const SDL_Rect target{x, y - h / 2, w, h};
    SDL_RenderCopy(renderer_.get(), texture, nullptr, &target);
}

}  // namespace minesweeper::sdl
