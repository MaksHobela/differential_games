#include "GameManager.hpp"                                              // Заголовний файл менеджера
#include "BS_thread_pool.hpp"                                           // Бібліотека тредпула
#include "TimerUtils.hpp"
#include <cmath>                                                        // Математичні функції
#include <algorithm>                                                    // Сортування та пошук
#include <deque>                                                        // Для траєкторій втікачів
#include <random>                                                       // Генератор випадковості
#include <stdexcept>                                                    // Обробка помилок
#include <vector>                                                       // Основний контейнер
#include <thread>                                                       // Робота з потоками
#include <future>                                                       // Для очікування результатів пулу

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
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);           // Діапазон координат 0-100

    for (size_t i = 0; i < num_p; ++i) {                                // Ініціалізація переслідувачів
        initial_pursuers[i].my_coordinate = {dist(gen), dist(gen), dist(gen)}; // Випадкова точка
        initial_pursuers[i].v_p = 2.0f;                                 // Швидкість переслідувача
        initial_pursuers[i].v_e = 1.0f;                                 // Оцінка швидкості втікача
        initial_pursuers[i].updateBeta();                               // Розрахунок коефіцієнта швидкостей
        initial_pursuers[i].setID(i);                                   // Призначаємо унікальний ID
    }

    for (size_t i = 0; i < num_e; ++i) {                                // Ініціалізація втікачів
        initial_escapers[i] = escaper(dist(gen), dist(gen), 700.0f);    // Початкова точка (Z=700)
        initial_escapers[i].setID(i);                                   // Призначаємо унікальний ID
    }

    num_threads = std::max(2u, std::thread::hardware_concurrency());    // Визначаємо кількість ядер
}

GameResult GameManager::run_single_simulation(int id, int max_steps) {
/**
 * Головний цикл симуляції.
 * Вибирає стратегію паралелізму залежно від кількості об'єктів.
 */
    p_active = initial_pursuers;                                        // Копіюємо початковий стан переслідувачів
    e_active = initial_escapers;                                        // Копіюємо початковий стан втікачів
    std::vector<CaptureEvent> capture_log;                              // Список усіх успішних перехоплень
    
    assignTargetsBalanced();                                            // Перший повний розподіл цілей

    auto start_time = get_current_time_fenced();
    size_t steps = 0;                                                   // Лічильник кроків
    // auto start = std::chrono::steady_clock::now();                      // Початок відліку часу

    for (int s = 0; s < max_steps; ++s) {                               // Цикл життя симуляції
        if (p_active.empty() || e_active.empty()) break;                // Вихід, якщо хтось переміг
        ++steps;                                                        // Рахуємо крок

        size_t total_agents = p_active.size() + e_active.size();        // Загальна кількість об'єктів
        
        std::vector<Coordinates> p_coords;                              // Буфер для координат переслідувачів
        p_coords.reserve(p_active.size());                              // Оптимізуємо пам'ять
        for(const auto& p : p_active) p_coords.push_back(p.getCoordinates()); // Копіюємо поточні позиції

        if (total_agents > 50) {                                        // СТРАТЕГІЯ: ТРЕДПУЛ
            auto f_esc = pool.submit_loop(0u, e_active.size(), [this, &p_coords](size_t i) {
                std::deque<Coordinates> dq(p_coords.begin(), p_coords.end()); // Координати для втікача
                e_active[i].calculate_trajectory(dq);                   // Рахуємо шлях
                e_active[i].turn(dq);                                   // Робимо поворот
            });

            auto f_purs = pool.submit_loop(0u, p_active.size(), [this](size_t i) {
                Coordinates ec = p_active[i].escaper_coordinate;        // Беремо координату закріпленої цілі
                Coordinates pc = p_active[i].getCoordinates();          // Власна позиція
                Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z }; // Вектор на ціль
                p_active[i].calculate_new_circle(esc_vector);           // Аполлоній
                p_active[i].makeMove(0.1f);                             // Рух
            });

            f_esc.wait();                                               // Бар'єр: чекаємо втікачів
            f_purs.wait();                                              // Бар'єр: чекаємо переслідувачів
        } 
        
        // СТВОРЮЮТЬСЯ ПОТОКИ НА КОЖНОМУ КРОЦІ

        // else if (total_agents > 30){                                                     // СТРАТЕГІЯ: РУЧНІ ПОТОКИ
        //     std::vector<std::thread> threads;                           // Вектор потоків
        //     auto esc_work = [this, &p_coords](size_t start, size_t end) { // Лямбда для втікачів
        //         std::deque<Coordinates> dq(p_coords.begin(), p_coords.end());
        //         for (size_t i = start; i < end; ++i) {
        //             e_active[i].calculate_trajectory(dq);
        //             e_active[i].turn(dq);
        //         }
        //     };
        //     launch_custom_work(threads, esc_work, e_active.size());     // Запуск втікачів
        //     launch_work(threads, &GameManager::process_pursuers, p_active.size()); // Запуск переслідувачів
        //     for (auto& t : threads) if (t.joinable()) t.join();         // Чекаємо завершення всіх
        // }
        else {
            // --- ПОСЛІДОВНИЙ РЕЖИМ (Serial) ---
            // Коли об'єктів мало, один потік працює швидше за будь-який пул.
            
            // Розрахунок втікачів
            std::deque<Coordinates> dq(p_coords.begin(), p_coords.end());
            for (auto& esc : e_active) {
                esc.calculate_trajectory(dq);
                esc.turn(dq);
            }

            // Розрахунок переслідувачів
            for (auto& purs : p_active) {
                Coordinates ec = purs.escaper_coordinate;
                Coordinates pc = purs.getCoordinates();
                Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z };
                purs.calculate_new_circle(esc_vector);
                purs.makeMove(0.1f);
            }
        }

        check_collisions(capture_log);                                             // Перевірка зіткнень та перерозподіл "сиріт"
    }

    // auto end = std::chrono::steady_clock::now();                        // Кінець відліку часу
    // std::chrono::duration<double> diff = end - start;                   // Рахуємо секунди
    auto finish_time = get_current_time_fenced();
    auto total_duration = finish_time - start_time;
    double final_time_sec = std::chrono::duration<double>(total_duration).count();

    if (p_active.empty()) return { id, final_time_sec, steps, GameResult::outcomes[1], capture_log}; // Перемога втікачів
    if (e_active.empty()) return { id, final_time_sec, steps, GameResult::outcomes[0], capture_log}; // Перемога переслідувачів
    return { id, final_time_sec, steps, GameResult::outcomes[2], capture_log};         // Нічия
    
    // return { id, (double)to_us(total_duration)/1000000.0, steps, "Finished", capture_log };
}

void GameManager::check_collisions(std::vector<CaptureEvent>& capture_log) {
// void GameManager::check_collisions() {
/**
 * Перевірка колізій та розумний перерозподіл цілей.
 * Якщо втікача збито, перераховуємо цілі тільки для тих, хто за ним гнався.
 */
    std::vector<int> caught_escaper_ids;                                // Список ID збитих втікачів

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
                    // ce                          // Координата перехоплення
                    cee.x,                                     // 3. x
                    cee.y,                                     // 4. y
                    cee.z                                      // 5. z
                });
                caught_escaper_ids.push_back(e_active[j].getID());      // Запам'ятовуємо ID жертви
                e_active.erase(e_active.begin() + j);                   // Видаляємо втікача
                caught = true;                                          // Фіксуємо факт
                break;                                                  // Цей переслідувач більше нікого не шукає
            }
        }
        if (caught) p_active.erase(p_active.begin() + i);               // Видаляємо переслідувача, що "врізався"
    }

    if (!caught_escaper_ids.empty() && !e_active.empty()) {             // Якщо були смерті і ще є кого ловити
        reassignOrphans(caught_escaper_ids);                            // Перепризначаємо тільки тих, хто втратив ціль
    }
}

void GameManager::reassignOrphans(const std::vector<int>& dead_ids) {
/**
 * Перепризначає цілі тільки для переслідувачів, чиї втікачі вибули.
 * Решта переслідувачів продовжують свій рух без змін.
 */
    int max_h = (int)std::ceil((float)p_active.size() / e_active.size()); // Новий ліміт балансу

    for (auto& pursuer : p_active) {                                    // Проходимо по всіх живих переслідувачах
        bool is_orphan = false;                                         // Чи він став "сиротою"
        for (int id : dead_ids) {                                       // Перевіряємо, чи його ціль серед мертвих
            if (pursuer.getTargetID() == id) {                          // Якщо так
                is_orphan = true;                                       // Він сирота
                break;
            }
        }

        if (is_orphan) {                                                // Якщо треба нова ціль
            float min_dist = 1e9;                                       // Початкова нескінченна відстань
            int best_idx = 0;                                           // Індекс найкращої цілі
            for (size_t j = 0; j < e_active.size(); ++j) {              // Шукаємо найближчого втікача
                float d = pursuer.getCoordinates().vectorTo(e_active[j].getCoordinates()).length();
                if (d < min_dist) {                                     // Тут можна додати перевірку на max_h, якщо хочеш жорсткий баланс
                    min_dist = d;
                    best_idx = j;
                }
            }
            pursuer.updateEscaper(e_active[best_idx]);                  // Призначаємо нову ціль
        }
    }
}

// void GameManager::launch_custom_work(std::vector<std::thread>& threads, 
//                                      std::function<void(size_t, size_t)> func, 
//                                      size_t size) {
// /**
//  * Утиліта для запуску довільних лямбда-функцій у потоках.
//  * Використовується для гібридної паралельності при малих N.
//  */
//     size_t n = std::min((size_t)num_threads, size);                     // Скільки потоків реально треба
//     if (n == 0) return;                                                 // Якщо 0 - виходимо
//     size_t chunk = size / n;                                            // Розмір шматка даних

//     for (unsigned int i = 0; i < n; ++i) {                              // Цикл створення потоків
//         size_t start = i * chunk;                                       // Початок
//         size_t end = (i == n - 1) ? size : start + chunk;               // Кінець (останній забирає все)
//         threads.emplace_back(func, start, end);                         // Запуск
//     }
// }

void GameManager::process_pursuers(size_t s, size_t e) {
/**
 * Потокова обробка руху переслідувачів для ручної стратегії.
 */
    for (size_t i = s; i < e && i < p_active.size(); ++i) {             // По призначеному діапазону
        Coordinates pc = p_active[i].getCoordinates();                  // Своя позиція
        Coordinates ec = p_active[i].escaper_coordinate;                // Позиція цілі
        Vector esc_vector{ ec.x - pc.x, ec.y - pc.y, ec.z - pc.z };      // Розрахунок вектору
        p_active[i].calculate_new_circle(esc_vector);                   // Математика Аполлонія
        p_active[i].makeMove(0.1f);                                     // Крок
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