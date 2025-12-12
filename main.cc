#include <argh.h>

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "src/SnowflakeLockfreeTest.h"
#include "src/SnowflakeLockingTest.h"
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
  cmdl.parse(argv, argc);

  if (cmdl[{"-h", "--help"}]) {
    std::cout << "Options List:\n";
    std::cout << "-h --help                  Help\n";
    std::cout << "-t --threads <n>           Specify number of threads to use, "
                 "default: 4\n";
    std::cout << "-i --iterations <n>        Specify number of iterations per "
                 "thread, default: 1024\n";
    std::cout << "-lf --lockfree             Use lock free algorithm, default: "
                 "use locking\n";
    return 0;
  }

  /* Settings */
  bool useLockfree = false;
  if (cmdl[{"-lf", "--lockfree "}]) {
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

  if (useLockfree) {
    using namespace std::literals::string_view_literals;
    std::initializer_list<std::unique_ptr<ISnowflakeTest>> tests = {
        // std::make_unique<SnowFlakeTest<lockless::v1::get>>(
        //     "lockless::v1::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockless::v2a::get>>(
            "lockless::v2a::get"sv, threadCount, iterationCount),
        std::make_unique<SnowFlakeTest<lockless::v2b::get>>(
            "lockless::v2b::get"sv, threadCount, iterationCount),
        // std::make_unique<SnowFlakeTest<lockless::v3::get>>(
        //     "lockless::v3::get"sv, threadCount, iterationCount),
    };

    for (auto& test : tests) {
      test->runTest();
      test->runAnalysis();
    }
  } else {
    using namespace std::literals::string_view_literals;
    std::initializer_list<std::unique_ptr<ISnowflakeTest>> tests = {
        std::make_unique<SnowFlakeTest<locking::v1::get>>(
            "locking::v1::get"sv, threadCount, iterationCount),
    };

    for (auto& test : tests) {
      test->runTest();
      test->runAnalysis();
    }
  }

  return 0;
}