#include "pursuer.hpp"
#include <cmath>

void Pursuer::updateBeta() { beta = 0.0f; }
void Pursuer::calculate_new_circle(const Vector& escaper_vector) { r_Apoll = 0.0f; }
Coordinates Pursuer::get_Apoll_dots(const Coordinates& C) { return C; }

Coordinates Pursuer::interceptionPoint(const Coordinates& evader_pos) {
    return evader_pos; // pure pursuit (оптимально для v_p > v_e)
}

Coordinates Pursuer::getCoordinates() const { return my_coordinate; }

void Pursuer::makeMove(float dt) {
    Coordinates target = interceptionPoint(escaper_coordinate);
    Vector dir = my_coordinate.vectorTo(target);
    float len = dir.length();
    if (len > 0.0f) {
        dir = dir.normalize() * v_p;
    }
    my_vector = dir;
    my_coordinate = my_coordinate + dir * dt;
}

void Pursuer::setData(float x, float y, float z, float ve) {
    my_coordinate = Coordinates(x, y, z);
    v_e = ve;
}
