#include "stdafx.h"
#include "BlockPos.h"
#include "Entity.h"
#include "Direction.h"
#include "BlockSource.h"
#include <sstream>
#include <cmath>

const BlockPos BlockPos::ZERO = BlockPos(0, 0, 0);

// Costruttori
BlockPos::BlockPos() : Vec3i(0, 0, 0) {}

BlockPos::BlockPos(int x, int y, int z) : Vec3i(x, y, z) {}

BlockPos::BlockPos(double x, double y, double z)
    : Vec3i((int)std::floor(x), (int)std::floor(y), (int)std::floor(z)) {}

BlockPos::BlockPos(const std::shared_ptr<Entity>& entity) : Vec3i(0, 0, 0) {
    if (entity) {
        x = (int)std::floor(entity->x);
        y = (int)std::floor(entity->y);
        z = (int)std::floor(entity->z);
    }
}

BlockPos::BlockPos(const double* pos)
    : Vec3i((int)std::floor(pos[0]), (int)std::floor(pos[1]), (int)std::floor(pos[2])) {}

BlockPos::BlockPos(const double* pos, bool fromEntity) : Vec3i(0, 0, 0) {
    if (fromEntity) {
        x = (int)std::floor(pos[10]);
        y = (int)std::floor(pos[11]);
        z = (int)std::floor(pos[12]);
    } else {
        x = (int)std::floor(pos[0]);
        y = (int)std::floor(pos[1]);
        z = (int)std::floor(pos[2]);
    }
}

BlockPos::BlockPos(const Vec3i& other)
    : Vec3i(other.getX(), other.getY(), other.getZ()) {}

BlockPos::BlockPos(int compressed) : Vec3i(0, 0, 0) {
    *this = decompress(compressed);
}

BlockPos::BlockPos(BlockSource& source)
    : Vec3i(source.getBlockX(), source.getBlockY(), source.getBlockZ()) {}

// Metodi di confronto
bool BlockPos::equals(const BlockPos& other) const {
    return x == other.x && y == other.y && z == other.z;
}

bool BlockPos::equals(int x, int y, int z) const {
    return this->x == x && this->y == y && this->z == z;
}

int BlockPos::hashCode() const {
    int result = 0x1F * (y + 0x1F * z);
    return result + x;
}

// Offset
BlockPos BlockPos::offset(int dx, int dy, int dz) const {
    return BlockPos(x + dx, y + dy, z + dz);
}

BlockPos BlockPos::offset(double dx, double dy, double dz) const {
    return BlockPos(x + dx, y + dy, z + dz);
}

BlockPos BlockPos::offset(const BlockPos& delta) const {
    return BlockPos(x + delta.x, y + delta.y, z + delta.z);
}

BlockPos BlockPos::relative(int direction, int distance) const {
    int dx = 0, dz = 0;
    switch (direction) {
        case Direction::NORTH: dz = -distance; break;
        case Direction::SOUTH: dz = distance; break;
        case Direction::WEST:  dx = -distance; break;
        case Direction::EAST:  dx = distance; break;
        default: break;
    }
    return BlockPos(x + dx, y, z + dz);
}

// Metodi direzionali
BlockPos BlockPos::above(int distance) const {
    return BlockPos(x, y + distance, z);
}

BlockPos BlockPos::below(int distance) const {
    return BlockPos(x, y - distance, z);
}

BlockPos BlockPos::north(int distance) const {
    return relative(Direction::NORTH, distance);
}

BlockPos BlockPos::south(int distance) const {
    return relative(Direction::SOUTH, distance);
}

BlockPos BlockPos::east(int distance) const {
    return relative(Direction::EAST, distance);
}

BlockPos BlockPos::west(int distance) const {
    return relative(Direction::WEST, distance);
}

BlockPos BlockPos::multiply(int factor) const {
    if (factor == 0) return ZERO;
    if (factor == 1) return *this;
    return BlockPos(x * factor, y * factor, z * factor);
}

// Compressione
int BlockPos::compress() const {
    static const int MASK_X = (1 << BITS_X) - 1;
    static const int MASK_Y = (1 << BITS_Y) - 1;
    static const int MASK_Z = (1 << BITS_Z) - 1;
    static const int SHIFT_Z = 0;
    static const int SHIFT_Y = BITS_Z;
    static const int SHIFT_X = BITS_Z + BITS_Y;
    
    int cx = x & MASK_X;
    int cy = y & MASK_Y;
    int cz = z & MASK_Z;
    
    return (cx << SHIFT_X) | (cy << SHIFT_Y) | (cz << SHIFT_Z);
}

BlockPos BlockPos::decompress(int compressed) {
    static const int MASK_X = (1 << BITS_X) - 1;
    static const int MASK_Y = (1 << BITS_Y) - 1;
    static const int MASK_Z = (1 << BITS_Z) - 1;
    static const int SHIFT_Z = 0;
    static const int SHIFT_Y = BITS_Z;
    static const int SHIFT_X = BITS_Z + BITS_Y;
    
    int x = (compressed >> SHIFT_X) & MASK_X;
    int y = (compressed >> SHIFT_Y) & MASK_Y;
    int z = (compressed >> SHIFT_Z) & MASK_Z;
    
    if (x & (1 << (BITS_X - 1))) x |= ~MASK_X;
    if (y & (1 << (BITS_Y - 1))) y |= ~MASK_Y;
    if (z & (1 << (BITS_Z - 1))) z |= ~MASK_Z;
    
    return BlockPos(x, y, z);
}


BlockPos BlockPos::operator+(const BlockPos& other) const {
    return BlockPos(x + other.x, y + other.y, z + other.z);
}

BlockPos BlockPos::operator-(const BlockPos& other) const {
    return BlockPos(x - other.x, y - other.y, z - other.z);
}

BlockPos BlockPos::operator*(int scalar) const {
    return multiply(scalar);
}

BlockPos& BlockPos::operator+=(const BlockPos& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

BlockPos& BlockPos::operator-=(const BlockPos& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

std::string BlockPos::toString() const {
    std::ostringstream oss;
    oss << "(" << x << ", " << y << ", " << z << ")";
    return oss.str();
}