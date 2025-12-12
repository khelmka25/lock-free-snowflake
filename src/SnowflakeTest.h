#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ISnowflakeTest.h"

template <std::uint64_t (*generator)(std::uint64_t)>
struct SnowFlakeTest : ISnowflakeTest {
  SnowFlakeTest(std::string_view t_name, std::uint64_t t_threadCount,
                std::uint64_t t_iterationCount)
      : ISnowflakeTest(t_name, t_threadCount, t_iterationCount) {}

  virtual void runTest() override {
    std::vector<std::jthread> jThreadPool;

    std::cout << "Running Test: " << name << std::endl;

    for (auto i = 0ull; i < threadCount; i++) {
      idSets[i].resize(iterationCount);

      auto callable = [&ids = idSets[i]](std::chrono::nanoseconds& elapsed_time,
                                         std::uint64_t iterationCount) {
        const auto start = std::chrono::system_clock::now();
        std::uint64_t result = 0ull;
        for (auto i = 0ull; i < iterationCount; i++) {
          while (result = generator(0ull), result == 0ull) {
            std::this_thread::yield();
          }
          ids[i] = result;
        }

        const auto end = std::chrono::system_clock::now();
        elapsed_time = end - start;
      };

      jThreadPool.emplace_back(callable, std::ref(threadTime_ns[i]),
                               iterationCount);
    }
  };

  virtual void runAnalysis() override { ISnowflakeTest::runAnalysis(); }
};