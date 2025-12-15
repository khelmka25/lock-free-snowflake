#pragma once

#include <chrono>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace utils {

inline std::uint64_t millis() noexcept {
  std::chrono::milliseconds local_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch());
  return std::bit_cast<std::uint64_t>(local_time);
}

inline std::uint64_t epoch() noexcept {
  // epoch: Jan 1st, 2025, 00:00:00 UTC
  return 1'735'711'200'000ull;
}
}  // namespace utils

#ifndef MAKE_SNOWFLAKE

// ((time_point) & 0x01FF'FFFF'FFFF) | ((machine_id & 0x03FF) << 41) |
// ((sequence_id & 0x0FFF) << 51);
#define MAKE_SNOWFLAKE(time_point, machine_id, sequence_id)              \
 ((((machine_id) & 0x03FF) << 51) | (((time_point) & 0x01FF'FFFF'FFFF) << 12) | ((sequence_id) & 0x0FFF));
#endif

#define MAKE_SNOWFLAKE_FAST(machine_id, sequence)\
  ((machine_id << 54) | (sequence))

class ISnowflakeTest {
 public:
  explicit ISnowflakeTest(std::string_view t_name, std::uint64_t t_threadCount,
                          std::uint64_t t_iterationCount);

  virtual void runTest() = 0;
  virtual void runAnalysis() = 0;

  virtual ~ISnowflakeTest() noexcept = default;

 protected:
  static_assert(sizeof(std::uint64_t) == 8, "");

  std::string_view name;

  std::uint64_t threadCount;
  std::uint64_t iterationCount;

  struct Workspace {
    // sequence of unique ids for this thread
    std::vector<std::uint64_t> idSequence;
    // start and end times for this thread
    std::chrono::nanoseconds duration_ns;
  };

  std::vector<Workspace> workspaces;

 protected:
  class comma_numpunct : public std::numpunct<char> {
   protected:
    virtual char do_thousands_sep() const { return ','; }
    virtual std::string do_grouping() const { return "\03"; }
  };
};