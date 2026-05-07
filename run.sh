#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SCRIPTS_DIR="$ROOT_DIR/scripts"
DATA_DIR="$ROOT_DIR/data"
NUM_PROCS=4
SCENARIO_FILE="$DATA_DIR/escaper_scenarios.csv"

print_help() {
    echo "Використання: bash run.sh <команда> [аргументи]"
    echo "Команди Python скриптів:"
    echo "  gen                 - generate_escaper_lock.py"
    echo "  v                   - vizualization.py"
    echo "  graphs              - 10_graphs.py"
    echo "  anim <лог> <мп4>    - dinemic_viz.py (передає аргументи далі)"
    echo "  analyze [args]      - analize_8to25_graps.py (передає аргументи далі)"
    echo ""
    echo "Команди C++ симуляції:"
    echo "  clean               - Очищення білда"
    echo "  sim [args]          - Запуск симуляції (або просто bash run.sh [args])"
    exit 1
}

case "$1" in
    clean)
        echo ">> Повне очищення..."
        if [ -d "$DATA_DIR" ]; then
                    find "$DATA_DIR" -mindepth 1 ! -name "escaper_scenarios.csv" -exec rm -rf {} +
                fi
        exit 0
        ;;
    gen)
        python3 "$SCRIPTS_DIR/generate_escaper_lock.py"
        exit 0
        ;;
    v)
        python3 "$SCRIPTS_DIR/vizualization.py"
        exit 0
        ;;
    graphs)
        python3 "$SCRIPTS_DIR/10_graphs.py"
        exit 0
        ;;
    # video)
    #     shift
    #     python3 "$SCRIPTS_DIR/create_video.py" "$@" data/simulation_log_rank_0.csv data/video.mp4
    #     exit 0
    #     ;;
    analyze)
        shift
        python3 "$SCRIPTS_DIR/analize_8to25_graps.py" "$@"
        exit 0
        ;;
    help|-h|--help)
        print_help
        ;;
    viz)
        shift
        python3 "$SCRIPTS_DIR/generation_for_big_visualization.py"
        exit 0
        ;;
    sim)
        shift 
        ;;
esac

if [ ! -f "$SCENARIO_FILE" ]; then
    echo ">> Файл сценаріїв не знайдено. Генеруємо $SCENARIO_FILE..."
    python3 "$SCRIPTS_DIR/generate_escaper_lock.py"
fi

PROGRAM_ARGS=""
ANALYSIS_ARGS=""
SHOW_GRAPH=0
MAX_NP=100

if [[ "$1" =~ ^[0-9]+$ ]]; then
    MAX_NP=$1
    PROGRAM_ARGS="$PROGRAM_ARGS --max-np $MAX_NP"
    shift
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        -L) PROGRAM_ARGS="$PROGRAM_ARGS -L"; ANALYSIS_ARGS="$ANALYSIS_ARGS -L" ;;
        -C) PROGRAM_ARGS="$PROGRAM_ARGS -C"; ANALYSIS_ARGS="$ANALYSIS_ARGS -C" ;;
        -A) PROGRAM_ARGS="$PROGRAM_ARGS -A"; ANALYSIS_ARGS="$ANALYSIS_ARGS -A" ;;
        --full) PROGRAM_ARGS="$PROGRAM_ARGS --full" ;;
        --quick) PROGRAM_ARGS="$PROGRAM_ARGS --quick" ;;
        --show) SHOW_GRAPH=1 ;;
        *)
            echo "Помилка: Невідома опція $1"
            print_help
            ;;
    esac
    shift
done

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ -d "$DATA_DIR/results" ]; then
    rm -f $DATA_DIR/results/results_np_*.csv
fi
mkdir -p $DATA_DIR/results

if [ ! -f "CMakeCache.txt" ]; then
    echo ">> Конфігурація CMake..."
    cmake ..
fi

echo ">> Компіляція..."
make -j$(nproc)

echo ">> Запуск паралельної симуляції на $NUM_PROCS процесах..."
mpirun -np $NUM_PROCS ./program $PROGRAM_ARGS $DATA_DIR/escaper_scenarios.csv

if [ "$SHOW_GRAPH" -eq 1 ]; then
    mkdir -p "$ROOT_DIR/png"
    python3 "$SCRIPTS_DIR/analize_8to25_graps.py" --png-dir "$ROOT_DIR/png" --max-np "$MAX_NP" $ANALYSIS_ARGS --show
fi

echo ">> Результати збережено у $DATA_DIR/results/"