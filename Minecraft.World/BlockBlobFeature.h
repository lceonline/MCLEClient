#pragma once
#include "Feature.h"

class BlockBlobFeature : public Feature
{
private:
    int blockId;
    int startRadius;

public:
    BlockBlobFeature(int blockId, int startRadius);
    bool place(Level* level, Random* random, int x, int y, int z)override;;
};