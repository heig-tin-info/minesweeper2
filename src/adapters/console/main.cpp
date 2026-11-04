#include <iostream>

#include "adapters/console/console_ui.hpp"
#include "launcher/launch.hpp"

int main(int argc, char* argv[]) {
    using namespace minesweeper;
    return launch(argc, argv, [](Session& session) {
        console::ConsoleUi ui{std::cin, std::cout, {.colour = console::stdoutSupportsColour()}};
        ui.run(session, session);
    });
}
