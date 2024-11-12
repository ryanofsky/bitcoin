// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BENCH_BENCH_BITCOIN_SETTINGS_H
#define BENCH_BENCH_BITCOIN_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

static const char* DEFAULT_BENCH_FILTER = ".*";
static constexpr int64_t DEFAULT_MIN_TIME_MS{10};
/** Priority level default value, run "all" priority levels */
static const std::string DEFAULT_PRIORITY{"all"};

using AsymptoteSetting = common::Setting<
    "-asymptote=<n1,n2,n3,...>", std::string, {.legacy = true},
    "Test asymptotic growth of the runtime of an algorithm, if supported by the benchmark">;

using FilterSetting = common::Setting<
    "-filter=<regex>", std::string, {.legacy = true},
    "Regular expression filter to select benchmark by name (default: %s)">
    ::DefaultFn<[] { return DEFAULT_BENCH_FILTER; }>;

using ListSetting = common::Setting<
    "-list", bool, {.legacy = true},
    "List benchmarks without executing them">;

using MinTimeSetting = common::Setting<
    "-min-time=<milliseconds>", int64_t, {.legacy = true, .disallow_negation = true},
    "Minimum runtime per benchmark, in milliseconds (default: %d)">
    ::Default<DEFAULT_MIN_TIME_MS>;

using OutputCsvSetting = common::Setting<
    "-output-csv=<output.csv>", fs::path, {.legacy = true},
    "Generate CSV file with the most important benchmark results">;

using OutputJsonSetting = common::Setting<
    "-output-json=<output.json>", fs::path, {.legacy = true},
    "Generate JSON file with all benchmark results">;

using SanityCheckSetting = common::Setting<
    "-sanity-check", bool, {.legacy = true},
    "Run benchmarks for only one iteration with no output">;

using PriorityLevelSetting = common::Setting<
    "-priority-level=<l1,l2,l3>", std::string, {.legacy = true},
    "Run benchmarks of one or multiple priority level(s) (%s), default: '%s'">
    ::DefaultFn<[] { return DEFAULT_PRIORITY; }>
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, benchmark::ListPriorities(), DEFAULT_PRIORITY); }>;

#endif // BENCH_BENCH_BITCOIN_SETTINGS_H
