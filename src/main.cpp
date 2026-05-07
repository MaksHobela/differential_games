#include <mpi.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "GameManager.hpp"
#include "TimerUtils.hpp"

namespace fs = std::filesystem;

// Структура для зберігання сценарію з його типом
// struct Scenario {
//     std::string type;
//     std::vector<CoordPack> coords;
// };

// Функція парсингу рядка CSV у структуру Scenario
Scenario parseLineToScenario(const std::string& line) {
    Scenario sc;
    std::stringstream ss(line);
    std::string segment;

    // 1. Зчитуємо тип (wedge, swarm, stairs, random)
    if (!std::getline(ss, sc.type, ',')) return sc;
    
    // 2. Пропускаємо scenario_id
    std::getline(ss, segment, ',');

    // 3. Зчитуємо координати втікачів
    while (std::getline(ss, segment, ',')) {
        std::stringstream space_ss(segment);
        float x, y, z;
        if (space_ss >> x >> y >> z) {
            sc.coords.push_back({x, y, z});
        }
    }
    return sc;
}

// Функція зчитування всіх сценаріїв у пам'ять
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

    // Масиви для зручного запису імен у CSV
    std::vector<std::string> strategy_names = {"", "Line", "Chess", "Arc"};
    std::vector<int> selected_strategies;
    bool use_full_mode = false;
    std::string scenario_path;
    int max_np = 100;  // За замовчуванням

    // Парсинг параметрів командного рядка
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-L") {
            selected_strategies.push_back(1);
        } else if (arg == "-C") {
            selected_strategies.push_back(2);
        } else if (arg == "-A") {
            selected_strategies.push_back(3);
        } else if (arg == "--full") {
            use_full_mode = true;
        } else if (arg == "--quick") {
            use_full_mode = false;
        } else if (arg == "--max-np") {
            if (i + 1 < argc) {
                max_np = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --max-np requires a number" << std::endl;
                MPI_Finalize();
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            if (rank == 0) {
                std::cout << "Usage: ./program [--full|--quick] [-L] [-C] [-A] [scenario_file]" << std::endl;
                std::cout << "  -L   run Line strategy only" << std::endl;
                std::cout << "  -C   run Chess strategy only" << std::endl;
                std::cout << "  -A   run Arc strategy only" << std::endl;
                std::cout << "If no strategy flag is provided, all strategies are run." << std::endl;
            }
            MPI_Finalize();
            return 0;
        } else if (scenario_path.empty()) {
            scenario_path = arg;
        }
    }

    if (selected_strategies.empty()) {
        selected_strategies = {1, 2, 3};
    }

    if (scenario_path.empty()) {
        if (fs::exists("escaper_scenarios.csv")) {
            scenario_path = "escaper_scenarios.csv";
        } else if (fs::exists("../escaper_scenarios.csv")) {
            scenario_path = "../escaper_scenarios.csv";
        } else {
            scenario_path = "escaper_scenarios.csv";
        }
    }

    if (rank == 0) {
        std::cout << "Current working dir: " << fs::current_path() << std::endl;
        std::cout << "Using scenario file: " << scenario_path << std::endl;
        std::cout << "Run mode: " << (use_full_mode ? "FULL" : "QUICK") << std::endl;
        std::cout << "Selected strategies: ";
        for (int st : selected_strategies) std::cout << strategy_names[st] << " ";
        std::cout << std::endl;
    }

    auto scenarios = read_all_scenarios(scenario_path);

    if (scenarios.empty()) {
        if (rank == 0) {
            std::cerr << "Error: No scenarios loaded! Check file path and make sure the CSV exists." << std::endl;
            if (!fs::exists(scenario_path)) {
                std::cerr << "Missing file: " << scenario_path << std::endl;
            }
        }
        MPI_Finalize();
        return 1;
    }

    if (!use_full_mode && scenarios.size() > 20) {
        if (rank == 0) std::cout << "Quick mode: using first 20 scenarios of " << scenarios.size() << std::endl;
        scenarios.resize(20);
    }

    std::vector<int> np_values = {2};
    // for (int i = 1; i <= max_np; ++i) {
    //     np_values.push_back(i);
    // }

    // --- ПОЧАТОК ВИМІРЮВАННЯ ЗАГАЛЬНОГО ЧАСУ ---
    MPI_Barrier(MPI_COMM_WORLD); // Чекаємо, поки всі процеси будуть готові
    auto start_time = get_current_time_fenced();

    fs::path results_dir = "results";
    if (rank == 0 && !fs::exists(results_dir)) {
        fs::create_directories(results_dir);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Цикл розподілу навантаження: кожен rank бере свої значення Np
    for (int i = rank; i < (int)np_values.size(); i += world_size) {
        int np = np_values[i];
        
        // Час для конкретного блоку (Np)
        auto block_start = get_current_time_fenced();

        std::stringstream ss;
        ss << "results_np_" << np << ".csv"; 
        fs::path output_file = results_dir / ss.str();
        std::ofstream out(output_file);
        if (!out) {
            std::cerr << "Rank " << rank << " failed to open output file: " << output_file << std::endl;
            continue;
        }
        
        // Заголовок CSV: додано колонку EscaperType
        out << "Np,Strategy,EscaperType,ScenarioID,Captures,SimTime,Outcome\n";

        for (int st : selected_strategies) {
            for (int sc_id = 0; sc_id < (int)scenarios.size(); ++sc_id) {
                
                // Ініціалізація симуляції
                GameManager gm(2, 2, 1.0f); 
                gm.loadScenario(np, st, scenarios[sc_id].coords); 
                
                // Запуск математичного ядра
                GameResult res = gm.run_single_simulation(sc_id, 10000, rank);
                
                // Запис результату
                out << np << ","
                    << strategy_names[st] << ","
                    << scenarios[sc_id].type << ","
                    << sc_id << ","
                    << res.captures.size() << ","
                    << res.total_time_sec << ","
                    << std::quoted(res.outcome) << "\n";
            }
        }
        out.close();

        auto block_end = get_current_time_fenced();
        double block_duration = to_us(block_end - block_start) / 1000000.0;
        
        std::cout << "[Rank " << rank << "] Finished Np=" << np 
                  << " | Time: " << block_duration << " s" << std::endl;
    }

    // --- ЗАВЕРШЕННЯ ВИМІРЮВАННЯ ---
    MPI_Barrier(MPI_COMM_WORLD); // Чекаємо найповільнішого
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


// #include <mpi.h>
// #include <vector>
// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include "GameManager.hpp"

// // Реалізація парсера (можна тримати прямо тут або в helpers)
// // std::vector<CoordPack> parseLineToCoords(const std::string& line) {
// //     std::vector<CoordPack> result;
// //     std::stringstream ss(line);
// //     std::string segment;

// //     std::getline(ss, segment, ','); // тип
// //     std::getline(ss, segment, ','); // id

// //     while (std::getline(ss, segment, ',')) {
// //         std::stringstream space_ss(segment);
// //         float x, y, z;
// //         if (space_ss >> x >> y >> z) {
// //             result.push_back({x, y, z});
// //         }
// //     }
// //     return result;
// // }
// Scenario parseLineToScenario(const std::string& line) {
//     Scenario sc;
//     std::stringstream ss(line);
//     std::string segment;

//     std::getline(ss, sc.type, ','); // Зберігаємо тип (wedge, swarm...)
//     std::getline(ss, segment, ','); // Пропускаємо ID (число)

//     while (std::getline(ss, segment, ',')) {
//         std::stringstream space_ss(segment);
//         float x, y, z;
//         if (space_ss >> x >> y >> z) {
//             sc.coords.push_back({x, y, z});
//         }
//     }
//     return sc;
// }

// // Завантаження всіх сценаріїв у пам'ять один раз
// std::vector<std::vector<CoordPack>> read_all_scenarios(const std::string& filename) {
//     std::vector<std::vector<CoordPack>> all_scenarios;
//     std::ifstream file(filename);
//     std::string line;
//     while (std::getline(file, line)) {
//         auto scenario = parseLineToCoords(line);
//         if (!scenario.empty()) all_scenarios.push_back(scenario);
//     }
//     return all_scenarios;
// }

// int main(int argc, char** argv) {
//     MPI_Init(&argc, &argv);
//     int world_size, rank;
//     MPI_Comm_size(MPI_COMM_WORLD, &world_size);
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);

//     // Всі зчитують сценарії (це швидко, всього 100 рядків)
//     auto scenarios = read_all_scenarios("../escaper_scenarios.csv");
    
//     if (scenarios.empty()) {
//         if (rank == 0) std::cerr << "Error: No scenarios loaded!" << std::endl;
//         MPI_Finalize();
//         return 1;
//     }

//     int min_np = 8;
//     int max_np = 25;
    
//     // СТАТИЧНИЙ РОЗПОДІЛ: Кожен процес бере свою кількість перехоплювачів
//     for (int np = min_np + rank; np <= max_np; np += world_size) {
        
//         std::stringstream ss;
//         ss << "results_np_" << np << ".csv"; 
//         std::ofstream out(ss.str());
//         out << "Np,Strategy,ScenarioID,Captures,Time,Outcome\n";

//         for (int st = 1; st <= 3; ++st) {
//             for (int sc_id = 0; sc_id < (int)scenarios.size(); ++sc_id) {
                
//                 // Створюємо менеджер (np перехоплювачів, 10 втікачів)
//                 GameManager gm(np, 10, 1.0f); 
                
//                 // Виклик loadScenario (як у твоєму GameManager.hpp)
//                 gm.loadScenario(np, st, scenarios[sc_id].coords); 
                
//                 // Симуляція
//                 GameResult res = gm.run_single_simulation(sc_id, 10000);
                
//                 // Запис
//                 // out << np << "," << st << "," << sc_id << "," 
//                 //     << res.captures.size() << "," << res.total_time_sec << "," 
//                 //     << res.outcome << "\n";
//                 out << np << "," 
//                 << strategy_names[st] << ","      // "Line", "Arc"...
//                 << scenarios[sc_id].type << ","   // "wedge", "stairs"...
//                 << sc_id << "," 
//                 << res.captures.size() << "," 
//                 << res.total_time_sec << "," 
//                 << res.outcome << "\n";
//             }
//         }
//         out.close();
//         std::cout << "[Rank " << rank << "] Saved results for Np=" << np << std::endl;
//     }

//     MPI_Finalize();
//     return 0;
// }


// #include "GameManager.hpp"
// #include <vector>
// #include <iostream>
// #include <algorithm>
// #include <cstdio> // Для перейменування файлів
// #include <filesystem>

// int main() {
//     try {
//         // capture_radius = 1.5f
//         GameManager gm(10, 10, 1.0f); 
        
//         std::vector<std::string> strategy_names = {"Line", "Chess", "Arc"};
//         std::vector<GameResult> summary;

//         for (int st = 1; st <= 3; ++st) {
//             std::cout << "\n>>> EXECUTING STRATEGY: " << strategy_names[st] << " <<<" << std::endl;
            
//             // Завантажуємо розстановку
//             gm.loadScenario("", st, 1000.0f); 

//             // Запускаємо симуляцію
//             GameResult res = gm.run_single_simulation(st, 10000);
//             summary.push_back(res);

//             // РУХ ФАЙЛУ: Перейменовуємо стандартний "simulation_log.csv" на специфічний для стратегії
//             // std::string old_name = "simulation_log.csv";
//             // std::string new_name = "log_" + strategy_names[st] + ".csv";
            
//             // std::remove(new_name.c_str()); // Видаляємо старий, якщо є
//             namespace fs = std::filesystem;

//             // Замість remove + rename:
//             fs::path old_p = "simulation_log.csv";
//             fs::path new_p = "log_" + strategy_names[st] + ".csv";

//             if (fs::exists(old_p)) {
//                 fs::rename(old_p, new_p); 
//             }
//             // if (std::rename(old_name.c_str(), new_name.c_str()) == 0) {
//             //     std::cout << "  [Data saved to " << new_name << "]" << std::endl;
//             // }

//             // Вивід статистики
//             std::cout << "  Outcome:    " << res.outcome << std::endl;
//             std::cout << "  Sim Time:   " << res.total_time_sec << " sec" << std::endl;
//             std::cout << "  Iterations: " << res.iterations << std::endl;
//             std::cout << "  Captures:   " << res.captures.size() << std::endl;

//             if (!res.captures.empty()) {
//                 for (const auto& c : res.captures) {
//                     printf("    - P#%d -> E#%d at [%.1f, %.1f]\n", c.pursuer_id, c.escaper_id, c.x, c.y);
//                 }
//             }
//         }

//         // Фінальне порівняння для захисту
//         std::cout << "\n--- FINAL COMPARISON ---" << std::endl;
//         std::cout << "Strategy\tTime\tCaptures\tResult" << std::endl;
//         for (size_t i = 0; i < summary.size(); ++i) {
//             std::cout << strategy_names[i+1] << "\t\t" 
//                       << summary[i].total_time_sec << "s\t" 
//                       << summary[i].captures.size() << "\t\t" 
//                       << summary[i].outcome << std::endl;
//         }

//     } catch (const std::exception& e) {
//         std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
//     }
//     return 0;
// }

// // #include "GameManager.hpp"
// // #include <vector>
// // #include <iostream>
// // #include <algorithm>


// // int main() {
// //     try {
// //         // Створюємо менеджер (параметри 10 і 3 будуть перевизначені в loadScenario)
// //         GameManager gm(10, 3, 1.5f); 
        
// //         // Масив стратегій для тесту
// //         std::vector<std::string> strategy_names = {"Random", "Line", "Chess", "Arc"};

// //         for (int st = 1; st <= 3; ++st) {
// //             std::cout << "\nTesting Strategy: " << strategy_names[st] << std::endl;
            
// //             // Завантажуємо розстановку (втікач на відстані 500м)
// //             gm.loadScenario("", st, 500.0f); 

// //             GameResult res = gm.run_single_simulation(st, 15000);
            
// //             std::cout << "Outcome: " << res.outcome 
// //                       << " | Time: " << res.total_time_sec << "s"
// //                       << " | Captures: " << res.captures.size() << std::endl;
            
// //             // Тут можна додати логіку збереження в окремий CSV для кожного типу
// //         }

// //     } catch (const std::exception& e) {
// //         std::cerr << "Error: " << e.what() << std::endl;
// //     }
// //     return 0;
// // }
// // // int main() {
// // //     int num_sims = 2; 
// // //     try {
// // //     GameManager gm(10, 3, 0.8f);
// // //     std::vector<GameResult> all_runs;

// // //     std::cout << "Starting " << num_sims << " simulations..." << std::endl;

// // //     for (int i = 0; i < num_sims; ++i) {
// // //         GameResult res = gm.run_single_simulation(i + 1, 10000);
// // //         all_runs.push_back(res);
        
// // //         std::cout << "Simulation #" << res.sim_id << ": " << res.total_time_sec << " sec. Ourcome: " << res.outcome << std::endl;

// // //         if (!res.captures.empty()) {
// // //                 std::cout << "  Captures log:" << std::endl;
// // //                 for (const auto& event : res.captures) {
// // //                     printf("    - Pursuer #%d caught Escaper #%d at [%.2f, %.2f, %.2f]\n", 
// // //                            event.pursuer_id, event.escaper_id, event.x, event.y, event.z);
// // //                 }
// // //             } else {
// // //                 std::cout << "  No captures occurred." << std::endl;
// // //             }
// // //     }

// // //     auto best_it = std::min_element(all_runs.begin(), all_runs.end(), 
// // //         [](const GameResult& a, const GameResult& b) {
// // //             return a.total_time_sec < b.total_time_sec;
// // //         });

// // //     if (best_it != all_runs.end()) {
// // //         std::cout << "\n--- Results ---" << std::endl;
// // //         std::cout << "Best time: " << best_it->total_time_sec << " sec(Simulation #" << best_it->sim_id << ")" << std::endl;
// // //         std::cout << "Best iterarions: " << best_it->iterations << std::endl;
// // //     }
// // //     } catch (std::exception e ) { std::cerr << e.what(); }
// // //     return 0;
// // // }