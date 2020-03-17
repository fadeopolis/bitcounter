
#pragma once

#include "result.hpp"   // bc::Result
#include <cstdint>      // uint8_t
#include <optional>     // std::nullopt_t
#include <string>       // std::string
#include <system_error> // std::error_code

namespace bc::sys {

/// ***** misc

/// get system memory page size
Result<size_t,std::error_code> get_page_size();

/// ***** file system

/// open file, readonly
Result<int,std::error_code> open(const std::string &file);

/// close file
Result<std::nullopt_t,std::error_code> close(int fd);

/// read chunk from file
Result<ssize_t,std::error_code> read(int fd, size_t count, uint8_t *buf);

/// map entire file into memory, readonly
Result<void*,std::error_code> mmap(int fd, size_t length);

Result<std::nullopt_t,std::error_code> munmap(void*, size_t length);

struct Stat {
  enum File_Type {
    DIRECTORY,
    REGULAR,
    BLOCK,
    OTHER,
  };

  File_Type type;
  size_t    size;
};

Result<Stat,std::error_code> stat(int fd);

} // end namespace bc::sys
