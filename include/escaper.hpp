#ifndef ESCAPER_HPP
#define ESCAPER_HPP

#include <vector>
#include <deque>
#include "common.hpp" // Твій файл з координатами та векторами

// Важливо: forward declaration, щоб уникнути помилки "incomplete type"
class Pursuer; 

class escaper {
public:
    escaper(float x = 0, float y = 0, float z = 200.0f, float ve = 1.0f, int prob = 20);
    ~escaper();

    void calculate_trajectory(const std::deque<Coordinates>& pursuer_coords);
    void turn(const std::deque<Coordinates>& pursuer_coords);
    
    void setData(float x, float y, float z, float ve);
    void setID(double ID);
    int getID() const;
    
    void addPursuer(Pursuer& new_pursuer);
    std::vector<Pursuer*>& getPursuervector();
    void sentPursuersPrivPosition();
    Vector getVelocityVector() const;
    
    Coordinates getCoordinates() const { return position; }

private:
    Coordinates position;
    Coordinates priv_pos;   // Попередня позиція для передачі переслідувачам
    float v_e;              // Швидкість
    int turn_prob;          // Ймовірність повороту
    float theta;            // Кут азимута
    float phi;              // Кут підйому
    int ID;

    // --- НОВІ ПОЛЯ (виправляють помилку "undefined") ---
    float last_theta;       
    float smoothed_desired; 

    std::vector<Pursuer*> my_persuers; // Список переслідувачів, закріплених за цим втікачем
};

#endif



// #pragma once

// #include <deque>
// #include "common.hpp"
// #include <vector>
// class Pursuer;

// class escaper {
// public:
//     Coordinates priv_pos;
//     Coordinates position;
//     float v_e;
//     int turn_prob;
//     int ID;
//     std::vector<Pursuer*> my_persuers;

// private:
//     float theta;
//     float phi;

// public:
//     escaper(float x = 0.0f, float y = 0.0f, float z = 0.0f, float ve = 1.0f, int prob = 30);
//     ~escaper();
//     void setID(double ID);
//     int getID() const;
//     void addPursuer(Pursuer& new_persuers);
//     std::vector<Pursuer*>& getPursuervector();
//     void sentPursuersPrivPosition();
//     void calculate_trajectory(const std::deque<Coordinates>& pursuer_coords);
//     void turn(const std::deque<Coordinates>& pursuer_coords);
//     Coordinates getCoordinates() const {
//         return position;
//     }
    
//     void setData(float x, float y, float z, float ve);
// };