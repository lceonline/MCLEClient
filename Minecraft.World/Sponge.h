#pragma once

#include "Tile.h"

class Sponge : public Tile
{
	friend class Tile;
public:
	static const int SPONGE_NAMES_LENGTH = 2;


	static const unsigned int SPONGE_NAMES[SPONGE_NAMES_LENGTH];
	static const unsigned int SPONGE_DESCRIPTIONS[SPONGE_NAMES_LENGTH];
	static const wstring TEXTURE_NAMES[];

	bool wet;
private:
	Icon* icon_wet;
	Icon* icon;
protected:
	Sponge(int id);
	void onPlace(Level* level, int x, int y, int z);
	void neighborChanged(Level* level, int x, int y, int z, int type);
	void tryAbsorb(Level* level, int x, int y, int z);
	bool absorb(Level* level, int x, int y, int z);
	void animateTick(Level* level, int x, int y, int z, Random* random);
	void registerIcons(IconRegister* iconRegister);
	unsigned int getDescriptionId(int iData);
	Icon* getTexture(int face, int data);
	int getSpawnResourcesAuxValue(int data);
};