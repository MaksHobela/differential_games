#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

struct Point {
    float x, y, z;
};

void generateStrategies() {
    const int count_p = 10;
    const float width = 2000.0f;
    // Нова точка старту
    const float start_x = 50.0f; 
    // Центр для дуги (середня точка між 50 та 2050)
    const float center_x = start_x + (width / 2.0f); // 1050.0f
    
    const float p_step = width / (count_p - 1);
    const float radius = 500.0f; 

    for (int strategy = 1; strategy <= 3; ++strategy) {
        std::cout << "\n--- STRATEGY " << strategy << " ---" << std::endl;
        
        if (strategy == 1) std::cout << "// LINEAR (50 to 2050)" << std::endl;
        if (strategy == 2) std::cout << "// STAGGERED (CHESS)" << std::endl;
        if (strategy == 3) std::cout << "// ARC (CRESCENT)" << std::endl;

        for (int i = 0; i < count_p; ++i) {
            float x = 0, y = 0, z = 0;

            if (strategy == 1) {
                // Пряма лінія від 50 до 2050
                x = start_x + (i * p_step);
                y = 0.0f;
            } 
            else if (strategy == 2) {
                // Шахи зі зміщенням по X
                x = start_x + (i * p_step);
                y = (i % 2 == 0) ? 0.0f : 150.0f;
            } 
            else if (strategy == 3) {
                // Дуга: кут від PI (ліворуч) до 0 (праворуч)
                float angle = M_PI - (M_PI * (float)i / (count_p - 1));
                // Розраховуємо x відносно центру (1050)
                x = center_x + (width / 2.0f) * std::cos(angle);
                y = 492.40-radius * std::sin(angle);
            }

            std::cout << "(" << std::fixed << std::setprecision(2) 
                      << x << ", " << y << ", " << z << ")" 
                      << (i < count_p - 1 ? ", " : "") << std::endl;
        }
    }
}

int main() {
    generateStrategies();
    return 0;
}