#pragma once
#include "Layer.h"
#include <memory>

class RareBiomeSpotLayer : public Layer
{
public:
	RareBiomeSpotLayer(int64_t seed, std::shared_ptr<Layer> parent, int64_t seedMixup);
	virtual intArray getArea(int xo, int yo, int w, int h) override;
};
