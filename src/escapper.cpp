#include "escapper.hpp"
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
    current_strategy = Strategy::SMART_ZIGZAG;
    
    // Початковий напрямок (наприклад, вздовж осі Y)
    // base_theta = M_PI / 2.0f; 
    base_theta = -M_PI/2; 
}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
    priv_pos = position;
    const float dt = 0.01f;

    // 1. Пошук найближчих загроз для прийняття рішення
    std::vector<Coordinates> nearby;
    float min_dist_to_threat = 999999.0f;
    
    for (const auto& p : pursuer_coords) {
        float d = position.vectorTo(p).length();
        if (d > 0.1f) {
            if (d < 500.0f) nearby.push_back(p);
            if (d < min_dist_to_threat) min_dist_to_threat = d;
        }
    }

    // 2. Вибір поведінки всередині стратегій
    if (current_strategy == Strategy::EVASIVE) {
        // Завжди втікаємо (твій оригінальний код)
        run_evasive_logic(nearby); 
    } 
    else if (current_strategy == Strategy::ZIGZAG) {
        // Чистий зигзаг без оглядки на ракети
        run_zigzag_logic(dt);
    } 
    else if (current_strategy == Strategy::SMART_ZIGZAG) {
        // Гібридна стратегія:
        // Якщо загроза ближче ніж 100 метрів — активне ухилення
        // Інакше — продовжуємо зигзаг
        if (min_dist_to_threat < 100.0f && !nearby.empty()) {
            run_evasive_logic(nearby);
        } else {
            run_zigzag_logic(dt);
        }
    }
}

void escaper::run_zigzag_logic(float dt) {
    zigzag_timer += dt;
    bool phase = (std::fmod(zigzag_timer, 2.0f * T_half) < T_half);
    smoothed_desired = phase ? (base_theta + alpha_rad) : (base_theta - alpha_rad);
}

void escaper::run_evasive_logic(const std::vector<Coordinates>& nearby) {
    if (nearby.empty()) return;

    Vector new_dir(0, 0, 0);
    if (nearby.size() == 1) {
        new_dir = nearby[0].vectorTo(position);
    } else {
        Vector line = nearby[0].vectorTo(nearby[1]);
        Vector perp(line.y, -line.x, 0.0f);
        new_dir = perp;
    }

    Vector unit = new_dir.normalize();
    float instant_target_theta = std::atan2(unit.y, unit.x);

    float delta = instant_target_theta - smoothed_desired;
    delta = std::fmod(delta + M_PI, 2.0f * M_PI) - M_PI;
    
    float max_smooth = 0.0072f; 
    if (delta > max_smooth) delta = max_smooth;
    if (delta < -max_smooth) delta = -max_smooth;

    smoothed_desired += delta;
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