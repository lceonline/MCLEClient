#include "stdafx.h"
#include "ImposeContinentsLayer.h"
#include "IntCache.h"

bool dword_10073FD8[100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

ImposeContinentsLayer::ImposeContinentsLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
    this->parent = parent;
}

ImposeContinentsLayer::~ImposeContinentsLayer()
{
}

intArray ImposeContinentsLayer::getArea(int xo, int yo, int w, int h)
{
    intArray parentArea = parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
    intArray resultArea = IntCache::allocate(w * h);

    int seedOffset = static_cast<int>(this->seedMixup);

    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            int absX = xo + j;
            int absY = yo + i;
            
            int val = parentArea[(j + 1) + (i + 1) * (w + 2)];

            if (absX != 0 || absY != 0)
            {
                int modX = (absX + seedOffset) % 10;
                int modY = (absY + seedOffset) % 10;
                
                if (modX < 0)
                {
                    modX += 10;
                }
                if (modY < 0)
                {
                    modY += 10;
                }

                if (!dword_10073FD8[modY * 10 + modX])
                {
                    val = 0;
                }
            }

            resultArea[j + i * w] = val;
        }
    }

    return resultArea;
}