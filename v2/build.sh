#!/bin/bash

cd "$(dirname "$0")"
cmake="$prefix/bin/cmake"
build="./build"

set -ex

"$cmake" "-B${build}" -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
"$cmake" --build "$build" --verbose

