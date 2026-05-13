#pragma once

#include "Layer.h"

class SmoothLayer : public Layer
{
public:
	SmoothLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup);

	virtual intArray getArea(int xo, int yo, int w, int h);
};