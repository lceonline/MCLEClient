#pragma once
#include "HalfSlabTile.h"

class ChunkRebuildData;

class StoneSlabTile2 : public HalfSlabTile
{
    friend class ChunkRebuildData;
public:
    static const int RED_SANDSTONE_SLAB = 0;
    static const int SLAB_NAMES_LENGTH  = 1;
    static const unsigned int SLAB_NAMES[SLAB_NAMES_LENGTH];

private:
    Icon *iconSide;

public:
    StoneSlabTile2(int id);

    virtual int isFullSize() = 0;

    virtual Icon        *getTexture(int face, int data) override;
    virtual void         registerIcons(IconRegister *iconRegister) override;
    virtual int          getResource(int data, Random *random, int playerBonusLevel) override;
    virtual unsigned int getDescriptionId(int iData = -1) override;
    virtual int          getAuxName(int auxValue) override;

protected:
    virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data) override;
};

class HalfStoneSlabTile2 : public StoneSlabTile2
{
public:
    HalfStoneSlabTile2(int id) : StoneSlabTile2(id) { DerivedInit(); }
    virtual int isFullSize() override { return 0; }
};

class FullStoneSlabTile2 : public StoneSlabTile2
{
public:
    FullStoneSlabTile2(int id) : StoneSlabTile2(id) { DerivedInit(); }
    virtual int isFullSize() override { return 1; }
};