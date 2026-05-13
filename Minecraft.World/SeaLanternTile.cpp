#include "stdafx.h"
#include "SeaLanternTile.h"
#include "net.minecraft.world.item.h"
#include "IconRegister.h"

SeaLanternTile::SeaLanternTile(int id, Material* material) : Tile(id, material)
{
}
int SeaLanternTile::getResourceCountForLootBonus(int bonusLevel, Random* random)
{
	return Mth::clamp(getResourceCount(random) + random->nextInt(bonusLevel + 1), 1, 4);
}

int SeaLanternTile::getResourceCount(Random* random)
{
	return 2 + random->nextInt(3);
}

int SeaLanternTile::getResource(int data, Random* random, int playerBonusLevel)
{
	return Item::prismarine_crystal->id;
}

void SeaLanternTile::registerIcons(IconRegister* iconRegister)
{
    icon = iconRegister->registerIcon(L"sea_lantern");
}

Icon* SeaLanternTile::getTexture(int face, int data)
{
    return icon;
}