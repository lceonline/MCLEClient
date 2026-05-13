#pragma once

#include "ArrayWithLength.h"

class LevelType;

#ifndef _CONTENT_PACAKGE
#define _BIOME_OVERRIDE
#endif

class Layer
{
private:
	int64_t seed;

protected:
	shared_ptr<Layer>parent;

protected:
	int64_t rval;
	int64_t seedMixup;

public:
	static LayerArray getDefaultLayers(int64_t seed, LevelType *levelType, void* superflatConfig = nullptr);

	Layer(int64_t seedMixup);

	virtual void init(int64_t seed);
	bool isOcean(int biomeId);
	bool isSame(int biomeIdA, int biomeIdB);
	virtual void initRandom(int64_t x, int64_t y);

protected:
	int nextRandom(int max);
	int random(int i, int j, int k, int l);
	int random(int i, int j);
	int modeOrRandom(int i, int j, int k, int l);
public:
	virtual intArray getArea(int xo, int yo, int w, int h) = 0;
};