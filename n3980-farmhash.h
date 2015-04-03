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

// N3980-based implementation of FarmHash, based on farmhashna::Hash64() from
// https://code.google.com/p/farmhash by Geoff Pike.

#ifndef HASHING_DEMO_N3980_FARMHASH_H
#define HASHING_DEMO_N3980_FARMHASH_H

#include <utility>

namespace hashing {
namespace n3980 {

class farmhash {
  uint64_t x_;
  uint64_t y_;
  uint64_t z_;
  std::pair<uint64_t, uint64_t> v_;
  std::pair<uint64_t, uint64_t> w_;

  uint64_t buffer_[8];
  unsigned char* buffer_next_;
  bool mixed_ = false;

 public:
  using result_type = size_t;

  farmhash()
      : buffer_next_(reinterpret_cast<unsigned char*>(&buffer_)) {}

  inline void operator()(const void* key, size_t length);
  inline explicit operator result_type();

 private:
  // Some primes between 2^63 and 2^64 for various uses.
  static constexpr uint64_t k0 = 0xc3a5c85c97cb3127ULL;
  static constexpr uint64_t k1 = 0xb492b66fbe98f273ULL;
  static constexpr uint64_t k2 = 0x9ae16a3b2f90404fULL;

  static constexpr uint64_t kSeed = 81;

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

  inline void initialize();
  inline void mix();
  inline size_t final_mix(size_t len);
};

void farmhash::operator()(const void* key, size_t length) {
  unsigned char* const buffer_bytes =
      reinterpret_cast<unsigned char*>(buffer_);
  const unsigned char* input_bytes =
      reinterpret_cast<const unsigned char*>(key);
  const size_t buffer_remaining = buffer_bytes + 64 - buffer_next_;
  if (length <= buffer_remaining) {
    memcpy(buffer_next_, key, length);
    buffer_next_ += length;
  } else {
    int bytes_processed = 0;
    memcpy(buffer_next_, key, buffer_remaining);
    bytes_processed += buffer_remaining;
    mix();
    while (length - bytes_processed > 64) {
      memcpy(buffer_, input_bytes + bytes_processed, 64);
      bytes_processed += 64;
      mix();
    }
    memcpy(buffer_, input_bytes + bytes_processed, length - bytes_processed);
    buffer_next_ = buffer_bytes + (length - bytes_processed);
  }
}

farmhash::operator result_type() {
  const size_t len =
      buffer_next_ - reinterpret_cast<unsigned char*>(buffer_);
  if (!mixed_) {
    if (len <= 32) {
      if (len <= 16) {
        return HashLen0to16(
            reinterpret_cast<unsigned char*>(buffer_), len);
      } else {
        return HashLen17to32(
            reinterpret_cast<unsigned char*>(buffer_), len);
      }
    } else {
      return HashLen33to64(
          reinterpret_cast<unsigned char*>(buffer_), len);
    }
  } else {
    return final_mix(len);
  }
}

uint64_t farmhash::Fetch64(const unsigned char *p) {
  uint64_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

uint32_t farmhash::Fetch32(const unsigned char *p) {
  uint32_t result;
  memcpy(&result, p, sizeof(result));
  return result;
}

uint64_t farmhash::ShiftMix(uint64_t val) {
  return val ^ (val >> 47);
}

uint64_t farmhash::Rotate(uint64_t val, int shift) {
  // Avoid shifting by 64: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

uint64_t farmhash::HashLen16(uint64_t u, uint64_t v, uint64_t mul) {
  // Murmur-inspired hashing.
  uint64_t a = (u ^ v) * mul;
  a ^= (a >> 47);
  uint64_t b = (v ^ a) * mul;
  b ^= (b >> 47);
  b *= mul;
  return b;
}

uint64_t farmhash::HashLen0to16(const unsigned char* s, size_t len) {
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

uint64_t farmhash::HashLen17to32(const unsigned char *s, size_t len) {
  uint64_t mul = k2 + len * 2;
  uint64_t a = Fetch64(s) * k1;
  uint64_t b = Fetch64(s + 8);
  uint64_t c = Fetch64(s + len - 8) * mul;
  uint64_t d = Fetch64(s + len - 16) * k2;
  return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                   a + Rotate(b + k2, 18) + c, mul);
}

uint64_t farmhash::HashLen33to64(const unsigned char *s, size_t len) {
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

std::pair<uint64_t, uint64_t> farmhash::WeakHashLen32WithSeeds(
    const uint64_t s[], uint64_t a, uint64_t b) {
  a += s[0];
  b = Rotate(b + a + s[3], 21);
  uint64_t c = a;
  a += s[1];
  a += s[2];
  b += Rotate(a, 44);
  return {a + s[3], b + c};
}

void farmhash::mix() {
  if (!mixed_) {
    x_ = kSeed;
    y_ = kSeed * k1 + 113;
    z_ = ShiftMix(y_ * k2 + 113) * k2;
    v_ = {0, 0};
    w_ = {0, 0};
    x_ = x_ * k2 + buffer_[0];
    mixed_ = true;
  }
  x_ = Rotate(x_ + y_ + v_.first + buffer_[1], 37) * k1;
  y_ = Rotate(y_ + v_.second + buffer_[6], 42) * k1;
  x_ ^= w_.second;
  y_ += v_.first + buffer_[5];
  z_ = Rotate(z_ + w_.first, 33) * k1;
  v_ = WeakHashLen32WithSeeds(buffer_, v_.second * k1, x_ + w_.first);
  w_ = WeakHashLen32WithSeeds(buffer_ + 4, z_ + w_.second, y_ + buffer_[2]);
  std::swap(z_, x_);
}

size_t farmhash::final_mix(size_t len) {
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

}  // namespace n3980
}  // namespace hashing


#endif  // HASHING_DEMO_N3980_FARMHASH_H
