#pragma once

#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ports.hpp"

class ConsoleUI {
   public:
    bool ready() const { return true; }

    void render(const IBoardReadable& b, const HudSnapshot& hud) {
        const int W = b.width(), H = b.height();

        auto top = [&] {
            std::cout << "\n";
            std::cout << "  " << "┌";
            for (int x = 0; x < W; ++x)
                std::cout << "───" << (x == W - 1 ? "┐" : "┬");
            std::cout << "\n";
        };
        auto sep = [&](bool printCount) {
            std::cout << "  " << "├";
            for (int x = 0; x < W; ++x)
                std::cout << "───" << (x == W - 1 ? "┤" : "┼");
            if (printCount) std::cout << " Count: " << hud.count;
            std::cout << "\n";
        };
        auto bottom = [&] {
            std::cout << "  " << "└";
            for (int x = 0; x < W; ++x)
                std::cout << "───" << (x == W - 1 ? "┘" : "┴");
            std::cout << "\n";
        };
        auto cols = [&] {
            std::cout << "  ";
            for (int x = 0; x < W; ++x)
                std::cout << "  " << char('A' + x) << " ";
            std::cout << "\n";
        };

        top();
        for (int ry = 0; ry < H; ++ry) {
            int y = H - 1 - ry;  // draw from top to bottom

            // ligne de contenu: numéro (sans setw), puis cellules " │ X │"
            std::cout << (y + 1) << " " << "│";
            for (int x = 0; x < W; ++x) {
                std::string s = b.at(x, y);
                if (s.empty()) s = " ";
                std::cout << " " << s << " │";
            }
            if (ry == 0) std::cout << " Time: " << hud.time_mmss;
            std::cout << "\n";

            if (ry != H - 1) sep(ry == 0);
        }
        bottom();
        cols();
    }

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
    static void trim(std::string& s) {
        auto sp = [](unsigned char c) { return std::isspace(c); };
        while (!s.empty() && sp(s.front())) s.erase(s.begin());
        while (!s.empty() && sp(s.back())) s.pop_back();
    }
    static bool is(const std::string& s, std::string_view t) {
        if (s.size() != t.size()) return false;
        for (size_t i = 0; i < s.size(); ++i)
            if (std::tolower((unsigned char)s[i]) !=
                std::tolower((unsigned char)t[i]))
                return false;
        return true;
    }
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
