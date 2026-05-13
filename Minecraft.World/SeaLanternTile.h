#pragma once
#include "Tile.h"

class SeaLanternTile : public Tile
{
public:
    SeaLanternTile(int id, Material* material);
    virtual int getResourceCountForLootBonus(int bonusLevel, Random* random);
    virtual int getResourceCount(Random* random);
    virtual int getResource(int data, Random* random, int playerBonusLevel);
    virtual void registerIcons(IconRegister* iconRegister);
    virtual Icon* getTexture(int face, int data);
private:
    Icon* icon;
};