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


GameManager::GameManager(int num_p, int num_e, float capture_r)
    : capture_radius(capture_r), pool(std::thread::hardware_concurrency()) {
/**
 * Конструктор менеджера гри.
 * Валідує кількість гравців, ініціалізує позиції та створює пул потоків.
 */
    if(num_p == 0 || num_e == 0 || num_p < num_e)                       // Перевірка на коректність вводу
        throw std::runtime_error("Incorrect numbers");                  // Викидаємо помилку, якщо логіка порушена

    initial_pursuers.resize(num_p);                                     // Виділяємо пам'ять під переслідувачів
    initial_escapers.resize(num_e);                                     // Виділяємо пам'ять під втікачів

    std::random_device rd;                                              // Джерело випадковості
    std::mt19937 gen(rd());                                             // Генератор Мерсенна
    std::uniform_real_distribution<float> dist(0.0f, 200.0f);           // Діапазон координат 0-100

    for (size_t i = 0; i < num_p; ++i) {                                // Ініціалізація переслідувачів
        initial_pursuers[i].my_coordinate = {dist(gen), dist(gen), 0.0f}; // Випадкова точка
        initial_pursuers[i].v_p = 2.0f;                                 // Швидкість переслідувача
        initial_pursuers[i].v_e = 1.0f;                                 // Оцінка швидкості втікача
        initial_pursuers[i].updateBeta();                               // Розрахунок коефіцієнта швидкостей
        initial_pursuers[i].setID(i);                                   // Призначаємо унікальний ID
    }

    for (size_t i = 0; i < num_e; ++i) {                                // Ініціалізація втікачів
        initial_escapers[i] = escaper(dist(gen), dist(gen), 200.0f);    // Початкова точка (Z=200)
        initial_escapers[i].setID(i);                                   // Призначаємо унікальний ID
    }

    num_threads = std::max(2u, std::thread::hardware_concurrency());    // Визначаємо кількість ядер
}

GameResult GameManager::run_single_simulation(int id, int max_steps) {
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
        for(const auto& esc : e_active) logger.log(s, esc.getID(), "escaper", esc.getCoordinates());
        for(const auto& purs : p_active) logger.log(s, purs.getID(), "pursuer", purs.getCoordinates());

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
                p_active[i].makeMove(2.0f);                             // Рух
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

            // for (auto& purs : p_active) {
            //     Coordinates ec = purs.escaper_coordinate;
            //     Coordinates pc = purs.getCoordinates();
            //     Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z };
            //     purs.calculate_new_circle(esc_vector);
            //     purs.makeMove(0.1f);
            // }




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
                purs.makeMove(2.0f);                                     // Рух
            } else {
                // Якщо ціль зникла (збита іншим), перерозподіляємо цілі
                assignTargetsBalanced(); 
            }
        }
            // for (auto& purs : p_active) {
            //     // 1. Отримуємо об'єкт утікача, за яким закріплений цей переслідувач
            //     // Припустимо, у тебе в pursuer є вказівник my_escaper
            //     if (purs.my_escaper) { 
            //         Vector real_velocity = purs.my_escaper->getVelocityVector();
                    
            //         // 2. Тепер переслідувач знає справжній курс цілі
            //         purs.calculate_new_circle(real_velocity);
            //         purs.makeMove(2.0f);
            //     }
            // }
        }

        check_collisions(capture_log);                                  // Перевірка зіткнень
    }

    logger.saveToCSV("simulation_log.csv");                             // ЗБЕРЕЖЕННЯ ФАЙЛУ (ДОДАНО)

    auto finish_time = get_current_time_fenced();
    auto total_duration = finish_time - start_time;
    double final_time_sec = std::chrono::duration<double>(total_duration).count();

    if (p_active.empty()) return { id, final_time_sec, steps, GameResult::outcomes[1], capture_log}; // Перемога втікачів
    if (e_active.empty()) return { id, final_time_sec, steps, GameResult::outcomes[0], capture_log}; // Перемога переслідувачів
    return { id, final_time_sec, steps, GameResult::outcomes[2], capture_log};         // Нічия
}

void GameManager::check_collisions(std::vector<CaptureEvent>& capture_log) {
/**
 * Перевірка колізій та розумний перерозподіл цілей.
 * Якщо втікача збито, перераховуємо цілі тільки для тих, хто за ним гнався.
 */
    for (int i = (int)p_active.size() - 1; i >= 0; --i) {               // Йдемо по переслідувачах з кінця
        bool caught = false;                                            // Чи спіймав цей бот когось
        Coordinates cp = p_active[i].getCoordinates();                  // Позиція переслідувача
        
        for (int j = (int)e_active.size() - 1; j >= 0; --j) {           // Йдемо по втікачах
            Coordinates cee = e_active[j].getCoordinates();              // Позиція втікача
            float distance = cp.vectorTo(cee).length();                  // Відстань між ними

            if (distance < capture_radius) {                            // Якщо колізія
                capture_log.push_back({
                    p_active[i].getID(),        // ID переслідувача
                    e_active[j].getID(),        // ID втікача 
                    cee.x, cee.y, cee.z         // Координата перехоплення                                                
                });
                int dead_escaper_id = e_active[j].getID();
                e_active.erase(e_active.begin() + j); // ВИДАЛЯЄМО ВТІКАЧА       
                
                for (size_t p_idx = 0; p_idx < p_active.size(); ++p_idx) { // МИТТЄВИЙ ПЕРЕРОЗПОДІЛ
                    if (p_active[p_idx].getTargetID() == dead_escaper_id && (int)p_idx != i) {
                        float min_dist = 1e9;
                        int best_target_idx = -1;

                        for (size_t target_idx = 0; target_idx < e_active.size(); ++target_idx) {
                            float d = p_active[p_idx].getCoordinates().vectorTo(e_active[target_idx].getCoordinates()).length();
                            if (d < min_dist) {
                                min_dist = d;
                                best_target_idx = (int)target_idx;
                            }
                        }

                        if (best_target_idx != -1) {
                            p_active[p_idx].updateEscaper(e_active[best_target_idx]);
                        }
                    }
                }
                caught = true;
                break; 
            }
        }
        if (caught) { p_active.erase(p_active.begin() + i); }
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
        p_active[i].makeMove(2.0f);                                     // Крок
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
        for (size_t j = 0; j < e_active.size(); ++j) {
            float d = p_active[i].getCoordinates().vectorTo(e_active[j].getCoordinates()).length();
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