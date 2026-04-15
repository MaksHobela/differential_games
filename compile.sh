#!/bin/bash
set -e

NUM_E=${1:-1}   # якщо не вказано — 1 Шахед

echo "=== Симуляція: $NUM_E Шахед(ів) ==="
rm -rf build
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..

echo "=== Запуск симуляції ==="
./build/program $NUM_E

echo "=== Запуск 3D анімації ==="
python3 visualize.py

echo "=== ГОТОВО! ==="