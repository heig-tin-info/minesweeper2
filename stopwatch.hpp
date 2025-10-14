#pragma once

#include <chrono>

/**
 * Simple stopwatch utility to measure elapsed time in seconds.
 */
class Stopwatch {
   public:
    void start() { start_ = std::chrono::steady_clock::now(); }
    int elapsed() const {
        using namespace std::chrono;
        return (int)std::chrono::duration_cast<seconds>(
            steady_clock::now() - start_).count();
    }

   private:
    std::chrono::steady_clock::time_point start_{
        std::chrono::steady_clock::now()};
};