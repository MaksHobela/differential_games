#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <vector>
#include <string>
#include <fstream>
#include "common.hpp"

struct AgentState {
    int step;
    int id;
    std::string type; // "pursuer" або "escaper"
    float x, y, z;
};

class Logger {
private:
    std::vector<AgentState> data;

public:
    void log(int step, int id, const std::string& type, Coordinates pos) {
        data.push_back({step, id, type, pos.x, pos.y, pos.z});
    }

    void saveToCSV(const std::string& filename) {
        std::ofstream file(filename);
        file << "step,id,type,x,y,z\n";
        for (const auto& s : data) {
            file << s.step << "," << s.id << "," << s.type << "," 
                 << s.x << "," << s.y << "," << s.z << "\n";
        }
        file.close();
    }
};

#endif