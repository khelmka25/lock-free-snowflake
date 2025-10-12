#pragma once

#include <chrono>

namespace utils {
    inline constexpr std::size_t epoch() noexcept {
        // epoch: Jan 1st, 2025, 00:00:00 UTC
        return 1'735'711'200'000ull;
    }

    inline std::size_t millis() noexcept {
        std::chrono::steady_clock::duration local_time = std::chrono::steady_clock::now().time_since_epoch();
        return std::bit_cast<std::size_t>(local_time);
    }
}