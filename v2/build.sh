#!/bin/bash

cd "$(dirname "$0")"
cmake="$prefix/bin/cmake"
build="$PWD/build"

set -ex

"$cmake" "-B${build}" -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
make -C "$build" VERBOSE=1

