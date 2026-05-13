#pragma once
#include "Feature.h"

class IceSpikeFeature : public Feature
{
public:
    IceSpikeFeature();
    
    virtual bool place(Level *level, Random *random, int x, int y, int z) override;
};