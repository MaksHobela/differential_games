#include <cmath>                                                       
#include "pursuer.hpp"                                                  
#include "escaper.hpp"
void Pursuer::updateBeta() {
/**
 * @brief Оновлення коефіцієнта співвідношення швидкостей.
 * @details Розраховує $\beta$, де $\beta = \frac{v_p}{v_e}$. Це значення визначає параметри кола Аполлонія.
 * @modifies beta
 */
    if (v_e != 0.0f) {                                                  // Перевірка, щоб уникнути ділення на нуль, якщо втікач стоїть
        beta = v_p / v_e;                                               // Розрахунок відношення швидкості переслідувача до швидкості втікача
    } else {
        beta = 1.0f;                                                    // Дефолтне значення, якщо швидкість втікача нульова
    }
}

void Pursuer::updateEscaper(escaper new_escaper) {
/**
 оновлюємо за ким женемось
 */
    this -> my_escaper = new_escaper;
    this -> escaper_coordinate = new_escaper.getCoordinates();
}
void Pursuer::setID(double ID){
    this -> ID = ID;
}
int Pursuer::getID(){
    return this -> ID;
}
int Pursuer::getTargetID(){
    return this -> my_escaper.ID;
}

void Pursuer::calculate_new_circle(const Vector& escaper_vector) {
/**
 * @brief Розрахунок параметрів сфери (кола) Аполлонія.
 * @details Визначає радіус та центр геометричного місця точок можливого перехоплення.
 * @param escaper_vector Поточний вектор напрямку руху втікача.
 * @modifies r_Apoll, escaper_vector
 */
    this->escaper_vector = escaper_vector;                              // Оновлення внутрішнього вектора напрямку втікача

    float denominator = (beta - 1.0f) * (beta + 1.0f);                  // Розрахунок знаменника для формул ($\beta^2 - 1$)
    Vector r0 = my_coordinate.vectorTo(escaper_coordinate);             // Вектор від переслідувача до втікача
    float r0_len = r0.length();                                         // Обчислення поточної відстані між об'єктами

    if (denominator != 0.0f) {                                          // Перевірка на випадок рівних швидкостей (beta=1), коли сфера стає площиною
        r_Apoll = (beta / denominator) * r0_len;                        // Розрахунок радіуса Аполлонієвої сфери
        float c = r0_len / denominator;                                 // Коефіцієнт для знаходження зміщення центру сфери

        Vector diff = my_coordinate.vectorTo(escaper_coordinate);       // Повторний розрахунок вектора напрямку до цілі
        Vector centerVec = diff * (c / (r0_len != 0.0f ? r0_len : 1.0f)); // Масштабування вектора для пошуку центру
        Coordinates C = my_coordinate + centerVec;                      // Знаходження координат центру C сфери Аполлонія

        get_Apoll_dots(C);                                              // Виклик методу для пошуку точки перетину траєкторії зі сферою
    } 
}

Coordinates Pursuer::get_Apoll_dots(const Coordinates& C) {
/**
 * @brief Пошук точки перетину траєкторії втікача зі сферою Аполлонія.
 * @details Розв'язує квадратне рівняння для знаходження точки, де переслідувач і втікач зустрінуться.
 * @param C Координати центру сфери Аполлонія.
 * @return Координати точки зустрічі.
 * @modifies my_vector (встановлює новий напрямок руху переслідувача)
 */
    Vector d = escaper_coordinate.vectorTo(C);                          // Вектор від початкової точки втікача до центру сфери
    Vector u = escaper_vector.normalize();                              // Нормалізований вектор напрямку втікача (одиничний вектор)

    float du = d.dot(u);                                                // Скалярний добуток вектора d на напрямок u
    float dd = d.dot(d);                                                // Квадрат довжини вектора d

    float D = du * du - (dd - r_Apoll * r_Apoll);                       // Розрахунок дискримінанта для пошуку перетину лінії зі сферою
    if (D < 0.0f) {                                                     // Якщо дискримінант від'ємний — перетину немає
        return escaper_coordinate;                                      // Повертаємо поточну позицію втікача як fallback
    }

    float t1 = -du - std::sqrt(D);                                      // Перший корінь рівняння (ближча точка перетину)
    float t2 = -du + std::sqrt(D);                                      // Другий корінь рівняння (дальша точка перетину)

    float t = (t1 > 0.0f) ? t1 : t2;                                    // Вибір позитивного значення параметра t (час/відстань вздовж вектора)
    Vector res = my_coordinate.vectorTo(escaper_coordinate + u * t);    // Вектор від переслідувача до точки майбутньої зустрічі
    my_vector = res.normalize();                                        // Оновлення вектора руху переслідувача (курс на перехоплення)
    return escaper_coordinate + u * t;                                  // Повернення обчислених координат точки зустрічі
}

Coordinates Pursuer::interceptionPoint(const Coordinates& evader_pos) {
/**
 * @brief Заглушка для розрахунку точки перехоплення.
 * @param evader_pos Позиція об'єкта, що уникає перехоплення.
 * @return Поточні координати (поки не реалізовано повноцінно).
 */
    return my_coordinate;                                               // Повертає власні координати (заглушка)
}

Coordinates Pursuer::getCoordinates() const {
/**
 * @brief Отримання поточних координат переслідувача.
 * @return Об'єкт Coordinates з поточною позицією.
 */
    return my_coordinate;                                               // Повернення поточної позиції переслідувача
}

void Pursuer::makeMove(float dt) {
/**
 * @brief Здійснення кроку руху переслідувача.
 * @details Оновлює координати об'єкта на основі його вектора швидкості та проміжку часу.
 * @param dt Крок часу (delta time).
 */
    my_coordinate = my_coordinate + my_vector * (v_p * dt);             // Нова позиція = Стара позиція + (Напрямок * Швидкість * Час)
}

void Pursuer::setData(float x, float y, float z, float ve) {
/**
 * @brief Встановлення базових параметрів переслідувача.
 * @param x Координата X.
 * @param y Координата Y.
 * @param z Координата Z.
 * @param ve Швидкість втікача (необхідна для розрахунку beta).
 */
    my_coordinate = Coordinates(x, y, z);                               // Встановлення початкової або нової позиції
    v_e = ve;                                                           // Оновлення відомої швидкості втікача
}