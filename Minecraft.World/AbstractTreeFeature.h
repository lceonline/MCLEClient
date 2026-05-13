#pragma once
#include "Feature.h"
#include "Level.h"
#include "Tile.h"

class AbstractTreeFeature : public Feature
{
public:
    AbstractTreeFeature(bool doUpdate) : Feature(doUpdate) {}
    virtual ~AbstractTreeFeature() {}

    static bool isFree(Level* level, int x, int y, int z)
    {
        int tile = level->getTile(x, y, z);
        return tile == 0
            || tile == Tile::leaves_Id
            || tile == Tile::leaves2_Id
            || tile == Tile::treeTrunk_Id
            || tile == Tile::tree2Trunk_Id
            || tile == Tile::vine_Id
            || tile == Tile::tallgrass_Id
            || tile == Tile::flower_Id;
    }

    void setDirtAt(Level* level, int x, int y, int z)
    {
        if (level->getTile(x, y, z) != Tile::dirt_Id)
        {
            placeBlock(level, x, y, z, Tile::dirt_Id, 0);
        }
    }
    virtual void postPlaceTree() {}
};