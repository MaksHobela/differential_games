#include "escaper.hpp"


escaper::escaper(float x, float y, float z, float ve, int prob)
/**
 * @brief Конструктор класу escaper.
 * @details Ініціалізує початкові координати, швидкість та ймовірність повороту. 
 * Також встановлює випадковий початковий напрямок руху в 3D просторі.
 * @param x Початкова координата X.
 * @param y Початкова координата Y.
 * @param z Початкова координата Z.
 * @param ve Початкова лінійна швидкість об'єкта.
 * @param prob Ймовірність зміни напрямку (у відсотках).
 */
    : position(x,y,z=700.0), v_e(ve), turn_prob(prob), theta(0.0f), phi(0.0f) { 
    std::random_device rd;                                              // Створення пристрою для отримання зерна рандомізації
    std::mt19937 gen(rd());                                             // Ініціалізація генератора чисел Мересенна-Твістера
    std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * static_cast<float>(M_PI)); // Розподіл для кута азимута (0 до 2π)
    std::uniform_real_distribution<float> elevation(-static_cast<float>(M_PI) / 2.0f, static_cast<float>(M_PI) / 2.0f); // Розподіл для кута підйому (-π/2 до π/2)
    theta = azimuth(gen);                                               // Генерація випадкового початкового кута theta
    //phi = elevation(gen);                                               // Генерація випадкового початкового кута phi
    phi = 0.0; //якщо по прямій тут летимо
}

escaper::~escaper() { 
 /**
 * @brief Деструктор класу escaper.
 * @details Наразі не виконує специфічних дій, оскільки динамічна пам'ять не виділялася.
 */
}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
/**
 * @brief Розрахунок поточної траєкторії руху.
 * @details Оновлює позицію об'єкта в просторі на основі його швидкості та поточних кутів напрямку.
 * Використовує формули переходу від сферичних координат до декартових:
 * $\Delta x = v_e \cdot dt \cdot \cos(\phi) \cdot \cos(\theta)$
 * @param pursuer_coords Константне посилання на чергу координат переслідувачів (не використовується безпосередньо в цій функції).
 */
    const float dt = 0.1f;                                              // Крок часу для ітерації руху
    position.x += v_e * dt * std::cos(phi) * std::cos(theta);           // Оновлення X: враховуємо швидкість, час та проекцію на вісь X
    position.y += v_e * dt * std::cos(phi) * std::sin(theta);           // Оновлення Y: враховуємо швидкість, час та проекцію на вісь Y
    //position.z += v_e * dt * std::sin(phi);                             // Оновлення Z: враховуємо швидкість, час та вертикальну проекцію
}

/**
 * @brief Логіка маневрування з урахуванням групи найближчих переслідувачів.
 * @details Функція визначає "зону загрози". Якщо кілька переслідувачів знаходяться на приблизно однаковій 
 * мінімальній відстані (з похибкою 1.0 м), об'єкт обчислює середній вектор втечі від них усіх.
 * @param pursuer_coords Черга з координатами всіх переслідувачів.
 * @return Змінює внутрішні кути theta та phi для орієнтації в просторі.
 */
void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
    std::random_device rd;                                              // Джерело випадкових чисел для ініціалізації генератора
    std::mt19937 gen(rd());                                             // Генератор Мересенна-Твістера для рандомізації
    std::uniform_int_distribution<int> dis(1, 100);                     // Розподіл для визначення ймовірності маневру
    
    if (dis(gen) < turn_prob && !pursuer_coords.empty()) {              // Якщо випав шанс повороту і є кого боятися
        float min_dist_sq = -1.0f;                                      // Змінна для збереження квадрата мінімальної відстані

        for (const auto& p : pursuer_coords) {                          // ПЕРШИЙ ПРОХІД: Пошук найближчого переслідувача
            float dx = p.x - position.x;                                // Різниця координат по осі X
            float dy = p.y - position.y;                                // Різниця координат по осі Y
            float dz = p.z - position.z;                                // Різниця координат по осі Z
            float d2 = dx*dx + dy*dy + dz*dz;                           // Обчислення квадрата відстані (без sqrt)
            if (min_dist_sq < 0.0f || d2 < min_dist_sq) min_dist_sq = d2; // Оновлення мінімуму, якщо знайдено ближчий об'єкт
        }

        float min_dist = std::sqrt(min_dist_sq);                        // Обчислення реальної мінімальної відстані (один раз)
        float avg_dx = 0.0f, avg_dy = 0.0f, avg_dz = 0.0f;              // Накопичувачі для середнього вектора загрози
        int count = 0;                                                  // Лічильник ворогів у "критичній зоні"

        for (const auto& p : pursuer_coords) {                          // ДРУГИЙ ПРОХІД: Збір усіх "близьких" ворогів
            float dx = p.x - position.x;                                // Відносна координата X ворога
            float dy = p.y - position.y;                                // Відносна координата Y ворога
            float dz = p.z - position.z;                                // Відносна координата Z ворога
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);              // Відстань до поточного ворога
            
            if (dist <= min_dist + 1.0f) {                              // Перевірка, чи входить ворог у поріг +1 метр
                avg_dx += dx / dist;                                    // Додавання нормалізованого вектора (X) до загрози
                avg_dy += dy / dist;                                    // Додавання нормалізованого вектора (Y) до загрози
                avg_dz += dz / dist;                                    // Додавання нормалізованого вектора (Z) до загрози
                count++;                                                // Інкремент кількості знайдених загроз
            }
        }

        if (count > 0) {                                                // Якщо знайдено хоча б одну загрозу (завжди істинно тут)
            theta = std::atan2(avg_dy, avg_dx) + static_cast<float>(M_PI); // Встановлення азимута в протилежний від сумарної загрози бік
            //float combined_dist = std::sqrt(avg_dx*avg_dx + avg_dy*avg_dy + avg_dz*avg_dz); // Довжина сумарного вектора загрози
            //phi = -std::asin(avg_dz / combined_dist);                   // Встановлення кута підйому для втечі від групи
        }
    } else if (!pursuer_coords.empty() == false) {                      // Якщо ворогів немає взагалі (випадок вільного руху)
        std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * M_PI); // Розподіл для випадкового азимута
        //std::uniform_real_distribution<float> elevation(-M_PI/2, M_PI/2); // Розподіл для випадкового кута висоти
        theta = azimuth(gen);                                           // Призначення нового випадкового напрямку theta
        //phi = elevation(gen);                                           // Призначення нового випадкового напрямку phi
    }
}

void escaper::setData(float x, float y, float z, float ve) {
/**
 * @brief Оновлення базових даних об'єкта.
 */
    position.x = x;                                                     // Перезапис поточної координати X
    position.y = y;                                                     // Перезапис поточної координати Y
    position.z = z;                                                     // Перезапис поточної координати Z
    v_e = ve;                                                           // Оновлення значення швидкості
}

void escaper::setID(double ID){
    this -> ID = ID;
}

int escaper::getID(){
    return this -> ID;
}