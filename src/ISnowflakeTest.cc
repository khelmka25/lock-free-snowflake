#include "ISnowflakeTest.h"

#include <fstream>

ISnowflakeTest::ISnowflakeTest(std::string_view t_name,
                                        std::uint64_t t_threadCount,
                                        std::uint64_t t_iterationCount)
    : name(t_name),
      threadCount(t_threadCount),
      iterationCount(t_iterationCount) {}

// default function for runAnalysis
void ISnowflakeTest::runAnalysis() {
  // join into a single map
  std::unordered_map<std::uint64_t, std::uint64_t> values;
  for (const auto& workspace : workspaces) {
    for (const auto id : workspace.idSequence) {
      values[id]++;
    }
  }

  if (iterationCount <= 65535ull) {
    std::vector<std::uint64_t> out;
    for (auto [key, count] : values) {
      out.push_back(key);
    }
    std::sort(out.begin(), out.end());
    const auto minout = (out.front() & 0x01FF'FFFF'FFFF);

    for (auto [key, count] : values) {
      if (count != 1ull) {
        std::cout << "duplicate: " << (key - minout) << " count: " << count << std::endl;
      }
    }

    std::string filename(name);
    filename.append(".txt");
    std::ofstream file(filename);
    // extract first 48 bits only
    for (auto o : out) {
      auto output = (o - minout);
      auto timestamp = output >> 12;
      auto sequence = output & 0xfff;  

      file << timestamp << ',' << std::setw(4) << std::setfill('0') << sequence << std::endl;
    }
    file.close();
  }

  std::locale comma_locale(std::locale(), new comma_numpunct());
  std::cout.imbue(comma_locale);

  std::chrono::nanoseconds totalDuration_ns{};
  for (const auto& workspace : workspaces) {
    totalDuration_ns += workspace.duration_ns;
  }

  // average runtime for all the threads to complete kIterations
  double totalThreadTime_ns = double(totalDuration_ns.count());
  double averageThreadTime_ns = totalThreadTime_ns / (double)threadCount;

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Execution Time: " << totalThreadTime_ns << std::endl;
  std::cout << "Avg Thread Time: " << averageThreadTime_ns << std::endl;

  const auto totalIdCount = iterationCount * threadCount;

  std::cout << "ID Count: " << values.size() << "/" << totalIdCount;
  if (totalIdCount != values.size()) {
    std::cout << " [FAILED]";
  }
  std::cout << std::endl;

  std::cout << "# ids/ms: " << ((double)iterationCount / averageThreadTime_ns * 1e6) << std::endl;

  std::cout << "--------------------------------" << std::endl << std::endl;
}