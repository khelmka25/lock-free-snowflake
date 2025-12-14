#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <thread>
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

    std::atomic_flag flag(false);
    std::atomic<std::uint64_t> counter(0);

    workspaces.resize(threadCount);
    for (auto& workspace : workspaces) {
      workspace.idSequence.resize(iterationCount);
      auto callable = [&flag, &counter](Workspace& workspace) -> void {
        // wait for thread synchronization
        counter.fetch_add(1ull, std::memory_order_acq_rel);
        flag.wait(false, std::memory_order_acquire);

        const auto begin = std::chrono::steady_clock::now();

        std::uint64_t val;
        for (auto i = 0ull; i < workspace.idSequence.size(); i++) {
          while (val = generator(0ull), val == 0ull) {
            std::this_thread::yield();
          }
          workspace.idSequence[i] = val;
        }

        const auto end = std::chrono::steady_clock::now();
        workspace.duration_ns = end - begin;
      };

      jThreadPool.emplace_back(callable, std::ref(workspace));
    }

    // synchronize threads
    while (counter.load(std::memory_order_acquire) != threadCount) {
      std::this_thread::yield();
    }
    flag.test_and_set(std::memory_order_release);
    flag.notify_all();
  };

  virtual void runAnalysis() override { ISnowflakeTest::runAnalysis(); }
};