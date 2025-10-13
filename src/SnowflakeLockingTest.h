#pragma once

#include <thread>
#include <mutex>

#include "ISnowflakeTest.h"

class SnowflakeLockingTest : public ISnowflakeTest {
public:
    SnowflakeLockingTest(std::size_t t_threadCount, std::size_t t_iterationCount)
        : ISnowflakeTest(t_threadCount, t_iterationCount) {

        std::cout << "Running Locking Snowflake Algorithm..." << std::endl;
    }

    virtual void runTest() override {
        std::vector<std::jthread> jThreadPool;
        jThreadPool.resize(threadCount);

        for (auto i = 0ull; i < threadCount; i++) {

            idSets[i].resize(iterationCount);

            jThreadPool[i] = std::move(std::jthread([&thisSet = idSets[i], &elapsed_time = elapsed_times[i], tester = this]() -> void {
                const auto start = std::chrono::system_clock::now();
                for (auto i = 0ull; i < tester->iterationCount; i++) {
                    const auto result = tester->getV1(0ull);
                    thisSet[i] = result;
                }

                const auto end = std::chrono::system_clock::now();
                elapsed_time = end - start;
            }));
        }
    };

    virtual void runAnalysis() override {
        ISnowflakeTest::runAnalysis();
    }

private:
    std::mutex lockingMutex;

    std::size_t getV1(std::size_t machine_id) noexcept {
        // previous millis is shared among threads
        static std::size_t global_time;
        // sequenceId_ is shared among threads
        static std::size_t sequence;
        std::size_t local_id;
        std::size_t local_time = utils::millis();

        {
            // begin critical section: only one thread at a time
            std::lock_guard<std::mutex> lock(lockingMutex);

            local_id = sequence++;

            // if this threads time is newer than the last global time
            if (local_time > global_time) {
                global_time = local_time;
                // reset sequence
                sequence = 1ull;
                local_id = 0ull;
                // return a snowflake
            }

            // if the sequence is equal to 4096, but local_id is 4095 and valid 
            if (sequence > 4095ull) {
                // use global time as last timestamp
                // update this threads time until
                while (global_time <= local_time) {
                    global_time = utils::millis();
                }

                sequence = 1ull;
            }

            // end critical section
        }

        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
    }
};