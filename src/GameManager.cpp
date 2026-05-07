#include "GameManager.hpp"
#include "BS_thread_pool.hpp"
#include "TimerUtils.hpp"
#include "Logger.hpp"
#include <cmath>
#include <algorithm>
#include <deque>
#include <random>
#include <stdexcept>
#include <vector>
#include <thread>
#include <future>
#include <fstream>
#include <string>
#include <sstream>
#include <immintrin.h>
GameManager::GameManager(int num_p, int num_e, float capture_r)
    : capture_radius(capture_r),
      count_p(num_p),
      count_e(num_e),
      num_threads(std::thread::hardware_concurrency()),
      pool(std::thread::hardware_concurrency())
{
    initial_pursuers.resize(num_p);
    initial_escapers.resize(num_e);
}

void GameManager::loadScenario(int num_pursuers, int strategy_type, const std::vector<CoordPack>& escaper_coords) {
    initial_escapers.clear();
    for (int i = 0; i < escaper_coords.size(); ++i) {
        escaper e(escaper_coords[i].x, escaper_coords[i].y, escaper_coords[i].z, 50.0f);
        e.setID(i);
        initial_escapers.push_back(e);
    }
    const float p_width = 2000.0f;
    const float p_start_x = 50.0f;
    const float p_center_x = p_start_x + (p_width / 2.0f);
    const float p_step = p_width / (num_pursuers - 1);
    for (int i = 0; i < num_pursuers; ++i) {
        float x = 0, y = 0, z = 0;
        if (strategy_type == 1) {
            x = p_center_x + (i - count_p/2) * p_step;
            y = 0;
        } 
        else if (strategy_type == 2) {
            x = p_center_x + (i - count_p/2) * p_step;
            y = (i % 2 == 0) ? 0.0f : -50;
        }
        else if (strategy_type == 3) {
            float angle = M_PI - (M_PI * (float)i / (count_p - 1));
            x = p_center_x + (p_width / 2.0f) * std::cos(angle);
            y = 492.40-500 * std::sin(angle);
        }
        Pursuer p;
        p.setData(x, y, 0.0f, 70, 50);
        p.updateBeta();
        p.setID(i);
        initial_pursuers.push_back(p);
    }
}

GameResult GameManager::run_single_simulation(int id, int max_steps, std::string log_filename) {
/**
 * Головний цикл симуляції.
 * Вибирає стратегію паралелізму залежно від кількості об'єктів.
 */
    Logger logger; 
    bool enable_logging = !log_filename.empty();

    p_active = initial_pursuers;                                      
    e_active = initial_escapers;                                       
    std::vector<CaptureEvent> capture_log;                              // Список усіх успішних перехоплень
    
    assignTargetsBalanced();                                            // Початковий розподіл цілей

    auto start_time = get_current_time_fenced();
    size_t steps = 0;                                                   

    for (int s = 0; s < max_steps; ++s) {                              
        if (p_active.empty() || e_active.empty()) break;                // Вихід, якщо хтось переміг
        ++steps;                                                        
        size_t total_agents = p_active.size() + e_active.size();        
        
        std::vector<Coordinates> p_coords;                             
        p_coords.reserve(p_active.size());                              
        for(const auto& p : p_active) p_coords.push_back(p.getCoordinates()); // Копіюємо поточні позиції

            std::deque<Coordinates> dq(p_coords.begin(), p_coords.end());
            for (auto& esc : e_active) {
                esc.calculate_trajectory(dq);
                esc.turn(dq);
                esc.sentPursuersPrivPosition();                         // ОНОВЛЮЄМО ЦІЛІ
            }

            for (auto& purs : p_active) {
            int tid = purs.getTargetID();                              
            
            auto it = std::find_if(e_active.begin(), e_active.end(), 
                                   [tid](const escaper& e) { return e.getID() == tid; });

            if (it != e_active.end()) { 
                // Оновлюємо координати цілі в об'єкті переслідувача
                purs.escaper_coordinate = it->getCoordinates();
                Vector real_velocity = it->getVelocityVector();          // Вектор цілі
                
                purs.calculate_new_circle(real_velocity);                // Розрахунок кола Апалона
                purs.makeMove(0.01f);                                     
            } else {
                // Якщо ціль збита, перерозподіляємо цілі
                assignTargetsBalanced(); 
            }
        }

        check_collisions(capture_log);                                  // Перевірка зіткнень
    }
    if (enable_logging) {
        logger.saveToCSV(log_filename);
    }

    auto finish_time = get_current_time_fenced();
    auto total_duration = finish_time - start_time;
    double final_time_sec = std::chrono::duration<double>(total_duration).count();

    int escaped_count = 0;
    for (const auto& event : capture_log) {
        if (event.pursuer_id == -1) {
            escaped_count++;
        }
    }
    int captured_count = (int)capture_log.size() - escaped_count;

    std::string final_status;
    if (escaped_count == count_e) {
        final_status = "Escapers Victory (All " + std::to_string(escaped_count) + " breached)";
    } else if (escaped_count > 0) {
        final_status = "Partial Breach (" + std::to_string(escaped_count) + " escaped, " 
                       + std::to_string(captured_count) + " captured)";
    } else if (captured_count == count_e) {
        final_status = "Pursuers Victory (All intercepted)";
    } else {
        final_status = "Draw / Timeout (Remaining on field)";
    }

    return { id, final_time_sec, steps, final_status, capture_log };
}

void GameManager::check_collisions(std::vector<CaptureEvent>& capture_log) {
    bool targets_changed = false;
    const size_t SIMD_THRESHOLD = 16;
    const float rad2 = capture_radius * capture_radius;

    // В - В
    for (int i = (int)e_active.size() - 1; i >= 0; --i) {
        if (i >= (int)e_active.size()) i = (int)e_active.size() - 1;
        if (i < 0) break;
        
        bool collided = false;
        int j = i - 1;

        if (e_active.size() >= SIMD_THRESHOLD) {
            Coordinates cee = e_active[i].getCoordinates();
            __m256 cx_vec = _mm256_set1_ps(cee.x);
            __m256 cy_vec = _mm256_set1_ps(cee.y);
            __m256 cz_vec = _mm256_set1_ps(cee.z);
            __m256 rad2_vec = _mm256_set1_ps(rad2);

            for (; j >= 7; j -= 8) {
                __m256 ex_vec = _mm256_set_ps(
                    e_active[j-7].getCoordinates().x, e_active[j-6].getCoordinates().x, 
                    e_active[j-5].getCoordinates().x, e_active[j-4].getCoordinates().x,
                    e_active[j-3].getCoordinates().x, e_active[j-2].getCoordinates().x, 
                    e_active[j-1].getCoordinates().x, e_active[j].getCoordinates().x
                );
                __m256 ey_vec = _mm256_set_ps(
                    e_active[j-7].getCoordinates().y, e_active[j-6].getCoordinates().y, 
                    e_active[j-5].getCoordinates().y, e_active[j-4].getCoordinates().y,
                    e_active[j-3].getCoordinates().y, e_active[j-2].getCoordinates().y, 
                    e_active[j-1].getCoordinates().y, e_active[j].getCoordinates().y
                );
                __m256 ez_vec = _mm256_set_ps(
                    e_active[j-7].getCoordinates().z, e_active[j-6].getCoordinates().z, 
                    e_active[j-5].getCoordinates().z, e_active[j-4].getCoordinates().z,
                    e_active[j-3].getCoordinates().z, e_active[j-2].getCoordinates().z, 
                    e_active[j-1].getCoordinates().z, e_active[j].getCoordinates().z
                );

                __m256 dx = _mm256_sub_ps(ex_vec, cx_vec);
                __m256 dy = _mm256_sub_ps(ey_vec, cy_vec);
                __m256 dz = _mm256_sub_ps(ez_vec, cz_vec);

                __m256 dist2 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
                __m256 cmp = _mm256_cmp_ps(dist2, rad2_vec, _CMP_LT_OQ);
                
                int mask = _mm256_movemask_ps(cmp);
                if (mask != 0) {
                    for(int k = 0; k < 8; ++k) {
                        if ((mask & (1 << k)) != 0) {
                            int target_j = j - k;
                            e_active.erase(e_active.begin() + i);      // Видаляємо більший індекс
                            e_active.erase(e_active.begin() + target_j); // Видаляємо менший
                            targets_changed = true;
                            i--;
                            collided = true;
                            break;
                        }
                    }
                    if (collided) break;
                }
            }
        }

        // Скалярна обробка (для < 8 дронів)
        if (!collided) {
            for (; j >= 0; --j) {
                float dist = e_active[i].getCoordinates().vectorTo(e_active[j].getCoordinates()).length();
                if (dist < capture_radius) {
                    e_active.erase(e_active.begin() + i);
                    e_active.erase(e_active.begin() + j);
                    targets_changed = true;
                    i--;
                    break;
                }
            }
        }
    }

    // П - П 
    for (int i = (int)p_active.size() - 1; i >= 0; --i) {
        if (i >= (int)p_active.size()) i = (int)p_active.size() - 1;
        if (i < 0) break;
        
        bool collided = false;
        int j = i - 1;

        if (p_active.size() >= SIMD_THRESHOLD) {
            Coordinates cpp = p_active[i].getCoordinates();
            __m256 cx_vec = _mm256_set1_ps(cpp.x);
            __m256 cy_vec = _mm256_set1_ps(cpp.y);
            __m256 cz_vec = _mm256_set1_ps(cpp.z);
            __m256 rad2_vec = _mm256_set1_ps(rad2);

            for (; j >= 7; j -= 8) {
                __m256 px_vec = _mm256_set_ps(
                    p_active[j-7].getCoordinates().x, p_active[j-6].getCoordinates().x, 
                    p_active[j-5].getCoordinates().x, p_active[j-4].getCoordinates().x,
                    p_active[j-3].getCoordinates().x, p_active[j-2].getCoordinates().x, 
                    p_active[j-1].getCoordinates().x, p_active[j].getCoordinates().x
                );
                __m256 py_vec = _mm256_set_ps(
                    p_active[j-7].getCoordinates().y, p_active[j-6].getCoordinates().y, 
                    p_active[j-5].getCoordinates().y, p_active[j-4].getCoordinates().y,
                    p_active[j-3].getCoordinates().y, p_active[j-2].getCoordinates().y, 
                    p_active[j-1].getCoordinates().y, p_active[j].getCoordinates().y
                );
                __m256 pz_vec = _mm256_set_ps(
                    p_active[j-7].getCoordinates().z, p_active[j-6].getCoordinates().z, 
                    p_active[j-5].getCoordinates().z, p_active[j-4].getCoordinates().z,
                    p_active[j-3].getCoordinates().z, p_active[j-2].getCoordinates().z, 
                    p_active[j-1].getCoordinates().z, p_active[j].getCoordinates().z
                );

                __m256 dx = _mm256_sub_ps(px_vec, cx_vec);
                __m256 dy = _mm256_sub_ps(py_vec, cy_vec);
                __m256 dz = _mm256_sub_ps(pz_vec, cz_vec);

                __m256 dist2 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
                __m256 cmp = _mm256_cmp_ps(dist2, rad2_vec, _CMP_LT_OQ);
                
                int mask = _mm256_movemask_ps(cmp);
                if (mask != 0) {
                    for(int k = 0; k < 8; ++k) {
                        if ((mask & (1 << k)) != 0) {
                            int target_j = j - k;
                            p_active.erase(p_active.begin() + i);
                            p_active.erase(p_active.begin() + target_j);
                            targets_changed = true;
                            i--;
                            collided = true;
                            break;
                        }
                    }
                    if (collided) break;
                }
            }
        }

        if (!collided) {
            for (; j >= 0; --j) {
                float dist = p_active[i].getCoordinates().vectorTo(p_active[j].getCoordinates()).length();
                if (dist < capture_radius) {
                    p_active.erase(p_active.begin() + i);
                    p_active.erase(p_active.begin() + j);
                    targets_changed = true;
                    i--;
                    break;
                }
            }
        }
    }

    if (targets_changed) {
        assignTargetsBalanced();
    }

    // П-В
    for (int j = (int)e_active.size() - 1; j >= 0; --j) {
        Coordinates cee = e_active[j].getCoordinates();
        int esc_id = e_active[j].getID();
        bool removed = false;

        if (cee.y < -10.0f) {
            capture_log.push_back({-1, esc_id, cee.x, cee.y, cee.z});
            removed = true;
        } 
        else {
            __m256 cx_vec = _mm256_set1_ps(cee.x);
            __m256 cy_vec = _mm256_set1_ps(cee.y);
            __m256 cz_vec = _mm256_set1_ps(cee.z);
            __m256 rad2_vec = _mm256_set1_ps(rad2);

            int i = (int)p_active.size() - 1;
            for (; i >= 7; i -= 8) {
                __m256 px_vec = _mm256_set_ps(
                    p_active[i-7].getCoordinates().x, p_active[i-6].getCoordinates().x, 
                    p_active[i-5].getCoordinates().x, p_active[i-4].getCoordinates().x,
                    p_active[i-3].getCoordinates().x, p_active[i-2].getCoordinates().x, 
                    p_active[i-1].getCoordinates().x, p_active[i].getCoordinates().x
                );
                __m256 py_vec = _mm256_set_ps(
                    p_active[i-7].getCoordinates().y, p_active[i-6].getCoordinates().y, 
                    p_active[i-5].getCoordinates().y, p_active[i-4].getCoordinates().y,
                    p_active[i-3].getCoordinates().y, p_active[i-2].getCoordinates().y, 
                    p_active[i-1].getCoordinates().y, p_active[i].getCoordinates().y
                );
                __m256 pz_vec = _mm256_set_ps(
                    p_active[i-7].getCoordinates().z, p_active[i-6].getCoordinates().z, 
                    p_active[i-5].getCoordinates().z, p_active[i-4].getCoordinates().z,
                    p_active[i-3].getCoordinates().z, p_active[i-2].getCoordinates().z, 
                    p_active[i-1].getCoordinates().z, p_active[i].getCoordinates().z
                );

                __m256 dx = _mm256_sub_ps(px_vec, cx_vec);
                __m256 dy = _mm256_sub_ps(py_vec, cy_vec);
                __m256 dz = _mm256_sub_ps(pz_vec, cz_vec);

                __m256 dist2 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
                __m256 cmp = _mm256_cmp_ps(dist2, rad2_vec, _CMP_LT_OQ);
                
                int mask = _mm256_movemask_ps(cmp);
                if (mask != 0) {
                    for(int k = 0; k < 8; ++k) {
                        if ((mask & (1 << k)) != 0) {
                            int target_idx = i - k;
                            capture_log.push_back({p_active[target_idx].getID(), esc_id, cee.x, cee.y, cee.z});
                            p_active.erase(p_active.begin() + target_idx);
                            removed = true;
                            break;
                        }
                    }
                    if (removed) break;
                }
            }

            if (!removed) {
                for (; i >= 0; --i) {
                    float dist = p_active[i].getCoordinates().vectorTo(cee).length();
                    if (dist < capture_radius) {
                        capture_log.push_back({p_active[i].getID(), esc_id, cee.x, cee.y, cee.z});
                        p_active.erase(p_active.begin() + i);
                        removed = true;
                        break;
                    }
                }
            }
        }

        if (removed) {
            e_active.erase(e_active.begin() + j);
            bool need_reassign = false;
            for (const auto& purs : p_active) {
                if (purs.getTargetID() == esc_id) {
                    need_reassign = true;
                    break;
                }
            }
            if (need_reassign) assignTargetsBalanced();
        }
    }
}

void GameManager::process_pursuers(size_t s, size_t e) {
    for (size_t i = s; i < e && i < p_active.size(); ++i) {             
        Coordinates pc = p_active[i].getCoordinates();                  
        Coordinates ec = p_active[i].escaper_coordinate;                
        Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z };     
        p_active[i].calculate_new_circle(esc_vector);                   
        p_active[i].makeMove(0.01f);                                    
    }
}

void GameManager::assignTargetsBalanced() {
    if (p_active.empty() || e_active.empty()) return;

    struct AssignmentPair { size_t p_idx; size_t e_idx; float dist; };
    std::vector<AssignmentPair> pairs;
    pairs.reserve(p_active.size() * e_active.size());

    for (size_t i = 0; i < p_active.size(); ++i) {
        Coordinates p_coord = p_active[i].getCoordinates();
        __m256 px_vec = _mm256_set1_ps(p_coord.x);
        __m256 py_vec = _mm256_set1_ps(p_coord.y);
        __m256 pz_vec = _mm256_set1_ps(p_coord.z);

        size_t j = 0;
        for (; j + 7 < e_active.size(); j += 8) {
            __m256 ex_vec = _mm256_set_ps(
                e_active[j+7].getCoordinates().x, e_active[j+6].getCoordinates().x, 
                e_active[j+5].getCoordinates().x, e_active[j+4].getCoordinates().x,
                e_active[j+3].getCoordinates().x, e_active[j+2].getCoordinates().x, 
                e_active[j+1].getCoordinates().x, e_active[j].getCoordinates().x
            );
            __m256 ey_vec = _mm256_set_ps(
                e_active[j+7].getCoordinates().y, e_active[j+6].getCoordinates().y, 
                e_active[j+5].getCoordinates().y, e_active[j+4].getCoordinates().y,
                e_active[j+3].getCoordinates().y, e_active[j+2].getCoordinates().y, 
                e_active[j+1].getCoordinates().y, e_active[j].getCoordinates().y
            );
            __m256 ez_vec = _mm256_set_ps(
                e_active[j+7].getCoordinates().z, e_active[j+6].getCoordinates().z, 
                e_active[j+5].getCoordinates().z, e_active[j+4].getCoordinates().z,
                e_active[j+3].getCoordinates().z, e_active[j+2].getCoordinates().z, 
                e_active[j+1].getCoordinates().z, e_active[j].getCoordinates().z
            );

            __m256 dx = _mm256_sub_ps(px_vec, ex_vec);
            __m256 dy = _mm256_sub_ps(py_vec, ey_vec);
            __m256 dz = _mm256_sub_ps(pz_vec, ez_vec);

            __m256 dist2 = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
            __m256 dist = _mm256_sqrt_ps(dist2);

            float dist_arr[8];
            _mm256_storeu_ps(dist_arr, dist);

            for (int k = 0; k < 8; ++k) {
                pairs.push_back({i, j + k, dist_arr[k]});
            }
        }

        for (; j < e_active.size(); ++j) {
            float d = p_coord.vectorTo(e_active[j].getCoordinates()).length();
            pairs.push_back({i, j, d});
        }
    }

    std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){ return a.dist < b.dist; });

    std::vector<bool> p_done(p_active.size(), false);
    std::vector<int> e_count(e_active.size(), 0);
    int max_h = (int)std::ceil((float)p_active.size() / e_active.size());

    for (const auto& pair : pairs) {
        if (!p_done[pair.p_idx] && e_count[pair.e_idx] < max_h) {
            p_active[pair.p_idx].updateEscaper(e_active[pair.e_idx]);
            p_done[pair.p_idx] = true;
            e_count[pair.e_idx]++;
        }
    }
}