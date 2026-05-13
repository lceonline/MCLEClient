#pragma once
#include <memory>
using namespace std;


template<class T> class ListTag;
class FloatTag;

class Rotations
{
public:
    float x, y, z;

public:
    
    Rotations() : x(0.0f), y(0.0f), z(0.0f) {}

   
    Rotations(float x, float y, float z) : x(x), y(y), z(z) {}

    
    Rotations(const Rotations& other) : x(other.x), y(other.y), z(other.z) {}

    
    Rotations(ListTag<FloatTag>* list);

   
    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }

    
    bool operator==(const Rotations& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const Rotations& other) const
    {
        return !(*this == other);
    }

    
    ListTag<FloatTag>* save() const;
};