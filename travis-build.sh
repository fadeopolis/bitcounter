#!/bin/bash

set -euo pipefail

USE_OPENMP=ON

if [[ "${CC:-}" == clang ]]
then
  ## Binaries produced by Clang in Travis cannot find libomp.so at runtime :/
  USE_OPENMP=OFF
fi

mkdir -p build-debug
pushd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DBC_USE_OPENMP="$USE_OPENMP" ..
VERBOSE=1 cmake --build . -- -j $(nproc)
ctest -j $(nproc) --output-on-failure
popd

mkdir -p build-release
pushd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DBC_USE_OPENMP="$USE_OPENMP" ..
VERBOSE=1 cmake --build . -- -j $(nproc)
ctest -j $(nproc) --output-on-failure
popd
