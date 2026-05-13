#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.h"
#include "GrassTile.h"
#include "Rose.h"

const unsigned int Rose::FLOWER_NAMES[FLOWER_NAMES_LENGTH] = { 
	IDS_TILE_ROSE, IDS_TILE_BLUE_ORCHID, IDS_TILE_ALLIUM, IDS_TILE_HOUSTONIA, IDS_TILE_TULIP_RED,
	IDS_TILE_TULIP_ORANGE, IDS_TILE_TULIP_WHITE, IDS_TILE_TULIP_PINK, IDS_TILE_OXEYE_DAISY 
};

const unsigned int Rose::FLOWER_DESCRIPTIONS[FLOWER_NAMES_LENGTH] = {
	IDS_DESC_ROSE, IDS_DESC_BLUE_ORCHID, IDS_DESC_ALLIUM, IDS_DESC_HOUSTONIA, IDS_DESC_TULIP_RED,
	IDS_DESC_TULIP_ORANGE, IDS_DESC_TULIP_WHITE, IDS_DESC_TULIP_PINK, IDS_DESC_OXEYE_DAISY
};

const wstring Rose::TEXTURE_NAMES[] = { 
	L"flower_rose",
	L"flower_blue_orchid", 
	L"flower_allium",
	L"flower_houstonia", 
	L"flower_tulip_red",
	L"flower_tulip_orange", 
	L"flower_tulip_white",
	L"flower_tulip_pink",
	L"flower_oxeye_daisy"
};

bool Rose::mayPlace(Level* level, int x, int y, int z)
{
	return Tile::mayPlace(level, x, y, z) && mayPlaceOn(level->getTile(x, y - 1, z));
}

bool Rose::mayPlaceOn(int tile)
{
	return tile == Tile::grass_Id || tile == Tile::dirt_Id || tile == Tile::farmland_Id;
}

// 4J Added override
void Rose::updateDefaultShape()
{
	float ss = 0.2f;
	setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 3, 0.5f + ss);
}


void Rose::_init()
{
	setTicking(true);
	updateDefaultShape();
}

Rose::Rose(int id) : Tile(id, Material::plant, isSolidRender())
{
	icons = nullptr;
	_init();
}

unsigned int Rose::getDescriptionId(int iData)
{
	if (iData < 0 || iData >= FLOWER_NAMES_LENGTH) iData = 0;

	return FLOWER_DESCRIPTIONS[iData];
}

int Rose::getSpawnResourcesAuxValue(int data)
{
	return data;
}

Icon* Rose::getTexture(int face, int data)
{
	if (data < 0 || data >= FLOWER_NAMES_LENGTH)
	{
		data = 0;
	}
	return icons[data];
}

void Rose::registerIcons(IconRegister* iconRegister)
{
	icons = new Icon * [FLOWER_NAMES_LENGTH];

	for (int i = 0; i < FLOWER_NAMES_LENGTH; i++)
	{
#ifndef _CONTENT_PACKAGE
		wprintf(L"Rose::registerIcons: idx=%d name=%ls\n", i, TEXTURE_NAMES[i].c_str());
#endif
		icons[i] = iconRegister->registerIcon(TEXTURE_NAMES[i]);
	}
}

void Rose::neighborChanged(Level* level, int x, int y, int z, int type)
{
	Tile::neighborChanged(level, x, y, z, type);
	checkAlive(level, x, y, z);
}

void Rose::tick(Level* level, int x, int y, int z, Random* random)
{
	checkAlive(level, x, y, z);
}

void Rose::checkAlive(Level* level, int x, int y, int z)
{
	if (!canSurvive(level, x, y, z))
	{
		this->spawnResources(level, x, y, z, level->getData(x, y, z), 0);
		level->setTileAndData(x, y, z, 0, 0, UPDATE_CLIENTS);
	}
}

bool Rose::canSurvive(Level* level, int x, int y, int z)
{
	return (level->getDaytimeRawBrightness(x, y, z) >= 8 || (level->canSeeSky(x, y, z))) && mayPlaceOn(level->getTile(x, y - 1, z));
}

AABB* Rose::getAABB(Level* level, int x, int y, int z)
{
	return nullptr;
}

bool Rose::blocksLight()
{
	return false;
}

bool Rose::isSolidRender(bool isServerLevel)
{
	return false;
}

bool Rose::isCubeShaped()
{
	return false;
}

int Rose::getRenderShape()
{
	return Tile::SHAPE_CROSS_TEXTURE;
}
