#include "escaper.hpp"
#include <cmath>

escaper::escaper(float x, float y, float z, float ve, int prob)
    : position(x, y, z), v_e(ve), turn_prob(prob) {
    theta = 0.0f; 
    phi = 0.0f; // Горизонтальний політ спочатку
}

escaper::~escaper() {}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
    const float dt = 0.1f;
    
    // Якщо ППО близько, пробуємо змінити курс
    turn(pursuer_coords);

    position.x += v_e * dt * std::cos(phi) * std::cos(theta);
    position.y += v_e * dt * std::cos(phi) * std::sin(theta);
    position.z += v_e * dt * std::sin(phi);
}

void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
    if (pursuer_coords.empty()) return;

    // Дуже малий кут повороту (імітація низької маневреності)
    const float max_turn = 0.01f; 
    
    Coordinates p = pursuer_coords.front();
    float dx = p.x - position.x;
    float dy = p.y - position.y;
    float target_theta = std::atan2(dy, dx);

    // Шахед намагається відвернути від найближчої ракети, але ледь-ледь
    if (theta < target_theta) theta += max_turn;
    else theta -= max_turn;
}

void escaper::setData(float x, float y, float z, float ve) {
    position = {x, y, z};
    v_e = ve;
}
