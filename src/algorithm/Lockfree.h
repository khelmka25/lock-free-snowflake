#pragma once

#include <atomic>
#include <thread>

#include "ISnowflakeTest.h"

namespace lockfree {

namespace v0 {
inline std::atomic<std::uint64_t> sequence(0);
inline std::uint64_t get(std::uint64_t machine_id) noexcept {
  (void)machine_id;
  return sequence.fetch_add(1ull, std::memory_order_relaxed);
}
}  // namespace v0

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
  std::uint64_t current_id = atm_sequence.load(std::memory_order_relaxed);
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
}  // namespace v2b

namespace v3a {
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
  return MAKE_SNOWFLAKE(timestamp - utils::epoch(), machine_id, local_id);
}
}  // namespace v3a

namespace v3b {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

inline u64 get(u64 machine_id) noexcept {
  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // order after a write operation
  u64 current_id = atm_sequence.load(std::memory_order_relaxed);
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
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }

  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE(timestamp - utils::epoch(), machine_id, local_id);
}
}  // namespace v3b

namespace v3c {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

inline u64 get(u64 machine_id) noexcept {
  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // order after a write operation
  u64 current_id = atm_sequence.load(std::memory_order_relaxed);
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
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }

  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE(timestamp - utils::epoch(), machine_id, local_id);
}
}  // namespace v3c

namespace v3d {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence;

inline u64 get(u64 machine_id) noexcept {
  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // order after a write operation
  u64 current_id, timestamp;
  do {
    // get the current id and timestamp
    current_id = atm_sequence.load(std::memory_order_relaxed);
    timestamp = (utils::millis());
    // only loop while the timestamp is outdated
  } while ((current_id >> 12) > (timestamp));

  // overflow has occured if the timestamp is greater than the timestamp
  // wait until the time has caught up
  u64 local_id = (current_id & 0xfff) | (timestamp << 12);
  // attempt to increment
  // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
  if (!atm_sequence.compare_exchange_strong(current_id, local_id + 1ull,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }

  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE(timestamp - utils::epoch(), machine_id, local_id);
}
}  // namespace v3d

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
  return MAKE_SNOWFLAKE_V3(timestamp - utils::epoch(), machine_id, local_id);
}

}  // namespace v3

namespace v4a {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence(0ull);

inline constexpr u64 kIdMask = (0xfffull);
inline constexpr u64 kTimestampMask = ~(kIdMask);

inline u64 get(u64 machine_id) noexcept {
  // v4a Goal: previous iterations did not reset the sequence if the millisecond
  // edge had been triggered

  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // acquire global sequence after any writes (includes id and timestamp)
  auto sequence = atm_sequence.load(std::memory_order_relaxed);
  // acquire most recent system time
  auto timestamp = utils::millis();

  /* Sequence's timestamp != system timestamp, one of the following has occured:
     1. Overflow of 12 bit max sequence (sequence timestamp > system timestamp)
     2. System timestamp has changed (sequence timestamp < system timestamp)
  */
  if ((sequence >> 12) != timestamp) {
    const auto reset_sequence = timestamp << 12;
    // if we can't reset the sequence, then we are too late, exit function
    if (!atm_sequence.compare_exchange_strong(sequence, reset_sequence + 1ull,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
      return 0ull;
    }
    // make snowflake of sequence_id = 0
    return MAKE_SNOWFLAKE_FAST(machine_id, reset_sequence);
  }

  // update with new timestamp
  u64 local_id = (sequence & kIdMask) | (timestamp << 12);
  // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
  if (!atm_sequence.compare_exchange_strong(sequence, local_id + 1ull,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }
  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE_FAST(machine_id, local_id);
}

}  // namespace v4a

namespace v4b {
using u64 = std::uint64_t;
inline std::atomic<u64> atm_sequence(0ull);

inline constexpr u64 kSequenceIdMask = (0xfffull);
inline constexpr u64 kSequenceTimestampMask = ~(kSequenceIdMask);

inline u64 get(u64 machine_id) noexcept {
  // v4a Goal: previous iterations did not reset the sequence if the millisecond
  // edge had been triggered

  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // acquire global sequence after any writes (includes id and timestamp)
  auto sequence = atm_sequence.load(std::memory_order_relaxed);
  const auto sequence_timestamp = sequence >> 12;
  // acquire most recent system time
  const auto system_timestamp = utils::millis();

  /* Sequence's timestamp != system timestamp, one of the following has occured:
     1. Overflow of 12 bit max sequence (sequence timestamp > system timestamp)
     2. System timestamp has changed (sequence timestamp < system timestamp)
  */
  if ((sequence >> 12) != system_timestamp) {
    // case 1. overflow of 12 bit max sequence (unlikely as thread count grows)
    // the sequence timestamp is now greater than the system timestamp
    // we should wait until the next millisecond (just return from function)
    if (sequence_timestamp > system_timestamp) {
      return 0ull;
    }

    // case 2. attempt to reset the sequence to 0
    const auto reset_sequence = (system_timestamp << 12);
    // if we can't reset the sequence, then we are too late, exit function
    if (!atm_sequence.compare_exchange_strong(sequence, reset_sequence + 1ull,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
      return 0ull;
    }
    // make snowflake of sequence_id = 0
    return MAKE_SNOWFLAKE_FAST(machine_id, reset_sequence);
  }

  // update with new timestamp
  u64 local_id = (sequence & kSequenceIdMask) | (system_timestamp << 12);
  // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
  if (!atm_sequence.compare_exchange_strong(sequence, local_id + 1ull,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
    return 0ull;
  }
  // we own local_id, create snowflake
  return MAKE_SNOWFLAKE_FAST(machine_id, local_id);
}

}  // namespace v4b

}  // namespace lockfree