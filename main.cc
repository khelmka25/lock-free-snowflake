#include <argh.h>

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "algorithm/Lockfree.h"
#include "algorithm/Locking.h"
#include "src/SnowflakeTest.h"

/*main is a test for the snowflake generation*/
auto main(int argc, char** argv) -> int {
  if (argc < 2) {
    std::string_view exe(argv[0]);
    exe.remove_prefix(std::min(exe.find_last_of('/'), std::size(exe)));
    std::cout << "Usage " << exe << " -h" << std::endl;
    return -1;
  }

  // https://github.com/adishavit/argh
  argh::parser cmdl{};
  cmdl.add_param({"-t"});
  cmdl.add_param({"-i"});
  cmdl.add_param({"-I"});
  cmdl.add_param({"-lf"});
  cmdl.parse(argv, argc);

  if (cmdl[{"-h", "--help"}]) {
    std::cout << "Options List:\n";
    std::cout << "-h     Help\n";
    std::cout << "-t <n> Number of threads to use, default: 4\n";
    std::cout << "-i <n> Number of iterations per thread, default: 1024\n";
    std::cout << "-I <n> Number of total iterations, default: 4096\n";
    std::cout << "-lf    Use lock free algorithm, default: use locking\n";
    return 0;
  }

  /* Settings */
  bool useLockfree = false;
  if (cmdl["lf"]) {
    useLockfree = true;
  }

  auto iterationCount = 1024ull;
  if (cmdl("i")) {
    cmdl("i") >> iterationCount;
  }

  auto threadCount = 4ull;
  if (cmdl("t")) {
    cmdl("t") >> threadCount;
  }

  if (cmdl("I")) {
    cmdl("I") >> iterationCount;
    iterationCount = iterationCount / threadCount;
  }

  if (useLockfree) {
    using namespace std::literals::string_view_literals;
    std::initializer_list<std::unique_ptr<ISnowflakeTest>> tests = {
        std::make_unique<SnowFlakeTest<lockfree::v0::get>>(
            "lockfree::v0a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v1::get>>(
            "lockfree::v1a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v2a::get>>(
            "lockfree::v2a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v2b::get>>(
            "lockfree::v2b::get"sv, threadCount, iterationCount),

        std::make_unique<SnowFlakeTest<lockfree::v3a::get>>(
            "lockfree::v3a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v3b::get>>(
            "lockfree::v3b::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v3c::get>>(
            "lockfree::v3c::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v3d::get>>(
            "lockfree::v3d::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v3::get>>(
            "lockfree::v3::get"sv, threadCount, iterationCount),

        // make sequence reset on new millisecond
        std::make_unique<SnowFlakeTest<lockfree::v4a::get>>(
            "lockfree::v4a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v4b::get>>(
            "lockfree::v4b::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v4c::get>>(
            "lockfree::v4c::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockfree::v4d::get>>(
            "lockfree::v4d::get"sv, threadCount, iterationCount),
    };

    for (auto& test : tests) {
      test->runTest();
      test->runAnalysis();
    }
  } else {
    using namespace std::literals::string_view_literals;
    std::initializer_list<std::unique_ptr<ISnowflakeTest>> tests = {
        std::make_unique<SnowFlakeTest<locking::v1::get>>("locking::v1::get"sv, threadCount, iterationCount),
    };

    for (auto& test : tests) {
      test->runTest();
      test->runAnalysis();
    }
  }

  return 0;
}