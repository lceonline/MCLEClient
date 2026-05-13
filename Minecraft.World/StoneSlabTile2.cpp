#include "stdafx.h"
#include "net.minecraft.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.h"
#include "StoneSlabTile2.h"
#include "RedSandStoneTile.h"

const unsigned int StoneSlabTile2::SLAB_NAMES[SLAB_NAMES_LENGTH] = {
    IDS_TILE_STONESLAB_REDSAND,
};

StoneSlabTile2::StoneSlabTile2(int id)
    : HalfSlabTile(id, Material::stone)
{
}

Icon *StoneSlabTile2::getTexture(int face, int data)
{
    int type = data & TYPE_MASK;

    if (isFullSize() && (data & TOP_SLOT_BIT) != 0)
        face = Facing::UP;

    switch (type)
    {
    case RED_SANDSTONE_SLAB:
        return Tile::red_sandstone->getTexture(face);
    }

    return icon;
}

void StoneSlabTile2::registerIcons(IconRegister *iconRegister)
{
    icon     = iconRegister->registerIcon(L"red_sandstone_top");
    iconSide = iconRegister->registerIcon(L"red_sandstone_normal");
}

int StoneSlabTile2::getResource(int data, Random *random, int playerBonusLevel)
{
    return Tile::stone_slab2_Id;
}

unsigned int StoneSlabTile2::getDescriptionId(int iData)
{
    if (iData < 0) iData = 0;
    return StoneSlabTile2::SLAB_NAMES[iData];
}

int StoneSlabTile2::getAuxName(int auxValue)
{
    if (auxValue < 0 || auxValue >= SLAB_NAMES_LENGTH)
        auxValue = 0;
    return SLAB_NAMES[auxValue];//super.getDescriptionId() + "." + SLAB_NAMES[auxValue];
}

shared_ptr<ItemInstance> StoneSlabTile2::getSilkTouchItemInstance(int data)
{
    return make_shared<ItemInstance>(Tile::stone_slab2_Id, 2, data & TYPE_MASK);
}