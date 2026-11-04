#include "adapters/sdl/sdl_ui.hpp"
#include "launcher/launch.hpp"

int main(int argc, char* argv[]) {
    using namespace minesweeper;
    return launch(argc, argv, [](Session& session) {
        sdl::SdlUi ui{session.size()};
        ui.run(session, session);
    });
}
