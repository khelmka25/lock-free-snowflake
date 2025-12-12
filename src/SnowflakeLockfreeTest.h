#pragma once

#include <mutex>
#include <thread>

#include "SnowflakeTest.h"

namespace lockless {

namespace v1 {
// Lockless Implementation
inline std::atomic<std::uint64_t> global_time;
inline std::atomic<std::uint64_t> sequence;
inline std::atomic_bool time_update_flag;

inline std::uint64_t get(std::uint64_t machine_id) noexcept {
  // idea: for this thread, get the expected id
  std::uint64_t local_id = 0ull, expected_sequence;
  std::uint64_t local_time = utils::millis();

  bool expected_state = false;

  // check that the time has not changed
  if (local_time != global_time) {
    // clock_reset:

    if (!time_update_flag.compare_exchange_strong(expected_state, true,
                                                  std::memory_order_seq_cst)) {
      /*if the flag has already been set*/
      // expected_state == true
      while (global_time == local_time) {
        std::this_thread::yield();
      }

      if (sequence.compare_exchange_strong(local_id, 1ull,
                                           std::memory_order_seq_cst)) {
        /* If we are the first to reset the sequenceId */
        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, 1ull);
      } else {
        /* We are not the first thread to reset, get retrieve the current value
         * and increment */
        local_id = sequence.fetch_add(1, std::memory_order_seq_cst);
        // if (local_id > 4095ull) {
        //     expected_state = false;
        //     goto clock_reset;
        // }
        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id,
                              local_id);
      }
    } else {
      /*flag has not been set and we are the first to set it*/
      while (global_time == utils::millis()) {
        std::this_thread::yield();
      }

      global_time = utils::millis();

      expected_sequence = 4096ull;
      if (sequence.compare_exchange_strong(expected_sequence, 0ull)) {
        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id, 0ull);
      } else {
        local_id = sequence.fetch_add(1ull);
        return MAKE_SNOWFLAKE(local_time - utils::epoch(), machine_id,
                              local_id);
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
}  // namespace v1

namespace v2a {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

inline u64 get(u64 machine_id) noexcept {
  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // order after a write operation
  std::uint64_t current_id = atm_sequence.load(std::memory_order_acquire);
  // // extract timestamp
  std::uint64_t timestamp = current_id >> 12;
  // overflow has occured if the timestamp is greater than the timestamp
  // wait until the time has caught up
  if (timestamp > utils::millis()) {
    return 0ull;
  }

  std::uint64_t local_id = (current_id & 0xfff) | (utils::millis() << 12);
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
}  // namespace v2a

namespace v2b {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

inline u64 get(u64 machine_id) noexcept {
  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // order after a write operation
  u64 current_id = atm_sequence.load(std::memory_order_acquire);
  // overflow has occured if the timestamp is greater than the timestamp
  // wait until the time has caught up
  u64 timestamp = utils::millis();
  while (((current_id & (~0xfffull)) >> 12) > timestamp) {
    timestamp = utils::millis();
    current_id = atm_sequence.load(std::memory_order_relaxed);
    std::this_thread::yield();
  }

  u64 local_id = (current_id & 0xfff) | (timestamp << 12);
  // attempt to increment
  // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
  if (!atm_sequence.compare_exchange_strong(current_id, local_id + 1ull,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }
  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE(timestamp, machine_id, local_id);
}
}  // namespace v2b

namespace v3 {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

#define MAKE_SNOWFLAKE_V3(time_point, machine_id, sequence_id)           \
  (((time_point) & 0x01FF'FFFF'FFFF) | (((machine_id) & 0x03FF) << 41) | \
   (((sequence_id) & 0xffff'0000'0000'0000ull) << 4))

u64 get(u64 machine_id) {
  // sequence is stored in the following format:
  // |--- 16 bit id sequence ----|-------- 48 bit timestamp [ms] ----|
  // |- 4 bit pad -|- 12 bit id -|-------- 48 bit timestamp [ms] ----|

  // order after a write operation
  u64 current_id = atm_sequence.load(std::memory_order_acquire);
  // // extract timestamp
  u64 current_timestamp = current_id & (0xffff'ffff'ffffull);
  // overflow has occured if the timestamp is greater than the timestamp
  // wait until the time has caught up
  u64 timestamp = utils::millis();

  if (current_timestamp > timestamp) {
    return 0ull;
  }

  u64 local_id = (current_id & 0xffff'0000'0000'0000ull) | (timestamp);
  // attempt to increment
  // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
  if (!atm_sequence.compare_exchange_strong(current_id, local_id + (1ull << 48),
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }

  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE_V3(timestamp, machine_id, local_id);
}

}  // namespace v3

}  // namespace lockless