#pragma once

#include <cmath>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

struct Vector {
    float x;
    float y;
    float z;

    Vector() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector normalize() const {
        float len = length();
        if (len == 0.0f) {
            return Vector(0.0f, 0.0f, 0.0f);
        }
        return Vector(x / len, y / len, z / len);
    }

    float dot(const Vector& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vector operator+(const Vector& v) const {
        return Vector(x + v.x, y + v.y, z + v.z);
    }

    Vector operator-(const Vector& v) const {
        return Vector(x - v.x, y - v.y, z - v.z);
    }

    Vector operator*(float k) const {
        return Vector(x * k, y * k, z * k);
    }
};

struct Coordinates {
    float x;
    float y;
    float z;

    Coordinates() : x(0.0f), y(0.0f), z(0.0f) {}
    Coordinates(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector vectorTo(const Coordinates& other) const {
        return Vector(other.x - x, other.y - y, other.z - z);
    }

    Coordinates operator+(const Vector& v) const {
        return Coordinates(x + v.x, y + v.y, z + v.z);
    }

    Coordinates operator-(const Vector& v) const {
        return Coordinates(x - v.x, y - v.y, z - v.z);
    }
};
