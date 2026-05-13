#pragma once
#include "Tile.h"

class DirtTile : public Tile
{
friend class Tile;
public:
	static const int COARSE_DIRT = 1;
	static const int PODZOL = 2;

	static const int DIRT_NAMES_LENGTH = 3;

	static const unsigned int DIRT_NAMES[DIRT_NAMES_LENGTH];
	static const unsigned int DIRT_DESCRIPTIONS[DIRT_NAMES_LENGTH];
	static const wstring TEXTURE_NAMES[];
private:
	Icon** icons;
	Icon* podzolTop;
	Icon* podzolSide;

public:
	DirtTile(int id);
	virtual Icon* getTexture(int face, int data);
	virtual unsigned int getDescriptionId(int iData = -1);
	virtual int getSpawnResourcesAuxValue(int data);
	void registerIcons(IconRegister* iconRegister);
};