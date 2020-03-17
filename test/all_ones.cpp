
#include "bitcnt.hpp"
#include <cstring> // for memset
#include <cstdio>  // for fprintf

using namespace bc;

int main() {
  const size_t MAX_SIZE = 2 * 4096;
  Bitcount_Buffer buffer = Bitcount_Buffer::allocate(MAX_SIZE);

  memset(buffer.get(), 0xFF, MAX_SIZE);

  for (size_t i = 0; i < MAX_SIZE; i++) {
    const Count cnt = bc::bitcount(i, buffer.get());

    const size_t want_ones = i * 8;
    const size_t want_zeroes = 0;

    if (cnt.ones != want_ones) {
      fprintf(stderr, "expected %zu ones, got %zu\n", want_ones, cnt.ones);
      return 1;
    }

    if (cnt.zeroes != want_zeroes) {
      fprintf(stderr, "expected %zu zeroes, got %zu\n", want_zeroes, cnt.zeroes);
      return 1;
    }
  }
}
