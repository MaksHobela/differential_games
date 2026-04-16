#pragma once


#include "pursuer.hpp"
#include "escaper.hpp"
#include <array>

struct GameResult {
    static constexpr std::array<const char*, 4> outcomes{
        "Pursuers have won",
        "Escapers have won",
        "Draw",
        "Error"
    };

    int sim_id;
    double total_time_sec;
    size_t iterations;
    std::string outcome;
};

class GameManager {
private:
    std::vector<Pursuer> initial_pursuers;
    std::vector<escaper> initial_escapers;
    std::vector<Pursuer> p_active;
    std::vector<escaper> e_active;
    
    unsigned int num_threads;
    float capture_radius;

    void launch_work(std::vector<std::thread>& threads, void (GameManager::*f)(size_t, size_t), size_t size);
    void check_collisions();
    void process_pursuers(size_t s, size_t e);
    void process_escapers(size_t s, size_t e);

public:
    GameManager(int num_p, int num_e, float capture_r);
    GameResult run_single_simulation(int id, int max_steps);
};
