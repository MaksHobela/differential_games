#pragma once

// Для роботи M_PI на деяких системах
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <vector>
#include <string>
#include <array>
#include <thread>
#include <cmath>

#include "pursuer.hpp"
#include "escaper.hpp"
#include "BS_thread_pool.hpp"

struct CoordPack {
    float x, y, z;
};

struct Scenario {
    std::string type;
    std::vector<CoordPack> coords;
};

// Подія захоплення
struct CaptureEvent {
    int pursuer_id;
    int escaper_id;
    float x, y, z;
};

// Результат симуляції
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
    std::vector<CaptureEvent> captures;
};

// Пара для розподілу цілей
struct AssignmentPair {
    size_t pursuer_idx;
    size_t escaper_idx;
    float distance;
};

class GameManager {
private:

    int count_p; // бажана к-ть переслідувачів
    int count_e; // бажана к-ть втікачів
    // Початкові стани для рестартів
    std::vector<Pursuer> initial_pursuers;
    std::vector<escaper> initial_escapers;
    
    // Активні учасники поточної симуляції
    std::vector<Pursuer> p_active;
    std::vector<escaper> e_active;
    
    BS::thread_pool<> pool;
    unsigned int num_threads;
    float capture_radius;

    // Внутрішні методи логіки
    void check_collisions(std::vector<CaptureEvent>& capture_log);
    void process_pursuers(size_t s, size_t e);
    void process_escapers(size_t s, size_t e);
    void assignTargetsBalanced();
    void reassignOrphans(const std::vector<int>& dead_ids);
    
    // Допоміжний метод для паралельної роботи (якщо використовується)
    void launch_work(std::vector<std::thread>& threads, void (GameManager::*f)(size_t, size_t), size_t size);

public:
    // Конструктор
    GameManager(int num_p, int num_e, float capture_r);
    
    // Завантаження сценарію розстановки (Лінія, Шахи, Півмісяць)
    void loadScenario(int num_pursuers, int strategy_type, const std::vector<CoordPack>& escaper_coords);
    
    // Запуск одного циклу симуляції
    GameResult run_single_simulation(int id, int max_steps, int rank = 0);
};


// #pragma once

// #include "pursuer.hpp"
// #include "escaper.hpp"
// #include "BS_thread_pool.hpp"
// #include <array>

// struct CaptureEvent {
//     int pursuer_id;
//     int escaper_id;
//     float x, y, z;
//     // Coordinates pos;
// };

// struct GameResult {
//     static constexpr std::array<const char*, 4> outcomes{
//         "Pursuers have won",
//         "Escapers have won",
//         "Draw",
//         "Error"
//     };

//     int sim_id;
//     double total_time_sec;
//     size_t iterations;
//     std::string outcome;
//     std::vector<CaptureEvent> captures;
// };

// struct AssignmentPair {
//     size_t pursuer_idx;
//     size_t escaper_idx;
//     float distance;
// };

// class GameManager {
// private:
//     std::vector<Pursuer> initial_pursuers;
//     std::vector<escaper> initial_escapers;
//     std::vector<Pursuer> p_active;
//     std::vector<escaper> e_active;
//     BS::thread_pool<> pool;
    
//     unsigned int num_threads;
//     float capture_radius;

    
//     void launch_work(std::vector<std::thread>& threads, void (GameManager::*f)(size_t, size_t), size_t size);
//     // void check_collisions();
//     void check_collisions(std::vector<CaptureEvent>& capture_log);
//     void process_pursuers(size_t s, size_t e);
//     void process_escapers(size_t s, size_t e);
//     void assignTargetsBalanced();
//     void reassignOrphans(const std::vector<int>& dead_ids);

// public:
//     GameManager(int num_p, int num_e, float capture_r);
//     void loadScenario(const std::string& filename, int strategy_type, float detection_dist);
//     GameResult run_single_simulation(int id, int max_steps);
// };

