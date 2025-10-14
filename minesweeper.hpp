// High-level application core for Minesweeper. This layer coordinates the
// domain model (`Board`), time tracking (`Stopwatch`), and presentation-facing
// ports so that both UIs can drive the same behaviour. Everything here follows
// a ports-and-adapters mindset: the core exposes intent sinks and read-only
// snapshot providers, while concrete UIs remain oblivious to the internal data
// structures.
#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "board.hpp"
#include "ports.hpp"
#include "presenter.hpp"
#include "stopwatch.hpp"

// `GameCore` is the orchestrator: it implements both the input port
// (`IIntentSink`) and the output snapshot interface (`IOutSnapshot`). UIs talk
// only to this class, making it the single source of truth for game state and
// derived HUD data.
class GameCore : public IIntentSink, public IOutSnapshot {
   public:
    // Public entry point used by UIs to spawn a fully configured game. The
    // explicit constructor keeps validation separate so that the call site
    // remains readable.
    GameCore(int rows = 7, int cols = 7, int bombs = 5);

    // Receives intents from the presentation layer (clicks, restarts, etc.)
    // and mutates the board accordingly.
    void handle(const UserIntent& intent) override;

    // Expose read-only snapshots that let UIs render without holding direct
    // references to the underlying board.
    const IBoardReadable& board() const override;
    HudSnapshot hud() const override;
    GameState state() const override { return state_; }

    // Convenience helpers used by presenters to avoid threading board width
    // through multiple layers.
    int width() const { return static_cast<int>(board_.width()); }
    int height() const { return static_cast<int>(board_.height()); }
    const Cell& cell(int x, int y) const;

   private:
    // Owned sub-systems that together represent the state of the world.
    Board board_;
    GameState state_{GameState::InProgress};
    Stopwatch stopwatch_;
    bool timerRunning_{false};
    int finalElapsed_{0};
    BoardPresenter presenter_;

    // Cached dimensions for quick boundary checks from user input.
    const int rows_;
    const int cols_;

    // Helpers that encapsulate specific lifecycle responsibilities.
    void newGame();
    bool inBounds(Pos p) const;
    static std::string formatElapsed(int seconds);
    void stopTimer();

    // Canonicalised configuration data so we can carry both size_t (for board)
    // and int (for UI) representations without repeated casting.
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

// Performs defensive validation on incoming parameters so that every public
// constructor call funnels through a single gatekeeper.
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

// Lightweight delegating constructor that keeps caller syntax terse.
inline GameCore::GameCore(int rows, int cols, int bombs)
    : GameCore(validateConfig(rows, cols, bombs)) {}

// Builds the composed object graph once the configuration is known to be
// valid, then immediately starts a game.
inline GameCore::GameCore(Dimensions dims)
    : board_(dims.rows, dims.cols, dims.bombs),
      presenter_(*this),
      rows_(dims.rows_i),
      cols_(dims.cols_i) {
    newGame();
}

// Resets the board and timers so a fresh game can begin.
inline void GameCore::newGame() {
    board_.reset();
    state_ = GameState::InProgress;
    stopwatch_.start();
    timerRunning_ = true;
    finalElapsed_ = 0;
}

// Guard shared by multiple handlers; keeps the rules for valid user clicks in
// one obvious place.
inline bool GameCore::inBounds(Pos p) const {
    return p.x >= 0 && p.y >= 0 && p.x < cols_ && p.y < rows_;
}

// Convenience accessor used by the presenter to translate the board into UI
// tokens.
inline const Cell& GameCore::cell(int x, int y) const {
    if (x < 0 || y < 0) throw std::out_of_range("Negative cell coordinate");
    return board_.cellAt(static_cast<size_t>(x), static_cast<size_t>(y));
}

// Formats elapsed seconds into MM:SS for the HUD.
inline std::string GameCore::formatElapsed(int seconds) {
    if (seconds < 0) seconds = 0;
    const int minutes = seconds / 60;
    const int secs = seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
        << std::setfill('0') << secs;
    return oss.str();
}

// Captures the elapsed time when a game ends so the HUD stays stable.
inline void GameCore::stopTimer() {
    if (!timerRunning_) return;
    finalElapsed_ = stopwatch_.elapsed();
    timerRunning_ = false;
}

// Central intent handler: routes commands to the board while managing victory
// and defeat transitions.
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

    const Coord coord{static_cast<index_t>(intent.where->y),
                      static_cast<index_t>(intent.where->x)};

    if (intent.action == Action::ToggleFlag) {
        board_.toggleFlag(coord);
        return;
    }

    const auto result = board_.reveal(coord);
    if (result == Board::RevealResult::HitBomb) {
        state_ = GameState::Lost;
        board_.revealAll();
        stopTimer();
        return;
    }

    if (result == Board::RevealResult::Victory ||
        board_.unrevealedSafeCount() == 0) {
        state_ = GameState::Won;
        board_.revealAll();
        stopTimer();
    }
}

// Exposes a read-only adapter around the board for rendering layers.
inline const IBoardReadable& GameCore::board() const { return presenter_; }

// Assembles HUD data (time + remaining bombs) for any presentation layer.
inline HudSnapshot GameCore::hud() const {
    const int elapsed = timerRunning_ ? stopwatch_.elapsed() : finalElapsed_;
    HudSnapshot hud;
    hud.time_mmss = formatElapsed(elapsed);
    hud.count = std::to_string(board_.bombsRemaining());
    return hud;
}

// Pass-through helpers that satisfy the IBoardReadable contract. They let the
// presenter surface board data without handing over mutating access.
inline int BoardPresenter::width() const { return game.width(); }

inline int BoardPresenter::height() const { return game.height(); }

// Produces the single-character token representing a cell, which is what every
// UI ultimately renders.
inline CellToken BoardPresenter::at(int x, int y) const {
    const auto& c = game.cell(x, y);
    return std::string(1, c.getSymbol());
}
