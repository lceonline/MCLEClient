#pragma once
#include "Feature.h"

class MegaPineTreeFeature : public Feature
{
private:
	bool useBaseHeight;

public:
	MegaPineTreeFeature(bool doUpdate, bool useBaseHeight);
	virtual bool place(Level *level, Random *random, int x, int y, int z) override;

private:
	void placeLeavesLayer(Level *level, int x, int z, int y, int radius, Random *random);
	void placeCrown(Level *level, int x, int z, int treeTop, Random *random);
	void placeBase(Level *level, Random *random, int x, int z, int y);
	void func_175933_b(Level *level, int x, int z, int y);
	void func_175934_c(Level *level, int x, int z, int y);
	bool canPlace(Level *level, Random *random, int x, int y, int z, int height);
	bool isAirLeaves(Level *level, int x, int y, int z);
	bool isReplaceable(int tileId);
};
