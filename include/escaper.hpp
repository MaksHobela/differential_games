#pragma once

#include <deque>
#include "common.hpp"

class escaper {
public:
    Coordinates position;
    float v_e;
    int turn_prob;

private:
    float theta;
    float phi;

public:
    escaper(float x = 0.0f, float y = 0.0f, float z = 0.0f, float ve = 1.0f, int prob = 30);
    ~escaper();
    void calculate_trajectory(const std::deque<Coordinates>& pursuer_coords);
    void turn(const std::deque<Coordinates>& pursuer_coords);
    Coordinates getCoordinates() const {
        return position;
    }
    
    void setData(float x, float y, float z, float ve);
};
