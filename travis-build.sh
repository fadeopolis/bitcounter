#!/bin/bash

set -euo pipefail

if [[ "${CXX:-}" == clang++ ]]
then
  ## Binaries produced by Clang in Travis cannot find libomp.so at runtime :/
  ## It is at /usr/local/clang*/lib/libomp.so (where '*' is the clang version)
  ## We assume clang is installed at /usr/local/clang*/bin/clang
  CLANG_LIB_DIR="$(dirname "$(dirname "$(which clang++)")")"/lib
  export LD_LIBRARY_PATH="$CLANG_LIB_DIR"
fi

mkdir -p build-debug
pushd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
VERBOSE=1 cmake --build . -- -j $(nproc)
ctest -j $(nproc) --output-on-failure
popd

mkdir -p build-release
pushd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
VERBOSE=1 cmake --build . -- -j $(nproc)
ctest -j $(nproc) --output-on-failure
popd
