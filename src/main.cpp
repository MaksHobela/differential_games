#include "GameManager.hpp"
#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    int num_sims = 8; 
    try {
    GameManager gm(10, 3, 0.8f);
    std::vector<GameResult> all_runs;

    std::cout << "Starting " << num_sims << " simulations..." << std::endl;

    for (int i = 0; i < num_sims; ++i) {
        GameResult res = gm.run_single_simulation(i + 1, 100000);
        all_runs.push_back(res);
        
        std::cout << "Simulation #" << res.sim_id << ": " << res.total_time_sec << " sec. Ourcome: " << res.outcome << std::endl;

        if (!res.captures.empty()) {
                std::cout << "  Captures log:" << std::endl;
                for (const auto& event : res.captures) {
                    printf("    - Pursuer #%d caught Escaper #%d at [%.2f, %.2f, %.2f]\n", 
                           event.pursuer_id, event.escaper_id, event.x, event.y, event.z);
                }
            } else {
                std::cout << "  No captures occurred." << std::endl;
            }
    }

    auto best_it = std::min_element(all_runs.begin(), all_runs.end(), 
        [](const GameResult& a, const GameResult& b) {
            return a.total_time_sec < b.total_time_sec;
        });

    if (best_it != all_runs.end()) {
        std::cout << "\n--- Results ---" << std::endl;
        std::cout << "Best time: " << best_it->total_time_sec << " sec(Simulation #" << best_it->sim_id << ")" << std::endl;
        std::cout << "Best iterarions: " << best_it->iterations << std::endl;
    }
    } catch (std::exception e ) { std::cerr << e.what(); }
    return 0;
}