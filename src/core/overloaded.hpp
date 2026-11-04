#pragma once

namespace minesweeper {

// Builds one visitor out of several lambdas, for use with std::visit:
//
//     std::visit(overloaded{[](cell::Hidden) {...}, [](cell::Mine m) {...}}, v);
template <class... Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};

}  // namespace minesweeper
