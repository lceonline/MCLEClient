#include "stdafx.h"
#include "BlockBlobFeature.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"

BlockBlobFeature::BlockBlobFeature(int blockId, int startRadius) : Feature(false)
{
    this->blockId = blockId;
    this->startRadius = startRadius;
}

bool BlockBlobFeature::place(Level *level, Random *random, int x, int y, int z)
{
    while (true)
    {
        if (y > 3)
        {
            int tileBelow = level->getTile(x, y - 1, z);
            if (tileBelow != 0 && (tileBelow == Tile::grass_Id || tileBelow == Tile::dirt_Id || tileBelow == Tile::stone_Id))
            {
                break;
            }
        }

        if (y <= 3)
        {
            return false;
        }
        y--;
    }

    int radius = this->startRadius;

    for (int step = 0; radius >= 0 && step < 3; ++step)
    {
        int rX = radius + random->nextInt(2);
        int rY = radius + random->nextInt(2);
        int rZ = radius + random->nextInt(2);

        float limit = (float)(rX + rY + rZ) * 0.333F + 0.5F;

        for (int dx = x - rX; dx <= x + rX; ++dx)
        {
            for (int dy = y - rY; dy <= y + rY; ++dy)
            {
                for (int dz = z - rZ; dz <= z + rZ; ++dz)
                {
                    float distSq = (dx - x) * (dx - x) + (dy - y) * (dy - y) + (dz - z) * (dz - z);

                    if (distSq <= limit * limit)
                    {
                        placeBlock(level, dx, dy, dz, this->blockId);
                    }
                }
            }
        }

        x += -(radius + 1) + random->nextInt(2 + radius * 2);
        y += -random->nextInt(2);
        z += -(radius + 1) + random->nextInt(2 + radius * 2);
    }

    return true;
}