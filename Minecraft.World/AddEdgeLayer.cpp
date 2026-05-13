#include "stdafx.h"
#include "AddEdgeLayer.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"

AddEdgeLayer::AddEdgeLayer(int64_t seed, std::shared_ptr<Layer> parent, int64_t seedMixup, int mode)
    : Layer(seedMixup)
{
    this->parent = parent;
    this->mode = mode;
}

intArray AddEdgeLayer::getArea(int xo, int yo, int w, int h)
{
    if (mode == 1) return heatIce(xo, yo, w, h);
    if (mode == 2) return introduceSpecial(xo, yo, w, h);
    return coolWarm(xo, yo, w, h);
}


intArray AddEdgeLayer::coolWarm(int xo, int yo, int w, int h)
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
            if (center == 1)
            {
                int north = b[(ix + 1) + (iy + 0) * stride];
                int east  = b[(ix + 2) + (iy + 1) * stride];
                int west  = b[(ix + 0) + (iy + 1) * stride];
                int south = b[(ix + 1) + (iy + 2) * stride];

                bool hasCold = (north == 3 || east == 3 || west == 3 || south == 3);
                bool hasIcy  = (north == 4 || east == 4 || west == 4 || south == 4);

                if (hasCold || hasIcy)
                    center = 2;
            }
            result[ix + iy * w] = center;
        }
    }
    return result;
}


intArray AddEdgeLayer::heatIce(int xo, int yo, int w, int h)
{
    intArray b = parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
    intArray result = IntCache::allocate(w * h);
    int stride = w + 2;

    for (int iy = 0; iy < h; ++iy)
    {
        for (int ix = 0; ix < w; ++ix)
        {
            int center = b[(ix + 1) + (iy + 1) * stride];
            if (center == 4)
            {
                int north = b[(ix + 1) + (iy + 0) * stride];
                int east  = b[(ix + 2) + (iy + 1) * stride];
                int west  = b[(ix + 0) + (iy + 1) * stride];
                int south = b[(ix + 1) + (iy + 2) * stride];

                bool nearWarm = (north == 2 || east == 2 || west == 2 || south == 2);
                bool nearHot  = (north == 1 || east == 1 || west == 1 || south == 1);

                if (nearHot || nearWarm || nearWarm)
                    center = 3;
            }
            result[ix + iy * w] = center;
        }
    }
    return result;
}


intArray AddEdgeLayer::introduceSpecial(int xo, int yo, int w, int h)
{
    intArray b = parent->getArea(xo, yo, w, h);
    intArray result = IntCache::allocate(w * h);

    for (int iy = 0; iy < h; ++iy)
    {
        for (int ix = 0; ix < w; ++ix)
        {
            initRandom(ix + xo, iy + yo);

            int val = b[ix + iy * w];
            if (val != 0)
            {
                if (nextRandom(13) == 0)
                    val |= ((nextRandom(15) + 1) << 8) & 0xF00;
            }
            result[ix + iy * w] = val;
        }
    }
    return result;
}
