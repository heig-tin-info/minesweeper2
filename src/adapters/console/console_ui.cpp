#include "adapters/console/console_ui.hpp"

#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "adapters/console/notation.hpp"

namespace minesweeper::console {

ConsoleUi::ConsoleUi(std::istream& in, std::ostream& out, Style style)
    : in_(in), out_(out), style_(style) {}

void ConsoleUi::run(const GameView& view, CommandSink& commands) {
    render(out_, view, style_);
    std::string line;
    for (;;) {
        out_ << "> " << std::flush;
        if (!std::getline(in_, line)) return;

        const auto input = parseInput(line, view.size());
        if (!input) {
            out_ << "Unknown command. Examples: B3, B3?, n, q\n";
            continue;
        }
        if (std::holds_alternative<Quit>(*input)) return;

        commands.handle(std::get<Command>(*input));
        render(out_, view, style_);
    }
}

bool stdoutSupportsColour() {
#ifdef _WIN32
    const bool tty = _isatty(_fileno(stdout));
#else
    const bool tty = ::isatty(::fileno(stdout));
#endif
    const char* term = std::getenv("TERM");
    return tty && !std::getenv("NO_COLOR") && !(term && std::string_view{term} == "dumb");
}

}  // namespace minesweeper::console
