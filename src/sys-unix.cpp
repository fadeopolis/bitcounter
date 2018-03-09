
#include "sys.hpp"
#include <unistd.h>
#include <fcntl.h>     // for O_RDONLY, O_CLOEXEC
#include <sys/stat.h>  // for fstat
#include <sys/mman.h>  // for mmap, MAP_PRIVATE, MAP_FAILED, ...

using namespace bc;
using namespace bc::sys;

template <typename FailT, typename Fun, typename... Args>
static inline auto retry_after_signal(const FailT &Fail, const Fun &F,
                             const Args &... As) -> decltype(F(As...)) {
  decltype(F(As...)) Res;
  do
    Res = F(As...);
  while (Res == Fail && errno == EINTR);
  return Res;
}

static inline std::error_code error_from_errno() {
  return std::error_code(errno, std::generic_category());
}

Result<int,std::error_code> bc::sys::open(const std::string &file) {
  int open_flags = O_RDONLY;
#ifdef O_CLOEXEC
  open_flags |= O_CLOEXEC;
#endif

  int fd;
  if ((fd = retry_after_signal(-1, ::open, file.c_str(), open_flags)) < 0)
    return error_from_errno();

#ifndef O_CLOEXEC
  {
    int r = fcntl(fd, F_SETFD, FD_CLOEXEC);
    (void)r;
    assert(r == 0 && "fcntl(F_SETFD, FD_CLOEXEC) failed");
  }
#endif

  return fd;

  /// TODO:
  // Attempt to get the real name of the file, if the user asked
//   if(!RealPath)
//     return std::error_code();
//   RealPath->clear();
// #if defined(F_GETPATH)
//   // When F_GETPATH is availble, it is the quickest way to get
//   // the real path name.
//   char Buffer[MAXPATHLEN];
//   if (::fcntl(ResultFD, F_GETPATH, Buffer) != -1)
//     RealPath->append(Buffer, Buffer + strlen(Buffer));
// #else
//   char Buffer[PATH_MAX];
//   if (hasProcSelfFD()) {
//     char ProcPath[64];
//     snprintf(ProcPath, sizeof(ProcPath), "/proc/self/fd/%d", ResultFD);
//     ssize_t CharCount = ::readlink(ProcPath, Buffer, sizeof(Buffer));
//     if (CharCount > 0)
//       RealPath->append(Buffer, Buffer + CharCount);
//   } else {
//     // Use ::realpath to get the real path name
//     if (::realpath(P.begin(), Buffer) != nullptr)
//       RealPath->append(Buffer, Buffer + strlen(Buffer));
//   }
// #endif
}

Result<Unit,std::error_code> bc::sys::close(int fd) {
  int ret = retry_after_signal(-1, ::close, fd);

  if (ret == -1) {
    return error_from_errno();
  }

  return UNIT;
}

Result<ssize_t,std::error_code> bc::sys::read(int fd, size_t count, Byte *buffer) {
  ssize_t bytes_read = retry_after_signal(-1, ::read, fd, (void*) buffer, count);

  if (bytes_read == -1)
    return error_from_errno();

  return bytes_read;
}

Result<void*,std::error_code> bc::sys::mmap(int fd, size_t length) {
  assert(length != 0);

  int flags = MAP_PRIVATE;
  int prot  = PROT_READ;

#if defined(__APPLE__)
  //----------------------------------------------------------------------
  // Newer versions of MacOSX have a flag that will allow us to read from
  // binaries whose code signature is invalid without crashing by using
  // the MAP_RESILIENT_CODESIGN flag. Also if a file from removable media
  // is mapped we can avoid crashing and return zeroes to any pages we try
  // to read if the media becomes unavailable by using the
  // MAP_RESILIENT_MEDIA flag.  These flags are only usable when mapping
  // with PROT_READ, so take care not to specify them otherwise.
  //----------------------------------------------------------------------
  if (Mode == readonly) {
#if defined(MAP_RESILIENT_CODESIGN)
    flags |= MAP_RESILIENT_CODESIGN;
#endif
#if defined(MAP_RESILIENT_MEDIA)
    flags |= MAP_RESILIENT_MEDIA;
#endif
  }
#endif // #if defined (__APPLE__)

  void *const mapping = ::mmap(nullptr, length, prot, flags, fd, 0);
  if (mapping == MAP_FAILED)
    return error_from_errno();

  return mapping;
}

Result<Unit,std::error_code> bc::sys::munmap(void *mapping, size_t length) {
  const int ret = ::munmap(mapping, length);

  if (ret == -1) {
    return error_from_errno();
  }

  return UNIT;
}

Result<Stat,std::error_code> bc::sys::stat(int fd) {
  struct stat status;
  const int ret = ::fstat(fd, &status);

  if (ret != 0) {
    return error_from_errno();
  }

  Stat out;

  if (S_ISDIR(status.st_mode)) {
    out.type = Stat::DIRECTORY;
  } else if (S_ISREG(status.st_mode)) {
    out.type = Stat::REGULAR;
  } else if (S_ISBLK(status.st_mode)) {
    out.type = Stat::BLOCK;
  } else if (S_ISCHR(status.st_mode)) {
    out.type = Stat::OTHER;
  } else if (S_ISFIFO(status.st_mode)) {
    out.type = Stat::OTHER;
  } else if (S_ISSOCK(status.st_mode)){
    out.type = Stat::OTHER;
  } else if (S_ISLNK(status.st_mode)) {
    out.type = Stat::OTHER;
  }

  out.size = status.st_size;

  return out;
}

Result<size_t,std::error_code> bc::sys::get_page_size() {
  long sz = sysconf(_SC_PAGESIZE);

  if (sz == -1) {
    return error_from_errno();
  }

  return size_t(sz);
}
