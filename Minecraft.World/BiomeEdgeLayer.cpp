#include "stdafx.h"
#include "BiomeEdgeLayer.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"

BiomeEdgeLayer::BiomeEdgeLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup) : Layer(seedMixup)
{
    this->parent = parent;
}

intArray BiomeEdgeLayer::getArea(int xo, int yo, int w, int h)
{
    intArray b = parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
    intArray result = IntCache::allocate(w * h);
    int stride = w + 2;

    for (int iy = 0; iy < h; ++iy)
    {
        for (int ix = 0; ix < w; ++ix)
        {
            initRandom(ix + xo, iy + yo);
            int center = b[(ix + 1) + (iy + 1) * stride];

            if (!checkEdge(b, result, ix, iy, stride, Biome::extremeHills->id, Biome::smallerExtremeHills->id, center) &&
                !checkEdge(b, result, ix, iy, stride, Biome::mesaPlateauF->id, Biome::mesaPlateau->id, center) &&
                !checkEdge(b, result, ix, iy, stride, Biome::mesaPlateau->id, Biome::mesaPlateau->id, center) &&
                !checkEdge(b, result, ix, iy, stride, Biome::megaTaiga->id, Biome::megaTaiga->id, center))
            {
               if (center == Biome::desert->id) {
                    int n = b[(ix + 1) + (iy + 0) * stride];
                    int e = b[(ix + 2) + (iy + 1) * stride];
                    int w1 = b[(ix + 0) + (iy + 1) * stride];
                    int s = b[(ix + 1) + (iy + 2) * stride];
                    if (n == Biome::iceFlats->id || e == Biome::iceFlats->id || w1 == Biome::iceFlats->id || s == Biome::iceFlats->id) {
                        center = Biome::extremeHills->id;
                    }
               }
               result[ix + iy * w] = center;
            }
        }
    }
    return result;
}

bool BiomeEdgeLayer::checkEdge(intArray& b, intArray& result, int x, int y, int w, int biome, int target, int center)
{
    if (center != biome) return false;

    int n = b[(x + 1) + (y + 0) * w];
    int e = b[(x + 2) + (y + 1) * w];
    int w1 = b[(x + 0) + (y + 1) * w];
    int s = b[(x + 1) + (y + 2) * w];

    if (n != biome || e != biome || w1 != biome || s != biome)
    {
        result[x + y * (w-2)] = target;
        return true;
    }
    return false;
}

bool BiomeEdgeLayer::isValidTemperatureEdge(int a1biome, int a2biome)
{
    if (a1biome == a2biome) return true;
    
    return true; 
}
