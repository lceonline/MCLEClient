#include "stdafx.h"
#include "RareBiomeSpotLayer.h"
#include "IntCache.h"
#include "net.minecraft.world.level.biome.h"

RareBiomeSpotLayer::RareBiomeSpotLayer(int64_t seed, std::shared_ptr<Layer> parent, int64_t seedMixup) : Layer(seedMixup)
{
	this->parent = parent;
}

intArray RareBiomeSpotLayer::getArea(int xo, int yo, int w, int h)
{
	intArray b = parent->getArea(xo, yo, w, h);
	intArray result = IntCache::allocate(w * h);

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			initRandom(x + xo, y + yo);
			int biomeId = b[x + y * w];

			if (nextRandom(57) == 0 && biomeId == Biome::plains->id)
				result[x + y * w] = biomeId + 128;
			else
				result[x + y * w] = biomeId;
		}
	}

	return result;
}
