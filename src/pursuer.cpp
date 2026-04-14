#include "pursuer.hpp"
#include <cmath>

void Pursuer::updateBeta() {
    if (v_e != 0.0f) beta = v_p / v_e;
    else beta = 1.0f;
}

Coordinates Pursuer::getCoordinates() const {
    return my_coordinate;
}

void Pursuer::calculate_new_circle(const Vector& escaper_vector) {
    this->escaper_vector = escaper_vector;
    float denominator = (beta - 1.0f) * (beta + 1.0f);
    Vector r0 = my_coordinate.vectorTo(escaper_coordinate);
    float r0_len = r0.length();

    if (std::abs(denominator) > 0.0001f) {
        r_Apoll = (beta / std::abs(denominator)) * r0_len;
        float c = r0_len / denominator;
        Vector diff = my_coordinate.vectorTo(escaper_coordinate);
        Vector centerVec = diff * (c / (r0_len != 0.0f ? r0_len : 1.0f));
        Coordinates C = my_coordinate + centerVec;
        // Оновлюємо внутрішній стан, якщо потрібно для інших методів
    }
}

Coordinates Pursuer::interceptionPoint(const Coordinates& evader_pos) {
    // Спрощена логіка перехоплення для стабільності
    return evader_pos; 
}

void Pursuer::setData(float x, float y, float z, float ve) {
    my_coordinate = {x, y, z};
    v_p = 150.0f; 
    v_e = ve;
    updateBeta();
}

void Pursuer::makeMove(float dt) {
    Coordinates target = interceptionPoint(escaper_coordinate);
    Vector diff = my_coordinate.vectorTo(target);
    
    if (diff.length() > 0.1f) {
        Vector dir = diff.normalize();
        my_coordinate.x += dir.x * v_p * dt;
        my_coordinate.y += dir.y * v_p * dt;
        my_coordinate.z += dir.z * v_p * dt;
    }
}
