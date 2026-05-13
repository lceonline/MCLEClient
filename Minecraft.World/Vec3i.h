#pragma once
#include <cmath>
#include <string>

class Vec3i {
protected:
    int x, y, z;
public:
    Vec3i() : x(0), y(0), z(0) {}
    Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
    Vec3i(double x, double y, double z)
        : x((int)std::floor(x)), y((int)std::floor(y)), z((int)std::floor(z)) {}

    int getX() const { return x; }
    int getY() const { return y; }
    int getZ() const { return z; }

    void set(int x, int y, int z) { this->x = x; this->y = y; this->z = z; }

    static void cross(Vec3i& result, const Vec3i& a, const Vec3i& b);

    double dist(int x2, int y2, int z2) const;
    double distSqr(double x2, double y2, double z2) const;
    double distSqrToCenter(double x2, double y2, double z2) const;
    double distSqr(const Vec3i& other) const;

    bool equals(const Vec3i& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    Vec3i above() const { return Vec3i(x, y + 1, z); }
    Vec3i below() const { return Vec3i(x, y - 1, z); }
    Vec3i north() const { return Vec3i(x, y, z - 1); }
    Vec3i south() const { return Vec3i(x, y, z + 1); }
    Vec3i east()  const { return Vec3i(x + 1, y, z); }
    Vec3i west()  const { return Vec3i(x - 1, y, z); }
};