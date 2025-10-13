#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "board.hpp"
#include "ports.hpp"
#include "presenter.hpp"
#include "stopwatch.hpp"

class GameCore : public IIntentSink, public IOutSnapshot {
   public:
    GameCore(int rows = 7, int cols = 7, int bombs = 5);

    void handle(const UserIntent& intent) override;

    const IBoardReadable& board() const override;
    HudSnapshot hud() const override;
    GameState state() const override { return state_; }

    int width() const { return static_cast<int>(board_.width()); }
    int height() const { return static_cast<int>(board_.height()); }
    const Cell& cell(int x, int y) const;

   private:
    using coord_t = Board::coord_t;

    Board board_;
    GameState state_{GameState::InProgress};
    Stopwatch stopwatch_;
    bool timerRunning_{false};
    int finalElapsed_{0};
    BoardPresenter presenter_;

    const int rows_;
    const int cols_;

    void newGame();
    bool inBounds(Pos p) const;
    static std::string formatElapsed(int seconds);
    void stopTimer();

    struct Dimensions {
        size_t rows;
        size_t cols;
        size_t bombs;
        int rows_i;
        int cols_i;
    };

    static Dimensions validateConfig(int rows, int cols, int bombs);
    explicit GameCore(Dimensions dims);
};

inline GameCore::Dimensions GameCore::validateConfig(int rows, int cols,
                                                     int bombs) {
    if (rows <= 0 || cols <= 0)
        throw std::invalid_argument("Dimensions must be positive");
    if (bombs <= 0) throw std::invalid_argument("Bomb count must be positive");

    const size_t sRows = static_cast<size_t>(rows);
    const size_t sCols = static_cast<size_t>(cols);
    const size_t sBombs = static_cast<size_t>(bombs);
    if (sBombs >= sRows * sCols)
        throw std::invalid_argument(
            "Bomb count must leave at least one safe cell");

    return Dimensions{sRows, sCols, sBombs, rows, cols};
}

inline GameCore::GameCore(int rows, int cols, int bombs)
    : GameCore(validateConfig(rows, cols, bombs)) {}

inline GameCore::GameCore(Dimensions dims)
    : board_(dims.rows, dims.cols, dims.bombs),
      presenter_(*this),
      rows_(dims.rows_i),
      cols_(dims.cols_i) {
    newGame();
}

inline void GameCore::newGame() {
    board_.reset();
    state_ = GameState::InProgress;
    stopwatch_.start();
    timerRunning_ = true;
    finalElapsed_ = 0;
}

inline bool GameCore::inBounds(Pos p) const {
    return p.x >= 0 && p.y >= 0 && p.x < cols_ && p.y < rows_;
}

inline const Cell& GameCore::cell(int x, int y) const {
    if (x < 0 || y < 0) throw std::out_of_range("Negative cell coordinate");
    return board_.cellAt(static_cast<size_t>(x), static_cast<size_t>(y));
}

inline std::string GameCore::formatElapsed(int seconds) {
    if (seconds < 0) seconds = 0;
    const int minutes = seconds / 60;
    const int secs = seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
        << std::setfill('0') << secs;
    return oss.str();
}

inline void GameCore::stopTimer() {
    if (!timerRunning_) return;
    finalElapsed_ = stopwatch_.elapsed();
    timerRunning_ = false;
}

inline void GameCore::handle(const UserIntent& intent) {
    switch (intent.action) {
        case Action::NewGame:
            newGame();
            return;
        case Action::Quit:
        case Action::Redraw:
            return;
        case Action::Reveal:
        case Action::ToggleFlag:
            break;
    }

    if (state_ != GameState::InProgress) return;
    if (!intent.where || !inBounds(*intent.where)) return;

    const coord_t coord{static_cast<size_t>(intent.where->y),
                        static_cast<size_t>(intent.where->x)};

    if (intent.action == Action::ToggleFlag) {
        board_.toggleFlag(coord);
        return;
    }

    const bool safe = board_.reveal(coord);
    if (!safe) {
        state_ = GameState::Lost;
        board_.revealAll();
        stopTimer();
        return;
    }

    if (board_.unrevealedSafeCount() == 0) {
        state_ = GameState::Won;
        board_.revealAll();
        stopTimer();
    }
}

inline const IBoardReadable& GameCore::board() const { return presenter_; }

inline HudSnapshot GameCore::hud() const {
    const int elapsed = timerRunning_ ? stopwatch_.elapsed() : finalElapsed_;
    HudSnapshot hud;
    hud.time_mmss = formatElapsed(elapsed);
    hud.count = std::to_string(board_.bombsRemaining());
    return hud;
}

inline int BoardPresenter::width() const { return game.width(); }

inline int BoardPresenter::height() const { return game.height(); }

inline CellToken BoardPresenter::at(int x, int y) const {
    const auto& c = game.cell(x, y);
    return std::string(1, c.getSymbol());
}
