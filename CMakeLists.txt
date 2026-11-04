cmake_minimum_required(VERSION 3.20)

project(minesweeper LANGUAGES CXX)

option(MINESWEEPER_BUILD_SDL "Build the SDL2 front-end when SDL2 is available" ON)
option(MINESWEEPER_BUILD_TESTS "Build the unit tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

# One library per layer. The link graph is the dependency rule:
#   adapters -> app -> core, and never the other way round.

add_library(minesweeper_core STATIC
    src/core/game.cpp
    src/core/mine_layout.cpp
    src/core/minefield.cpp
)
target_include_directories(minesweeper_core PUBLIC src)

add_library(minesweeper_app STATIC src/app/session.cpp)
target_link_libraries(minesweeper_app PUBLIC minesweeper_core)

add_library(minesweeper_launcher STATIC src/launcher/launch.cpp)
target_link_libraries(minesweeper_launcher PUBLIC minesweeper_app)

add_library(minesweeper_console STATIC
    src/adapters/console/console_ui.cpp
    src/adapters/console/notation.cpp
    src/adapters/console/renderer.cpp
)
target_link_libraries(minesweeper_console PUBLIC minesweeper_app)

add_executable(minesweeper-console src/adapters/console/main.cpp)
target_link_libraries(minesweeper-console PRIVATE minesweeper_console minesweeper_launcher)
install(TARGETS minesweeper-console RUNTIME DESTINATION bin)

if(MINESWEEPER_BUILD_SDL)
    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_ttf CONFIG QUIET)
    if(TARGET SDL2::SDL2 AND TARGET SDL2_ttf::SDL2_ttf)
        add_executable(minesweeper-sdl
            src/adapters/sdl/main.cpp
            src/adapters/sdl/sdl_ui.cpp
        )
        target_link_libraries(minesweeper-sdl PRIVATE
            minesweeper_launcher
            SDL2::SDL2
            SDL2_ttf::SDL2_ttf
            $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        )
        target_compile_definitions(minesweeper-sdl PRIVATE
            MINESWEEPER_FONT="${PROJECT_SOURCE_DIR}/assets/collegiate.ttf")
        install(TARGETS minesweeper-sdl RUNTIME DESTINATION bin)
    else()
        message(WARNING "SDL2 or SDL2_ttf not found: the SDL front-end is skipped")
    endif()
endif()

if(MINESWEEPER_BUILD_TESTS)
    enable_testing()

    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(minesweeper-tests
        tests/console_tests.cpp
        tests/game_tests.cpp
        tests/launcher_tests.cpp
        tests/mine_layout_tests.cpp
        tests/minefield_tests.cpp
        tests/position_tests.cpp
        tests/session_tests.cpp
    )
    target_link_libraries(minesweeper-tests PRIVATE
        minesweeper_console
        minesweeper_launcher
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(minesweeper-tests)

    # Architecture test: the dependency rule, checked like any other test.
    add_test(NAME architecture.dependency_rule
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}/src
                -P ${PROJECT_SOURCE_DIR}/tests/check_layers.cmake)
endif()
