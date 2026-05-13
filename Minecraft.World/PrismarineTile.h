#pragma once
#include "Tile.h"

class ChunkRebuildData;
class PrismarineTile : public Tile
{
    friend class ChunkRebuildData;
public:
    static const int TYPE_DEFAULT = 0;
    static const int TYPE_BRICKS = 1;
    static const int TYPE_DARK = 2;

    static const wstring TEXTURE_NAMES[];

    static const int PRISMARINE_NAMES_LENGTH = 3;

    static const unsigned int PRISMARINE_NAMES[PRISMARINE_NAMES_LENGTH];
    static const unsigned int PRISMARINE_USE_DESCS[PRISMARINE_NAMES_LENGTH];

private:
    Icon **icons;

public:

    PrismarineTile(int id);

public:
    virtual Icon *getTexture(int face, int data);

    virtual unsigned int getDescriptionId(int iData = -1);
    virtual unsigned int getUseDescriptionId(int iData = -1);
    virtual int getSpawnResourcesAuxValue(int data);
    void registerIcons(IconRegister *iconRegister);
};