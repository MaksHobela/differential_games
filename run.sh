#!/bin/bash
set -e

BUILD_DIR="build"

if [ "$1" == "clean" ]; then
    echo ">> Повне очищення..."
    rm -rf $BUILD_DIR
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

if [ ! -f "CMakeCache.txt" ]; then
    echo ">> Конфігурація CMake..."
    cmake ..
fi

echo ">> Компіляція..."
cmake --build .

./program
# python3 ./../10_graphs.py