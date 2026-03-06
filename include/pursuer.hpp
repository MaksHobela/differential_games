#pragma once

#include "common.hpp"

class Pursuer {
public:
    float r_Apoll;
    float v_p;
    float v_e;
    float beta;

    Vector escaper_vector;
    Coordinates escaper_coordinate;
    Vector my_vector;
    Coordinates my_coordinate;

    void updateBeta();
    void calculate_new_circle(const Vector& escaper_vector);
    Coordinates get_Apoll_dots(const Coordinates& C);
    Coordinates interceptionPoint(const Coordinates& evader_pos);

    Coordinates getCoordinates() const;
    void makeMove(float dt);

    void setData(float x, float y, float z, float ve);
};
