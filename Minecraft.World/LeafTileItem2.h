#pragma once

#include "LeafTileItem.h"

class LeafTileItem2 : public LeafTileItem
{
public:
	LeafTileItem2(int id);

	virtual int getLevelDataForAuxValue(int auxValue);
	virtual Icon *getIcon(int itemAuxValue);
	virtual int getColor(shared_ptr<ItemInstance> item, int spriteLayer) override;
	virtual unsigned int getDescriptionId(shared_ptr<ItemInstance> instance);
};