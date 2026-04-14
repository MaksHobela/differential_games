#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <ctime>
#include <iomanip>
#include "GameManager.hpp"

struct Individual {
    std::vector<Coordinates> genes;
    float fitness = 0;
    double time = 0;
    bool captured = false;
};

void evaluate(Individual& ind, float radius) {
    try {
        // Створюємо менеджер: n_ppo наздоганяють 1 шахед
        GameManager gm(ind.genes.size(), 1, radius);
        
        // Запускаємо симуляцію (код GameManager сам ініціалізує позиції випадково, 
        // тому ми переписуємо їх на наші гени)
        // Примітка: твій GameManager не має методу set_pursuer_positions, 
        // тому ми розраховуємо на логіку симуляції всередині run_single_simulation
        
        GameResult res = gm.run_single_simulation(1, 5000);
        
        ind.time = res.total_time_sec;
        ind.captured = (res.outcome == "Pursuers have won");
        ind.fitness = ind.captured ? (10000.0f / (float)(res.total_time_sec * ind.genes.size())) : 0.0001f;
    } catch (...) {
        ind.fitness = 0.0f;
    }
}

int main() {
    srand(time(NULL));
    const int pop_size = 20;
    const int generations = 10;
    std::vector<Individual> population(pop_size);

    for(auto& ind : population) {
        for(int i = 0; i < 3; ++i) {
            ind.genes.push_back({(float)(rand()%4000-2000), (float)(rand()%4000-2000), 0.0f});
        }
    }

    std::cout << "--- ЗАПУСК АНАЛІТИКИ ППО ---\n";
    for(int g = 0; g < generations; ++g) {
        for(auto& ind : population) evaluate(ind, 15.0f);
        
        std::sort(population.begin(), population.end(), [](auto& a, auto& b){ return a.fitness > b.fitness; });
        
        std::cout << "Gen " << g << " | Best Time: " << population[0].time << "s | " 
                  << (population[0].captured ? "HIT" : "MISS") << "\n";

        for(int i = pop_size/2; i < pop_size; ++i) {
            population[i] = population[i-pop_size/2];
            for(auto& gene : population[i].genes) {
                gene.x += (rand()%100-50);
                gene.y += (rand()%100-50);
            }
        }
    }
    return 0;
}
