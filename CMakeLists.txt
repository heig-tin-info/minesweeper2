cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Everything except main(): shared by the game and the tests.
add_library(minesweeper_lib STATIC
    src/game.cpp
    src/play.cpp
    src/random_layout.cpp
    src/console.cpp
)

add_executable(minesweeper src/main.cpp)
target_link_libraries(minesweeper PRIVATE minesweeper_lib)

enable_testing()
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE minesweeper_lib)
add_test(NAME tests COMMAND tests)
