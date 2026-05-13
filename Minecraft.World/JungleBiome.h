#pragma once

#include "Biome.h"

class JungleBiome : public Biome
{
private:
	bool isEdge;

public:
	JungleBiome(int id, bool isEdge);

	virtual Feature *getTreeFeature(Random *random) override;
	virtual Feature *getGrassFeature(Random *random) override;
	virtual void decorate(Level *level, Random *random, int xo, int zo) override;
};