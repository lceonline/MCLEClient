#pragma once
#include "Feature.h"

class Level;
class Random;

class DoublePlantFeature : public Feature
{
private:
    int m_plantType;

public:
    DoublePlantFeature(bool doUpdate = false);
    void setPlantType(int plantType);
    bool place(Level* level, Random* rand, int x, int y, int z);
};