#pragma once
#include "Tile.h"

class Random;

class ObsidianTile : public Tile
{
public:
	ObsidianTile(int id);
    virtual int getResourceCount(Random *random);
    virtual int getResource(int data, Random *random, int playerBonusLevel);
};