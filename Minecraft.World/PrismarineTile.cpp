#include "stdafx.h"
#include "PrismarineTile.h"
#include "net.minecraft.world.h"
#include "Item.h"
#include "IconRegister.h"

const wstring PrismarineTile::TEXTURE_NAMES[] = {L"", L"bricks", L"dark"};

const unsigned int PrismarineTile::PRISMARINE_NAMES[PRISMARINE_NAMES_LENGTH] = {    
    IDS_TILE_PRISMARINE,
    IDS_TILE_PRISMARINE_BRICKS,
    IDS_TILE_PRISMARINE_DARK
};

const unsigned int PrismarineTile::PRISMARINE_USE_DESCS[PRISMARINE_NAMES_LENGTH] = {    
    IDS_ITEM_PRISMARINE_DESC,
    IDS_ITEM_PRISMARINE_BRICK_DESC,
    IDS_ITEM_PRISMARINE_DARK_DESC
};

PrismarineTile::PrismarineTile(int id) : Tile(id, Material::stone)
{
    setBaseItemTypeAndMaterial(Item::eBaseItemType_structblock, Item::eMaterial_stone);
    icons = nullptr;
}

Icon *PrismarineTile::getTexture(int face, int data)
{
    if (data < 0 || data >= PRISMARINE_NAMES_LENGTH) data = 0;
    return icons[data];
}

int PrismarineTile::getSpawnResourcesAuxValue(int data)
{
    return data;
}

unsigned int PrismarineTile::getDescriptionId(int iData )
{
    if(iData < 0 ) iData = 0;
    return PrismarineTile::PRISMARINE_NAMES[iData];
}

unsigned int PrismarineTile::getUseDescriptionId(int iData )
{
    if(iData < 0 ) iData = 0;
    return PrismarineTile::PRISMARINE_USE_DESCS[iData];
}

void PrismarineTile::registerIcons(IconRegister *iconRegister)
{
    icons = new Icon*[PRISMARINE_NAMES_LENGTH];

    for (int i = 0; i < PRISMARINE_NAMES_LENGTH; i++)
    {
        wstring name = getIconName();
        if (!TEXTURE_NAMES[i].empty() ) name += L"_" + TEXTURE_NAMES[i];
        icons[i] = iconRegister->registerIcon(name);
    }
}