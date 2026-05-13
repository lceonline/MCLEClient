#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.entity.item.h"
#include "SandTile.h"
#include "FireTile.h"
#include "net.minecraft.world.h"

const unsigned int SandTile::SAND_NAMES[SAND_NAMES_LENGTH] = { IDS_TILE_SAND, IDS_TILE_RED_SAND };
const wstring SandTile::TEXTURE_NAMES[] = { L"sand", L"red_sand" };

SandTile::SandTile(int type, bool isSolidRender) : HeavyTile(type, Material::sand, isSolidRender)
{
    icons = nullptr;
}

int SandTile::getSpawnResourcesAuxValue(int data)
{
    if (data < 0 || data >= SAND_NAMES_LENGTH) data = 0;
    return data;
}

Icon* SandTile::getTexture(int face, int data)
{
    if (data < 0 || data >= SAND_NAMES_LENGTH) data = 0;
    return icons[data];
}

void SandTile::registerIcons(IconRegister* iconRegister)
{
    icons = new Icon * [SAND_NAMES_LENGTH];
    for (int i = 0; i < SAND_NAMES_LENGTH; i++)
    {
        icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
    }
}