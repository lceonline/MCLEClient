#pragma once
#include "Vec3i.h"
#include <memory>
#include <string>

class Entity;
class BlockSource;

class BlockPos : public Vec3i {
public:
    BlockPos();
    BlockPos(int x, int y, int z);
    BlockPos(double x, double y, double z);
    explicit BlockPos(const std::shared_ptr<Entity>& entity);
    explicit BlockPos(const double* pos);
    BlockPos(const double* pos, bool fromEntity);
    explicit BlockPos(const Vec3i& other);
    explicit BlockPos(int compressed);
    explicit BlockPos(BlockSource& source);

    int getX() const { return x; }
    int getY() const { return y; }
    int getZ() const { return z; }

    void set(int x, int y, int z) { this->x = x; this->y = y; this->z = z; }
    void set(const BlockPos& other) { x = other.x; y = other.y; z = other.z; }

    bool equals(const BlockPos& other) const;
    bool equals(int x, int y, int z) const;
    bool operator==(const BlockPos& other) const { return equals(other); }
    bool operator!=(const BlockPos& other) const { return !equals(other); }

    int hashCode() const;

    BlockPos offset(int dx, int dy, int dz) const;
    BlockPos offset(double dx, double dy, double dz) const;
    BlockPos offset(const BlockPos& delta) const;
    BlockPos relative(int direction, int distance = 1) const;

    BlockPos above(int distance = 1) const;
    BlockPos below(int distance = 1) const;
    BlockPos north(int distance = 1) const;
    BlockPos south(int distance = 1) const;
    BlockPos east(int distance = 1) const;
    BlockPos west(int distance = 1) const;

    BlockPos multiply(int factor) const;

    int compress() const;
    static BlockPos decompress(int compressed);

    BlockPos operator+(const BlockPos& other) const;
    BlockPos operator-(const BlockPos& other) const;
    BlockPos operator*(int scalar) const;
    BlockPos& operator+=(const BlockPos& other);
    BlockPos& operator-=(const BlockPos& other);

    bool isZero() const { return x == 0 && y == 0 && z == 0; }
    std::string toString() const;

    static const BlockPos ZERO;
    static const int BITS_X = 26;
    static const int BITS_Y = 12;
    static const int BITS_Z = 26;
};