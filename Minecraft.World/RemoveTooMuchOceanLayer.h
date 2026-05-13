#pragma once

#include "Layer.h"

class RemoveTooMuchOceanLayer : public Layer
{
public:
	RemoveTooMuchOceanLayer(int64_t seed, shared_ptr<Layer> parent, int64_t seedMixup);
	virtual ~RemoveTooMuchOceanLayer() {}
	virtual intArray getArea(int xo, int yo, int w, int h) override;
};
