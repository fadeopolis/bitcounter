
#include "bc_openmp.hpp"
#include "bitcnt.hpp"
#include "config.h"
#include <cassert> // for assert
#include <cstdint> // for uint32_t

using namespace bc;

static constexpr size_t ALIGNMENT = 64;

constexpr static uint32_t popcount_swar_32(uint32_t n) {
  /// SWAR == SIMD within a register
  /// PC(b0 b1 ... bN) == popcount of a N-bit bit-vector
  /// NOTE: PC of  2 bits returns a 2-bit bit-vector.
  ///       PC of  4 bits returns a 3-bit bit-vector.
  ///       PC of  8 bits returns a 4-bit bit-vector.
  ///       PC of >8 bits returns a 5-bit bit-vector.

  /// replicate byte 4 times
  constexpr const auto rep4 = [](uint8_t byte) -> uint32_t {
    const uint32_t n = byte;

    return (n << 24) | (n << 16) | (n << 8) | n;
  };

  /// The algorithm has two basic steps:
  ///   1) Treat input as 16 wide vector of 2-bit ints.
  ///      Compute popcount of all 2-bit ints.
  ///   2) Do a horizontal sum of all the partial popcounts.
  /// Step 2 is done in 3 sub-steps:
  ///   2.1) Sum adjacent 2-bit popcounts into 4-bit popcounts
  ///   2.2) Sum adjacent 4-bit popcounts into 8-bit popcounts
  ///   2.3) Sum all 8-bit popcounts together.

  /// Step 1)
  ///   a == <b31 b30   ... b3 b2     b1 b0>
  ///   b == <0   b31   ... 0  b3     0  b1>
  ///   c == <PC(b0 b1) ... PC(b2 b3) PC(b30 b31)>
  const uint32_t a = n;
  const uint32_t b = (a >> 1) & rep4(0b01'01'01'01);
  const uint32_t c = a - b;

  /// Step 2.1)
  /// d == <00 PC(b0 b1)     00 PC(b2 b3)     ... 00 PC(b30 b31)>
  /// e == <00 PC(b2 b3)     00 PC(b6 b7)     ... 00 PC(b30 b31)>
  /// f == <0  PC(b0 ... b3) 0  PC(b4 ... b7) ... 0  PC(b28 ... b31)>
  const uint32_t d = (c >> 2) & rep4(0b00'11'00'11);
  const uint32_t e = c & rep4(0b00'11'00'11);
  const uint32_t f = d + e;

  /// Step 2.2)
  /// g == <0000 PC(b0 ... b7) 0000 PC(b8 ... b15) ... 0000 PC(b24 ... b31)>
  const uint32_t g = (f + (f >> 4)) & rep4(0b0000'1111);

  /// Step 2.3)
  /// h == <0000 PC(b0 ... b7)  000 PC(b0 ... b15) 000  PC(b0 ... b23) 000 PC(b0 ... b31)>
  /// i == <000  PC(b0 ... b31) 000 00000          000  00000          000 00000>
  const uint32_t h = g * rep4(0b0000'0001);
  const uint32_t i = h >> 24;

  /// Step 2.3 is the tricky bit, we basically abuse multiplication to do a horizontal prefix sum.

  return i;
}

/// popcount for one 32-bit word
static uint32_t popcount_32(const uint32_t n) {
  if constexpr (BC_USE_BUILTIN_POPCOUNT) {
    return __builtin_popcount(n);
  } else {
    return popcount_swar_32(n);
  }
}

struct alignas(ALIGNMENT) Chunk {
  static constexpr size_t SIZE = ALIGNMENT / sizeof(uint32_t);

  uint32_t data[SIZE];
};

/// popcount for a whole chunk of 32-bit words, vectorization friendly
static uint64_t popcount_chunk(const Chunk &data) {
  uint64_t sum = 0;

  BC_OMP(simd reduction(+: sum))
  for (size_t i = 0; i < Chunk::SIZE; i++) {
    /// GCC (at least 9.2.1) and Clang (at least 9.0.1) cannot vectorize __builtin_popcount
    /// So we use our SWAR version which they can vectorize
    sum += popcount_swar_32(data.data[i]);
  }

  return sum;
}

template<typename DstT, typename PtrT>
static const DstT *align_down(PtrT *ptr) {
  const uintptr_t raw     = (uintptr_t) ptr;
  const uintptr_t aligned = raw - (raw % sizeof(DstT));

  return (const DstT*) aligned;
}

Count bc::bitcount(size_t size, const uint8_t *data) {
  assert((uintptr_t(data) % ALIGNMENT == 0) && "Data is not sufficiently aligned");

  const size_t num_bits = size * 8;
  size_t num_ones = 0;

  const Chunk *__restrict__ const chunks_bgn = (const Chunk*) data;
  const Chunk *__restrict__ const chunks_end = align_down<Chunk>(data + size);

  assert(chunks_bgn <= chunks_end);

  const uint32_t *__restrict__ const words_bgn = (const uint32_t*) chunks_end;
  const uint32_t *__restrict__ const words_end = align_down<uint32_t>(data + size);

  assert(words_bgn <= words_end);

  const uint8_t *__restrict__ const bytes_bgn = (const uint8_t*) words_end;
  const uint8_t *__restrict__ const bytes_end = data + size;

  assert(bytes_bgn <= bytes_end);

  /// First do 64 byte (= 512 bit = 16 uint32_t) chunks.
  /// Use SWAR popcount which the compiler should be able to vectorize.
  /// 512 bits is completely arbiratry and not at all related to AVX512 register size.
  for (const Chunk *it = chunks_bgn; it != chunks_end; it++) {
    num_ones += popcount_chunk(*it);
  }

  /// Do 32-bit chunks

  for (const uint32_t *it = words_bgn; it != words_end; it++) {
    num_ones += popcount_32(*it);
  }

  /// Do remaining couple of bytes

  for (const uint8_t *it = bytes_bgn; it != bytes_end; it++) {
    const size_t ones = popcount_32(*it);

    assert(ones <= 8);

    num_ones += ones;
  }

  Count cnt;
  cnt.ones   = num_ones;
  cnt.zeroes = num_bits - num_ones;
  return cnt;
}

Bitcount_Buffer bc::Bitcount_Buffer::allocate(size_t size) {
  uint8_t *data = (uint8_t*) aligned_alloc(ALIGNMENT, size);
  if (!data) {
    fprintf(stderr, "bc::alloc_bitcount_buffer: out of memory\n");
    abort();
  }

  return Bitcount_Buffer{data};
}

bc::Bitcount_Buffer::~Bitcount_Buffer() {
  std::free(_ptr);
  _ptr = nullptr;
}
