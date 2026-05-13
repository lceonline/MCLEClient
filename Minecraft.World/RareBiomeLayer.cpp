#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "IntCache.h"
#include "RareBiomeLayer.h"

RareBiomeLayer::RareBiomeLayer(int64_t seed, shared_ptr<Layer> parent) : Layer(seed)
{
	this->parent = parent;
}

intArray RareBiomeLayer::getArea(int xo, int yo, int w, int h)
{
	intArray aint = this->parent->getArea(xo - 1, yo - 1, w + 2, h + 2);
	intArray aint1 = IntCache::allocate(w * h);

	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
		{
			this->initRandom((int64_t)(j + xo), (int64_t)(i + yo));
			int k = aint[j + 1 + (i + 1) * (w + 2)];

			if (this->nextRandom(57) == 0)
			{
				int mutated = k;
				if      (k == Biome::plains->id)           mutated = Biome::sunflowersPlains->id;    // 1  -> 129
				else if (k == Biome::desert->id)           mutated = Biome::desertM->id;             // 2  -> 130
				else if (k == Biome::extremeHills->id)     mutated = Biome::extremeHillsM->id;       // 3  -> 131
				else if (k == Biome::forest->id)           mutated = Biome::flowerForest->id;        // 4  -> 132
				else if (k == Biome::taiga->id)            mutated = Biome::taigaM->id;              // 5  -> 133
				else if (k == Biome::swampland->id)        mutated = Biome::swamplandM->id;          // 6  -> 134
				else if (k == Biome::iceFlats->id)         mutated = Biome::iceSpikes->id;           // 12 -> 140
				else if (k == Biome::jungle->id)           mutated = Biome::jungleM->id;             // 21 -> 149
				else if (k == Biome::jungleEdge->id)       mutated = Biome::jungleEdgeM->id;         // 23 -> 151
				else if (k == Biome::birchForest->id)      mutated = Biome::birchForestM->id;        // 27 -> 155
				else if (k == Biome::birchForestHills->id) mutated = Biome::birchForestHillsM->id;   // 28 -> 156
				else if (k == Biome::roofedForest->id)     mutated = Biome::roofedForestM->id;       // 29 -> 157
				else if (k == Biome::coldTaiga->id)        mutated = Biome::coldTaigaM->id;          // 30 -> 158
				else if (k == Biome::megaTaiga->id)        mutated = Biome::redwoodTaiga->id;        // 32 -> 160
				else if (k == Biome::megaTaigaHills->id)   mutated = Biome::redwoodTaigaHills->id;   // 33 -> 161
				else if (k == Biome::extremeHills_plus->id) mutated = Biome::extremeHills_plusM->id; // 34 -> 162
				else if (k == Biome::savanna->id)          mutated = Biome::savannaM->id;            // 35 -> 163
				else if (k == Biome::savannaPlateau->id)   mutated = Biome::savannaPlateauM->id;     // 36 -> 164
				else if (k == Biome::mesa->id)             mutated = Biome::mesaBryce->id;           // 37 -> 165
				else if (k == Biome::mesaPlateauF->id)     mutated = Biome::mesaPlateauFM->id;       // 38 -> 166
				else if (k == Biome::mesaPlateau->id)      mutated = Biome::mesaPlateauM->id;        // 39 -> 167
				aint1[j + i * w] = mutated;
			}
			else
			{
				aint1[j + i * w] = k;
			}
		}
	}

	return aint1;
}