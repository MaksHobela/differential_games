#!/bin/bash
set -e

BUILD_DIR="build"
NUM_PROCS=4  # Кількість ядер, яку хочеш використати

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
make -j$(nproc) # Використовуємо всі ядра для швидкої збірки

echo ">> Запуск паралельної симуляції на $NUM_PROCS процесах..."
mpirun -np $NUM_PROCS ./program


# #!/bin/bash
# set -e

# BUILD_DIR="build"

# if [ "$1" == "clean" ]; then
#     echo ">> Повне очищення..."
#     rm -rf $BUILD_DIR
# fi

# mkdir -p $BUILD_DIR
# cd $BUILD_DIR

# if [ ! -f "CMakeCache.txt" ]; then
#     echo ">> Конфігурація CMake..."
#     cmake ..
# fi

# echo ">> Компіляція..."
# cmake --build .

# ./program
# # python3 ./../10_graphs.py