#pragma once
#include "HalfSlabTile.h"

class Player;

class WoodSlabTile : public HalfSlabTile
{
    friend class Tile;
public:
    static const int SLAB_NAMES_LENGTH = 6;
    static const unsigned int SLAB_NAMES[SLAB_NAMES_LENGTH];

public:
    WoodSlabTile(int id); 

    virtual int isFullSize() = 0;

    virtual Icon        *getTexture(int face, int data) override;
    virtual int          getResource(int data, Random *random, int playerBonusLevel) override;
    virtual int          getAuxName(int auxValue) override;
    virtual void         registerIcons(IconRegister *iconRegister) override;

protected:
    virtual shared_ptr<ItemInstance> getSilkTouchItemInstance(int data) override;
};


class HalfWoodSlabTile : public WoodSlabTile
{
public:
    HalfWoodSlabTile(int id) : WoodSlabTile(id) { DerivedInit(); }
    virtual int isFullSize() override { return 0; }
};

class FullWoodSlabTile : public WoodSlabTile
{
public:
    FullWoodSlabTile(int id) : WoodSlabTile(id) { DerivedInit(); }
    virtual int isFullSize() override { return 1; }
};