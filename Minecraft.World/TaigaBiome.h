#pragma once
#include "Biome.h"

class TaigaBiome : public Biome
{
private:
	int type;
public:
	TaigaBiome(int id, int type = 0);

	virtual Feature *getTreeFeature(Random *random) override;
	virtual void decorate(Level *level, Random *random, int xo, int zo) override;
	virtual void buildSurfaceAtDefault(Level *level, Random *random, byte* chunkBlocks, int x, int z, double noiseVal) override;
};