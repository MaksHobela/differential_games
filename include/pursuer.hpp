#pragma once
#include "escaper.hpp"
#include "common.hpp"

enum class InterceptStrategy {
    DIRECT_APOLLONIUS, // Стандартна (те, що є зараз)
    VERTICAL_FIRST     // Спочатку висота, потім Аполлоній
};

class Pursuer {
public:
    float r_Apoll;
    float v_p;
    float v_e;
    float beta;
    int ID;
    InterceptStrategy current_strategy;
    bool height_reached = false;

    escaper* my_escaper=nullptr;
    Vector escaper_vector;
    Coordinates escaper_coordinate;
    Vector my_vector;
    Coordinates my_coordinate;

    void setID(double ID);
    int getID() const; 
// Coordinates getCoordinates() const;
    int getTargetID();
    int getTargetID() const;
    void updateBeta();
    void updateEscaper(escaper& new_escaper);
    void calculate_new_circle(const Vector& escaper_vector);
    Coordinates get_Apoll_dots(const Coordinates& C);
    void interceptionPoint(const Coordinates& evader_pos);
    void applySmoothTurn(const Coordinates& targetPoint, bool ignore_pitch_limit = false);

    Coordinates getCoordinates() const;
    void makeMove(float dt);

    void setData(float x, float y, float z, float vp, float ve);
};
