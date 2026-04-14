#!/bin/bash
set -e
rm -rf build
mkdir build
cd build
cmake .. > /dev/null
make
cd ..
./build/program