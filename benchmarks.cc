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

#include <algorithm>
#include <array>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "benchmark/benchmark.h"

#include "farmhash.h"
#include "farmhash-direct.h"
#include "n3980.h"
#include "n3980-farmhash.h"
#include "std.h"

static const int kNumBytes = 10'000'000;
static const std::array<unsigned char, kNumBytes>& Bytes() {
  static const std::array<unsigned char, kNumBytes> kBytes = [](){
    std::array<unsigned char, kNumBytes> bytes;
    std::independent_bits_engine<
      std::default_random_engine, 8, unsigned char> engine;
    std::generate(bytes.begin(), bytes.end(), engine);
    return bytes;
  }();
  return kBytes;
}

struct string_piece {
  string_piece(const unsigned char* begin, const unsigned char* end)
      : begin(reinterpret_cast<const char*>(begin)),
        end(reinterpret_cast<const char*>(end)) {}

  const char* begin;
  const char* end;
};

template <typename HashCode>
HashCode hash_decompose(HashCode code, string_piece str) {
  return hash_combine(
      hash_combine_range(std::move(code), str.begin, str.end),
      str.end - str.begin);
}

template <typename HashAlgorithm>
void hash_append(HashAlgorithm& h, string_piece str) {
  using std::hash_append;
  h(str.begin, str.end - str.begin);
  hash_append(h, str.end - str.begin);
}

struct farmhash_string_direct {
  size_t operator()(string_piece s) {
    return hashing::direct::farmhash::Hash64(s.begin, s.end - s.begin);
  }
};

template <class H>
static void BM_HashStrings(benchmark::State& state) {
  const std::array<unsigned char, kNumBytes>& bytes = Bytes();

  const int string_size = state.range_x();
  assert(string_size <= kNumBytes/10);

  int i = 0;
  H h;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(h(string_piece{&bytes[i], &bytes[i+string_size]}));
    i = (i + 1) % (kNumBytes - string_size);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                          string_size);
}

BENCHMARK_TEMPLATE(BM_HashStrings, farmhash_string_direct)
    ->Range(1, 1000 * 1000);

BENCHMARK_TEMPLATE(BM_HashStrings, std::hasher<string_piece, hashing::farmhash>)
    ->Range(1, 1000 * 1000);

BENCHMARK_TEMPLATE(BM_HashStrings, std::uhash<hashing::n3980::farmhash>)
    ->Range(1, 1000 * 1000);

BENCHMARK_MAIN();
