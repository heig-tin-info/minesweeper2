#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cmath>
#include <optional>
#include <string>
#include <vector>
#include <utility>

#include "ports.hpp"

class SDLUI {
   public:
    struct Config {
        int tile_px = 32;
        int navbar_px = 56;
        std::string font_path = "assets/collegiate.ttf";
        int font_navbar_px = 26;
        int font_cell_px = 22;
    };

    SDLUI(int cols, int rows);
    SDLUI(int cols, int rows, Config cfg);
    ~SDLUI();

    SDLUI(const SDLUI&) = delete;
    SDLUI& operator=(const SDLUI&) = delete;

    bool ready() const { return ready_; }

    std::optional<UserIntent> pollIntent();
    void render(const IBoardReadable& board,
                const HudSnapshot& hud,
                GameState state);

   private:
    enum class CellKind { Hidden, Flagged, Empty, Number, Bomb };

    int cols_;
    int rows_;
    Config cfg_;

    bool ready_{false};
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    TTF_Font* fontNavbar_{nullptr};
    TTF_Font* fontCell_{nullptr};

    int tileSize_{32};
    int navbarHeight_{56};
    int windowWidth_{0};
    int windowHeight_{0};

    SDL_Color background_{34, 40, 49, 255};
    SDL_Color boardBg_{57, 62, 70, 255};
    SDL_Color navbarColor_{45, 52, 54, 255};
    SDL_Color hiddenLight_{164, 190, 123, 255};
    SDL_Color hiddenDark_{141, 169, 113, 255};
    SDL_Color revealedLight_{238, 239, 213, 255};
    SDL_Color revealedDark_{215, 212, 200, 255};
    SDL_Color border_{52, 73, 94, 255};
    SDL_Color flagColor_{231, 76, 60, 255};
    SDL_Color textLight_{236, 240, 241, 255};

    std::vector<SDL_Color> numberColors_{
        SDL_Color{52, 152, 219, 255}, SDL_Color{46, 204, 113, 255},
        SDL_Color{231, 76, 60, 255},  SDL_Color{155, 89, 182, 255},
        SDL_Color{243, 156, 18, 255}, SDL_Color{26, 188, 156, 255},
        SDL_Color{52, 73, 94, 255},   SDL_Color{127, 140, 141, 255}};

    void computeGeometry();
    bool initSDL();
    bool loadFonts();
    void teardown();

    void clearWithColor(SDL_Color color);
    void fillRect(SDL_Rect rect, SDL_Color color);
    void drawRect(SDL_Rect rect, SDL_Color color);
    void renderText(const std::string& text,
                    TTF_Font* font,
                    SDL_Color color,
                    int x,
                    int y,
                    bool centerX = false,
                    bool centerY = false);

    std::pair<CellKind, char> classifyToken(const CellToken& token) const;
    SDL_Color colorForKind(CellKind kind, int parity) const;
    SDL_Color colorForDigit(char digit) const;

    std::optional<Pos> cellFromMouse(int mx, int my) const;
};

inline SDLUI::SDLUI(int cols, int rows)
    : SDLUI(cols, rows, Config{}) {}

inline SDLUI::SDLUI(int cols, int rows, Config cfg)
    : cols_(cols), rows_(rows), cfg_(std::move(cfg)) {
    computeGeometry();
    ready_ = initSDL() && loadFonts();
    if (!ready_) teardown();
}

inline SDLUI::~SDLUI() { teardown(); }

inline void SDLUI::computeGeometry() {
    tileSize_ = std::max(16, cfg_.tile_px);
    navbarHeight_ = std::max(40, cfg_.navbar_px);
    windowWidth_ = cols_ * tileSize_;
    windowHeight_ = rows_ * tileSize_ + navbarHeight_;
}

inline bool SDLUI::initSDL() {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }
    }
    if (TTF_WasInit() == 0) {
        if (TTF_Init() != 0) {
            SDL_Log("TTF_Init failed: %s", TTF_GetError());
            return false;
        }
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    window_ = SDL_CreateWindow("Minesweeper",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               windowWidth_,
                               windowHeight_,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    renderer_ =
        SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

inline bool SDLUI::loadFonts() {
    auto tryOpenFont = [](const std::string& path, int size) -> TTF_Font* {
        if (path.empty()) return nullptr;
        TTF_Font* f = TTF_OpenFont(path.c_str(), size);
        if (!f) SDL_Log("TTF_OpenFont failed for %s: %s", path.c_str(), TTF_GetError());
        return f;
    };

    fontNavbar_ = tryOpenFont(cfg_.font_path, cfg_.font_navbar_px);
    fontCell_ = tryOpenFont(cfg_.font_path, cfg_.font_cell_px);

    if (!fontNavbar_ || !fontCell_) {
        const char* fallback =
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";  // common on Linux
        fontNavbar_ = fontNavbar_ ? fontNavbar_
                                  : tryOpenFont(fallback, cfg_.font_navbar_px);
        fontCell_ =
            fontCell_ ? fontCell_ : tryOpenFont(fallback, cfg_.font_cell_px);
    }

    return fontNavbar_ && fontCell_;
}

inline void SDLUI::teardown() {
    if (fontCell_) {
        TTF_CloseFont(fontCell_);
        fontCell_ = nullptr;
    }
    if (fontNavbar_) {
        TTF_CloseFont(fontNavbar_);
        fontNavbar_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (TTF_WasInit()) TTF_Quit();
    if (SDL_WasInit(SDL_INIT_VIDEO)) SDL_Quit();
}

inline void SDLUI::clearWithColor(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

inline void SDLUI::fillRect(SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
}

inline void SDLUI::drawRect(SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer_, &rect);
}

inline void SDLUI::renderText(const std::string& text,
                              TTF_Font* font,
                              SDL_Color color,
                              int x,
                              int y,
                              bool centerX,
                              bool centerY) {
    if (!font || text.empty()) return;
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_Rect dst{x, y, surface->w, surface->h};
    if (centerX) dst.x -= dst.w / 2;
    if (centerY) dst.y -= dst.h / 2;
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

inline std::pair<SDLUI::CellKind, char> SDLUI::classifyToken(const CellToken& token) const {
    if (token.empty()) return {CellKind::Empty, ' '};
    char c = token.front();
    switch (c) {
        case '.':
            return {CellKind::Hidden, c};
        case '?':
            return {CellKind::Flagged, c};
        case '*':
            return {CellKind::Bomb, c};
        case ' ':
            return {CellKind::Empty, c};
        default:
            if (c >= '1' && c <= '8') return {CellKind::Number, c};
            return {CellKind::Empty, c};
    }
}

inline SDL_Color SDLUI::colorForKind(CellKind kind, int parity) const {
    switch (kind) {
        case CellKind::Hidden:
            return parity == 0 ? hiddenLight_ : hiddenDark_;
        case CellKind::Flagged:
            return parity == 0 ? hiddenLight_ : hiddenDark_;
        case CellKind::Empty:
            return parity == 0 ? revealedLight_ : revealedDark_;
        case CellKind::Number:
            return parity == 0 ? revealedLight_ : revealedDark_;
        case CellKind::Bomb:
            return SDL_Color{90, 90, 90, 255};
    }
    return hiddenLight_;
}

inline SDL_Color SDLUI::colorForDigit(char digit) const {
    int idx = digit - '1';
    if (idx < 0 || idx >= static_cast<int>(numberColors_.size()))
        return textLight_;
    return numberColors_[idx];
}

inline std::optional<Pos> SDLUI::cellFromMouse(int mx, int my) const {
    if (my < navbarHeight_) return std::nullopt;
    int boardY = my - navbarHeight_;
    int col = mx / tileSize_;
    int row = boardY / tileSize_;
    if (col < 0 || col >= cols_ || row < 0 || row >= rows_) return std::nullopt;
    return Pos{col, row};
}

inline std::optional<UserIntent> SDLUI::pollIntent() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                return UserIntent{Action::Quit, std::nullopt};
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    return UserIntent{Action::Quit, std::nullopt};
                if (e.key.keysym.sym == SDLK_r)
                    return UserIntent{Action::NewGame, std::nullopt};
                if (e.key.keysym.sym == SDLK_SPACE)
                    return UserIntent{Action::Redraw, std::nullopt};
                break;
            case SDL_MOUSEBUTTONDOWN: {
                auto pos = cellFromMouse(e.button.x, e.button.y);
                if (!pos) break;
                if (e.button.button == SDL_BUTTON_LEFT)
                    return UserIntent{Action::Reveal, pos};
                if (e.button.button == SDL_BUTTON_RIGHT)
                    return UserIntent{Action::ToggleFlag, pos};
                break;
            }
            default:
                break;
        }
    }
    return std::nullopt;
}

inline void SDLUI::render(const IBoardReadable& board,
                          const HudSnapshot& hud,
                          GameState state) {
    if (!ready_) return;

    clearWithColor(background_);

    SDL_Rect navbar{0, 0, windowWidth_, navbarHeight_};
    fillRect(navbar, navbarColor_);
    renderText("Time: " + hud.time_mmss, fontNavbar_, textLight_, 16, navbarHeight_ / 2, false, true);
    std::string bombs = "Bombs: " + hud.count;
    renderText(bombs,
               fontNavbar_,
               textLight_,
               windowWidth_ - 16,
               navbarHeight_ / 2,
               true,
               true);

    SDL_Rect boardRect{0, navbarHeight_, windowWidth_, rows_ * tileSize_};
    fillRect(boardRect, boardBg_);

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            SDL_Rect cellRect{col * tileSize_,
                              navbarHeight_ + row * tileSize_,
                              tileSize_,
                              tileSize_};
            auto kind_token = classifyToken(board.at(col, row));
            SDL_Color tileColor = colorForKind(kind_token.first, (row + col) % 2);
            fillRect(cellRect, tileColor);
            drawRect(cellRect, border_);

            char glyph = kind_token.second;
            switch (kind_token.first) {
                case CellKind::Hidden:
                    break;
                case CellKind::Flagged:
                    renderText("F",
                               fontCell_,
                               flagColor_,
                               cellRect.x + tileSize_ / 2,
                               cellRect.y + tileSize_ / 2,
                               true,
                               true);
                    break;
                case CellKind::Empty:
                    break;
                case CellKind::Bomb:
                    renderText("*",
                               fontCell_,
                               textLight_,
                               cellRect.x + tileSize_ / 2,
                               cellRect.y + tileSize_ / 2,
                               true,
                               true);
                    break;
                case CellKind::Number: {
                    SDL_Color digitColor = colorForDigit(glyph);
                    std::string text(1, glyph);
                    renderText(text,
                               fontCell_,
                               digitColor,
                               cellRect.x + tileSize_ / 2,
                               cellRect.y + tileSize_ / 2,
                               true,
                               true);
                    break;
                }
            }
        }
    }

    if (state == GameState::Won || state == GameState::Lost) {
        SDL_Color overlayColor{0, 0, 0, 160};
        SDL_Rect overlay{0, navbarHeight_, windowWidth_, rows_ * tileSize_};
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        fillRect(overlay, overlayColor);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);

        std::string message =
            state == GameState::Won ? "You win!" : "Game over!";
        renderText(message,
                   fontNavbar_,
                   textLight_,
                   windowWidth_ / 2,
                   navbarHeight_ + (rows_ * tileSize_) / 2,
                   true,
                   true);
        renderText("Press R for new game",
                   fontCell_,
                   textLight_,
                   windowWidth_ / 2,
                   navbarHeight_ + (rows_ * tileSize_) / 2 + 40,
                   true,
                   true);
    }

    SDL_RenderPresent(renderer_);
}
