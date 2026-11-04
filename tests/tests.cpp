// Unit tests: plain asserts, no framework. Run with `make test`.
// The ports let us replace the random generator and the terminal by test
// doubles, so every test is deterministic and needs no keyboard.
#undef NDEBUG
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../src/console.hpp"
#include "../src/game.hpp"
#include "../src/play.hpp"
#include "../src/random_layout.hpp"

// Test double for MineLayout: the mines are where the test says.
struct FixedLayout : MineLayout {
    std::vector<Pos> mines;
    Pos firstClick{-1, -1};
    explicit FixedLayout(std::vector<Pos> m) : mines(m) {}
    std::vector<Pos> place(int, int, int, Pos first) override {
        firstClick = first;
        return mines;
    }
};

// Test double for UserInterface: replays a script of commands.
struct ScriptedUi : UserInterface {
    std::vector<Command> script;
    int shown = 0;
    void show(const Game&) override { ++shown; }
    Command ask() override {
        if (script.empty()) return {Command::Quit};
        Command c = script.front();
        script.erase(script.begin());
        return c;
    }
};

// 3x3 grid, one mine in the corner:   * 1 .
//                                     1 1 .
//                                     . . .
static void cascadeOpensTheEmptyRegionAndWins() {
    FixedLayout layout{{{0, 0}}};
    Game game{3, 3, 1, layout};
    game.reveal({2, 2});
    assert((layout.firstClick == Pos{2, 2}));
    assert(game.at({0, 1}).revealed && game.at({0, 1}).adjacent == 1);
    assert(!game.at({0, 0}).revealed);
    assert(game.status() == Status::Won);
}

static void revealingAMineLoses() {
    FixedLayout layout{{{0, 0}, {2, 2}}};
    Game game{3, 3, 2, layout};
    game.reveal({1, 1});
    assert(game.at({1, 1}).adjacent == 2);
    assert(game.status() == Status::Playing);
    game.reveal({0, 0});
    assert(game.status() == Status::Lost);
    game.reveal({0, 1});  // ignored once the game is over
    assert(!game.at({0, 1}).revealed);
}

static void aFlagProtectsACell() {
    FixedLayout layout{{{0, 0}}};
    Game game{3, 3, 1, layout};
    game.toggleFlag({2, 2});
    game.reveal({2, 2});
    assert(!game.at({2, 2}).revealed);
    game.toggleFlag({2, 2});
    assert(!game.at({2, 2}).flagged);
}

static void invalidArgumentsThrow() {
    FixedLayout layout{{}};
    bool thrown = false;
    try { Game{2, 2, 4, layout}; } catch (const std::invalid_argument&) { thrown = true; }
    assert(thrown);

    Game game{2, 2, 1, layout};
    thrown = false;
    try { game.reveal({2, 0}); } catch (const std::out_of_range&) { thrown = true; }
    assert(thrown);
}

static void sameSeedGivesSameMines() {
    auto a = RandomLayout{42}.place(9, 9, 10, {4, 4});
    auto b = RandomLayout{42}.place(9, 9, 10, {4, 4});
    assert(a == b && a.size() == 10);
    for (Pos p : a) assert((p != Pos{4, 4}));
}

static void parsingCommands() {
    assert(parse("r 2 3")->type == Command::Reveal);
    assert((parse("f 2 3")->pos == Pos{2, 3}));
    assert(parse("q")->type == Command::Quit);
    assert(!parse("r 2"));
    assert(!parse("x 1 1"));
}

static void consoleShowsTheRawGrid() {
    FixedLayout layout{{{0, 0}}};
    Game game{2, 2, 1, layout};
    game.toggleFlag({0, 0});
    std::istringstream in;
    std::ostringstream out;
    Console{in, out}.show(game);
    assert(out.str() == "\n    0 1\n  0 F #\n  1 # #\n");
}

static void playRunsTheScript() {
    FixedLayout layout{{{0, 0}}};
    Game game{3, 3, 1, layout};
    ScriptedUi ui;
    ui.script = {{Command::Reveal, {9, 9}}, {Command::Reveal, {2, 2}}, {Command::Quit}};
    play(game, ui);
    assert(game.status() == Status::Won);
    assert(ui.shown == 2);           // initial grid, then after the winning move
    assert(ui.script.size() == 1);   // the game ended before "quit"
}

int main() {
    cascadeOpensTheEmptyRegionAndWins();
    revealingAMineLoses();
    aFlagProtectsACell();
    invalidArgumentsThrow();
    sameSeedGivesSameMines();
    parsingCommands();
    consoleShowsTheRawGrid();
    playRunsTheScript();
    std::cout << "All tests passed\n";
}
