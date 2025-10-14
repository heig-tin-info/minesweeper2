#pragma once

// Minimal wall-clock timer used by `GameCore` to track how long the current
// session has been running. Keeping it header-only avoids an extra translation
// unit while showcasing basic chrono usage to students.
#include <chrono>

class Stopwatch {
   public:
    // Capture the starting time. Called whenever a new game begins.
    void start() { start_ = std::chrono::steady_clock::now(); }

    // Returns the number of whole seconds that have elapsed since the last
    // `start()`. We lean on `steady_clock` to avoid issues with wall-clock
    // adjustments.
    int elapsed() const {
        using namespace std::chrono;
        return static_cast<int>(
            duration_cast<seconds>(steady_clock::now() - start_).count());
    }

   private:
    // Anchor point captured at the last call to `start`.
    std::chrono::steady_clock::time_point start_{
        std::chrono::steady_clock::now()};
};
