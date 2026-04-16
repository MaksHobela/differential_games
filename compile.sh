#!/bin/bash
set -e

NUM_E=${1:-1}

rm -rf build
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..

echo "=== Simulation launch... ==="
./build/program $NUM_E

echo "=== Animation launch... ==="
python3 visualize.py
