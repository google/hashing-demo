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

// FarmHash implementation, based on farmhashna::Hash64() from
// https://code.google.com/p/farmhash by Geoff Pike.

#ifndef HASHING_DEMO_FARMHASH_H
#define HASHING_DEMO_FARMHASH_H

#include <array>
#include <cstdint>
#include <utility>

#include "std_impl.h"

using std::uint64_t;

namespace hashing {

// HashCode class representing the FarmHash algorithm.
//
// Like most modern hash algorithms, FarmHash processes its input in
// large chunks (64 bytes, in this case). Since we are computing the
// hash "on-line", we have to buffer those chunks, as well as store
// the 56-byte mixing state. We don't want to copy all that data
// unnecessarily, but hash_value() takes and returns HashCode instances
// by value, so this class is simply a lightweight user-facing wrapper.
//
// The actual hash algorithm is implemented in two parts: a non-movable
// 'state' class containing the heavyweight algorithm state, and a
// lightweight 'proxy' class which holds a pointer to a state. 'proxy'
// can be passed by value because it is small, and it models the HashCode
// concept. It is 'proxy', rather than 'farmhash', that gets passed through
// the hash_value/hash_combine recursion.
class farmhash {
  size_t result_;

 public:
  using result_type = size_t;

  // Move only
  farmhash(const farmhash&) = delete;
  farmhash& operator=(const farmhash&) = delete;
  farmhash(farmhash&&) = default;
  farmhash& operator=(farmhash&&) = default;

  farmhash() = default;
  explicit farmhash(size_t result) : result_(result) {}

  template <typename... Ts>
  friend farmhash hash_combine(farmhash state, Ts... values);

  template <typename InputIterator>
  friend farmhash hash_combine_range(
      farmhash state, InputIterator begin, InputIterator end);

  operator result_type() { return result_; }

 private:
  class state;
  class proxy;
};

class farmhash::state {
  uint64_t x_;
  uint64_t y_;
  uint64_t z_;
  std::pair<uint64_t, uint64_t> v_;
  std::pair<uint64_t, uint64_t> w_;
  uint64_t buffer_[8];

 public:

  // Non-movable
  state(const state&) = delete;
  state& operator=(const state&) = delete;
  state(state&&) = delete;
  state& operator=(state&&) = delete;

  // We deliberately leave the state members uninitialized, because we
  // can avoid ever initializing them in the common case.
  state() {}

 private:
  friend class farmhash::proxy;

  static constexpr uint64_t kSeed = 81;

  // Some primes between 2^63 and 2^64 for various uses.
  static constexpr uint64_t k0 = 0xc3a5c85c97cb3127ULL;
  static constexpr uint64_t k1 = 0xb492b66fbe98f273ULL;
  static constexpr uint64_t k2 = 0x9ae16a3b2f90404fULL;

  // Misc. low-level hashing utilities.
  inline static uint64_t Fetch64(const unsigned char *p);
  inline static uint32_t Fetch32(const unsigned char *p);
  inline static uint64_t ShiftMix(uint64_t val);
  inline static uint64_t Rotate(uint64_t val, int shift);
  inline static uint64_t HashLen16(uint64_t u, uint64_t v, uint64_t mul);
  inline static uint64_t HashLen0to16(const unsigned char* s, size_t len);
  inline static uint64_t HashLen17to32(const unsigned char *s, size_t len);
  inline static uint64_t HashLen33to64(const unsigned char *s, size_t len);
  inline static std::pair<uint64_t, uint64_t> WeakHashLen32WithSeeds(
      const uint64_t s[], uint64_t a, uint64_t b);

  // Initializes the hash mixing state.
  // Precondition: all bytes of buffer_ have been populated,
  // and initialize() has not been called before.
  inline void initialize();

  // Mixes the current buffer contents into the mixing state.
  // Precondition: buffer_ must contain 64 bytes of data that
  // has not already been mixed, and initialize() must already have
  // been called.
  inline void mix();

  // Computes a final hash value from the current mixing state and buffer.
  // No methods except the destructor should be called after this.
  // 'len' indicates the amount of unmixed data in the buffer.
  // Precondition: initialize() has been called, and 0 < len <= 64.
  inline size_t final_mix(size_t len);
};

class farmhash::proxy {
  state* state_;

  // We store the following state variables here instead of in 'state' because
  // they play a major role in the algorithm's control-flow, so if the
  // optimizer knows their values (e.g. due to inlining and constant-folding),
  // it can eliminate many branches (and the corresponding unused code).
  // Making them members of proxy, which is passed by value, ensures that
  // they are always local to the current stack frame. This helps the
  // optimizer track their current values via purely local reasoning.

  // Points to the location in state_->buffer_ where the next byte of input
  // should be buffered. It ranges from buffer_ + 1 to buffer_ + 64, with
  // the sole exception that it points to buffer_ + 0 when no input has been
  // processed.
  unsigned char* buffer_next_;

  // Indicates whether state_->mix() has been called at least once (i.e.
  // the input is at least 65 bytes). This helps us ensure that initialize()
  // is only called once, and enables us to use a much cheaper finalization
  // step for inputs of 64 bytes or less.
  bool mixed_ = false;

 public:
  using result_type = size_t;

  // Move only
  proxy(const proxy&) = delete;
  proxy& operator=(const proxy&) = delete;
  proxy(proxy&&) = default;
  proxy& operator=(proxy&&) = default;

  // Constructs a proxy pointing to s. This constructor should be invoked
  // only once for a given argument; that plus the fact that proxy is
  // move-only ensures that there is only one proxy pointing to a given state.
  proxy(state* s)
    : state_(s),
      buffer_next_(reinterpret_cast<unsigned char*>(s->buffer_)) {}

  template <typename T, typename... Ts>
  friend std::enable_if_t<!std::is_uniquely_represented<T>::value,
                          proxy>
  hash_combine(proxy hash_code, const T& value, const Ts&... values) {
    return hash_combine(hash_value(hash_code, value), values...);
  }

  template <typename T, typename... Ts>
  friend std::enable_if_t<std::is_uniquely_represented<T>::value,
                          proxy>
  hash_combine(proxy hash_code, const T& value, const Ts&... values) {
    unsigned char const* bytes = reinterpret_cast<unsigned char const*>(&value);
    return hash_combine(hash_combine_range(
        std::move(hash_code), bytes, bytes + sizeof(value)), values...);
  }

  friend proxy hash_combine(proxy hash_code) { return hash_code; }

  template <typename InputIterator>
  // Avoid ambiguity with the following overload
  friend std::enable_if_t<
      !(std::is_contiguous_iterator<InputIterator>::value &&
        std::is_uniquely_represented<
            typename std::iterator_traits<InputIterator>::value_type>::value),
      proxy>
  hash_combine_range(proxy hash_code, InputIterator begin, InputIterator end) {
    while (begin != end) {
      using std::hash_value;
      hash_code = hash_value(std::move(hash_code), *begin);
      ++begin;
    }
    return hash_code;
  }

  template <typename InputIterator>
  friend std::enable_if_t<
      std::is_contiguous_iterator<InputIterator>::value &&
          std::is_uniquely_represented<
              typename std::iterator_traits<InputIterator>::value_type>::value,
      proxy>
  hash_combine_range(proxy hash_code, InputIterator begin, InputIterator end) {
    using std::adl_pointer_from;
    const unsigned char* begin_ptr =
        reinterpret_cast<const unsigned char*>(adl_pointer_from(begin));
    const unsigned char* end_ptr =
        reinterpret_cast<const unsigned char*>(adl_pointer_from(end));
    return hash_combine_range(std::move(hash_code), begin_ptr, end_ptr);
  }

  friend proxy hash_combine_range(
      proxy hash_code, const unsigned char* begin, const unsigned char* end) {
    unsigned char* const buffer =
        reinterpret_cast <unsigned char*>(hash_code.state_->buffer_);
    const size_t buffer_remaining = buffer + 64 - hash_code.buffer_next_;
    if (end - begin <= buffer_remaining) {
      // The input will not saturate the buffer, so we just copy it.
      memcpy(hash_code.buffer_next_, begin, end - begin);
      hash_code.buffer_next_ += (end - begin);
    } else {
      // The input is large enough to saturate the buffer, so we have
      // to iteratively fill the buffer, and then mix it into the mixing
      // state.
      memcpy(hash_code.buffer_next_, begin, buffer_remaining);
      begin += buffer_remaining;
      if (!hash_code.mixed_) {
        hash_code.state_->initialize();
        hash_code.mixed_ = true;
      }
      hash_code.state_->mix();
      while (end - begin > 64) {
        memcpy(buffer, begin, 64);
        begin += 64;
        hash_code.state_->mix();
      }
      // Note that after this loop, the buffer always contains at least one
      // byte of unmixed input. The finalization step will rely on that.
      memcpy(buffer, begin, end - begin);
      hash_code.buffer_next_ = buffer + (end - begin);
    }
    return hash_code;
  }

  operator result_type() {
    const size_t len =
        buffer_next_ - reinterpret_cast<unsigned char*>(state_->buffer_);
    if (!mixed_) {
      // The buffer contains the entire input, so we can use special-case
      // logic for hashing short strings
      if (len <= 32) {
        if (len <= 16) {
          return state::HashLen0to16(
              reinterpret_cast<unsigned char*>(state_->buffer_), len);
        } else {
          return state::HashLen17to32(
              reinterpret_cast<unsigned char*>(state_->buffer_), len);
        }
      } else {
        return state::HashLen33to64(
            reinterpret_cast<unsigned char*>(state_->buffer_), len);
      }
    } else {
      // Note that 0 < len <= 64, due to the invariant of buffer_next_
      return state_->final_mix(len);
    }
  }
};

template <typename... Ts>
farmhash hash_combine(farmhash hash_code, Ts... values) {
  farmhash::state s;
  return farmhash(hash_combine(farmhash::proxy(&s), values...));
}

template <typename InputIterator>
farmhash hash_combine_range(
    farmhash hash_code, InputIterator begin, InputIterator end) {
  farmhash::state s;
  return farmhash(hash_combine_range(farmhash::proxy(&s), begin, end));
}

uint64_t farmhash::state::Fetch64(const unsigned char *p) {
  uint64_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

uint32_t farmhash::state::Fetch32(const unsigned char *p) {
  uint32_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

uint64_t farmhash::state::ShiftMix(uint64_t val) {
  return val ^ (val >> 47);
}

uint64_t farmhash::state::Rotate(uint64_t val, int shift) {
  // Avoid shifting by 64: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

uint64_t farmhash::state::HashLen16(uint64_t u, uint64_t v, uint64_t mul) {
  // Murmur-inspired hashing.
  uint64_t a = (u ^ v) * mul;
  a ^= (a >> 47);
  uint64_t b = (v ^ a) * mul;
  b ^= (b >> 47);
  b *= mul;
  return b;
}

uint64_t farmhash::state::HashLen0to16(const unsigned char* s, size_t len) {
  if (len >= 8) {
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch64(s) + k2;
    uint64_t b = Fetch64(s + len - 8);
    uint64_t c = Rotate(b, 37) * mul + a;
    uint64_t d = (Rotate(a, 25) + b) * mul;
    return HashLen16(c, d, mul);
  }
  if (len >= 4) {
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch32(s);
    return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
  }
  if (len > 0) {
    uint8_t a = s[0];
    uint8_t b = s[len >> 1];
    uint8_t c = s[len - 1];
    uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
    uint32_t z = len + (static_cast<uint32_t>(c) << 2);
    return ShiftMix(y * k2 ^ z * k0) * k2;
  }
  return k2;
}

uint64_t farmhash::state::HashLen17to32(const unsigned char *s, size_t len) {
  uint64_t mul = k2 + len * 2;
  uint64_t a = Fetch64(s) * k1;
  uint64_t b = Fetch64(s + 8);
  uint64_t c = Fetch64(s + len - 8) * mul;
  uint64_t d = Fetch64(s + len - 16) * k2;
  return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                   a + Rotate(b + k2, 18) + c, mul);
}

uint64_t farmhash::state::HashLen33to64(const unsigned char *s, size_t len) {
  uint64_t mul = k2 + len * 2;
  uint64_t a = Fetch64(s) * k2;
  uint64_t b = Fetch64(s + 8);
  uint64_t c = Fetch64(s + len - 8) * mul;
  uint64_t d = Fetch64(s + len - 16) * k2;
  uint64_t y = Rotate(a + b, 43) + Rotate(c, 30) + d;
  uint64_t z = HashLen16(y, a + Rotate(b + k2, 18) + c, mul);
  uint64_t e = Fetch64(s + 16) * mul;
  uint64_t f = Fetch64(s + 24);
  uint64_t g = (y + Fetch64(s + len - 32)) * mul;
  uint64_t h = (z + Fetch64(s + len - 24)) * mul;
  return HashLen16(Rotate(e + f, 43) + Rotate(g, 30) + h,
                   e + Rotate(f + a, 18) + g, mul);
}

std::pair<uint64_t, uint64_t> farmhash::state::WeakHashLen32WithSeeds(
    const uint64_t s[], uint64_t a, uint64_t b) {
  a += s[0];
  b = Rotate(b + a + s[3], 21);
  uint64_t c = a;
  a += s[1];
  a += s[2];
  b += Rotate(a, 44);
  return {a + s[3], b + c};
}

void farmhash::state::initialize() {
  x_ = kSeed;
  y_ = kSeed * k1 + 113;
  z_ = ShiftMix(y_ * k2 + 113) * k2;
  v_ = {0, 0};
  w_ = {0, 0};
  x_ = x_ * k2 + buffer_[0];
}

void farmhash::state::mix() {
  x_ = Rotate(x_ + y_ + v_.first + buffer_[1], 37) * k1;
  y_ = Rotate(y_ + v_.second + buffer_[6], 42) * k1;
  x_ ^= w_.second;
  y_ += v_.first + buffer_[5];
  z_ = Rotate(z_ + w_.first, 33) * k1;
  v_ = WeakHashLen32WithSeeds(buffer_, v_.second * k1, x_ + w_.first);
  w_ = WeakHashLen32WithSeeds(buffer_ + 4, z_ + w_.second, y_ + buffer_[2]);
  std::swap(z_, x_);
}

size_t farmhash::state::final_mix(size_t len) {
  // FarmHash's final mix operates on the final 64 bytes of input,
  // in order. buffer_ holds the last 64 bytes, but because it
  // acts as a circular buffer, we have to rotate it to put them
  // in order.
  unsigned char* buffer_as_bytes = reinterpret_cast<unsigned char*>(buffer_);
  std::rotate(buffer_as_bytes, buffer_as_bytes + len, buffer_as_bytes + 64);

  uint64_t mul = k1 + ((z_ & 0xff) << 1);
  w_.first += ((len - 1) & 63);
  v_.first += w_.first;
  w_.first += v_.first;
  x_ = Rotate(x_ + y_ + v_.first + buffer_[1], 37) * mul;
  y_ = Rotate(y_ + v_.second + buffer_[6], 42) * mul;
  x_ ^= w_.second * 9;
  y_ += v_.first * 9 + buffer_[5];
  z_ = Rotate(z_ + w_.first, 33) * mul;
  v_ = WeakHashLen32WithSeeds(buffer_, v_.second * mul, x_ + w_.first);
  w_ = WeakHashLen32WithSeeds(buffer_ + 4, z_ + w_.second, y_ + buffer_[2]);
  std::swap(z_,x_);
  return HashLen16(
      HashLen16(v_.first, w_.first, mul) + ShiftMix(y_) * k0 + z_,
      HashLen16(v_.second, w_.second, mul) + x_,
      mul);
}

}  // namespace hashing

#endif  // HASHING_DEMO_FARMHASH_H
