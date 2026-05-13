#pragma once
#include "Feature.h"
class Level;
class FlowerFeature : public Feature
{
private:
    int tile;
    int variant;
public:
    FlowerFeature(int tile, int variant = 0);
    bool place(Level* level, Random* random, int x, int y, int z);
};