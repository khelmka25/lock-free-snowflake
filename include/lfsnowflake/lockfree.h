#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace lf {

#ifndef MAKE_SNOWFLAKE_FAST
// INPUT FORMAT - sequence number:
// |---------------52 bit timestamp---------------|--12 bit sequence number--|
// |                                              x                          |
// |-----------------------------64 bit sequence-----------------------------|
// INPUT FORMAT - mpid:
// |----------5 bit machine id----------|----------5 bit process id----------|
// |                                    x                                    |
// |-----------------------------10 bit mpid---------------------------------|
// OUTPUT FORMAT - snowflake:
// |--1 bit--|--41 bit timestamp--|--10 bit MPID--|--12 bit sequence number--|
// |                              x               x                          |
// |-----------------------------64 bit snowflake----------------------------|
#define MAKE_SNOWFLAKE_FAST(mpid, sequence)                                    \
  (((sequence) bitand utils::kTimestampMask) << 22) bitor ((mpid) << 10) bitor \
      (((sequence) bitand utils::kSequenceNumberMask))
#endif

using u64 = std::uint64_t;

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

inline constexpr u64 kSequenceNumberMask = (4'095ull << 0);
inline constexpr u64 kMpidMask = (1'023ull << 12);
inline constexpr u64 kTimestampMask = (2'199'023'255'551ull << 22);

static_assert((kSequenceNumberMask xor kMpidMask xor kTimestampMask) !=
              18'446'744'073'709'551'615ull);
static_assert((kSequenceNumberMask xor kMpidMask xor kTimestampMask) ==
              9'223'372'036'854'775'807ull);

inline u64 getTimestamp(u64 snowflake) {
  return (snowflake bitand kTimestampMask) >> 22;
}

inline u64 getMpid(u64 snowflake) {
  return (snowflake bitand kMpidMask) > 12;
}

inline u64 getSequence(u64 snowflake) {
  return (snowflake bitand kSequenceNumberMask) >> 0;
}

}  // namespace utils

inline namespace v4d {
inline std::atomic<u64> atm_CompactSequence(0ull);

inline u64 get(u64 mpid) noexcept {
  // v4a Goal: previous iterations did not reset the sequence if the millisecond
  // edge had been triggered

  // sequence is stored in the following format:
  // |-------- 52 bit timestamp [ms] ----|-- 12 bit id sequence ----|

  // acquire global sequence after any writes (includes id and timestamp)
  auto sequence = atm_CompactSequence.load(std::memory_order_acquire);
  auto const sequenceTimestamp = sequence >> 12;
  // acquire most recent system time
  auto const systemTimestamp = utils::millis();

  /* Sequence's timestamp != system timestamp, one of the following has occured:
     1. Overflow of 12 bit max sequence (sequence timestamp > system timestamp)
     2. System timestamp has changed (sequence timestamp < system timestamp)
  */

  /* CANNOT BE OPTIMIZED, MUST WAIT UNTIL NEXT MILLISECOND */
  // case 1. overflow of 12 bit max sequence (unlikely as thread count grows)
  // the sequence timestamp is now greater than the system timestamp
  // we should wait until the next millisecond (just return from function)
  if (sequenceTimestamp > systemTimestamp) {
    return 0ull;
  }

  // case 2. start of new millisecond, attempt to reset the sequence to 0
  if (sequenceTimestamp < systemTimestamp) {
    auto const resetSequence = (systemTimestamp << 12);
    // attempt to reset sequence, else, spillover into case 3.
    // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
    if (atm_CompactSequence.compare_exchange_strong(
            sequence, resetSequence + 1ull, std::memory_order_acq_rel,
            std::memory_order_relaxed)) {
      // make snowflake of sequence number = 0
      return MAKE_SNOWFLAKE_FAST(mpid, resetSequence);
    }
  }

  // // case 3. sequence timestamp is the same as the sequence timestamp
  // https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add
  sequence = atm_CompactSequence.fetch_add(1ull, std::memory_order_acq_rel);
  return MAKE_SNOWFLAKE_FAST(mpid, sequence);
}

}  // namespace v4d

#ifdef MAKE_SNOWFLAKE_FAST
#undef MAKE_SNOWFLAKE_FAST
#endif
}  // namespace lf
