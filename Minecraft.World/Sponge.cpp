#include "stdafx.h"
#include "net.minecraft.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.h"

#include "Sponge.h"

const unsigned int Sponge::SPONGE_NAMES[SPONGE_NAMES_LENGTH] = { IDS_TILE_SPONGE, IDS_TILE_WET_SPONGE };
const unsigned int Sponge::SPONGE_DESCRIPTIONS[SPONGE_NAMES_LENGTH] = { IDS_DESC_SPONGE, IDS_DESC_WET_SPONGE };

Sponge::Sponge(int id) : Tile(id, Material::sponge)
{
	wet = false;
	icon = nullptr;
    icon_wet = nullptr;
}

void Sponge::onPlace(Level *level, int x, int y, int z)
{
    wet = level->getData(x, y, z) == 1;
    tryAbsorb(level, x, y, z);
}

unsigned int Sponge::getDescriptionId(int iData)
{
    if (iData < 0 || iData >= SPONGE_NAMES_LENGTH) iData = 0;

    return SPONGE_DESCRIPTIONS[iData];
}

void Sponge::neighborChanged(Level* level, int x, int y, int z, int type)
{
    tryAbsorb(level, x, y, z);
}

void Sponge::tryAbsorb(Level* level, int x, int y, int z) {
	if (!wet && absorb(level, x, y, z)) {
		wet = true;
        level->setTileAndData(x, y, z, Tile::sponge_Id, 1, UPDATE_CLIENTS);
	}
}

bool Sponge::absorb(Level* level, int x, int y, int z) {
    std::queue<std::tuple<int, int, int, int>> queue;
    std::vector<std::tuple<int, int, int>> absorbed;
    queue.push({ x, y, z, 0 });
    int count = 0;
    int dx[] = { 1,-1,0,0,0,0 };
    int dy[] = { 0,0,1,-1,0,0 };
    int dz[] = { 0,0,0,0,1,-1 };
    while (!queue.empty())
    {
        auto [bx, by, bz, depth] = queue.front();
        queue.pop();
        for (int i = 0; i < 6; i++)
        {
            int nx = bx + dx[i];
            int ny = by + dy[i];
            int nz = bz + dz[i];
            if (level->getMaterial(nx, ny, nz) == Material::water)
            {
                level->removeTile(nx, ny, nz);
                absorbed.push_back({ nx, ny, nz });
                ++count;
                if (depth < 6)
                    queue.push({ nx, ny, nz, depth + 1 });
            }
        }
        if (count > 64)
            break;
    }

    for (auto [ax, ay, az] : absorbed)
    {
        level->checkLight(ax, ay, az);
    }

    return count > 0;
}

void Sponge::animateTick(Level* level, int x, int y, int z, Random* random)
{
    if (wet) {
        int facings[] = { Facing::UP, Facing::DOWN, Facing::WEST, Facing::EAST };
        int facing = facings[rand() % 4];

        if (facing != Facing::UP && !level->isTopSolidBlocking(x + Facing::STEP_X[facing], y + Facing::STEP_Y[facing], z + Facing::STEP_Z[facing]))
        {
            double d0 = (double)x;
            double d1 = (double)y;
            double d2 = (double)z;

            if (facing == Facing::DOWN)
            {
                d1 = d1 - 0.05;
                d0 += (double)rand() / RAND_MAX;
                d2 += (double)rand() / RAND_MAX;
            }
            else
            {
                d1 = d1 + ((double)rand() / RAND_MAX) * 0.8;

                if (facing == Facing::EAST)
                {
                    d2 += (double)rand() / RAND_MAX;
                    ++d0;
                }
                else if (facing == Facing::WEST)
                {
                    d2 += (double)rand() / RAND_MAX;
                    d0 += 0.05;
                }
                else
                {
                    d0 += (double)rand() / RAND_MAX;
                    d2 += 0.05;
                }
            }

            level->addParticle(eParticleType_dripWater, d0, d1, d2, 0.0, 0.0, 0.0);
        }
    }
}

Icon* Sponge::getTexture(int face, int data)
{
    return data == 1 ? icon_wet : icon;
}

void Sponge::registerIcons(IconRegister* iconRegister)
{
    icon_wet = iconRegister->registerIcon(L"sponge_wet");
    icon = iconRegister->registerIcon(L"sponge");
}

int Sponge::getSpawnResourcesAuxValue(int data)
{
    return data;
}