#pragma once
#pragma once
#include "Feature.h"

class RoofTreeFeature : public Feature
{
public:
    RoofTreeFeature(bool doUpdate);
    virtual bool place(Level *level, Random *random, int x, int y, int z);

private:
    int baseHeight;
    
    void placeTrunk2x2(Level *level, int x, int y, int z);
    
    void placeLeaf(Level *level, int x, int y, int z);
    private:
    bool checkSpace(Level *worldIn, int x, int y, int z, int height);
    void placeLog(Level *worldIn, int x, int y, int z);
    
};