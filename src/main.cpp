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

    std::cout << "=== Симуляція: " << num_e << " Шахед(ів) ===\n";

    // === ЖОРСТКА УМОВА: мінімум ППО = кількість Шахедів ===
    int optimal_p = num_e;
    bool found = false;

    for (int np = num_e; np <= num_e + 5; ++np) {   // шукаємо від num_e і трохи більше
        GameManager gm(np, num_e, capture_r);
        int wins = 0;

        for (int s = 0; s < test_sims; ++s) {
            GameResult r = gm.run_single_simulation(-1, max_steps);
            if (r.outcome == "Pursuers have won") wins++;
        }

        float rate = static_cast<float>(wins) / test_sims * 100.0f;
        std::cout << np << " ППО проти " << num_e << " Шахедів → " << rate << "% успіху\n";

        if (rate >= 80.0f) {
            optimal_p = np;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "Навіть з " << optimal_p << " ППО успіх недостатній. Беремо максимум з тесту.\n";
    }

    std::cout << "\n=== ОПТИМАЛЬНА КІЛЬКІСТЬ ДРОНІВ-ПЕРЕХОПЛЮВАЧІВ: " << optimal_p << " ===\n";
    std::cout << "Запускаємо повну візуальну симуляцію...\n";

    GameManager gm_vis(optimal_p, num_e, capture_r);
    gm_vis.run_single_simulation(0, max_steps);

    std::cout << "Готово! Траєкторії збережено у trajectories.csv\n";
    return 0;
}
