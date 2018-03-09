
#include "bitcnt.hpp"
#include <type_traits>
#include <climits>

using namespace bc;

#ifndef NDEBUG
#  define ASSUME(X) do { if (!bool(X)) { __builtin_unreachable(); } } while (0)
#else
#  define ASSUME(X) assert(X)
#endif

using Word = unsigned long;
static constexpr const size_t word_bytes = sizeof(Word);

static_assert(std::is_same_v<size_t, Word>);
static_assert(sizeof(size_t) == 8);

#define TRUST_COMPILER 1
// #define TRUST_COMPILER 0

Count bc::bitcount(Bytes data) {
  if constexpr (TRUST_COMPILER) {
    size_t num_ones = 0;

    for (const Byte b : data) {
      const size_t pop = __builtin_popcountl(b);

      ASSUME(pop < 8);

      num_ones += pop;
    }

    const size_t num_bits = data.size() * CHAR_BIT;

    Count cnt;
    cnt.ones   = num_ones;
    cnt.zeroes = num_bits - num_ones;
    return cnt;
  } else {
    /// I would like these to be normal static functions,
    /// but when we trust the compiler to optimize they are unused
    /// and we get a warning that breaks -Werror builds.
    static const auto word_bitcount = [](Slice<Word> data) -> Count {
      size_t num_ones = 0;

      for (const Word w : data) {
        const size_t pop = __builtin_popcountl(w);

        num_ones += pop;
      }

      const size_t num_bits = data.size() * word_bytes * CHAR_BIT;

      Count cnt;
      cnt.ones   = num_ones;
      cnt.zeroes = num_bits - num_ones;
      return cnt;
    };

    static const auto byte_bitcount = [](Slice<Byte> data) -> Count {
      size_t num_ones = 0, num_zeroes = 0;

      for (const Byte b : data) {
        const size_t pop = __builtin_popcountl(b);

        num_ones   += pop;
        num_zeroes += CHAR_BIT - pop;
      }

      Count cnt;
      cnt.ones   = num_ones;
      cnt.zeroes = num_zeroes;
      return cnt;
    };

    const size_t word_bytes = (data.size() / sizeof(Word)) * sizeof(Word);

    const Slice<Word> words = data.slice(0, word_bytes).cast<Word>();
    const Slice<Byte> bytes = data.slice(word_bytes);

    Count cnt;
    cnt += word_bitcount(words);
    cnt += byte_bitcount(bytes);

    return cnt;
  }
}
