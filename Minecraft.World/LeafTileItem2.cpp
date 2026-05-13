#include "stdafx.h"
#include "LeafTileItem2.h"
#include "LeafTile2.h"
#include "net.minecraft.world.level.tile.h"
#include "FoliageColor.h"

LeafTileItem2::LeafTileItem2(int id) : LeafTileItem(id)
{
	setMaxDamage(0);
	setStackedByData(true);
}

int LeafTileItem2::getLevelDataForAuxValue(int auxValue) 
{
	return auxValue;
}

Icon *LeafTileItem2::getIcon(int itemAuxValue) 
{
	
	return Tile::leaves2->getTexture(0, itemAuxValue);
}

unsigned int LeafTileItem2::getDescriptionId(shared_ptr<ItemInstance> instance)
{
	int auxValue = instance->getAuxValue();
	if (auxValue < 0 || auxValue >= LeafTile2::LEAF2_NAMES_SIZE)
	{
		auxValue = 0;
	}
	return LeafTile2::LEAF2_NAMES[auxValue];
}

int LeafTileItem2::getColor(shared_ptr<ItemInstance> item, int spriteLayer)
{
	
	return FoliageColor::getDefaultColor();
}