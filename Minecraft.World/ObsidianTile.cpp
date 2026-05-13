#include "stdafx.h"
#include "ObsidianTile.h"

ObsidianTile::ObsidianTile(int id) : Tile(id, Material::stone)
{
}

int ObsidianTile::getResourceCount(Random *random)
{
	return 1;
}

int ObsidianTile::getResource(int data, Random *random, int playerBonusLevel)
{
	return Tile::obsidian_Id;
}