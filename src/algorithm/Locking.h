#pragma once

#include <mutex>

#include "ISnowflakeTest.h"

namespace locking {
namespace v1 {
// data
inline std::mutex lockingMutex;
// previous millis is shared among threads
inline std::uint64_t global_time;
// sequenceId_ is shared among threads
inline std::uint64_t sequence;

inline std::uint64_t get(std::uint64_t machine_id) noexcept {
  std::uint64_t local_id;
  std::uint64_t local_time = utils::millis();

  {
    // begin critical section: only one thread at a time
    std::scoped_lock<std::mutex> lock(lockingMutex);

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
}  // namespace v1
}  // namespace locking
