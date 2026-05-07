#include "escaper.hpp"

escaper::escaper(float x, float y, float z, float ve, int prob)
    : position(x,y,z), v_e(ve), turn_prob(prob), theta(0.0f), phi(0.0f) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * static_cast<float>(M_PI));
    std::uniform_real_distribution<float> elevation(-static_cast<float>(M_PI) / 2.0f, static_cast<float>(M_PI) / 2.0f);
    theta = azimuth(gen);
    phi = elevation(gen);
}

escaper::~escaper() {
}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
    const float dt = 0.1f;
    position.x += v_e * dt * std::cos(phi) * std::cos(theta);
    position.y += v_e * dt * std::cos(phi) * std::sin(theta);
    position.z += v_e * dt * std::sin(phi);
}

void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1, 100);
    int n = dis(gen);
    if (n < turn_prob) {
        if (!pursuer_coords.empty()) {
            float min_dist_sq = -1.0f;
            float threat_x = 0.0f, threat_y = 0.0f, threat_z = 0.0f;

            for (const auto& p : pursuer_coords) {
                float dx = p.x - position.x;
                float dy = p.y - position.y;
                float dz = p.z - position.z;
                float dist_sq = dx * dx + dy * dy + dz * dz;
                if (min_dist_sq < 0.0f || dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    threat_x = p.x;
                    threat_y = p.y;
                    threat_z = p.z;
                }
            }

            float dx = threat_x - position.x;
            float dy = threat_y - position.y;
            float dz = threat_z - position.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            theta = std::atan2(dy, dx) + static_cast<float>(M_PI);
            phi = -std::asin(dz / dist);
        } else {
            std::uniform_real_distribution<float> azimuth(0.0f, 2.0f * static_cast<float>(M_PI));
            std::uniform_real_distribution<float> elevation(-static_cast<float>(M_PI) / 2.0f, static_cast<float>(M_PI) / 2.0f);
            theta = azimuth(gen);
            phi = elevation(gen);
        }
    }
}

void escaper::setData(float x, float y, float z, float ve) {
    position.x = x;
    position.y = y;
    position.z = z;
    v_e = ve;
}