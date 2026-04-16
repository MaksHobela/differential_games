#include "pursuer.hpp"
#include <cmath>

void Pursuer::updateBeta() { beta = 0.0f; }
void Pursuer::calculate_new_circle(const Vector& escaper_vector) { r_Apoll = 0.0f; }
Coordinates Pursuer::get_Apoll_dots(const Coordinates& C) { return C; }

Coordinates Pursuer::interceptionPoint(const Coordinates& evader_pos) {
    return evader_pos;
}

Coordinates Pursuer::getCoordinates() const { return my_coordinate; }

void Pursuer::makeMove(float dt) {
    Coordinates target = interceptionPoint(escaper_coordinate);
    Vector desired = my_coordinate.vectorTo(target);
    float len = desired.length();
    if (len == 0.0f) return;

    desired = desired.normalize() * v_p;

    Vector current = my_vector;
    float current_len = current.length();
    if (current_len > 0.0f) current = current.normalize();

    float dot = current.dot(desired);
    dot = std::max(-1.0f, std::min(1.0f, dot));
    float angle = std::acos(dot);

    const float max_angle = 0.0262f;

    if (angle > max_angle) {
        Vector perp = desired - current * dot;
        float perp_len = perp.length();
        if (perp_len > 0.0f) {
            perp = perp * (1.0f / perp_len);
            desired = current * std::cos(max_angle) + perp * std::sin(max_angle);
            desired = desired * v_p;
        }
    }

    my_vector = desired;
    my_coordinate = my_coordinate + desired * dt;
}

void Pursuer::setData(float x, float y, float z, float ve) {
    my_coordinate = Coordinates(x, y, z);
    v_e = ve;
    my_vector = Vector(0, 0, 0);
}
