#pragma once
#include "HalfSlabTile.h"

class ChunkRebuildData;

class StoneSlabTile : public HalfSlabTile
{
    friend class ChunkRebuildData;
public:
    static const int STONE_SLAB        = 0;
    static const int SAND_SLAB         = 1;
    static const int WOOD_SLAB         = 2;
    static const int COBBLESTONE_SLAB  = 3;
    static const int BRICK_SLAB        = 4;
    static const int SMOOTHBRICK_SLAB  = 5;
    static const int NETHERBRICK_SLAB  = 6;
    static const int QUARTZ_SLAB       = 7;
    static const int SLAB_NAMES_LENGTH = 8;
    static const unsigned int SLAB_NAMES[SLAB_NAMES_LENGTH];

private:
    Icon *iconSide;

public:
    StoneSlabTile(int id); 

    virtual int isFullSize() = 0; 

    virtual Icon        *getTexture(int face, int data) override;
    virtual void         registerIcons(IconRegister *iconRegister) override;
    virtual int          getResource(int data, Random *random, int playerBonusLevel) override;
    virtual unsigned int getDescriptionId(int iData = -1) override;
    virtual int          getAuxName(int auxValue) override;

protected:
    virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data) override;
};



class HalfStoneSlabTile : public StoneSlabTile
{
public:
    HalfStoneSlabTile(int id) : StoneSlabTile(id) { DerivedInit(); }
    virtual int isFullSize() override { return 0; }
};

class FullStoneSlabTile : public StoneSlabTile
{
public:
    FullStoneSlabTile(int id) : StoneSlabTile(id) { DerivedInit(); }
    virtual int isFullSize() override { return 1; }
};