#pragma once

#include "Biome.h"

class PlainsBiome : public Biome
{
	friend class Biome;
protected:
	bool _plains;
protected:
	PlainsBiome(int id,bool plains);
	virtual Feature* getFlowerFeature(Random* random, int x, int y, int z) override;
	void decorate(Level* level, Random* rand, int xo, int zo)override;
	bool isPlains() { return _plains; };
	
};