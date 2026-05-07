#include <mpi.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "GameManager.hpp"
#include "TimerUtils.hpp"

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
DATA_DIR="$ROOT_DIR/data"
SCENARIO_FILE="$DATA_DIR/escaper_scenarios.csv"

namespace fs = std::filesystem;

// Функція парсингу рядка CSV у структуру Scenario
Scenario parseLineToScenario(const std::string& line) {
    Scenario sc;
    std::stringstream ss(line);
    std::string segment;

    if (!std::getline(ss, sc.type, ',')) return sc;
    std::getline(ss, segment, ','); // пропуск id

    while (std::getline(ss, segment, ',')) {
        std::stringstream space_ss(segment);
        float x, y, z;
        if (space_ss >> x >> y >> z) {
            sc.coords.push_back({x, y, z});
        }
    }
    return sc;
}

std::vector<Scenario> read_all_scenarios(const std::string& filename) {
    std::vector<Scenario> all_scenarios;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        auto sc = parseLineToScenario(line);
        if (!sc.coords.empty()) {
            all_scenarios.push_back(sc);
        }
    }
    return all_scenarios;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int world_size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::string mode = "--stats";
    if (argc > 1) mode = argv[1];

    std::vector<std::string> strategy_names = {"", "Line", "Chess", "Arc"};
    auto scenarios = read_all_scenarios("data/escaper_scenarios.csv");

    if (scenarios.empty()) {
        if (rank == 0) std::cerr << "Error: No scenarios loaded!" << std::endl;
        MPI_Finalize();
        return 1;
    }

    std::vector<int> np_values;
    if (mode == "--surgical"){
        for (int i = 8; i <= 40; ++i) np_values.push_back(i);
    } else {
        for (int i = 8; i <= 50; ++i) np_values.push_back(i);
        for (int i = 60; i <= 100; i += 10) np_values.push_back(i);
    }

    int target_sc_id = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    auto start_time = get_current_time_fenced();

    for (int i = rank; i < (int)np_values.size(); i += world_size) {
        int np = np_values[i];

        if (mode == "--surgical") {
            std::string folder = "surgical_viz/np_" + std::to_string(np);
            if (rank == 0) fs::create_directories(folder);
            std::string log_path = folder + "/move_" + ".csv";
            GameManager gm(np, 10, 1.0f);
            gm.loadScenario(np, 2, scenarios[target_sc_id].coords);
            gm.run_single_simulation(target_sc_id, 10000, log_path);
            
            if (rank == 0) std::cout << "[Surgical] Done Np=" << np << std::endl;
            continue;
        }
        auto block_start = get_current_time_fenced();

        std::string folder_name = "sim_logs/np_" + std::to_string(np);
        if (mode != "--stats" && rank == 0) {
            fs::create_directories(folder_name);
        }

        std::stringstream ss;
        ss << "results_np_" << np << ".csv";
        std::ofstream out(ss.str());
        out << "Np,Strategy,EscaperType,ScenarioID,Captures,SimTime,Outcome\n";

        for (int st = 1; st <= 3; ++st) {
            for (int sc_id = 0; sc_id < (int)scenarios.size(); ++sc_id) {
                GameManager gm(np, 10, 1.0f);
                gm.loadScenario(np, st, scenarios[sc_id].coords);

                GameResult res = gm.run_single_simulation(sc_id, 10000, "");

                out << np << "," << strategy_names[st] << "," << scenarios[sc_id].type << ","
                    << sc_id << "," << res.captures.size() << "," << res.total_time_sec << ","
                    << res.outcome << "\n";
            }
        }
        out.close();

        auto block_end = get_current_time_fenced();
        double block_duration = to_us(block_end - block_start) / 1000000.0;
        std::cout << "[Rank " << rank << "] Finished Np=" << np << " | Time: " << block_duration << " s" << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    auto end_time = get_current_time_fenced();

    if (rank == 0) {
        double total_wall_time = to_us(end_time - start_time) / 1000000.0;
        std::cout << "\n================================================" << std::endl;
        std::cout << "  TOTAL PARALLEL WALL-CLOCK TIME: " << total_wall_time << " s" << std::endl;
        std::cout << "================================================" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
