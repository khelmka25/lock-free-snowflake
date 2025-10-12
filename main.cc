#include <iostream>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <format>
#include <chrono>

#include "src/snowflake.h"

auto main() -> int {

    // main is a test for the snowflake generation
    const std::size_t kThreadCount = 8ull;
    constexpr std::size_t kIterationCount = 65535;

    std::array<std::unordered_map<std::size_t, std::size_t>, kThreadCount> idMaps;

    std::array<std::chrono::nanoseconds, kThreadCount> elapsed_times;
    {
        std::array<std::jthread, kThreadCount> jThreadPool;

        // spawn the threads
        for (auto i = 0ull; i < kThreadCount; i++) {
            // spawn a thread
            jThreadPool[i] = std::move(std::jthread([&thisMap = idMaps[i], &elapsed_time = elapsed_times[i]]() mutable -> void {
                // run lockless algorithm
                const auto start = std::chrono::system_clock::now();
                for (auto i = 0ull; i < kIterationCount; i++) {
                    auto result = lockless::getSnowflake(0ull);
                    thisMap[result]++;
                }
                const auto end = std::chrono::system_clock::now();
                elapsed_time = end - start;
            }));
        }
    }

    std::chrono::nanoseconds thread_runtime_ns{};
    for (std::size_t i = 0ull; i < kThreadCount; i++) {
        thread_runtime_ns += elapsed_times[i];
    }

    std::size_t avg_runtime_ms = thread_runtime_ns.count() / 1'000'000ull / kThreadCount;

    std::cout << std::format("Runtime: {} ms", avg_runtime_ms) << std::endl;

    // join into a single map
    std::unordered_map<std::size_t, std::size_t> values;

    for (auto& map : idMaps) {
        for (auto [key, count] : map) {
            values[key] += count;
        }
    }

    std::cout << std::format("# Unique values: {} (max: {})", values.size(), kThreadCount * kIterationCount) << std::endl;

    float idRate = (float)(values.size()) / ((float)(avg_runtime_ms) / 1000.f);
    std::cout << std::format("Avg # unique ids/ms {} (max: {})", idRate, 4'096'000) << std::endl;

    for (auto [key, count] : values) {
        if (count > 1ull) {
            std::cout << std::format("duplicate: {}; count: {}", key, count) << std::endl;
        }
    }

    return 0;
}