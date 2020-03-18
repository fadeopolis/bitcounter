
#include "bitcnt.hpp"
#include "result.hpp"
#include "sys.hpp"
#include "bc_openmp.hpp"
#include <cstdio>       // printf
#include <memory>       // unique_ptr
#include <system_error> // std::error_code

using namespace bc;

/// RAII helper, runs a piece of code on scope exit
template<typename Fn>
struct On_Exit final {
  On_Exit(Fn &&fn) : fn{fn} {}

  ~On_Exit() {
    fn();
  }
private:
  Fn fn;
};

template<typename Fn>
auto on_exit(Fn &&fn) {
  return On_Exit<Fn>(std::forward<Fn>(fn));
}

void print_count(Count cnt, const std::string &filename) {
  const double KILO = 1'000;
  const double MEGA = 1'000'000;
  const double GIGA = 1'000'000'000;

  const char *unit = "B ";
  double amount = cnt.bits() / 8;

  if (amount >= GIGA) {
    unit   = "GB";
    amount = amount / GIGA;
  } else if (amount >= MEGA) {
    unit   = "MB";
    amount = amount / MEGA;
  } else if (amount >= KILO) {
    unit   = "kB";
    amount = amount / KILO;
  }

  printf("%6.1f %s - %10.3f%% ones - %10.3f%% zeroes - %s\n",
    amount, unit,
    cnt.percent_ones() * 100,
    cnt.percent_zeroes() * 100,
    filename.c_str());
}

struct Error final {
  Error(std::error_code EC, const std::string &msg) : EC{EC}, msg{msg} {}

  template<typename T>
  Error(const Result<T, std::error_code> &R, const std::string &msg) {
    assert(!R);
    this->EC  = R.get_error();
    this->msg = msg;
  }

  std::string message() const {
    return msg + ": " + EC.message();
  }

  std::error_code EC;
  std::string     msg;
};

static std::string escape(const std::string &txt) {
  auto hexdigit = [](char C) {
    return (C < 10) ? ('0' + C) : ('a' + C - 10);
  };

  std::string out;

  for (unsigned char c : txt) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '\t':
      out += "\\t";
      break;
    case '\n':
      out += "\\n";
      break;
    case '"':
      out += "\\\"";
      break;
    default:
      if (std::isprint(c)) {
        out += c;
      } else {
        out += "\\x";
        out += hexdigit((c >> 4) & 0xF);
        out += hexdigit((c >> 0) & 0xF);
      }
      break;
    }
  }

  return out;
}

struct File_Bit_Counter final {
  explicit File_Bit_Counter(size_t chunk_size) : chunk_size{chunk_size} {}

  Result<Count, Error>
  bitcount(const std::string &file) const {
    auto fd = sys::open(file);
    if (!fd) {
      return Error{fd, "could not open file " + escape(file)};
    }
    auto closer = on_exit([&](){ sys::close(*fd); });

    return bitcount(*fd, file);
  }

  Result<Count, Error>
  bitcount(int fd, const std::string &name) const {
    auto stat = sys::stat(fd);

    if (stat && should_mmap(*stat)) {
      auto cnt = mmap_bitcount(fd, name, stat->size);
      if (cnt) {
        return *cnt;
      }
    }

    /// fall back to streaming if file is small or mmaping fails
    return stream_bitcount(fd, name);
  }
private:
  bool should_mmap(sys::Stat stat) const {
    // If this not a file or a block device (e.g. it's a named pipe
    // or character device), we can't trust the size.
    // Stream in chunk by chunk
    if (stat.type != sys::Stat::REGULAR && stat.type != sys::Stat::BLOCK)
      return false;

    // don't mmap small files
    if (stat.size < chunk_size) {
      return false;
    }

    return true;
  }

  /// read stream in chunk by chunk and do popcount of each chunk
  Result<Count, Error> stream_bitcount(int fd, const std::string &name) const {
    Bitcount_Buffer buffer = Bitcount_Buffer::allocate(chunk_size);

    Count accum;

    ssize_t bytes_read;
    // read until we hit EOF.
    do {
      auto ret = sys::read(fd, chunk_size, buffer.get());

      if (!ret) {
        return Error{ret, "error reading file " + escape(name)};
      } else {
        bytes_read = ret.get_value();
      }

      accum += bc::bitcount(bytes_read, buffer.get());
    } while (bytes_read != 0);

    return accum;
  }

  /// mmap file in one go and do popcount
  Result<Count, Error> mmap_bitcount(int fd, const std::string &name, size_t size) const {
    auto mmap = sys::mmap(fd, size);
    if (!mmap) {
      return Error{mmap, "could not mmap file " + escape(name)};
    }
    auto unmapper = on_exit([&]() { sys::munmap(*mmap, size); });

    const uint8_t *data = (const uint8_t*) *mmap;

    assert((uintptr_t(data) % 64 == 0) && "mmap returned unaligned data?");

    Count accum = bc::bitcount(size, data);
    return accum;
  }

  const size_t chunk_size;
};

int main(int argc, const char *const *argv) {
  const auto page_size = sys::get_page_size();
  if (!page_size) {
    fprintf(stderr, "error getting page size: %s\n", page_size.get_error().message().c_str());
    return 1;
  }

  const File_Bit_Counter files{4 * *page_size};

  if (argc < 2) {
    auto cnt = files.bitcount(0, "<stdin>");
    if (!cnt) {
      fprintf(stderr, "error: %s\n", cnt.get_error().message().c_str());
    } else {
      print_count(*cnt, "<stdin>");
    }
  } else {
    Count total;

    BC_OMP(parallel for shared(total) schedule(dynamic))
    for (int i = 1; i < argc; i++) {
      {
        const std::string filename = argv[i];

        auto cnt = files.bitcount(filename);
        if (!cnt) {
          fprintf(stderr, "error: %s\n", cnt.get_error().message().c_str());
        } else {
          print_count(*cnt, filename);

          BC_OMP(atomic)
          total.ones += cnt->ones;

          BC_OMP(atomic)
          total.zeroes += cnt->zeroes;
        }
      }
    }

    if (argc > 2) {
      print_count(total, "<total>");
    }
  }

  return 0;
}

