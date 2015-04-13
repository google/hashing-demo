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

// Based on N3980's "X", but data_ is non-contiguous, in order to exercise
// a different part of the performance space.
struct X {
  std::tuple<short, unsigned char, unsigned char> date_;
  std::vector<std::pair<char, int>>                data_;
};

template <typename HashCode>
HashCode hash_decompose(HashCode code, const X& x) {
  return hash_combine(std::move(code), x.date_, x.data_);
}

template <typename HashAlgorithm>
void hash_append(HashAlgorithm& h, const X& x) {
  using std::hash_append;
  hash_append(h, x.date_, x.data_);
}

template <class H>
static void BM_HashX(benchmark::State& state) {
  static std::uniform_int_distribution<short> years(1915, 2015);
  static std::uniform_int_distribution<unsigned char> months(1, 12);
  static std::uniform_int_distribution<unsigned char> days(1, 28);
  static std::uniform_int_distribution<char> cvalue(1, 10);
  static std::uniform_int_distribution<int> ivalue(-3, 3);

  static std::default_random_engine engine;

  const int max_data_size = state.range_x();
  std::uniform_int_distribution<std::size_t> data_size(0, max_data_size);

  const int num_xs = kNumBytes/max_data_size;
  std::vector<X> xs(num_xs);

  for (X& x : xs) {
    x.date_ = {years(engine), months(engine), days(engine)};
    x.data_.resize(data_size(engine));
    for (auto& p : x.data_) {
      p = {cvalue(engine), ivalue(engine)};
    }
  }

  int i = 0;
  int64_t cumulative_vector_size = 0;
  H h;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(h(xs[i]));
    i = (i + 1) % xs.size();
    cumulative_vector_size += xs[i].data_.size();
  }
  state.SetBytesProcessed(
      static_cast<int64_t>(state.iterations()) * sizeof(X)
      + cumulative_vector_size * sizeof(std::pair<int, int>));
}

BENCHMARK_TEMPLATE(BM_HashX, std::hasher<X, hashing::farmhash>)
    ->Range(1, 1000 * 1000);

BENCHMARK_TEMPLATE(BM_HashX, std::uhash<hashing::n3980::farmhash>)
    ->Range(1, 1000 * 1000);

BENCHMARK_MAIN();
