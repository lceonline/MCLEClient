#pragma once
#include "Layer.h"
#include <memory>

class RiverInitLayer : public Layer
{
public:
	RiverInitLayer(int64_t seed, std::shared_ptr<Layer> parent, int64_t seedMixup);

	intArray getArea(int xo, int yo, int w, int h);
};