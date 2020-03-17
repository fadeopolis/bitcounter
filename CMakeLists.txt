
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(bitcounter VERSION 0.1.0 LANGUAGES CXX)

include(FindOpenMP)
include(CheckCXXSourceCompiles)

################################################################################

## turn off -std=g++XX, use -std=c++XX instead
set(CMAKE_CXX_EXTENSIONS OFF)

## put binaries & libs in standard directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(BC_GENERATED_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gen")

################################################################################

option(BC_OPTIMIZE_FOR_HOST    "Pass compiler flags to optimize for the CPU of this machine." ON)
option(BC_USE_BUILTIN_POPCOUNT "Use __builtin_popcount when supported." OFF)

################################################################################

set(BC_WARNING_FLAGS)
set(BC_OPTIMIZE_FLAGS)
set(BC_OMP_FLAGS)
set(BC_OMP_LIBS)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(BC_WARNING_FLAGS /W4)

  ## TODO: figure which flavour of '/arch' to use
elseif(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
  set(BC_WARNING_FLAGS -Wall -Wextra -pedantic)

  if(BC_OPTIMIZE_FOR_HOST)
    set(BC_OPTIMIZE_FLAGS -march=native -mtune=native)
  endif(BC_OPTIMIZE_FOR_HOST)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # TODO
endif()

if(OpenMP_CXX_FOUND)
  set(BC_OMP_FLAGS ${OpenMP_CXX_FLAGS})
  set(BC_OMP_LIBS ${OpenMP_CXX_LIB_NAMES})
endif()

################################################################################

check_cxx_source_compiles("int main() { __builtin_popcount(32); }" BC_HAVE_BUILTIN_POPCOUNT)
if (NOT BC_HAVE_BUILTIN_POPCOUNT)
  set(BC_USE_BUILTIN_POPCOUNT OFF)
endif()

configure_file(src/config.h.in "${BC_GENERATED_OUTPUT_DIRECTORY}/config.h")

################################################################################

add_library(bc
  src/bitcnt.cpp
  src/bitcnt.hpp
  src/result.hpp
  src/sys.hpp
  src/sys-unix.cpp
  "${BC_GENERATED_OUTPUT_DIRECTORY}/config.h"
)
target_compile_options(bc PUBLIC ${BC_WARNING_FLAGS} ${BC_OPTIMIZE_FLAGS} ${BC_OMP_FLAGS})
target_compile_features(bc PUBLIC cxx_std_17)
target_link_libraries(bc PUBLIC ${BC_OMP_LIBS})
target_include_directories(bc PUBLIC src "${BC_GENERATED_OUTPUT_DIRECTORY}")

add_executable(bitcounter
  src/main.cpp
)
target_link_libraries(bitcounter PUBLIC bc)

enable_testing()
add_subdirectory(test)

