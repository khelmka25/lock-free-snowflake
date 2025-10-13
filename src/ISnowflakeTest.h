#pragma once

#include <cstddef>
#include <format>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <iostream>

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

#ifndef MAKE_SNOWFLAKE

// ((time_point) & 0x01FF'FFFF'FFFF) | ((machine_id & 0x03FF) << 41) | ((sequence_id & 0x0FFF) << 51);
#define MAKE_SNOWFLAKE(time_point, machine_id, sequence_id) \
    ( (((time_point)) & 0x01FF'FFFF'FFFF) \
    | (((machine_id) & 0x03FF) << 41) \
    | (((sequence_id) & 0x0FFF) << 51) )

#endif

class ISnowflakeTest {
public:
    explicit ISnowflakeTest(std::size_t t_threadCount, std::size_t iterationCount);

    virtual void runTest() = 0;
    virtual void runAnalysis() = 0;

    virtual ~ISnowflakeTest() noexcept = default;

protected:
    std::size_t threadCount;
    std::size_t iterationCount;
    std::size_t idCount;

    std::vector<std::vector<std::size_t>> idSets;
    std::vector<std::chrono::nanoseconds> elapsed_times;
};