#include "GameManager.hpp"                                              // Заголовний файл менеджера
#include "BS_thread_pool.hpp"                                           // Бібліотека тредпула
#include "TimerUtils.hpp"
#include "Logger.hpp"
#include <cmath>                                                        // Математичні функції
#include <algorithm>                                                    // Сортування та пошук
#include <deque>                                                        // Для траєкторій втікачів
#include <random>                                                       // Генератор випадковості
#include <stdexcept>                                                    // Обробка помилок
#include <vector>                                                       // Основний контейнер
#include <thread>                                                       // Робота з потоками
#include <future>                                                       // Для очікування результатів пулу
#include <fstream>                                                      // Для запису у файл (ДОДАНО)
#include <string>                                                       // Для назв типів (ДОДАНО)
#include <sstream>
#include <immintrin.h>                                                  // Для SIMD інтринсиків

// Додайте це в GameManager.hpp в секцію public
// void loadScenario(const std::string& filename, int type, float dist_to_enemy);
GameManager::GameManager(int num_p, int num_e, float capture_r)
    : capture_radius(capture_r), 
      count_p(num_p), // запам'ятовуємо 8
      count_e(num_e), // запам'ятовуємо 5
      num_threads(std::thread::hardware_concurrency()),
      pool(std::thread::hardware_concurrency()) 
{
    // Обов'язково ініціалізуємо розміри, щоб вектори не були нульовими
    initial_pursuers.resize(num_p);
    initial_escapers.resize(num_e);
}

void GameManager::loadScenario(int num_pursuers, int strategy_type, const std::vector<CoordPack>& escaper_coords) {
    // 1. Очищуємо поточні списки
    initial_pursuers.clear();
    initial_escapers.clear();

    // 2. Ставимо втікачів (наприклад, "з півночі", Y = detection_dist)
    // Розставляємо їх купкою або лінією на висоті 200
    for (int i = 0; i < escaper_coords.size(); ++i) {
        escaper e(escaper_coords[i].x, escaper_coords[i].y, escaper_coords[i].z, 50.0f);
        e.setID(i);
        initial_escapers.push_back(e);
    }

    // 3. Стратегії розстановки переслідувачів (на Y = 0)
    // int num_p = 10; 
    const float p_width = 2000.0f;
    const float p_start_x = 50.0f;
    const float p_center_x = p_start_x + (p_width / 2.0f);
    const float p_step = p_width / (num_pursuers - 1);

    for (int i = 0; i < num_pursuers; ++i) {
        float x = 0, y = 0, z = 0;

        if (strategy_type == 1) { // ЛІНІЯ
            x = p_center_x + (i - count_p/2) * p_step;
            y = 0;
        } 
        else if (strategy_type == 2) { // ШАХИ
            x = p_center_x + (i - count_p/2) * p_step;
            y = (i % 2 == 0) ? 0.0f : -p_step; // Другий ряд трохи глибше
        }
        else if (strategy_type == 3) { // ПІВМІСЯЦЬ (Дуга)
            float angle = M_PI * (float)i / (count_p - 1); // від 0 до 180 градусів
            float radius = 100.0f; // Радіус дуги
            x = p_center_x - radius * std::cos(angle);
            y = -radius * std::sin(angle); // Вигин у протилежний від ворога бік
        }

        // p.my_coordinate = {x, y, 0.0f};
        
        Pursuer p;
        p.setData(x, y, 0.0f, 70, 50);
        // p.v_p = 2.0f;
        // p.v_e = 1.0f;
        // p.v_p = 70.0f;
        // p.v_e = 50.0f;
        p.updateBeta();
        p.setID(i);
        initial_pursuers.push_back(p);
    }
}

GameResult GameManager::run_single_simulation(int id, int max_steps, int rank) {
/**
 * Головний цикл симуляції.
 * Вибирає стратегію паралелізму залежно від кількості об'єктів.
 */
    Logger logger; // Ініціалізація логера (ДОДАНО)

    p_active = initial_pursuers;                                        // Копіюємо початковий стан переслідувачів
    e_active = initial_escapers;                                        // Копіюємо початковий стан втікачів
    std::vector<CaptureEvent> capture_log;                              // Список усіх успішних перехоплень
    
    assignTargetsBalanced();                                            // Перший повний розподіл цілей

    auto start_time = get_current_time_fenced();
    size_t steps = 0;                                                   // Лічильник кроків

    for (int s = 0; s < max_steps; ++s) {                               // Цикл життя симуляції
        if (p_active.empty() || e_active.empty()) break;                // Вихід, якщо хтось переміг
        ++steps;                                                        // Рахуємо крок

        // ЗАПИС КООРДИНАТ ДЛЯ ВІЗУАЛІЗАЦІЇ (ДОДАНО)
        for(const auto& esc : e_active) logger.log(s, esc.getID(), id, "escaper", esc.getCoordinates());
        for(const auto& purs : p_active) logger.log(s, purs.getID(), id, "pursuer", purs.getCoordinates());

        size_t total_agents = p_active.size() + e_active.size();        // Загальна кількість об'єктів
        
        std::vector<Coordinates> p_coords;                              // Буфер для координат переслідувачів
        p_coords.reserve(p_active.size());                              // Оптимізуємо пам'ять
        for(const auto& p : p_active) p_coords.push_back(p.getCoordinates()); // Копіюємо поточні позиції

        if (total_agents > 50) {                                        // СТРАТЕГІЯ: ТРЕДПУЛ
            auto f_esc = pool.submit_loop(0u, e_active.size(), [this, &p_coords](size_t i) {
                std::deque<Coordinates> dq(p_coords.begin(), p_coords.end()); // Координати для втікача
                e_active[i].calculate_trajectory(dq);                   // Рахуємо шлях
                e_active[i].turn(dq);                                   // Робимо поворот
                e_active[i].sentPursuersPrivPosition();                 // ПЕРЕДАЄМО ДАНІ ПЕРЕСЛІДУВАЧАМ (NEW)
                });

            f_esc.wait();

            auto f_purs = pool.submit_loop(0u, p_active.size(), [this](size_t i) {
                Coordinates ec = p_active[i].escaper_coordinate;        // Беремо координату закріпленої цілі
                Coordinates pc = p_active[i].getCoordinates();          // Власна позиція
                Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z }; // Вектор на ціль
                p_active[i].calculate_new_circle(esc_vector);           // Аполлоній
                p_active[i].makeMove(0.01f);                             // Рух
            });

            f_purs.wait();                                              // Бар'єр: чекаємо переслідувачів
        } 
        else {
            // --- ПОСЛІДОВНИЙ РЕЖИМ (Serial) ---
            std::deque<Coordinates> dq(p_coords.begin(), p_coords.end());
            for (auto& esc : e_active) {
                esc.calculate_trajectory(dq);
                esc.turn(dq);
                // esc.makeMove(0.1f);
                esc.sentPursuersPrivPosition();                         // ОНОВЛЮЄМО ЦІЛІ
            }

            for (auto& purs : p_active) {
            int tid = purs.getTargetID();                                // Отримуємо ID цілі
            
            // Шукаємо актуальний об'єкт утікача в контейнері
            auto it = std::find_if(e_active.begin(), e_active.end(), 
                                   [tid](const escaper& e) { return e.getID() == tid; });

            if (it != e_active.end()) { 
                // Оновлюємо координати цілі в об'єкті переслідувача
                purs.escaper_coordinate = it->getCoordinates();
                Vector real_velocity = it->getVelocityVector();          // Справжній вектор цілі
                
                purs.calculate_new_circle(real_velocity);                // Розрахунок перехоплення
                purs.makeMove(0.01f);                                     // Рух
            } else {
                // Якщо ціль зникла (збита іншим), перерозподіляємо цілі
                assignTargetsBalanced(); 
            }
        }
        }

        check_collisions(capture_log);                                  // Перевірка зіткнень
    }

    std::stringstream log_name;
    log_name << "simulation_log_rank_" << rank << ".csv";
    logger.saveToCSV(log_name.str());                               // ЗБЕРЕЖЕННЯ ФАЙЛУ (ДОДАНО)

    auto finish_time = get_current_time_fenced();
    auto total_duration = finish_time - start_time;
    double final_time_sec = std::chrono::duration<double>(total_duration).count();

    // 1. ПІДРАХУНОК РЕЗУЛЬТАТІВ (БЕЗ ДУБЛЮВАННЯ)
    int escaped_count = 0;
    for (const auto& event : capture_log) {
        if (event.pursuer_id == -1) {
            escaped_count++;
        }
    }
    int captured_count = (int)capture_log.size() - escaped_count;

    // 2. ФОРМУВАННЯ ДИНАМІЧНОГО РЕЗУЛЬТАТУ
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

    // Повертаємо об'єкт з нашим детальним статусом
    return { id, final_time_sec, steps, final_status, capture_log };
}

void GameManager::check_collisions(std::vector<CaptureEvent>& capture_log) {
    // Йдемо одним циклом по втікачах (з кінця, бо видаляємо)
    for (int j = (int)e_active.size() - 1; j >= 0; --j) {
        Coordinates cee = e_active[j].getCoordinates();
        int esc_id = e_active[j].getID();
        bool removed = false;

        // 1. ПЕРЕВІРКА ПРОРИВУ (Y < -10)
        if (cee.y < -10.0f) {
            capture_log.push_back({-1, esc_id, cee.x, cee.y, cee.z});
            removed = true;
        } 
        else {
            // 2. ПЕРЕВІРКА ПЕРЕХОПЛЕННЯ (тільки якщо не прорвався)
            __m256 cx_vec = _mm256_set1_ps(cee.x);
            __m256 cy_vec = _mm256_set1_ps(cee.y);
            __m256 cz_vec = _mm256_set1_ps(cee.z);
            __m256 rad2_vec = _mm256_set1_ps(capture_radius * capture_radius); // Порівнюємо квадрат відстані для швидкодії

            int i = (int)p_active.size() - 1;
            for (; i >= 7; i -= 8) {
                // SIMD: Обробка 8 переслідувачів одночасно (задом наперед для збереження логіки)
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
                    // Знайдено колізію, шукаємо точний індекс зі збереженням пріоритету (від i до i-7)
                    for(int k = 0; k < 8; ++k) {
                        if ((mask & (1 << k)) != 0) {
                            int target_idx = i - k;
                            capture_log.push_back({p_active[target_idx].getID(), esc_id, cee.x, cee.y, cee.z});
                            p_active.erase(p_active.begin() + target_idx); // Переслідувач "витрачений"
                            removed = true;
                            break;
                        }
                    }
                    if (removed) break; // Цього втікача вже збито, виходимо з циклу переслідувачів
                }
            }

            // Обробка залишку
            if (!removed) {
                for (; i >= 0; --i) {
                    float dist = p_active[i].getCoordinates().vectorTo(cee).length();
                    if (dist < capture_radius) {
                        capture_log.push_back({p_active[i].getID(), esc_id, cee.x, cee.y, cee.z});
                        p_active.erase(p_active.begin() + i); // Переслідувач "витрачений"
                        removed = true;
                        break; // Цього втікача вже збито, виходимо з циклу переслідувачів
                    }
                }
            }
        }

        // Якщо втікача збито або він прорвався — видаляємо його і оновлюємо цілі
        if (removed) {
            e_active.erase(e_active.begin() + j);
            
            // Перевіряємо, чи хтось із переслідувачів залишився без цілі
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
/**
 * Потокова обробка руху переслідувачів для ручної стратегії.
 */
    for (size_t i = s; i < e && i < p_active.size(); ++i) {             // По призначеному діапазону
        Coordinates pc = p_active[i].getCoordinates();                  // Своя позиція
        Coordinates ec = p_active[i].escaper_coordinate;                // Позиція цілі
        Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z };      // Розрахунок вектору
        p_active[i].calculate_new_circle(esc_vector);                   // Математика Аполлонія
        p_active[i].makeMove(0.01f);                                     // Крок
    }
}

void GameManager::assignTargetsBalanced() {
/**
 * Повний збалансований розподіл цілей (Угорський/Greedy).
 * Викликається на початку або при критичних змінах.
 */
    if (p_active.empty() || e_active.empty()) return;                   // Якщо нема кого ділити

    struct AssignmentPair { size_t p_idx; size_t e_idx; float dist; };  // Тимчасова структура пари
    std::vector<AssignmentPair> pairs;                                  // Матриця пар
    pairs.reserve(p_active.size() * e_active.size());                   // Резерв

    for (size_t i = 0; i < p_active.size(); ++i) {                      // Рахуємо всі відстані
        Coordinates p_coord = p_active[i].getCoordinates();
        __m256 px_vec = _mm256_set1_ps(p_coord.x);
        __m256 py_vec = _mm256_set1_ps(p_coord.y);
        __m256 pz_vec = _mm256_set1_ps(p_coord.z);

        size_t j = 0;
        for (; j + 7 < e_active.size(); j += 8) {
            // SIMD: Обробка 8 втікачів одночасно
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

        // Обробка залишку
        for (; j < e_active.size(); ++j) {
            float d = p_coord.vectorTo(e_active[j].getCoordinates()).length();
            pairs.push_back({i, j, d});
        }
    }

    std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){ return a.dist < b.dist; }); // Сортуємо за близькістю

    std::vector<bool> p_done(p_active.size(), false);                   // Хто вже отримав ціль
    std::vector<int> e_count(e_active.size(), 0);                       // Скільки хвостів у кожного втікача
    int max_h = (int)std::ceil((float)p_active.size() / e_active.size()); // Ліміт на втікача

    for (const auto& pair : pairs) {                                    // Жадібний розподіл
        if (!p_done[pair.p_idx] && e_count[pair.e_idx] < max_h) {       // Якщо бот вільний і ціль не перевантажена
            p_active[pair.p_idx].updateEscaper(e_active[pair.e_idx]);   // Призначаємо
            p_done[pair.p_idx] = true;                                  // Помічаємо виконаним
            e_count[pair.e_idx]++;                                      // Рахуємо хвіст
        }
    }
}