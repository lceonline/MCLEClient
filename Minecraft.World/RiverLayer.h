#pragma once

#include "Layer.h"

class RiverLayer : public Layer
{
public:
	RiverLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup);
	intArray getArea(int xo, int yo, int w, int h);
};