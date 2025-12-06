#pragma once

#include <mutex>
#include <thread>

#include "ISnowflakeTest.h"

class SnowflakeLockfreeTest : public ISnowflakeTest {
 public:
  SnowflakeLockfreeTest(std::size_t t_threadCount, std::size_t t_iterationCount)
      : ISnowflakeTest(t_threadCount, t_iterationCount) {
    std::cout << "Running Locking Snowflake Algorithm..." << std::endl;
  }

  virtual void runTest() override {
    std::vector<std::jthread> jThreadPool;
    jThreadPool.resize(threadCount);

    for (auto i = 0ull; i < threadCount; i++) {
      idSets[i].resize(iterationCount);

      jThreadPool[i] = std::move(
          std::jthread([&thisSet = idSets[i], &elapsed_time = elapsed_times[i],
                        tester = this]() -> void {
            const auto start = std::chrono::system_clock::now();
            std::size_t result = 0ul;
            ;
            for (auto i = 0ull; i < tester->iterationCount; i++) {
              while (result = tester->getV1(0ull), result == 0ull) {
                std::this_thread::yield();
              }
              thisSet[i] = result;
            }

            const auto end = std::chrono::system_clock::now();
            elapsed_time = end - start;
          }));
    }
  };

  virtual void runAnalysis() override { ISnowflakeTest::runAnalysis(); }

 private:
  std::atomic<std::size_t> atm_sequence;

  std::size_t getV1(std::size_t machine_id) noexcept {
    // sequence is stored in the following format:
    // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

    // order after a write operation
    std::size_t current_id = atm_sequence.load(std::memory_order_acquire);
    // // extract timestamp
    std::size_t timestamp = current_id >> 12;
    // overflow has occured if the timestamp is greater than the timestamp
    // wait until the time has caught up
    if (timestamp > utils::millis()) {
      return 0ull;
    }

    std::size_t local_id = (current_id & 0xfff) | (utils::millis() << 12);
    // attempt to increment
    // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
    if (!atm_sequence.compare_exchange_strong(current_id, local_id + 1ull,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
      return 0ull;
    }
    // we own local_id, create snowflake
    return MAKE_SNOWFLAKE(timestamp - utils::epoch(), machine_id, local_id);
  }
};