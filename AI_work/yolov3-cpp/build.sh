#!/bin/bash
SAMPLES_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release $SAMPLES_PATH
make -j8

