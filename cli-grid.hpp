#pragma once

// Small helper dedicated to drawing Minesweeper-style grids using Unicode box
// drawing characters. The renderer stays header-only so the console front-end
// can share it without adding another translation unit. Optional colour output
// is exposed through ANSI escape codes, making it trivial to tweak the look of
// the grid from the UI layer.
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>

class CliGrid {
   public:
    struct Options {
        bool show_row_labels = true;
        bool show_column_labels = true;
        bool colour_grid = false;
        bool colour_cells = false;
        std::string grid_colour_code = "\033[90m";  // bright black / grey
        std::string reset_code = "\033[0m";
        std::function<std::string(std::string_view)> cell_colouriser{};
    };

    struct RowContext {
        int display_row;  // 0 at top
        int logical_row;  // 0 at bottom
    };

    struct SeparatorContext {
        int display_row;           // 0 at top separator
        int logical_row_above;     // row index above separator
        int logical_row_below;     // row index below separator
    };

    struct Decorations {
        std::function<void(std::ostream&, const RowContext&)> row_suffix{};
        std::function<void(std::ostream&, const SeparatorContext&)> separator_suffix{};
    };

    CliGrid() = default;
    explicit CliGrid(Options opts) : options_(std::move(opts)) {}

    void setOptions(Options opts) { options_ = std::move(opts); }
    const Options& options() const { return options_; }

    void render(std::ostream& os, int width, int height,
                const std::function<std::string(int, int)>& cellAccessor) const {
        render(os, width, height, cellAccessor, Decorations{});
    }

    void render(std::ostream& os, int width, int height,
                const std::function<std::string(int, int)>& cellAccessor,
                const Decorations& decorations) const {
        if (width <= 0 || height <= 0) return;

        const int rowLabelWidth =
            options_.show_row_labels ? static_cast<int>(std::to_string(height).size())
                                     : 0;
        const int indentWidth = options_.show_row_labels ? rowLabelWidth + 1 : 0;
        const std::string indent(indentWidth, ' ');

        auto emitGrid = [&](std::string_view glyph) {
            if (options_.colour_grid && !options_.grid_colour_code.empty()) {
                os << options_.grid_colour_code << glyph << options_.reset_code;
            } else {
                os << glyph;
            }
        };

        auto formatCell = [&](std::string_view token) {
            std::string text = token.empty() ? " " : std::string(token);
            if (!options_.colour_cells || !options_.cell_colouriser) return text;
            std::string prefix = options_.cell_colouriser(text);
            if (prefix.empty()) return text;
            return prefix + text + options_.reset_code;
        };

        os << '\n';
        os << indent;
        emitGrid("┌");
        for (int x = 0; x < width; ++x) {
            emitGrid("───");
            emitGrid(x == width - 1 ? "┐" : "┬");
        }
        os << '\n';

        for (int display = 0; display < height; ++display) {
            const int y = height - 1 - display;
            if (options_.show_row_labels) {
                os << std::setw(rowLabelWidth) << (y + 1) << ' ';
            } else if (indentWidth > 0) {
                os << indent;
            }

            emitGrid("│");
            for (int x = 0; x < width; ++x) {
                os << ' ' << formatCell(cellAccessor(x, y)) << ' ';
                emitGrid("│");
            }

            if (decorations.row_suffix) {
                decorations.row_suffix(
                    os, RowContext{display, y});
            }
            os << '\n';

            if (display == height - 1) continue;

            if (options_.show_row_labels) {
                os << indent;
            } else if (indentWidth > 0) {
                os << indent;
            }

            emitGrid("├");
            for (int x = 0; x < width; ++x) {
                emitGrid("───");
                emitGrid(x == width - 1 ? "┤" : "┼");
            }
            if (decorations.separator_suffix) {
                decorations.separator_suffix(
                    os, SeparatorContext{display, y, y - 1});
            }
            os << '\n';
        }

        os << indent;
        emitGrid("└");
        for (int x = 0; x < width; ++x) {
            emitGrid("───");
            emitGrid(x == width - 1 ? "┘" : "┴");
        }
        os << '\n';

        if (options_.show_column_labels) {
            os << indent << ' ';
            for (int x = 0; x < width; ++x) {
                os << ' ' << columnLabel(x) << "  ";
            }
            os << '\n';
        }
    }

   private:
    Options options_{};

    static std::string columnLabel(int idx) {
        std::string label;
        int value = idx;
        do {
            int rem = value % 26;
            label.push_back(static_cast<char>('A' + rem));
            value = value / 26 - 1;
        } while (value >= 0);
        std::reverse(label.begin(), label.end());
        return label;
    }
};
