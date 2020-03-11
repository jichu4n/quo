#!/bin/bash

cd "$(dirname "$0")"
cmake="$prefix/bin/cmake"
build="$PWD/build"

set -ex

mkdir -p "$build"
cd "$build"
"$cmake" ../
# "$cmake" --build .
make VERBOSE=1

