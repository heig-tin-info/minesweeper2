#include "adapters/console/renderer.hpp"

#include <array>
#include <format>
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>

#include "adapters/console/notation.hpp"
#include "core/overloaded.hpp"

namespace minesweeper::console {

namespace {

struct Glyph {
    std::string_view text;
    std::string_view colour;  // ANSI escape sequence, empty for none
};

constexpr std::string_view kReset = "\033[0m";
constexpr std::string_view kFrame = "\033[90m";
constexpr std::array<std::string_view, 9> kDigits{" ", "1", "2", "3", "4", "5", "6", "7", "8"};
constexpr std::array<std::string_view, 9> kDigitColours{
    "", "\033[34m", "\033[32m", "\033[31m", "\033[35m",
    "\033[33;1m", "\033[36m", "\033[34;1m", "\033[90m"};

// The single place that decides how a cell looks in a terminal. The visitor
// must handle every alternative of CellView, or this does not compile.
Glyph glyphOf(const CellView& view) {
    return std::visit(overloaded{
                          [](cell::Hidden) { return Glyph{".", "\033[90m"}; },
                          [](cell::Flagged) { return Glyph{"?", "\033[33m"}; },
                          [](cell::Revealed r) {
                              return Glyph{kDigits[r.adjacentMines], kDigitColours[r.adjacentMines]};
                          },
                          [](cell::Mine m) {
                              return m.exploded ? Glyph{"X", "\033[41;97;1m"} : Glyph{"*", "\033[31;1m"};
                          },
                          [](cell::WrongFlag) { return Glyph{"!", "\033[35;1m"}; },
                      },
                      view);
}

class Painter {
public:
    Painter(std::ostream& out, Style style) : out_(out), style_(style) {}

    void paint(std::string_view text, std::string_view colour) {
        if (style_.colour && !colour.empty())
            out_ << colour << text << kReset;
        else
            out_ << text;
    }

    void frame(std::string_view text) { paint(text, kFrame); }

    // One horizontal rule such as "├───┼───┤".
    void rule(int cols, std::string_view left, std::string_view middle, std::string_view right) {
        frame(left);
        for (int c = 0; c < cols; ++c) {
            frame("───");
            frame(c + 1 < cols ? middle : right);
        }
    }

private:
    std::ostream& out_;
    Style style_;
};

std::string clock(std::chrono::seconds elapsed) {
    return std::format("{:02}:{:02}", elapsed.count() / 60, elapsed.count() % 60);
}

std::string_view statusLine(Status status) {
    switch (status) {
        case Status::Won: return "You win!  n: new game, q: quit";
        case Status::Lost: return "Boom! You lose.  n: new game, q: quit";
        case Status::Ready:
        case Status::Playing: break;
    }
    return "B3: reveal, B3?: flag, n: new game, q: quit";
}

}  // namespace

void render(std::ostream& out, const GameView& view, Style style) {
    const Size size = view.size();
    const auto labelWidth = static_cast<int>(rowLabel(0, size).size());
    const std::string indent(static_cast<std::size_t>(labelWidth + 1), ' ');
    Painter painter{out, style};

    out << '\n' << indent << "Mines: " << view.minesLeft() << "   Time: " << clock(view.elapsed()) << "\n";

    out << indent;
    painter.rule(size.cols, "┌", "┬", "┐");
    out << '\n';
    for (int row = 0; row < size.rows; ++row) {
        out << std::setw(labelWidth) << rowLabel(row, size) << ' ';
        painter.frame("│");
        for (int col = 0; col < size.cols; ++col) {
            const Glyph glyph = glyphOf(view.cell({row, col}));
            out << ' ';
            painter.paint(glyph.text, glyph.colour);
            out << ' ';
            painter.frame("│");
        }
        out << '\n' << indent;
        if (row + 1 < size.rows)
            painter.rule(size.cols, "├", "┼", "┤");
        else
            painter.rule(size.cols, "└", "┴", "┘");
        out << '\n';
    }

    out << indent << "  ";
    for (int col = 0; col < size.cols; ++col) out << std::left << std::setw(4) << columnLabel(col);
    out << std::right << "\n\n" << statusLine(view.status()) << '\n';
}

}  // namespace minesweeper::console
