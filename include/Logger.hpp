#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <vector>
#include <string>
#include <fstream>
#include "common.hpp"

struct AgentState {
    int step;
    int id;
    int scenario_id;
    std::string type; // "pursuer" або "escaper"
    float x, y, z;
};

class Logger {
private:
    std::vector<AgentState> data;

public:
    void log(int step, int id, int scenario_id, const std::string& type, Coordinates pos) {
        data.push_back({step, id, scenario_id, type, pos.x, pos.y, pos.z});
    }

    void saveToCSV(const std::string& filename) {
        std::ofstream file(filename);
        file << "step,id,scenario_id,type,x,y,z\n";
        for (const auto& s : data) {
            file << s.step << "," << s.id << "," << s.scenario_id << "," << s.type << "," 
                 << s.x << "," << s.y << "," << s.z << "\n";
        }
        file.close();
    }
};

// #include "scenario_gen.hpp"
// #include <sstream>

// std::vector<CoordPack> parseLineToCoords(const std::string& line) {
//     std::vector<CoordPack> result;
//     std::stringstream ss(line);
//     std::string segment;

//     // 1. Пропускаємо перші два поля: тип стратегії та ID сценарію
//     std::getline(ss, segment, ','); // Пропуск "wedge"
//     std::getline(ss, segment, ','); // Пропуск "0"

//     // 2. Читаємо кожен сегмент (координати втікача), розділений комою
//     while (std::getline(ss, segment, ',')) {
//         if (segment.empty()) continue;

//         std::stringstream space_ss(segment);
//         float x, y, z;
        
//         // 3. Зчитуємо три числа, розділені пробілами всередині сегмента
//         if (space_ss >> x >> y >> z) {
//             result.push_back({x, y, z});
//         }
//     }

//     return result;
// }

#endif