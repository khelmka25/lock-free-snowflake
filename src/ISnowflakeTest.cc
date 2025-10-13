#include "ISnowflakeTest.h"

ISnowflakeTest::ISnowflakeTest(std::size_t t_threadCount, std::size_t t_iterationCount)
    : threadCount(t_threadCount)
    , iterationCount(t_iterationCount)
    , idCount(threadCount * iterationCount)
{
    idSets.resize(threadCount);
    elapsed_times.resize(threadCount);

    std::cout << std::format("Thread Count: {}\n Iterations: {}", threadCount, iterationCount) << std::endl;
}

void ISnowflakeTest::runAnalysis() {
    // join into a single map
    std::unordered_map<std::size_t, std::size_t> values;
    for (auto& set : idSets) {
        for (auto id : set) {
            values[id]++;
        }
    }

    for (auto [key, count] : values) {
        if (count > 1ull) {
            std::cout << std::format("duplicate: {}; count: {}", key, count) << std::endl;
        }
    }

    std::chrono::nanoseconds cpu_time_ns{};
    for (std::size_t i = 0ull; i < threadCount; i++) {
        cpu_time_ns += elapsed_times[i];
    }

    // average runtime for all the threads to complete kIterations
    float avg_thread_time_ms = float(cpu_time_ns.count() / 1'000'000ull) / float(threadCount);

    std::cout << std::format("Average Thread Time: {} ms", avg_thread_time_ms) << std::endl;

    std::cout << std::format("# Unique values: {} (max: {})", values.size(), idCount) << std::endl;
    const float idRate = (float)(values.size()) / ((float)(avg_thread_time_ms) / 1000.f);
    std::cout << std::format("Avg # unique ids/ms {} (max: {})", idRate, 4'096'000) << std::endl;
}