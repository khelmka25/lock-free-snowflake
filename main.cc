#include <iostream>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <format>
#include <chrono>
#include <string_view>

#include "src/ISnowflakeTest.h"
#include "src/ArgumentParser.h"
#include "src/SnowflakeLockfreeTest.h"
#include "src/SnowflakeLockingTest.h"

/*main is a test for the snowflake generation*/
auto main(int argc, char** argv) -> int {
    (void)argc;
    (void)argv;

    /*Settings*/
    std::size_t threadCount = 4ull;
    std::size_t iterationCount = 1024ull;
    bool useLockfree = false;

    {
        ArgumentParser parser{};
        parser.registerHandler({ "-h", "--help" }, [](std::string& args) {
            (void)args;
            std::cout <<
                "Options List:\n"
                "-h -help                   Help\n"
                "-t --threads <n>           Specify number of threads to use, default: 4\n"
                "-i --iterations <n>        Specify number of iterations per thread, default: 1024\n"
                "-lf --lockfree             Use lock free algorithm, default: use locking\n"
                << std::endl;
        }, OptionType::kHelper);

        parser.registerHandler({ "-t", "--threads" }, [&threadCount](std::string& args) {
            threadCount = std::stoull(args);
        }, OptionType::kRequired);

        parser.registerHandler({ "-i", "--iterations" }, [&iterationCount](std::string& args) {
            iterationCount = std::stoull(args);
        }, OptionType::kRequired);

        parser.registerHandler({ "-lf", "--lockfree" }, [&useLockfree](std::string& args) {
            (void)args;
            useLockfree = true;
        }, OptionType::kOptional);

        auto results = parser.parse(argv, argc);

        if (results != ParseResults::kOkay) {
            return 0;
        }
    }

    std::unique_ptr<ISnowflakeTest> snowflakeTest;
    if (useLockfree) {
        snowflakeTest = std::make_unique<SnowflakeLockfreeTest>(threadCount, iterationCount);
    }
    else {
        snowflakeTest = std::make_unique<SnowflakeLockingTest>(threadCount, iterationCount);
    }

    snowflakeTest->runTest();
    snowflakeTest->runAnalysis();

    return 0;
}