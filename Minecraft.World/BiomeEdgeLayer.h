#pragma once
#include "Layer.h"

class BiomeEdgeLayer : public Layer
{
public:
    BiomeEdgeLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup);
    virtual ~BiomeEdgeLayer() {}
    virtual intArray getArea(int xo, int yo, int w, int h) override;

private:
    static bool isValidTemperatureEdge(int a1biome, int a2biome);
    bool checkEdge(intArray& b, intArray& result, int x, int y, int w, int biome, int target, int replacement);
    bool checkEdgeStrict(intArray& b, intArray& result, int x, int y, int w, int biome, int target, int replacement);
};