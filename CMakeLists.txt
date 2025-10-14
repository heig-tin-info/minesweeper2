cmake_minimum_required(VERSION 3.16)

project(minesweeper LANGUAGES CXX)

option(MINESWEEPER_BUILD_SDL "Build the SDL2 front-end" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINESWEEPER_BUILD_TESTS "Build the GoogleTest-based unit test suite" ON)

set(PROJECT_HEADERS
    board.hpp
    minesweeper.hpp
    ports.hpp
    presenter.hpp
    stopwatch.hpp
    types.hpp
    ui-console.hpp
    ui-sdl.hpp
)

add_library(minesweeper_headers INTERFACE)
target_sources(minesweeper_headers INTERFACE ${PROJECT_HEADERS})
target_include_directories(minesweeper_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(minesweeper-console main.cpp)
target_link_libraries(minesweeper-console PRIVATE minesweeper_headers)
target_compile_definitions(minesweeper-console PRIVATE $<$<CONFIG:Debug>:DEBUG>)

if(MINESWEEPER_BUILD_SDL)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)

    add_executable(minesweeper-sdl main.cpp)
    target_link_libraries(minesweeper-sdl PRIVATE minesweeper_headers)
    target_compile_definitions(minesweeper-sdl PRIVATE SDL $<$<CONFIG:Debug>:DEBUG>)

    if(TARGET SDL2::SDL2)
        target_link_libraries(minesweeper-sdl PRIVATE SDL2::SDL2)
    endif()
    if(TARGET SDL2::SDL2main)
        target_link_libraries(minesweeper-sdl PRIVATE SDL2::SDL2main)
    endif()
    if(TARGET SDL2_ttf::SDL2_ttf)
        target_link_libraries(minesweeper-sdl PRIVATE SDL2_ttf::SDL2_ttf)
    endif()

    if(NOT TARGET SDL2::SDL2)
        target_include_directories(minesweeper-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(minesweeper-sdl PRIVATE ${SDL2_LIBRARIES})
        if(SDL2MAIN_LIBRARY)
            target_link_libraries(minesweeper-sdl PRIVATE ${SDL2MAIN_LIBRARY})
        endif()
    endif()

    if(NOT TARGET SDL2_ttf::SDL2_ttf)
        target_include_directories(minesweeper-sdl PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
        target_link_libraries(minesweeper-sdl PRIVATE ${SDL2_TTF_LIBRARIES})
    endif()
else()
    message(STATUS "SDL2 front-end disabled")
endif()

install(TARGETS minesweeper-console
        RUNTIME DESTINATION bin)

if(TARGET minesweeper-sdl)
    install(TARGETS minesweeper-sdl
            RUNTIME DESTINATION bin)
endif()

if(MINESWEEPER_BUILD_TESTS)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(minesweeper-tests
        tests/board_tests.cpp
        tests/gamecore_tests.cpp
    )
    target_link_libraries(minesweeper-tests PRIVATE minesweeper_headers GTest::gtest_main)
    target_compile_definitions(minesweeper-tests PRIVATE $<$<CONFIG:Debug>:DEBUG>)

    include(GoogleTest)
    gtest_discover_tests(minesweeper-tests)
endif()
