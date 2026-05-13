#pragma once
#include "Biome.h"

class IceBiome : public Biome
{
private:
    bool isSpikes;
public:
    IceBiome(int id, bool isSpikes = false);
    virtual void decorate(Level *level, Random *random, int xo, int zo) override;
    virtual Feature* getTreeFeature(Random* random) override;
};