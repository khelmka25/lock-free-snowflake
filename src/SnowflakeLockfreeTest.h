#pragma once

#include <thread>
#include <mutex>

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
    std::atomic<std::size_t> global_time;
    std::atomic<std::size_t> sequence;
    std::atomic_bool time_update_flag;
    std::atomic_bool sequence_update_flag;

    std::size_t getV1(std::size_t machine_id) noexcept {

        // idea: for this thread, get the expected id
        std::size_t local_id = 0ull, expected_sequence;
        std::size_t local_time = utils::millis();

        bool expected_state = false;

        // check that the time has not changed
    clock_reset:
        if (local_time <= global_time) {
            if (!time_update_flag.compare_exchange_strong(expected_state, true, std::memory_order_seq_cst)) {
                /*if the flag has already been set*/
                // expected_state == true
                while (global_time == local_time) {
                    std::this_thread::yield();
                }

                if (sequence.compare_exchange_strong(local_id, 1ull, std::memory_order_seq_cst)) {
                    /* If we are the first to reset the sequenceId */
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, 1ull);
                }
                else {
                    /* We are not the first thread to reset, get retrieve the current value and increment */
                    local_id = sequence.fetch_add(1, std::memory_order_seq_cst);
                    // if (local_id > 4095ull) {
                    //     goto sequence_reset;
                    // }

                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
                }
            }
            else {
                /*flag has not been set and we are the first to set it*/
                while (global_time >= utils::millis()) {
                    std::this_thread::yield();
                }

                global_time = utils::millis();

                expected_sequence = 4096ull;

                time_update_flag = false;

                if (sequence.compare_exchange_strong(expected_sequence, 0ull)) {
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, 0ull);
                }
                else {
                    local_id = sequence.fetch_add(1ull);
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
                }
            }
        }

        // sequence_reset:
            // check that a clock reset is not taking place


            // // continue as normal:
            // if (sequence < 4096ull) {
            //     local_id = sequence.fetch_add(1ull);


            // }


            // // TODO: if the local_id is invalid
            // if (local_id > 4095ull) {
            //     // if the id is greater han 4095, first check if the clock reset
            //     expected_state = false;
            //     goto clock_reset;
            // }

        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
    }
};