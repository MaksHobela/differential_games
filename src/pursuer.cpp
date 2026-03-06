#include <cmath>
#include "pursuer.hpp"

void Pursuer::updateBeta() {
    if (v_e != 0.0f) {
        beta = v_p / v_e;
    } else {
        beta = 1.0f;
    }
}

void Pursuer::calculate_new_circle(const Vector& escaper_vector) {
    this->escaper_vector = escaper_vector;

    float denominator = (beta - 1.0f) * (beta + 1.0f);
    Vector r0 = my_coordinate.vectorTo(escaper_coordinate);
    float r0_len = r0.length();

    if (denominator != 0.0f) {
        r_Apoll = (beta / denominator) * r0_len;
        float c = r0_len / denominator;

        Vector diff = my_coordinate.vectorTo(escaper_coordinate);
        Vector centerVec = diff * (c / (r0_len != 0.0f ? r0_len : 1.0f));
        Coordinates C = my_coordinate + centerVec;

        get_Apoll_dots(C);
    } 
    
}

Coordinates Pursuer::get_Apoll_dots(const Coordinates& C) {
    Vector d = escaper_coordinate.vectorTo(C);
    Vector u = escaper_vector.normalize();

    float du = d.dot(u);
    float dd = d.dot(d);

    float D = du * du - (dd - r_Apoll * r_Apoll);
    if (D < 0.0f) {
        return escaper_coordinate;
    }

    float t1 = -du - std::sqrt(D);
    float t2 = -du + std::sqrt(D);

    float t = (t1 > 0.0f) ? t1 : t2;
    Vector res = my_coordinate.vectorTo(escaper_coordinate + u * t);
    my_vector = res.normalize();
    return escaper_coordinate + u * t;
}

Coordinates Pursuer::interceptionPoint(const Coordinates& evader_pos) {
    return my_coordinate;
}

Coordinates Pursuer::getCoordinates() const {
    return my_coordinate;
}

void Pursuer::makeMove(float dt) {
    my_coordinate = my_coordinate + my_vector * (v_p * dt);
}

void Pursuer::setData(float x, float y, float z, float ve) {
    my_coordinate = Coordinates(x, y, z);
    v_e = ve;
}

