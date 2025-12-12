#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace utils {
inline constexpr std::uint64_t epoch() noexcept {
  // epoch: Jan 1st, 2025, 00:00:00 UTC
  return 1'735'711'200'000ull;
}

inline std::uint64_t millis() noexcept {
  std::chrono::milliseconds local_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch());
  return std::bit_cast<std::uint64_t>(local_time);
}
}  // namespace utils

#ifndef MAKE_SNOWFLAKE

// ((time_point) & 0x01FF'FFFF'FFFF) | ((machine_id & 0x03FF) << 41) |
// ((sequence_id & 0x0FFF) << 51);
#define MAKE_SNOWFLAKE(time_point, machine_id, sequence_id)              \
  (((time_point) & 0x01FF'FFFF'FFFF) | (((machine_id) & 0x03FF) << 41) | \
   (((sequence_id) & 0x0FFF) << 51))

#endif

class ISnowflakeTest {
 public:
  explicit ISnowflakeTest(std::string_view t_name, std::uint64_t t_threadCount,
                          std::uint64_t t_iterationCount)
      : name(t_name),
        threadCount(t_threadCount),
        iterationCount(t_iterationCount) {
    idSets.resize(threadCount);
    threadTime_ns.resize(threadCount);
  }

  class comma_numpunct : public std::numpunct<char> {
   protected:
    virtual char do_thousands_sep() const { return ','; }

    virtual std::string do_grouping() const { return "\03"; }
  };

  virtual void runTest() = 0;
  virtual void runAnalysis() = 0;

  virtual ~ISnowflakeTest() noexcept = default;

 protected:
  static_assert(sizeof(std::uint64_t) == 8, "");
  
  std::string_view name;
  std::uint64_t threadCount;
  std::uint64_t iterationCount;

  std::vector<std::vector<std::uint64_t>> idSets;
  std::vector<std::chrono::nanoseconds> threadTime_ns;
};

// default function for runAnalysis
void ISnowflakeTest::runAnalysis() {
  // join into a single map
  std::unordered_map<std::uint64_t, std::uint64_t> values;
  for (auto& set : idSets) {
    for (auto id : set) {
      values[id]++;
    }
  }

  for (auto [key, count] : values) {
    if (count > 1ull) {
      std::cout << std::format("duplicate: {}; count: {}", key, count)
                << std::endl;
    }
  }

  std::locale comma_locale(std::locale(), new comma_numpunct());
  std::cout.imbue(comma_locale);

  std::chrono::nanoseconds totalThreadTime_ns{};
  for (std::uint64_t i = 0ull; i < threadCount; i++) {
    auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(threadTime_ns[i]);
    std::cout << std::format("thread {}: ", i) << elapsed << std::endl;
    totalThreadTime_ns += threadTime_ns[i];
  }

  auto totalThreadTime_us = float(totalThreadTime_ns.count()) / 1e6f;
  // average runtime for all the threads to complete kIterations
  std::chrono::nanoseconds avg_thread_time_ns =
      totalThreadTime_ns / threadCount;

  std::cout << "Total Thread Time: " << totalThreadTime_ns << std::endl;
  std::cout << "Avg Thread Time: " << avg_thread_time_ns << std::endl;

  const auto totalIdCount = iterationCount * threadCount;

  std::cout << "# values: " << values.size() << " (# expected: " << totalIdCount << ")\n";

  const float idRate = (float)values.size() / totalThreadTime_us;

  std::cout << "# ids/ms: " << std::fixed << std::setprecision(2) << idRate << std::endl;

  std::cout << std::endl;
}