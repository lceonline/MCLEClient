#pragma once

#include "Layer.h"
#include <vector>

class LevelType;
class Biome;
struct CustomizableSourceSettings;

class BiomeInitLayer : public Layer
{
private:
    BiomeArray  desertBiomes;
    BiomeArray warmBiomes;
    BiomeArray  coolBiomes;
    BiomeArray  icyBiomes;
    
    CustomizableSourceSettings* customSettings;

public:
    BiomeInitLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup, LevelType *levelType, void* superflatConfig);
    virtual ~BiomeInitLayer();
    intArray getArea(int xo, int yo, int w, int h) override;
};