#pragma once
#include "HeavyTile.h"

class SandTile : public HeavyTile
{
public:
    static const int RED_SAND = 1;
    static const int SAND_NAMES_LENGTH = 2;
    static const unsigned int SAND_NAMES[SAND_NAMES_LENGTH];
    static const wstring TEXTURE_NAMES[];

    SandTile(int type, bool isSolidRender = true);
    virtual int getSpawnResourcesAuxValue(int data) override;
    virtual Icon* getTexture(int face, int data) override;
    void registerIcons(IconRegister* iconRegister) override;

private:
    Icon** icons;
};