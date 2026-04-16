#include "GameManager.hpp"
#include <iostream>
#include <fstream>
#include <deque>
#include <random>
#include <chrono>
#include <atomic>
#include <limits>

GameManager::GameManager(int num_p, int num_e, float capture_r)
    : num_threads(std::thread::hardware_concurrency()), capture_radius(capture_r) {
    float ve = 50.0f;
    float vp = 86.11f;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> y_dist(1000.0f, 9000.0f);
    std::uniform_real_distribution<float> z_dist(700.0f, 800.0f);

    initial_escapers.resize(num_e);
    for (auto& e : initial_escapers) {
        e.setData(0.0f, y_dist(gen), z_dist(gen), ve);
    }

    initial_pursuers.resize(num_p);
    const float PI = 3.1415926535f;
    float center_x = 2500.0f;
    float center_y = 2500.0f;
    float radius = 1000.0f;

    for (int i = 0; i < num_p; ++i) {
        float ang = 2.0f * PI * i / num_p;
        float px = center_x + radius * std::cos(ang);
        float py = center_y + radius * std::sin(ang);
        initial_pursuers[i].setData(px, py, 0.0f, ve);
        initial_pursuers[i].v_p = vp;
        initial_pursuers[i].v_e = ve;
        initial_pursuers[i].updateBeta();
    }
}

void GameManager::launch_work(std::vector<std::thread>& threads, void (GameManager::*f)(size_t, size_t), size_t size) {
    size_t chunk = (size + num_threads - 1) / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        size_t s = i * chunk;
        size_t e = std::min(s + chunk, size);
        if (s < size) {
            threads.emplace_back(f, this, s, e);
        }
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    threads.clear();
}

void GameManager::check_collisions() {}
void GameManager::process_pursuers(size_t s, size_t e) {}
void GameManager::process_escapers(size_t s, size_t e) {}

GameResult GameManager::run_single_simulation(int id, int max_steps) {
    p_active = initial_pursuers;
    e_active = initial_escapers;

    float dt = 0.5f;
    int delay_steps = 2;
    std::deque<std::vector<Coordinates>> e_history;

    std::vector<bool> p_destroyed(initial_pursuers.size(), false);
    std::vector<bool> e_destroyed(initial_escapers.size(), false);

    std::vector<Coordinates> last_known_e(initial_escapers.size());
    std::vector<Coordinates> last_known_p(initial_pursuers.size());
    for (size_t i = 0; i < initial_escapers.size(); ++i) last_known_e[i] = initial_escapers[i].position;
    for (size_t i = 0; i < initial_pursuers.size(); ++i) last_known_p[i] = initial_pursuers[i].my_coordinate;

    size_t iterations = 0;
    bool all_captured = false;

    auto start = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_acquire);

    static std::ofstream csv;
    if (id == 0) {
        csv.open("trajectories.csv");
        csv << "step";
        for (size_t i = 0; i < initial_pursuers.size(); ++i)
            csv << ",p" << i << "_x,p" << i << "_y,p" << i << "_z";
        for (size_t i = 0; i < initial_escapers.size(); ++i)
            csv << ",e" << i << "_x,e" << i << "_y,e" << i << "_z";
        csv << "\n";
    }

    while (iterations < max_steps) {
        iterations++;

        std::deque<Coordinates> p_coords;
        for (const auto& p : p_active) p_coords.push_back(p.getCoordinates());

        for (size_t i = 0; i < e_active.size(); ++i) {
            if (!e_destroyed[i]) {
                e_active[i].calculate_trajectory(p_coords);
                e_active[i].turn(p_coords);
            }
        }

        std::vector<Coordinates> cur_e;
        for (size_t i = 0; i < e_active.size(); ++i) {
            if (!e_destroyed[i]) {
                cur_e.push_back(e_active[i].position);
            }
        }
        e_history.push_back(cur_e);
        if (e_history.size() > delay_steps) e_history.pop_front();

        std::vector<Coordinates> delayed_e = e_history.empty() ? cur_e : e_history.front();

        for (size_t i = 0; i < p_active.size(); ++i) {
            if (!p_destroyed[i]) {
                if (delayed_e.empty()) break;
                Coordinates target = delayed_e[0];
                float min_d = 1e9f;
                for (const auto& de : delayed_e) {
                    float d = p_active[i].my_coordinate.vectorTo(de).length();
                    if (d < min_d) { min_d = d; target = de; }
                }
                p_active[i].escaper_coordinate = target;
                p_active[i].makeMove(dt);
            }
        }

        for (size_t i = 0; i < e_active.size(); ++i) {
            if (e_destroyed[i]) continue;
            bool hit = false;
            for (size_t j = 0; j < p_active.size(); ++j) {
                if (p_destroyed[j]) continue;
                float dx = p_active[j].my_coordinate.x - e_active[i].position.x;
                float dy = p_active[j].my_coordinate.y - e_active[i].position.y;
                float horiz_dist = std::sqrt(dx*dx + dy*dy);
                if (horiz_dist < capture_radius) {
                    hit = true;
                    last_known_p[j] = p_active[j].my_coordinate;
                    p_destroyed[j] = true;
                    break;
                }
            }
            if (hit) {
                last_known_e[i] = e_active[i].position;
                e_destroyed[i] = true;
            } else {
                last_known_e[i] = e_active[i].position;
            }
        }

        all_captured = true;
        for (bool d : e_destroyed) if (!d) { all_captured = false; break; }

        if (id == 0) {
            csv << iterations - 1;

            for (size_t i = 0; i < initial_pursuers.size(); ++i) {
                if (!p_destroyed[i]) {
                    csv << "," << p_active[i].my_coordinate.x << "," << p_active[i].my_coordinate.y << "," << p_active[i].my_coordinate.z;
                } else {
                    float nan = std::numeric_limits<float>::quiet_NaN();
                    csv << "," << nan << "," << nan << "," << nan;
                }
            }
            for (size_t i = 0; i < initial_escapers.size(); ++i) {
                if (!e_destroyed[i]) {
                    csv << "," << e_active[i].position.x << "," << e_active[i].position.y << "," << e_active[i].position.z;
                } else {
                    float nan = std::numeric_limits<float>::quiet_NaN();
                    csv << "," << nan << "," << nan << "," << nan;
                }
            }
            csv << "\n";
        }

        if (all_captured) break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_release);

    GameResult res;
    res.sim_id = id;
    res.total_time_sec = std::chrono::duration<double>(end - start).count();
    res.iterations = iterations;
    res.outcome = all_captured ? GameResult::outcomes[0] : GameResult::outcomes[1];
    return res;
}
