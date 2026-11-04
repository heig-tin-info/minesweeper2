#include "launcher/launch.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "core/mine_layout.hpp"

namespace minesweeper {

namespace {

// The real clock lives at the edge of the program, with the other side effects.
class SteadyClock final : public Clock {
public:
    TimePoint now() const override { return std::chrono::steady_clock::now(); }
};

template <class Int>
Int parseNumber(std::string_view text) {
    Int value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::invalid_argument("not a number: '" + std::string(text) + "'");
    return value;
}

}  // namespace

LaunchOptions parseArguments(std::span<const char* const> args) {
    LaunchOptions options;
    std::vector<int> numbers;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--seed") {
            if (++i == args.size()) throw std::invalid_argument("--seed needs a value");
            options.seed = parseNumber<std::uint64_t>(args[i]);
        } else {
            numbers.push_back(parseNumber<int>(arg));
        }
    }

    if (numbers.size() == 3)
        options.game = {.size = {numbers[0], numbers[1]}, .mines = numbers[2]};
    else if (!numbers.empty())
        throw std::invalid_argument("expected exactly three numbers: rows cols mines");
    return options;
}

std::string usage(const std::string& program) {
    return "Usage: " + program + " [rows cols mines] [--seed N]\n"
           "Example: " + program + " 16 16 40 --seed 42\n";
}

int launch(int argc, const char* const argv[], const std::function<void(Session&)>& play) {
    const std::string program = argc > 0 ? argv[0] : "minesweeper";
    try {
        const LaunchOptions options = parseArguments({argv + 1, argv + argc});
        if (options.help) {
            std::cout << usage(program);
            return 0;
        }

        RandomMineLayout layout{options.seed.value_or(std::random_device{}())};
        SteadyClock clock;
        Session session{options.game, layout, clock};
        play(session);
        return 0;
    } catch (const std::invalid_argument& e) {
        std::cerr << "error: " << e.what() << '\n' << usage(program);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}

}  // namespace minesweeper
