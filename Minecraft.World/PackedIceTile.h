#pragma once
#include "Tile.h"

class Random;

class PackedIceTile : public Tile
{
public:
	PackedIceTile(int id);
	virtual int getResource(int data, Random* random, int playerBonusLevel) { return 0; };
	virtual int getPistonPushReaction() { return Material::PUSH_NORMAL; };
};
