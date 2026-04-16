#include "GameManager.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    int num_e = 1;
    if (argc > 1) {
        num_e = std::stoi(argv[1]);
        if (num_e < 1) num_e = 1;
    }

    float capture_r = 50.0f;
    int max_steps = 10000;
    int test_sims = 10;

    std::cout << "=== SIMULATION: " << num_e << " escapers ===\n";

    int optimal_p = num_e;
    bool found = false;

    for (int np = num_e; np <= num_e + 5; ++np) {
        GameManager gm(np, num_e, capture_r);
        int wins = 0;

        for (int s = 0; s < test_sims; ++s) {
            GameResult r = gm.run_single_simulation(-1, max_steps);
            if (r.outcome == "Pursuers have won") wins++;
        }

        float rate = static_cast<float>(wins) / test_sims * 100.0f;
        std::cout << np << " Pursuers vs " << num_e << " escapers → " << rate << "% success\n";

        if (rate >= 80.0f) {
            optimal_p = np;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "Even with " << optimal_p << " Pursuer success is not enough\n";
    }

    std::cout << "\n=== OPTIMAL NUMBER OF PURSUERS: " << optimal_p << " ===\n";

    GameManager gm_vis(optimal_p, num_e, capture_r);
    gm_vis.run_single_simulation(0, max_steps);
    return 0;
}
