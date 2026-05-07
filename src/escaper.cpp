#include "escaper.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include "pursuer.hpp"
// Конструктор з ініціалізацією нових полів для згладжування
escaper::escaper(float x, float y, float z, float ve, int prob)
    : position(x, y, 200.0f), v_e(ve), turn_prob(prob), theta(0.0f), phi(0.0f) {
    last_theta = 0.0f;
    smoothed_desired = 0.0f;
    
    // Рандомний вибір стратегії при створенні
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> d(0, 1);
    // current_strategy = (d(gen) == 0) ? Strategy::EVASIVE : Strategy::ZIGZAG;
    current_strategy = Strategy::ZIGZAG;
    
    // Початковий напрямок (наприклад, вздовж осі Y)
    // base_theta = M_PI / 2.0f; 
    base_theta = -M_PI/2; 
}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
    priv_pos = position;
    const float dt = 0.01f;

    if (current_strategy == Strategy::EVASIVE) {
        // --- СТРАТЕГІЯ 1: УХИЛЕННЯ ВІД ЗАГРОЗ (Твій оригінальний код) ---
        std::vector<Coordinates> nearby;
        for (const auto& p : pursuer_coords) {
            float d = position.vectorTo(p).length();
            if (d < 500.0f && d > 0.1f) nearby.push_back(p);
        }

        if (nearby.empty()) {
            smoothed_desired = base_theta; // Тримаємо курс, якщо порожньо
            return;
        }

        Vector new_dir(0, 0, 0);
        if (nearby.size() == 1) {
            new_dir = nearby[0].vectorTo(position);
        } else {
            Vector line = nearby[0].vectorTo(nearby[1]);
            Vector perp(line.y, -line.x, 0.0f);
            if (perp.length() < 0.1f) perp = Vector(1.0f, 0.0f, 0.0f);
            new_dir = perp;
        }

        Vector unit = new_dir.normalize();
        float instant_target_theta = std::atan2(unit.y, unit.x);

        float delta = instant_target_theta - smoothed_desired;
        delta = std::fmod(delta + M_PI, 2.0f * M_PI) - M_PI;
        
        float max_smooth = 0.0072f; // 0.3 градуси (0.0052 рад — це ближче до твоїх 0.3)
        if (delta > max_smooth) delta = max_smooth;
        if (delta < -max_smooth) delta = -max_smooth;

        smoothed_desired += delta;

    } else {
        zigzag_timer += dt;
        
        // Додаємо невеликий дрейф до base_theta, щоб ескейпер все ж трохи відвертав від рою
        // навіть під час зигзагу (Гібридна логіка)
        Vector flee_dir(0, 0, 0);
        for (const auto& p : pursuer_coords) {
            float d = position.vectorTo(p).length();
            if (d < 300.0f) flee_dir = flee_dir + position.vectorTo(p).normalize() * (300.0f - d);
        }
        if (flee_dir.length() > 0.1f) {
            float target_flee = std::atan2(flee_dir.y, flee_dir.x);
            // Плавне підлаштування базового курсу
            base_theta += (target_flee - base_theta) * 0.01f; 
        }

        // Рандомізований зигзаг
        float current_T_half = T_half + std::sin(zigzag_timer * 0.5f) * 0.2f; // Період трохи дихає
        bool phase = (std::fmod(zigzag_timer, 2.0f * current_T_half) < current_T_half);
        
        // Рандомізація кута в момент зміни фази
        float noise = (std::sin(zigzag_timer * 10.0f)) * 0.05f; 
        
        if (phase) {
            smoothed_desired = base_theta + alpha_rad + noise;
        } else {
            smoothed_desired = base_theta - alpha_rad - noise;
        }
        // // --- СТРАТЕГІЯ 2: ЗИГЗАГ (За твоїми розрахунками) ---
        // zigzag_timer += dt;
        
        // // Визначаємо фазу (ліворуч чи праворуч)
        // // Якщо час в межах [0, T_half] — один бік, [T_half, 2*T_half] — інший
        // bool phase = (std::fmod(zigzag_timer, 2.0f * T_half) < T_half);
        
        // if (phase) {
        //     smoothed_desired = base_theta + alpha_rad;
        // } else {
        //     smoothed_desired = base_theta - alpha_rad;
        // }
    }
}

// Додай у public секцію класу escaper:
Vector escaper::getVelocityVector() const {
    float cx = std::cos(theta) * std::cos(phi);
    float cy = std::sin(theta) * std::cos(phi);
    float cz = std::sin(phi);
    return Vector(cx, cy, cz) * v_e;
}

void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> prob_dist(0.0f, 100.0f);

    // 1. Обчислюємо поворот (макс 5 градусів за крок)
    float delta = smoothed_desired - last_theta;
    delta = std::fmod(delta + M_PI, 2.0f * M_PI) - M_PI;
    
    float max_turn = 0.087f; // 5 градусів
    if (delta > max_turn) delta = max_turn;
    if (delta < -max_turn) delta = -max_turn;

    float new_theta = last_theta + delta;

    // 2. Додаємо випадковий "шум" (петляння), якщо спрацював turn_prob
    if (prob_dist(gen) < turn_prob) {
        std::uniform_real_distribution<float> noise(-0.035f, 0.035f); // +/- 2 градуси
        // std::uniform_real_distribution<float> noise(-0.035f, 0.035f);
        new_theta += noise(gen);
    }

    theta = new_theta;
    last_theta = theta;

    // 3. Невеликі коливання по висоті (phi)
    std::uniform_real_distribution<float> pitch_noise(-0.017f, 0.017f);
    phi += pitch_noise(gen) * 0.1f;
    // Обмеження кута набору/зниження (5 градусів)
    if (phi > 0.087f) phi = 0.087f;
    if (phi < -0.087f) phi = -0.087f;

    // 4. Фізичне оновлення позиції
    const float dt = 0.01f; // Використовуємо твій крок часу
    float cx = std::cos(theta) * std::cos(phi);
    float cy = std::sin(theta) * std::cos(phi);
    float cz = std::sin(phi);

    Vector dir(cx, cy, cz);
    position = position + dir.normalize() * v_e * dt;

    // 5. Тримаємо висоту в коридорі (наприклад, 650 - 750 метрів)
    if (position.z > 250.0f) position.z = 250.0f;
    if (position.z < 150.0f) position.z = 150.0f;
}

// Решта твоїх методів без змін
void escaper::setID(double ID) { this->ID = ID; }
int escaper::getID() const { return this->ID; }
void escaper::addPursuer(Pursuer& new_p) { this->my_persuers.push_back(&new_p); }
void escaper::sentPursuersPrivPosition() {
    for (Pursuer* purs : this->my_persuers) {
        if (purs) purs->escaper_coordinate = this->priv_pos;
    }
}
escaper::~escaper() {
    // Тут може бути порожньо, але саме тіло має існувати
}


// #include "escaper.hpp"
// #include "pursuer.hpp"


// escaper::escaper(float x, float y, float z, float ve, int prob)
// /**
//  * @brief Конструктор класу escaper.
//  * @details Ініціалізує початкові координати, швидкість та ймовірність повороту. 
//  * Також встановлює випадковий початковий напрямок руху в 3D просторі.
//  * @param x Початкова координата X.
//  * @param y Початкова координата Y.
//  * @param z Початкова координата Z.
//  * @param ve Початкова лінійна швидкість об'єкта.
//  * @param prob Ймовірність зміни напрямку (у відсотках).
//  */
//     : position(x,y,100.0f), v_e(ve), turn_prob(prob), theta(0.0f), phi(0.0f) { 
//     std::random_device rd;                                              // Створення пристрою для отримання зерна рандомізації
//     std::mt19937 gen(rd());                                             // Ініціалізація генератора чисел Мересенна-Твістера
//     std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * static_cast<float>(M_PI)); // Розподіл для кута азимута (0 до 2π)
//     std::uniform_real_distribution<float> elevation(-static_cast<float>(M_PI) / 2.0f, static_cast<float>(M_PI) / 2.0f); // Розподіл для кута підйому (-π/2 до π/2)
//     theta = azimuth(gen);                                               // Генерація випадкового початкового кута theta
//     //phi = elevation(gen);                                               // Генерація випадкового початкового кута phi
//     phi = 0.0; //якщо по прямій тут летимо
// }

// escaper::~escaper() { 
//  /**
//  * @brief Деструктор класу escaper.
//  * @details Наразі не виконує специфічних дій, оскільки динамічна пам'ять не виділялася.
//  */
// }

// void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
// /**
//  * @brief Розрахунок поточної траєкторії руху.
//  * @details Оновлює позицію об'єкта в просторі на основі його швидкості та поточних кутів напрямку.
//  * Використовує формули переходу від сферичних координат до декартових:
//  * $\Delta x = v_e \cdot dt \cdot \cos(\phi) \cdot \cos(\theta)$
//  * @param pursuer_coords Константне посилання на чергу координат переслідувачів (не використовується безпосередньо в цій функції).
//  */
//     priv_pos = position;

//     const float dt = 0.1f;                                              // Крок часу для ітерації руху
//     position.x += v_e * dt * std::cos(phi) * std::cos(theta);           // Оновлення X: враховуємо швидкість, час та проекцію на вісь X
//     position.y += v_e * dt * std::cos(phi) * std::sin(theta);           // Оновлення Y: враховуємо швидкість, час та проекцію на вісь Y
//     //position.z += v_e * dt * std::sin(phi);                             // Оновлення Z: враховуємо швидкість, час та вертикальну проекцію
// }

// /**
//  * @brief Логіка маневрування з урахуванням групи найближчих переслідувачів.
//  * @details Функція визначає "зону загрози". Якщо кілька переслідувачів знаходяться на приблизно однаковій 
//  * мінімальній відстані (з похибкою 1.0 м), об'єкт обчислює середній вектор втечі від них усіх.
//  * @param pursuer_coords Черга з координатами всіх переслідувачів.
//  * @return Змінює внутрішні кути theta та phi для орієнтації в просторі.
//  */
// void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
//     std::random_device rd;                                              // Джерело випадкових чисел для ініціалізації генератора
//     std::mt19937 gen(rd());                                             // Генератор Мересенна-Твістера для рандомізації
//     std::uniform_int_distribution<int> dis(1, 100);                     // Розподіл для визначення ймовірності маневру
    
//     if (dis(gen) < turn_prob && !pursuer_coords.empty()) {              // Якщо випав шанс повороту і є кого боятися
//         float min_dist_sq = -1.0f;                                      // Змінна для збереження квадрата мінімальної відстані

//         for (const auto& p : pursuer_coords) {                          // ПЕРШИЙ ПРОХІД: Пошук найближчого переслідувача
//             float dx = p.x - position.x;                                // Різниця координат по осі X
//             float dy = p.y - position.y;                                // Різниця координат по осі Y
//             float dz = p.z - position.z;                                // Різниця координат по осі Z
//             float d2 = dx*dx + dy*dy + dz*dz;                           // Обчислення квадрата відстані (без sqrt)
//             if (min_dist_sq < 0.0f || d2 < min_dist_sq) min_dist_sq = d2; // Оновлення мінімуму, якщо знайдено ближчий об'єкт
//         }

//         float min_dist = std::sqrt(min_dist_sq);                        // Обчислення реальної мінімальної відстані (один раз)
//         float avg_dx = 0.0f, avg_dy = 0.0f, avg_dz = 0.0f;              // Накопичувачі для середнього вектора загрози
//         int count = 0;                                                  // Лічильник ворогів у "критичній зоні"

//         for (const auto& p : pursuer_coords) {                          // ДРУГИЙ ПРОХІД: Збір усіх "близьких" ворогів
//             float dx = p.x - position.x;                                // Відносна координата X ворога
//             float dy = p.y - position.y;                                // Відносна координата Y ворога
//             float dz = p.z - position.z;                                // Відносна координата Z ворога
//             float dist = std::sqrt(dx*dx + dy*dy + dz*dz);              // Відстань до поточного ворога
            
//             if (dist <= min_dist + 1.0f) {                              // Перевірка, чи входить ворог у поріг +1 метр
//                 avg_dx += dx / dist;                                    // Додавання нормалізованого вектора (X) до загрози
//                 avg_dy += dy / dist;                                    // Додавання нормалізованого вектора (Y) до загрози
//                 avg_dz += dz / dist;                                    // Додавання нормалізованого вектора (Z) до загрози
//                 count++;                                                // Інкремент кількості знайдених загроз
//             }
//         }

//         if (count > 0) {                                                // Якщо знайдено хоча б одну загрозу (завжди істинно тут)
//             theta = std::atan2(avg_dy, avg_dx) + static_cast<float>(M_PI); // Встановлення азимута в протилежний від сумарної загрози бік
//             //float combined_dist = std::sqrt(avg_dx*avg_dx + avg_dy*avg_dy + avg_dz*avg_dz); // Довжина сумарного вектора загрози
//             //phi = -std::asin(avg_dz / combined_dist);                   // Встановлення кута підйому для втечі від групи
//         }
//     } else if (!pursuer_coords.empty() == false) {                      // Якщо ворогів немає взагалі (випадок вільного руху)
//         std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * M_PI); // Розподіл для випадкового азимута
//         //std::uniform_real_distribution<float> elevation(-M_PI/2, M_PI/2); // Розподіл для випадкового кута висоти
//         theta = azimuth(gen);                                           // Призначення нового випадкового напрямку theta
//         //phi = elevation(gen);                                           // Призначення нового випадкового напрямку phi
//     }
// }

// void escaper::setData(float x, float y, float z, float ve) {
// /**
//  * @brief Оновлення базових даних об'єкта.
//  */
//     position.x = x;                                                     // Перезапис поточної координати X
//     position.y = y;                                                     // Перезапис поточної координати Y
//     position.z = z;                                                     // Перезапис поточної координати Z
//     v_e = ve;                                                           // Оновлення значення швидкості
// }

// void escaper::setID(double ID){
//     this -> ID = ID;
// }

// int escaper::getID() const{
//     return this -> ID;
// }

// void escaper::addPursuer(Pursuer& new_persuers){
//     this->my_persuers.push_back(&new_persuers);
// }

// std::vector<Pursuer*>& escaper::getPursuervector(){
//     return this -> my_persuers;
// }

// void escaper::sentPursuersPrivPosition(){
//     for (Pursuer* purs : this->my_persuers) {
//         if (purs != nullptr) {
//             // Оновлюємо координату прямо в оригінальному об'єкті
//             purs->escaper_coordinate = this->priv_pos;
//         }
//     }
// }