#pragma once
#include "Tile.h"
#include "Material.h"
#include "Definitions.h"

class Random;
class Level;

class Rose : public Tile
{
friend class Tile;
public:
	static const int BLUE_ORCHID = 1;
	static const int ALLIUM = 2;
	static const int AZURE_BLUET = 3;
	static const int RED_TULIP = 4;
	static const int ORANGE_TULIP = 5;
	static const int WHITE_TULIP = 6;
	static const int PINK_TULIP = 7;
	static const int OXEYE_DAISY = 8;

	static const int FLOWER_NAMES_LENGTH = 9;
	static const unsigned int FLOWER_NAMES[FLOWER_NAMES_LENGTH];
	static const unsigned int FLOWER_DESCRIPTIONS[FLOWER_NAMES_LENGTH];
	static const wstring TEXTURE_NAMES[];

private:
	Icon** icons;
	void _init();

protected:
	Rose(int id);

public:
	virtual void updateDefaultShape();
	virtual bool mayPlace(Level* level, int x, int y, int z);

protected:
	virtual bool mayPlaceOn(int tile);

public:
	virtual void neighborChanged(Level* level, int x, int y, int z, int type);
	virtual void tick(Level* level, int x, int y, int z, Random* random);

protected:
	void checkAlive(Level* level, int x, int y, int z);

public:
	virtual bool canSurvive(Level* level, int x, int y, int z);
	virtual AABB* getAABB(Level* level, int x, int y, int z);
	virtual bool blocksLight();

	virtual bool isSolidRender(bool isServerLevel = false);
	virtual bool isCubeShaped();
	virtual int getRenderShape();

	virtual Icon* getTexture(int face, int data);
	virtual unsigned int getDescriptionId(int iData = -1);
	virtual int getSpawnResourcesAuxValue(int data);
	void registerIcons(IconRegister* iconRegister);
};
