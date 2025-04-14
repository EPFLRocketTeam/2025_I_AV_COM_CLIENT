//
// Created by alberts on 4/28/24.
//

#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <stdexcept>
#include "../Payload.h"

struct Vec3 {
    double x, y, z;

    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    static Vec3 Constant(double val){
        return {val, val, val};
    }

    Vec3 operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vec3 operator*(const Vec3& other) const {
        return {x * other.x, y * other.y, z * other.z};
    }

    Vec3 operator/(const Vec3& other) const {
        return {x / other.x, y / other.y, z / other.z};
    }


    Vec3 operator*(double scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    friend Vec3 operator*(double s, const Vec3& v){
        return v * s;
    }

    Vec3 operator/(double scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3 operator-() const{
        return {-x, -y, -z};
    }

    Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] double norm() const{
        return std::sqrt(x*x + y*y + z*z);
    }

    [[nodiscard]] Vec3 cwiseMax(const Vec3& other) const{
        return {std::max(x, other.x), std::max(y, other.y), std::max(z, other.z)};
    }

    [[nodiscard]] Vec3 cwiseMin(const Vec3& other) const{
        return {std::min(x, other.x), std::min(y, other.y), std::min(z, other.z)};
    }

    [[nodiscard]] Vec3 toRad() const{
        return {x * M_PI / 180, y * M_PI / 180, z * M_PI / 180};
    }

    [[nodiscard]] Vec3 toDegree() const{
        return {x * 180 / M_PI, y * 180 / M_PI, z * 180 / M_PI};
    }

    static Vec3 Zero(){
        return {0, 0, 0};
    }

    static Vec3 One(){
        return {1, 1, 1};
    }

    double& operator[](int i){
        switch (i)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;        
        }
    }

    void serialize(Payload& payload) const {
        payload.write(x);
        payload.write(y);
        payload.write(z);
    }

    void deserialize(Payload& payload) {
        payload.read(x);
        payload.read(y);
        payload.read(z);
    }
};

#endif //VEC3_H
