#include "stdafx.h"
#include "Vec3i.h"
#include <cmath>

void Vec3i::cross(Vec3i& result, const Vec3i& a, const Vec3i& b)
{
    result.set(
        a.getY() * b.getZ() - a.getZ() * b.getY(),
        a.getZ() * b.getX() - a.getX() * b.getZ(),
        a.getX() * b.getY() - a.getY() * b.getX()
    );
}

double Vec3i::dist(int x2, int y2, int z2) const
{
    double dx = x - x2;
    double dy = y - y2;
    double dz = z - z2;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

double Vec3i::distSqr(double x2, double y2, double z2) const
{
    double dx = x - x2;
    double dy = y - y2;
    double dz = z - z2;
    return dx*dx + dy*dy + dz*dz;
}

double Vec3i::distSqrToCenter(double x2, double y2, double z2) const
{
    double dx = (x + 0.5) - x2;
    double dy = (y + 0.5) - y2;
    double dz = (z + 0.5) - z2;
    return dx*dx + dy*dy + dz*dz;
}

double Vec3i::distSqr(const Vec3i& other) const
{
    return distSqr(other.getX(), other.getY(), other.getZ());
}