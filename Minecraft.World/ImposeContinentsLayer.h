#pragma once

#include "Layer.h"

class ImposeContinentsLayer : public Layer
{
public:
    ImposeContinentsLayer(int64_t seed, shared_ptr<Layer> parent);
    virtual ~ImposeContinentsLayer();

    virtual intArray getArea(int xo, int yo, int w, int h) override;
};