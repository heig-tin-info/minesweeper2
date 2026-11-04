#pragma once

#include <iosfwd>

#include "adapters/console/renderer.hpp"
#include "app/ports.hpp"

namespace minesweeper::console {

// Text adapter. It owns its loop, which blocks on the input stream: read a
// line, translate it into a command, hand it over, draw the result. The
// streams are injected, so a test can play a whole game with strings.
class ConsoleUi {
public:
    ConsoleUi(std::istream& in, std::ostream& out, Style style);

    // Returns when the player quits or the input ends.
    void run(const GameView& view, CommandSink& commands);

private:
    std::istream& in_;
    std::ostream& out_;
    Style style_;
};

// True when stdout is a terminal that accepts colours (honours NO_COLOR).
bool stdoutSupportsColour();

}  // namespace minesweeper::console
