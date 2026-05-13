#pragma once

#include "Biome.h"
#include "MutatedBiome.h"   
#include "BlockPos.h"

class ForestBiome : public Biome
{
public:
    ForestBiome(int id, int type = 0);
    virtual ~ForestBiome();

    virtual Feature* getTreeFeature(Random* random) override;
    virtual Biome* setColor(int color, bool b = false) override;
    virtual int getRandomFlower(Random* rand, const BlockPos& pos) const;
    virtual void decorate(Level* level, Random* rand, int xo, int zo) override;
    virtual unsigned int getGrassColor(const BlockPos& pos) const;
    
    virtual Feature* getFlowerFeature(Random* random, int x, int y, int z) override;
    virtual int getRandomDoublePlantType(Random* random) override;

    virtual Biome* createMutatedCopy(int id) const; 

    class MutatedBirchForestBiome : public MutatedBiome
    {
    public:
        
        MutatedBirchForestBiome(int id, Biome* baseBiome); 
        virtual ~MutatedBirchForestBiome();

        virtual Feature* getTreeFeature(Random* random) override;
    };

    class MutatedForestBiome : public MutatedBiome
    {
    public:
        // Rimosso 'const'
        MutatedForestBiome(int id, Biome* baseBiome); 
        virtual ~MutatedForestBiome();

        virtual void decorate(Level* level, Random* rand, int xo, int zo) override;
    };

private:
    int biomeType;
};