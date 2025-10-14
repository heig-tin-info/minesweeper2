#pragma once

// Text-based UI that demonstrates how the game core can be driven without any
// graphics stack. The implementation leans on simple std::iostream helpers so
// students can focus on the command parsing and presenter interaction.
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cli-grid.hpp"
#include "ports.hpp"

class ConsoleUI {
   public:
    struct Config {
        CliGrid::Options grid_options{};
        bool auto_enable_colours = true;
    };

    ConsoleUI() : ConsoleUI(Config{}) {}
    explicit ConsoleUI(Config cfg)
        : grid_(selectOptions(std::move(cfg.grid_options),
                              cfg.auto_enable_colours)) {}

    // Console mode has no asynchronous initialisation; always ready.
    bool ready() const { return true; }

    void setGridOptions(CliGrid::Options opts) { grid_.setOptions(std::move(opts)); }
    const CliGrid::Options& gridOptions() const { return grid_.options(); }

    static CliGrid::Options colourfulGridOptions() {
        CliGrid::Options opts;
        opts.colour_grid = true;
        opts.colour_cells = true;
        opts.cell_colouriser = [](std::string_view token) -> std::string {
            if (token.empty()) return "";
            const unsigned char ch = static_cast<unsigned char>(token.front());
            switch (ch) {
                case '.':
                    return "\033[90m";  // hidden cell
                case '?':
                    return "\033[33m";  // flag
                case '*':
                    return "\033[31;1m";  // bomb
                case ' ':
                    return "";
                default:
                    if (std::isdigit(ch)) {
                        switch (ch) {
                            case '1':
                                return "\033[34m";
                            case '2':
                                return "\033[32m";
                            case '3':
                                return "\033[31m";
                            case '4':
                                return "\033[35m";
                            case '5':
                                return "\033[33;1m";
                            case '6':
                                return "\033[36m";
                            case '7':
                                return "\033[34;1m";
                            case '8':
                                return "\033[90m";
                        }
                    }
                    break;
            }
            return "";
        };
        return opts;
    }

    // Renders the board using ASCII art. The snapshot passed in exposes only
    // read-only views, illustrating separation between core logic and UI.
    void render(const IBoardReadable& b, const HudSnapshot& hud) {
        const int W = b.width(), H = b.height();

        CliGrid::Decorations deco;
        deco.row_suffix = [&](std::ostream& os, const CliGrid::RowContext& ctx) {
            if (ctx.display_row == 0) os << " Time: " << hud.time_mmss;
        };
        deco.separator_suffix = [&](std::ostream& os,
                                    const CliGrid::SeparatorContext& ctx) {
            if (ctx.display_row == 0) os << " Count: " << hud.count;
        };

        grid_.render(
            std::cout, W, H,
            [&](int x, int y) {
                std::string cell = b.at(x, y);
                if (cell.empty()) cell = " ";
                return cell;
            },
            deco);
    }

    // Blocking read from stdin that turns user commands into domain intents.
    UserIntent wait_intent() {
        for (;;) {
            std::cout << "> ";
            std::cout.flush();

            std::string line;
            if (!std::getline(std::cin, line))
                return {Action::Quit, std::nullopt};
            trim(line);
            if (line.empty()) continue;

            if (is(line, "q") || is(line, "quit") || is(line, "exit"))
                return {Action::Quit, std::nullopt};
            if (is(line, "r") || is(line, "restart") || is(line, "new") ||
                is(line, "newgame"))
                return {Action::NewGame, std::nullopt};
            if (is(line, "redraw") || is(line, "cls"))
                return {Action::Redraw, std::nullopt};

            if (auto it = parse_coord(line)) return *it;

            std::cout << "[x] Invalid input. Examples: a3, b7?, r, q\n";
        }
    }

   private:
    CliGrid grid_{};

    static CliGrid::Options selectOptions(CliGrid::Options opts,
                                          bool auto_enable) {
        if (!auto_enable) return opts;
        if (opts.colour_grid || opts.colour_cells) return opts;
        if (!terminalSupportsColour()) return opts;
        return colourfulGridOptions();
    }

    static bool terminalSupportsColour() {
        if (!stdoutIsTty()) return false;
        if (std::getenv("NO_COLOR")) return false;
        if (const char* term = std::getenv("TERM")) {
            if (std::strcmp(term, "dumb") == 0) return false;
        }
        return true;
    }

    static bool stdoutIsTty() {
#ifdef _WIN32
        return _isatty(_fileno(stdout));
#else
        return ::isatty(::fileno(stdout));
#endif
    }

    // Utility that removes leading/trailing whitespace so the parser can stay
    // focused on the semantic pieces of the command.
    static void trim(std::string& s) {
        auto sp = [](unsigned char c) { return std::isspace(c); };
        while (!s.empty() && sp(s.front())) s.erase(s.begin());
        while (!s.empty() && sp(s.back())) s.pop_back();
    }
    // Case-insensitive comparison used to match short textual commands.
    static bool is(const std::string& s, std::string_view t) {
        if (s.size() != t.size()) return false;
        for (size_t i = 0; i < s.size(); ++i)
            if (std::tolower((unsigned char)s[i]) !=
                std::tolower((unsigned char)t[i]))
                return false;
        return true;
    }
    // Parses strings like "A3" or "B7?" into structured intents.
    static std::optional<UserIntent> parse_coord(const std::string& in) {
        if (in.size() < 2) return std::nullopt;
        size_t i = 0;
        char col = std::toupper((unsigned char)in[i++]);
        if (col < 'A' || col > 'Z') return std::nullopt;
        int row = 0;
        while (i < in.size() && std::isdigit((unsigned char)in[i])) {
            row = row * 10 + (in[i] - '0');
            ++i;
            if (row > 99) return std::nullopt;
        }
        if (row <= 0) return std::nullopt;
        bool flag = false;
        if (i < in.size() && in[i] == '?') {
            flag = true;
            ++i;
        }
        if (i != in.size()) return std::nullopt;

        Pos p{col - 'A', row - 1};
        return UserIntent{flag ? Action::ToggleFlag : Action::Reveal, p};
    }
};
