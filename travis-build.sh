#!/bin/bash

set -euo pipefail

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
