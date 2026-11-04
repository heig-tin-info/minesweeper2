#pragma once

#include <chrono>
#include <optional>

#include "app/ports.hpp"

namespace minesweeper {

// Measures the duration of one game. It never reads a clock itself: the time
// is always passed in, which makes it a pure, trivially testable value.
class Stopwatch {
public:
    using TimePoint = Clock::TimePoint;

    void start(TimePoint t) {
        start_ = t;
        stop_.reset();
    }

    void stop(TimePoint t) {
        if (start_) stop_ = t;
    }

    std::chrono::seconds elapsed(TimePoint now) const {
        if (!start_) return {};
        return std::chrono::duration_cast<std::chrono::seconds>(stop_.value_or(now) - *start_);
    }

private:
    std::optional<TimePoint> start_;
    std::optional<TimePoint> stop_;
};

}  // namespace minesweeper
