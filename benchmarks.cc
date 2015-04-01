// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>
#include <random>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

#include "farmhash.h"
#include "farmhash-direct.h"
#include "std.h"

template <class H>
static void BM_HashStrings(benchmark::State& state) {
  static const int kTotalStringBytes = 1000 * 1000;
  const int string_size = state.range_x();
  const int num_strings = kTotalStringBytes / string_size;

  std::vector<std::string> strings;
  strings.reserve(num_strings);

  std::default_random_engine rng;
  std::uniform_int_distribution<char> dist;
  for (int i = 0; i < num_strings; ++i) {
    strings.push_back({});
    strings.back().reserve(string_size);
    for (int j = 0; j < string_size; ++j) {
      strings.back().push_back(dist(rng));
    }
  }

  int i = 0;
  H h;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(h(strings[i++ % strings.size()]));
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          string_size);
}

BENCHMARK_TEMPLATE(BM_HashStrings, std::hash<std::string>)
    ->Range(1, 1000 * 1000);
BENCHMARK_TEMPLATE(BM_HashStrings, std::hasher<std::string, hashing::farmhash>)
    ->Range(100 * 1000, 1000 * 1000);

struct farmhash_string_direct {
  size_t operator()(const std::string& s) {
    return hashing::direct::farmhash::Hash64(s.data(), s.size());
  }
};

BENCHMARK_TEMPLATE(BM_HashStrings, farmhash_string_direct)
    ->Range(1, 1000 * 1000);

BENCHMARK_MAIN();
