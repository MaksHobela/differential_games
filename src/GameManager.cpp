#include "GameManager.hpp"
#include <cmath>
#include <algorithm>
#include <deque>
#include <random>
#include <stdexcept>

GameManager::GameManager(int num_p, int num_e, float capture_r)
    : capture_radius(capture_r) {
    if(num_p == 0 || num_e == 0 || num_p < num_e) throw std::runtime_error("Incorrect numbers");
    initial_pursuers.resize(num_p);
    initial_escapers.resize(num_e);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);

    for (size_t i = 0; i < num_p; ++i) {
        initial_pursuers[i].my_coordinate = {dist(gen), dist(gen), dist(gen)};
        initial_pursuers[i].v_p = 2.0f;
        initial_pursuers[i].v_e = 1.0f;
        initial_pursuers[i].updateBeta();
    }

    for (size_t i = 0; i < num_e; ++i) {
        initial_escapers[i] = escaper(dist(gen), dist(gen), dist(gen));
    }

    num_threads = std::max(2u, std::thread::hardware_concurrency());
}

GameResult GameManager::run_single_simulation(int id, int max_steps = 1000000) {
    p_active = initial_pursuers;
    e_active = initial_escapers;
    
    size_t steps = 0;
    auto start = std::chrono::steady_clock::now();

    for (int s = 0; s < max_steps; ++s) {
        if (p_active.empty() || e_active.empty()) break;
        ++steps;

        std::vector<std::thread> threads;
        launch_work(threads, &GameManager::process_pursuers, p_active.size());
        launch_work(threads, &GameManager::process_escapers, e_active.size());

        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        check_collisions();
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    size_t p_active_size = p_active.size();
    size_t e_active_size = e_active.size();

    if (p_active.empty()) {
        return { id, diff.count(), steps, GameResult::outcomes[1]};
    } 
    else if(e_active.empty()) {
        return { id, diff.count(), steps, GameResult::outcomes[0]};
    }
    else {
        return { id, diff.count(), steps, GameResult::outcomes[2]};
    }
    
}

void GameManager::launch_work(std::vector<std::thread>& threads, void (GameManager::*f)(size_t, size_t), size_t size) {
    size_t n = std::min((size_t)num_threads, size);
    if (n == 0) return;
    
    size_t chunk = size / n;
    for (unsigned int i = 0; i < n; ++i) {
        size_t start = i * chunk;
        size_t end = (i == n - 1) ? size : start + chunk;
        threads.emplace_back(f, this, start, end);
    }
}

void GameManager::check_collisions() {
    for (int i = p_active.size() - 1; i >= 0; --i) {
        bool caught = false;
        for (int j = e_active.size() - 1; j >= 0; --j) {
            Coordinates cp = p_active[i].getCoordinates();
            Coordinates ce = e_active[j].getCoordinates();

            float distance = cp.vectorTo(ce).length(); 

            if (distance <= capture_radius) {
                e_active.erase(e_active.begin() + j);
                caught = true;
                break; 
            }
        }
        if (caught) {
            p_active.erase(p_active.begin() + i);
        }
    }
}

void GameManager::process_pursuers(size_t s, size_t e) {
    for (size_t i = s; i < e && i < p_active.size(); ++i) {
        if (e_active.empty()) break;

        auto pc = p_active[i].getCoordinates();
        auto ec = e_active[i % e_active.size()].getCoordinates();

        p_active[i].my_coordinate = {
            pc.x,
            pc.y,
            pc.z,
        };
        p_active[i].escaper_coordinate = {
            ec.x,
            ec.y,
            ec.z,
        };

        Vector esc_vector{
            ec.x - pc.x,
            ec.y - pc.y,
            ec.z - pc.z,
        };

        p_active[i].calculate_new_circle(esc_vector);
        p_active[i].makeMove(0.1f);
    }
}

void GameManager::process_escapers(size_t s, size_t e) {
    std::deque<Coordinates> pursuer_coords;
    for (const auto& p : p_active) {
        pursuer_coords.push_back(p.getCoordinates());
    }

    for (size_t i = s; i < e && i < e_active.size(); ++i) {
        e_active[i].calculate_trajectory(pursuer_coords);
        e_active[i].turn(pursuer_coords);
    }
}
