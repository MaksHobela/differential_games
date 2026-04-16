#pragma once
#include "escaper.hpp"
#include "common.hpp"

class Pursuer {
public:
    float r_Apoll;
    float v_p;
    float v_e;
    float beta;
    int ID;

    escaper* my_escaper=nullptr;
    Vector escaper_vector;
    Coordinates escaper_coordinate;
    Vector my_vector;
    Coordinates my_coordinate;

    void setID(double ID);
    int getID() const; 
// Coordinates getCoordinates() const;
    int getTargetID();
    void updateBeta();
    void updateEscaper(escaper& new_escaper);
    void calculate_new_circle(const Vector& escaper_vector);
    Coordinates get_Apoll_dots(const Coordinates& C);
    void interceptionPoint(const Coordinates& evader_pos);

    Coordinates getCoordinates() const;
    void makeMove(float dt);

    void setData(float x, float y, float z, float ve);
};
