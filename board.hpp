#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

using index_t = std::ptrdiff_t;

struct Coord {
    index_t r{}, c{};
};

struct Cell {
    std::uint8_t neighborBombCount = 0;
    std::uint8_t flags = 0;  // bit0=bomb, bit1=revealed, bit2=flagged

    static constexpr std::uint8_t kBomb = 1, kRevealed = 2, kFlagged = 4;

    bool bomb() const noexcept { return flags & kBomb; }
    bool revealed() const noexcept { return flags & kRevealed; }
    bool flagged() const noexcept { return flags & kFlagged; }

    void setBomb(bool v) noexcept {
        flags = v ? (flags | kBomb) : (flags & ~kBomb);
    }
    void setRevealed(bool v) noexcept {
        flags = v ? (flags | kRevealed) : (flags & ~kRevealed);
    }
    void setFlagged(bool v) noexcept {
        flags = v ? (flags | kFlagged) : (flags & ~kFlagged);
    }

    [[nodiscard]] char getSymbol(bool debug = false) const noexcept {
        auto symbol = [this, debug]() noexcept -> char {
            if (bomb()) return '*';
            if (neighborBombCount > 0)
                return static_cast<char>('0' + neighborBombCount);
            return debug ? ':' : ' ';
        };
        if (debug || revealed()) return symbol();
        return flagged() ? '?' : '.';
    }
};

class Board {
   public:
    enum class RevealResult { Noop, HitBomb, Ok, Victory };

    Board(std::size_t rows = 7, std::size_t cols = 7, std::size_t bombs = 5,
          std::uint64_t seed = std::random_device{}())
        : _rows(rows), _cols(cols), _bombs(bombs), _rng(seed) {
        if (_rows == 0 || _cols == 0)
            throw std::invalid_argument("Board dimensions must be > 0");
        if (_bombs >= _rows * _cols)
            throw std::invalid_argument(
                "Bomb count must be less than total cells");
        _cells.resize(_rows * _cols);
        _leftToReveal = _rows * _cols - _bombs;
    }

    void reseed(std::uint64_t seed) noexcept { _rng.seed(seed); }

    [[nodiscard]] std::size_t width() const noexcept { return _cols; }
    [[nodiscard]] std::size_t height() const noexcept { return _rows; }

    // unchecked (assert en debug)
    Cell& operator()(Coord p) noexcept {
        assert(in_bounds(p));
        return _cells[index(p)];
    }
    const Cell& operator()(Coord p) const noexcept {
        assert(in_bounds(p));
        return _cells[index(p)];
    }

    // checked
    Cell& at(Coord p) {
        if (!in_bounds(p)) throw std::out_of_range("Invalid cell");
        return _cells[index(p)];
    }
    const Cell& at(Coord p) const {
        if (!in_bounds(p)) throw std::out_of_range("Invalid cell");
        return _cells[index(p)];
    }

    [[nodiscard]] bool in_bounds(Coord p) const noexcept {
        return p.r < _rows && p.c < _cols;
    }

    void reset() {
        for (auto& cell : _cells) cell = Cell{};
        _flagged = 0;
        _leftToReveal = _rows * _cols - _bombs;
        _initialized = false;
        _lost = false;
    }

    RevealResult reveal(Coord p) {
        if (!in_bounds(p)) throw std::out_of_range("Reveal outside board");
        auto& cell = at(p);
        if (cell.revealed() || cell.flagged()) return RevealResult::Noop;

        if (!_initialized)
            initialize(p);  // garantit au moins p sans bombe (et
                            // optionnellement sans voisins)
        if (cell.bomb()) {
            _lost = true;
            reveal_all_unchecked();
            return RevealResult::HitBomb;
        }

        flood_open_from(p);
        return (_leftToReveal == 0) ? RevealResult::Victory : RevealResult::Ok;
    }

    void toggleFlag(Coord p) {
        if (!in_bounds(p)) throw std::out_of_range("Flag outside board");
        auto& cell = at(p);
        if (cell.revealed()) return;
        if (cell.flagged()) {
            cell.setFlagged(false);
            --_flagged;
        } else {
            cell.setFlagged(true);
            ++_flagged;
        }
    }

    [[nodiscard]] int bombsRemaining() const noexcept {
        const std::size_t remaining =
            (_bombs > _flagged) ? (_bombs - _flagged) : 0;
        return static_cast<int>(remaining);
    }
    [[nodiscard]] std::size_t flaggedCount() const noexcept { return _flagged; }
    [[nodiscard]] std::size_t unrevealedSafeCount() const noexcept {
        return _leftToReveal;
    }
    [[nodiscard]] bool isInitialized() const noexcept { return _initialized; }
    [[nodiscard]] bool lost() const noexcept { return _lost; }
    [[nodiscard]] bool won() const noexcept {
        return !_lost && _initialized && _leftToReveal == 0;
    }

    void revealAll() noexcept {
        _flagged = 0;
        for (auto& cell : _cells) {
            cell.setRevealed(true);
            cell.setFlagged(false);
        }
        _leftToReveal = 0;
    }

    const Cell& cellAt(std::size_t col, std::size_t row) const {
        return at(Coord{static_cast<index_t>(row), static_cast<index_t>(col)});
    }

   private:
    std::vector<Cell> _cells;
    std::size_t _flagged = 0;
    std::size_t _leftToReveal = 0;
    std::mt19937_64 _rng;
    bool _initialized = false;
    bool _lost = false;
    const std::size_t _rows;
    const std::size_t _cols;
    const std::size_t _bombs;

    static constexpr std::array<std::pair<int, int>, 8> kDeltas{
        std::pair{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1},
        {1, -1},           {1, 0},  {1, 1}};

    [[nodiscard]] std::size_t index(Coord p) const noexcept {
        return p.r * _cols + p.c;
    }

    [[nodiscard]] int count_adjacent_bombs(Coord p) const noexcept {
        int count = 0;
        for (auto [dr, dc] : kDeltas) {
            const auto nr = static_cast<int>(p.r) + dr;
            const auto nc = static_cast<int>(p.c) + dc;
            if (nr < 0 || nc < 0) continue;
            if (nr >= static_cast<int>(_rows) || nc >= static_cast<int>(_cols))
                continue;
            const auto idx = static_cast<std::size_t>(nr) * _cols +
                             static_cast<std::size_t>(nc);
            if (_cells[idx].bomb()) ++count;
        }
        return count;
    }

    void update_adjacent_counts() noexcept {
        for (std::size_t r = 0; r < _rows; ++r) {
            for (std::size_t c = 0; c < _cols; ++c) {
                Coord pos{static_cast<index_t>(r), static_cast<index_t>(c)};
                if (!(*this)(pos).bomb()) {
                    (*this)(pos).neighborBombCount = count_adjacent_bombs(pos);
                }
            }
        }
    }

    // Option: rendre la 1re case vide ET sans voisins-bombe
    void initialize(Coord first) {
        if (_initialized) return;

        std::vector<std::size_t> idx(_cells.size());
        std::iota(idx.begin(), idx.end(), 0);

        const auto forbid = index(first);
        // Optionnel: interdire aussi les voisins pour un grand « opening »
        std::vector<bool> forbidden(_cells.size(), false);
        forbidden[forbid] = true;
        for_each_neighbor(first, [&](Coord n) { forbidden[index(n)] = true; });

        for (auto& c : _cells) {
            c = Cell{};
        }

        // Positions candidates
        std::vector<std::size_t> candidates;
        candidates.reserve(_cells.size());
        for (std::size_t i = 0; i < _cells.size(); ++i)
            if (!forbidden[i]) candidates.push_back(i);

        if (_bombs > candidates.size())
            throw std::runtime_error(
                "Unable to place all bombs with safe opening");

        std::shuffle(candidates.begin(), candidates.end(), _rng);
        for (std::size_t i = 0; i < _bombs; ++i)
            _cells[candidates[i]].setBomb(true);

        update_adjacent_counts();
        _initialized = true;

        // Si la case d’ouverture a des voisins-bombe (si vous ne voulez
        // interdire que la case), on peut reshuffle jusqu’à n==0 (option) Ici
        // déjà garanti par l’interdiction des voisins.
    }

    template <class F>
    void for_each_neighbor(Coord p, F&& f) const {
        for (auto [dr, dc] : kDeltas) {
            const auto nr = static_cast<int>(p.r) + dr;
            const auto nc = static_cast<int>(p.c) + dc;
            if (nr < 0 || nc < 0) continue;
            if (nr >= static_cast<int>(_rows) || nc >= static_cast<int>(_cols))
                continue;
            f(Coord{static_cast<index_t>(nr), static_cast<index_t>(nc)});
        }
    }

    void flood_open_from(Coord start) {
        auto& s = at(start);
        if (s.revealed() || s.flagged() || s.bomb()) return;

        auto reveal_one = [&](Coord p) {
            auto& cell = (*this)(p);
            if (cell.revealed() || cell.flagged() || cell.bomb()) return false;
            cell.setRevealed(true);
            if (_leftToReveal > 0) --_leftToReveal;
            return true;
        };

        reveal_one(start);
        if (at(start).neighborBombCount != 0) return;

        std::queue<Coord> q;
        q.push(start);
        while (!q.empty()) {
            Coord cur = q.front();
            q.pop();
            for_each_neighbor(cur, [&](Coord n) {
                auto& cn = (*this)(n);
                if (!cn.revealed() && !cn.bomb() && !cn.flagged()) {
                    reveal_one(n);
                    if (cn.neighborBombCount == 0) q.push(n);
                }
            });
        }
    }

    void reveal_all_unchecked() noexcept {
        for (auto& cell : _cells) {
            cell.setRevealed(true);
            cell.setFlagged(false);
        }
        _leftToReveal = 0;
        _flagged = 0;
    }
};
