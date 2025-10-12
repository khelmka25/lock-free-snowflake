#include "snowflake.h"

#include "utils.h"

#ifndef MAKE_SNOWFLAKE

// ((time_point) & 0x01FF'FFFF'FFFF) | ((machine_id & 0x03FF) << 41) | ((sequence_id & 0x0FFF) << 51);
#define MAKE_SNOWFLAKE(time_point, machine_id, sequence_id) \
    ( (((time_point)) & 0x01FF'FFFF'FFFF) \
    | (((machine_id) & 0x03FF) << 41) \
    | (((sequence_id) & 0x0FFF) << 51) )
#endif

namespace locking {
    // Locking
    std::size_t getSnowflake(std::size_t machine_id) noexcept {
        // previous millis is shared among threads
        static std::size_t global_time;
        // sequenceId_ is shared among threads
        static std::size_t sequence;
        std::size_t local_sequence;
        std::size_t local_time;

        {
            // begin critical section
            std::lock_guard<std::mutex> lock(lockingMutex);

            do { // loop if sequenceId_ is 4096 or greater until next milli second
                local_time = utils::millis();
            } while ((sequence > 4095ull) && global_time == local_time);

            global_time = local_time;

            // if there is a 1 millisecond difference between the timestamps, then we subtract
            // if sequencId_ is equal to 4096, reset the counter to 0
            if (sequence == 4096ull) {
                sequence = 0ull;
            }

            local_sequence = sequence++;
            // end critical section
        }

        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_sequence);
    }
}


namespace lockless {
    // Lockless Implementation
    std::atomic<std::size_t> global_time;
    std::atomic<std::size_t> sequence;
    std::atomic_bool time_update_flag;

    std::size_t getSnowflake(std::size_t machine_id) noexcept {

        // idea: for this thread, get the expected id
        std::size_t local_id = 0ull, expected_sequence;
        std::size_t local_time = utils::millis();

        bool expected_state = false;

        // check that the time has not changed
        if (local_time != global_time) {

        // clock_reset:

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
                    //     expected_state = false;
                    //     goto clock_reset;
                    // }
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
                }
            }
            else {
                /*flag has not been set and we are the first to set it*/
                while (global_time == utils::millis()) {
                    std::this_thread::yield();
                }

                global_time = utils::millis();

                expected_sequence = 4096ull;
                if (sequence.compare_exchange_strong(expected_sequence, 0ull)) {
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, 0ull);
                }
                else {
                    local_id = sequence.fetch_add(1ull);
                    return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
                }
            }
        }

        // continue as normal:
        local_id = sequence.fetch_add(1ull);
        
        // TODO: if the local_id is invalid
        // if (local_id > 4095ull) {
        //     // if the id is greater than 4095
        //     // first check that the id is not already 0

        //     // assume false, but could be in a time change as well (true)
        //     expected_state = true;
        //     goto clock_reset;
        // }

        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, local_id);
    }
}

#undef MAKE_SNOWFLAKE