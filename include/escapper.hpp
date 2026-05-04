#ifndef ESCAPER_HPP
#define ESCAPER_HPP

#include <vector>
#include <deque>
#include "common.hpp" 

class Pursuer; 

class escaper {
public:
    // Виносимо Strategy всередину класу
    enum class Strategy { EVASIVE, ZIGZAG, SMART_ZIGZAG };

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
    void run_zigzag_logic(float dt);
    void run_evasive_logic(const std::vector<Coordinates>& nearby);

private:
    Coordinates position;
    Coordinates priv_pos;   
    float v_e;              
    int turn_prob;          
    float theta;            
    float phi;              
    int ID;

    float last_theta;       
    float smoothed_desired; 

    // --- ТЕПЕР ЦЕ ПОЛЯ КЛАСУ (не будуть створювати конфліктів) ---
    Strategy current_strategy;
    float zigzag_timer;
    float base_theta;

    // Константи (однакові для всіх об'єктів)
    static constexpr float alpha_rad = 0.4363f; // 25 градусів
    static constexpr float T_half = 3.82f;     // Напівперіод

    std::vector<Pursuer*> my_persuers; 
};

#endif


// #ifndef ESCAPER_HPP
// #define ESCAPER_HPP

// #include <vector>
// #include <deque>
// #include "common.hpp" // Твій файл з координатами та векторами

// // Важливо: forward declaration, щоб уникнути помилки "incomplete type"
// class Pursuer; 

// class escaper {
// public:
//     escaper(float x = 0, float y = 0, float z = 200.0f, float ve = 1.0f, int prob = 20);
//     ~escaper();

//     void calculate_trajectory(const std::deque<Coordinates>& pursuer_coords);
//     void turn(const std::deque<Coordinates>& pursuer_coords);
    
//     void setData(float x, float y, float z, float ve);
//     void setID(double ID);
//     int getID() const;
    
//     void addPursuer(Pursuer& new_pursuer);
//     std::vector<Pursuer*>& getPursuervector();
//     void sentPursuersPrivPosition();
//     Vector getVelocityVector() const;
    
//     Coordinates getCoordinates() const { return position; }

// private:
//     Coordinates position;
//     Coordinates priv_pos;   // Попередня позиція для передачі переслідувачам
//     float v_e;              // Швидкість
//     int turn_prob;          // Ймовірність повороту
//     float theta;            // Кут азимута
//     float phi;              // Кут підйому
//     int ID;

//     // --- НОВІ ПОЛЯ (виправляють помилку "undefined") ---
//     float last_theta;       
//     float smoothed_desired; 

//     std::vector<Pursuer*> my_persuers; // Список переслідувачів, закріплених за цим втікачем
// };

// enum class Strategy { EVASIVE, ZIGZAG };
// Strategy current_strategy;

// // Параметри зигзагу
// float zigzag_timer = 0.0f;
// const float alpha_rad = 0.4363f; // 25 градусів у радіанах
// const float T_half = 11.82f;     // Напівперіод (половина від 23.65с)
// float base_theta = 0.0f;         // Основний курс, навколо якого йде зигзаг

// #endif



// // #pragma once

// // #include <deque>
// // #include "common.hpp"
// // #include <vector>
// // class Pursuer;

// // class escaper {
// // public:
// //     Coordinates priv_pos;
// //     Coordinates position;
// //     float v_e;
// //     int turn_prob;
// //     int ID;
// //     std::vector<Pursuer*> my_persuers;

// // private:
// //     float theta;
// //     float phi;

// // public:
// //     escaper(float x = 0.0f, float y = 0.0f, float z = 0.0f, float ve = 1.0f, int prob = 30);
// //     ~escaper();
// //     void setID(double ID);
// //     int getID() const;
// //     void addPursuer(Pursuer& new_persuers);
// //     std::vector<Pursuer*>& getPursuervector();
// //     void sentPursuersPrivPosition();
// //     void calculate_trajectory(const std::deque<Coordinates>& pursuer_coords);
// //     void turn(const std::deque<Coordinates>& pursuer_coords);
// //     Coordinates getCoordinates() const {
// //         return position;
// //     }
    
// //     void setData(float x, float y, float z, float ve);
// // };