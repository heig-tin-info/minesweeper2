#include "adapters/console/notation.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace minesweeper::console {

namespace {

std::string_view trim(std::string_view s) {
    auto space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && space(s.front())) s.remove_prefix(1);
    while (!s.empty() && space(s.back())) s.remove_suffix(1);
    return s;
}

std::string lower(std::string_view s) {
    std::string out(s);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

}  // namespace

// Bijective base 26: A..Z, then AA, AB...
std::string columnLabel(int col) {
    std::string label;
    for (int n = col + 1; n > 0; n = (n - 1) / 26)
        label.insert(label.begin(), static_cast<char>('A' + (n - 1) % 26));
    return label;
}

std::string rowLabel(int row, Size size) { return std::to_string(size.rows - row); }

std::optional<Pos> parsePos(std::string_view text, Size size) {
    constexpr std::size_t kMaxLetters = 3;
    std::size_t i = 0;
    int col = 0;
    for (; i < text.size() && i < kMaxLetters && std::isalpha(static_cast<unsigned char>(text[i])); ++i)
        col = col * 26 + (std::toupper(static_cast<unsigned char>(text[i])) - 'A' + 1);
    if (i == 0) return std::nullopt;

    int number = 0;
    const char* end = text.data() + text.size();
    const auto [last, error] = std::from_chars(text.data() + i, end, number);
    if (error != std::errc{} || last != end) return std::nullopt;

    const Pos p{size.rows - number, col - 1};
    if (!size.contains(p)) return std::nullopt;
    return p;
}

std::optional<Input> parseInput(std::string_view line, Size size) {
    line = trim(line);
    const std::string word = lower(line);
    if (word == "q" || word == "quit" || word == "exit") return Quit{};
    if (word == "n" || word == "new" || word == "r" || word == "restart") return Command{NewGame{}};

    const bool flag = line.ends_with('?');
    if (flag) line.remove_suffix(1);
    const auto pos = parsePos(line, size);
    if (!pos) return std::nullopt;
    if (flag) return Command{ToggleFlag{*pos}};
    return Command{Reveal{*pos}};
}

}  // namespace minesweeper::console
