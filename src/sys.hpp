
#pragma once

#include "slice.hpp"
#include "result.hpp"
#include "unit.hpp"
#include <string>
#include <system_error>

namespace bc::sys {

/// ***** misc

/// get system memory page size
Result<size_t,std::error_code> get_page_size();

/// ***** file system

/// open file, readonly
Result<int,std::error_code> open(const std::string &file);

/// close file
Result<Unit,std::error_code> close(int fd);

/// read chunk from file
Result<ssize_t,std::error_code> read(int fd, size_t count, Byte *buf);

/// map entire file into memory, readonly
Result<void*,std::error_code> mmap(int fd, size_t length);

Result<Unit,std::error_code> munmap(void*, size_t length);

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
