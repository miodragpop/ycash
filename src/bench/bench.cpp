// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2020-2023 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#include "bench.h"

#include "perf.h"

#include <assert.h>
#include <iostream>
#include <iomanip>

benchmark::BenchRunner::BenchmarkMap &benchmark::BenchRunner::benchmarks() {
    static std::map<std::string, benchmark::BenchFunction> benchmarks_map;
    return benchmarks_map;
}

benchmark::BenchRunner::BenchRunner(std::string name, benchmark::BenchFunction func)
{
    benchmarks().insert(std::make_pair(name, func));
}

void
benchmark::BenchRunner::RunAll(benchmark::duration elapsedTimeForOne)
{
    perf_init();
    if (std::ratio_less_equal<benchmark::clock::period, std::micro>::value) {
        std::cerr << "WARNING: Clock precision is worse than microsecond - benchmarks may be less accurate!\n";
    }
    // Column layout shared with State::KeepRunning() below (keep widths in sync).
    std::cout << std::left  << std::setw(30) << "Benchmark"
              << std::right << std::setw(10) << "evals"
              << std::setw(12) << "min(ns)"
              << std::setw(12) << "max(ns)"
              << std::setw(12) << "avg(ns)"
              << std::setw(14) << "min(cyc)"
              << std::setw(14) << "max(cyc)"
              << std::setw(14) << "avg(cyc)"
              << "\n";
    std::cout << std::string(118, '-') << "\n";

    for (const auto &p: benchmarks()) {
        State state(p.first, elapsedTimeForOne);
        p.second(state);
    }
    perf_fini();
}

bool benchmark::State::KeepRunning()
{
    if (count & countMask) {
      ++count;
      return true;
    }
    time_point now;

    uint64_t nowCycles;
    if (count == 0) {
        lastTime = beginTime = now = clock::now();
        lastCycles = beginCycles = nowCycles = perf_cpucycles();
    }
    else {
        now = clock::now();
        auto elapsed = now - lastTime;
        auto elapsedOne = elapsed / (countMask + 1);
        if (elapsedOne < minTime) minTime = elapsedOne;
        if (elapsedOne > maxTime) maxTime = elapsedOne;

        // We only use relative values, so don't have to handle 64-bit wrap-around specially
        nowCycles = perf_cpucycles();
        uint64_t elapsedOneCycles = (nowCycles - lastCycles) / (countMask + 1);
        if (elapsedOneCycles < minCycles) minCycles = elapsedOneCycles;
        if (elapsedOneCycles > maxCycles) maxCycles = elapsedOneCycles;

        if (elapsed*128 < maxElapsed) {
          // If the execution was much too fast (1/128th of maxElapsed), increase the count mask by 8x and restart timing.
          // The restart avoids including the overhead of this code in the measurement.
          countMask = ((countMask<<3)|7) & ((1LL<<60)-1);
          count = 0;
          minTime = duration::max();
          maxTime = duration::zero();
          minCycles = std::numeric_limits<uint64_t>::max();
          maxCycles = std::numeric_limits<uint64_t>::min();
          return true;
        }
        if (elapsed*16 < maxElapsed) {
          uint64_t newCountMask = ((countMask<<1)|1) & ((1LL<<60)-1);
          if ((count & newCountMask)==0) {
              countMask = newCountMask;
          }
        }
    }
    lastTime = now;
    lastCycles = nowCycles;
    ++count;

    if (now - beginTime < maxElapsed) return true; // Keep going

    --count;

    assert(count != 0 && "count == 0 => (now == 0 && beginTime == 0) => return above");

    // Output results
    // Duration casts are only necessary here because hardware with sub-nanosecond clocks
    // will lose precision.
    int64_t min_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(minTime).count();
    int64_t max_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(maxTime).count();
    int64_t avg_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>((now-beginTime)/count).count();
    int64_t averageCycles = (nowCycles-beginCycles)/count;
    // Fixed-width aligned columns (widths must match the header in RunAll()).
    std::cout << std::left  << std::setw(30) << name
              << std::right << std::setw(10) << count
              << std::setw(12) << min_elapsed
              << std::setw(12) << max_elapsed
              << std::setw(12) << avg_elapsed
              << std::setw(14) << minCycles
              << std::setw(14) << maxCycles
              << std::setw(14) << averageCycles
              << "\n";
    std::cout.copyfmt(std::ios(nullptr));

    return false;
}
