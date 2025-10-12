#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>
#include <utility>
#include <thread>
#include <mutex>

namespace locking {
    inline std::mutex lockingMutex;
    extern std::size_t getSnowflake(std::size_t machineId_) noexcept;
}

namespace lockless {
    extern std::size_t getSnowflake(std::size_t machineId_) noexcept;
}

