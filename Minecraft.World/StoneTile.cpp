#include "stdafx.h"
#include "StoneTile.h"
#include "net.minecraft.world.h"
#include "net.minecraft.h"

const unsigned int StoneTile::STONE_NAMES[STONE_NAMES_LENGTH] = { IDS_TILE_STONE,
	IDS_TILE_GRANITE,  IDS_TILE_POLISHED_GRANITE,
	IDS_TILE_DIORITE,  IDS_TILE_POLISHED_DIORITE,
	IDS_TILE_ANDESITE, IDS_TILE_POLISHED_ANDESITE 
};

const unsigned int StoneTile::STONE_DESCRIPTIONS[STONE_NAMES_LENGTH] = { IDS_DESC_STONE,
	IDS_DESC_GRANITE,  IDS_DESC_POLISHED_GRANITE,
	IDS_DESC_DIORITE,  IDS_DESC_POLISHED_DIORITE,
	IDS_DESC_ANDESITE, IDS_DESC_POLISHED_ANDESITE
};

const wstring StoneTile::TEXTURE_NAMES[] = { L"stone", 
		L"stone_granite", L"stone_granite_smooth",
		L"stone_diorite", L"stone_diorite_smooth",
		L"stone_andesite", L"stone_andesite_smooth"};

StoneTile::StoneTile(int id) : Tile(id, Material::stone)
{
	icons = nullptr;
}

unsigned int StoneTile::getDescriptionId(int iData)
{
	if (iData < 0 || iData >= STONE_NAMES_LENGTH) iData = 0;

	return STONE_DESCRIPTIONS[iData];
}

int StoneTile::getResource(int data, Random* random, int playerBonusLevel)
{

	if (data < 0 || data >= STONE_NAMES_LENGTH) data = 0;

	if (data == 0) {
		return Tile::cobblestone_Id;
	}

	return Tile::stone_Id;
}

int StoneTile::getSpawnResourcesAuxValue(int data)
{
	if (data < 0 || data >= STONE_NAMES_LENGTH) data = 0;

	return data;
}

Icon* StoneTile::getTexture(int face, int data)
{
	if (data < 0 || data >= STONE_NAMES_LENGTH)
	{
		data = 0;
	}
	return icons[data];
}

void StoneTile::registerIcons(IconRegister* iconRegister)
{
	icons = new Icon * [STONE_NAMES_LENGTH];

	for (int i = 0; i < STONE_NAMES_LENGTH; i++)
	{
		icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
	}
}