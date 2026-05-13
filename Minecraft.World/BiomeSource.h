#pragma once
#include "Biome.h"
#include "BiomeSource.h"
#include "BiomeCache.h"
#include "net.minecraft.world.level.levelgen.synth.h"

class ChunkPos;
class Level;
class Layer;
class TilePos;
class LevelType;

class BiomeSource
{
private:
    shared_ptr<Layer> layer;
    shared_ptr<Layer> zoomedLayer;
public:
    static const int CACHE_DIAMETER = 256;

private:
    BiomeCache *cache;
    vector<Biome *> playerSpawnBiomes;
    std::wstring generatorOptions;

protected:
    void _init();
    void _init(int64_t seed, LevelType *generator, int xzSize = 864);
    BiomeSource();

public:
    BiomeSource(int64_t seed, LevelType *generator, int xzSize = 864);
    BiomeSource(Level *level);

    ~BiomeSource();

#ifdef __PSVITA__
    static int64_t findSeed(LevelType *generator, bool* pServerRunning);
#else
    static int64_t findSeed(LevelType *generator);
#endif

    vector<Biome *> getPlayerSpawnBiomes() { return playerSpawnBiomes; }
    virtual Biome *getBiome(ChunkPos *cp);
    virtual Biome *getBiome(int x, int z);

    virtual float getDownfall(int x, int z) const;
    virtual floatArray getDownfallBlock(int x, int z, int w, int h) const;
    virtual void getDownfallBlock(floatArray &downfalls, int x, int z, int w, int h) const;

    virtual BiomeCache::Block *getBlockAt(int x, int y);
    virtual float getTemperature(int x, int y, int z) const;
    float scaleTemp(float temp, int y) const;
    
    virtual floatArray getTemperatureBlock(int x, int z, int w, int h) const;
    virtual void getTemperatureBlock(floatArray& temperatures, int x, int z, int w, int h) const;

    virtual BiomeArray getRawBiomeBlock(int x, int z, int w, int h) const;
    virtual void getRawBiomeBlock(BiomeArray &biomes, int x, int z, int w, int h) const;
    virtual void getRawBiomeIndices(intArray &biomes, int x, int z, int w, int h) const;
    virtual BiomeArray getBiomeBlock(int x, int z, int w, int h) const;
    virtual void getBiomeBlock(BiomeArray& biomes, int x, int z, int w, int h, bool useCache) const;

    virtual byteArray getBiomeIndexBlock(int x, int z, int w, int h) const;
    virtual void getBiomeIndexBlock(byteArray& biomeIndices, int x, int z, int w, int h, bool useCache) const;

    virtual bool containsOnly(int x, int z, int r, vector<Biome *> allowed);
    virtual bool containsOnly(int x, int z, int r, Biome *allowed);

    virtual TilePos *findBiome(int x, int z, int r, Biome *toFind, Random *random);
    virtual TilePos *findBiome(int x, int z, int r, vector<Biome *> allowed, Random *random);

    void update();

    int getBiomeGroup(int id);
    void getFracs(intArray indices, float* fracs, float* groupFracs);

private:
    static bool getIsMatch(float* fracs, float* groupFracs);
};