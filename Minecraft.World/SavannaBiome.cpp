#include "stdafx.h"
#include "net.minecraft.world.level.levelgen.feature.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.entity.animal.h"
#include "net.minecraft.world.entity.h"
#include "SavannaBiome.h"
#include "SavannaTreeFeature.h"
#include "DoublePlantFeature.h"
#include "TallGrass2.h"
#include "Level.h"
#include "Random.h"


SavannaBiome::SavannaBiome(int id) : Biome(id)
{
    friendlies.push_back(new MobSpawnerData(eTYPE_HORSE, 1, 2, 6));

    decorator->treeCount   = 1;
    decorator->flowerCount = 4;
    decorator->grassCount  = 20;
}

Feature* SavannaBiome::getTreeFeature(Random* random)
{
    if (random->nextInt(5) > 0)
    {
        return new SavannaTreeFeature(false);
    }
    return new TreeFeature(false);
}

int SavannaBiome::getGrassColor() const
{
    return 0xBFB755;
}

int SavannaBiome::getFolageColor() const
{
    return 0xAEA42A;
}

Feature* SavannaBiome::getFlowerFeature(Random* random, int x, int y, int z)
{
    return nullptr;
}

int SavannaBiome::getRandomDoublePlantType(Random* random)
{
    return 0;
}

void SavannaBiome::decorate(Level* level, Random* random, int xo, int zo)
{
    DOUBLE_PLANT_GENERATOR->setPlantType(TallGrass2::TALL_GRASS);

    for (int i = 0; i < 7; ++i)
    {
        int x = xo + random->nextInt(16) + 8;
        int z = zo + random->nextInt(16) + 8;
        int surfaceY = level->getHeightmapPos(x, z).getY();
        int y = random->nextInt(surfaceY + 32);
        DOUBLE_PLANT_GENERATOR->place(level, random, x, y, z);
    }

    Biome::decorate(level, random, xo, zo);
}

Biome* SavannaBiome::createMutatedCopy(int newId)
{
    MutatedSavannaBiome* mutated = new MutatedSavannaBiome(newId, this);


    mutated->scale       = (this->scale + 1.0f) * 0.5f;
    mutated->depth       = (this->depth * 0.5f) + 0.3f;
    mutated->temperature = (this->temperature * 0.5f) + 1.2f;

    return mutated;
}


MutatedSavannaBiome::MutatedSavannaBiome(int id, Biome* baseBiome)
    : MutatedBiome(id, baseBiome)
{
    decorator->treeCount   = 2;
    decorator->flowerCount = 2;  
    decorator->grassCount  = 5;
}

void MutatedSavannaBiome::buildSurfaceAt(Level* level, Random* random,
                                          ChunkPrimer* primer,
                                          int x, int z, double noiseVal)
{
    
    topMaterial     = static_cast<byte>(Tile::grass_Id);
    topMaterialData = 0;
    material        = static_cast<byte>(Tile::dirt_Id);
    materialData    = 0;

    if (noiseVal > 1.75)
    {
        
        topMaterial     = static_cast<byte>(Tile::stone_Id);
        topMaterialData = 0;
        material        = static_cast<byte>(Tile::stone_Id);
        materialData    = 0;
    }
    else if (noiseVal > -0.5)
    {
        
        topMaterial     = static_cast<byte>(Tile::dirt_Id);
        topMaterialData = 1;
        material        = static_cast<byte>(Tile::dirt_Id);
        materialData    = 0;
    }

    
    Biome::buildSurfaceAt(level, random, primer, x, z, noiseVal);
}

void MutatedSavannaBiome::decorate(Level* level, Random* random, int xo, int zo)
{

    decorator->decorate(level, random, xo, zo);
}