#include "escaper.hpp"
#include <cmath>
#include <random>
#include <vector>

escaper::escaper(float x, float y, float z, float ve, int prob)
    : position(x, y, z), v_e(ve), turn_prob(prob), theta(0.0f), phi(0.0f),
      last_theta(0.0f), smoothed_desired(0.0f) {}

escaper::~escaper() {}

void escaper::setData(float x, float y, float z, float ve) {
    position = Coordinates(x, y, z);
    v_e = ve;
    last_theta = 0.0f;
    smoothed_desired = 0.0f;
}

void escaper::calculate_trajectory(const std::deque<Coordinates>& pursuer_coords) {
    std::vector<Coordinates> nearby;
    for (const auto& p : pursuer_coords) {
        Vector d = position.vectorTo(p);
        if (d.length() < 5000.0f && d.length() > 0.1f) nearby.push_back(p);
    }

    Vector new_dir(0, 0, 0);

    if (nearby.empty()) {
        Coordinates goal(2500.0f, 0.0f, 1000.0f);
        new_dir = position.vectorTo(goal);
    } else if (nearby.size() == 1) {
        Vector p_to_e = nearby[0].vectorTo(position);
        new_dir = p_to_e;
    } else if (nearby.size() == 2) {
        Vector line = nearby[0].vectorTo(nearby[1]);
        Vector perp(line.y, -line.x, 0.0f);
        if (perp.length() < 0.1f) perp = Vector(1.0f, 0.0f, 0.0f);
        new_dir = perp;
    } else return;

    Vector unit = new_dir.normalize();
    float instant = std::atan2(unit.y, unit.x);

    float delta = instant - smoothed_desired;
    delta = std::fmod(delta + 3.1415926535f, 6.283185307f) - 3.1415926535f;
    if (delta > 0.0052f) delta = 0.0052f;
    if (delta < -0.0052f) delta = -0.0052f;

    smoothed_desired += delta;
}

void escaper::turn(const std::deque<Coordinates>& pursuer_coords) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> prob_dist(0.0f, 100.0f);

    float delta = smoothed_desired - last_theta;
    delta = std::fmod(delta + 3.1415926535f, 6.283185307f) - 3.1415926535f;
    if (delta > 0.087f) delta = 0.087f;
    if (delta < -0.087f) delta = -0.087f;

    float new_theta = last_theta + delta;

    if (prob_dist(gen) < turn_prob) {
        std::uniform_real_distribution<float> noise(-0.035f, 0.035f);
        new_theta += noise(gen);
    }

    theta = new_theta;
    last_theta = theta;

    std::uniform_real_distribution<float> pitch_noise(-0.017f, 0.017f);
    phi += pitch_noise(gen) * 0.1f;
    if (phi > 0.087f) phi = 0.087f;
    if (phi < -0.087f) phi = -0.087f;

    const float dt = 0.5f;
    float cx = std::cos(theta) * std::cos(phi);
    float cy = std::sin(theta) * std::cos(phi);
    float cz = std::sin(phi);

    Vector dir(cx, cy, cz);
    float len = dir.length();
    if (len > 0.0f) dir = dir * (1.0f / len);

    position = position + dir * v_e * dt;

    if (position.z > 1050.0f) position.z = 1050.0f;
    if (position.z < 950.0f)  position.z = 950.0f;
}
