#pragma once
#include "AbstractTreeFeature.h"

class SavannaTreeFeature : public AbstractTreeFeature
{
public:
    SavannaTreeFeature(bool doUpdate);
    virtual bool place(Level* level, Random* random, int x, int y, int z) override;

private:
    static int s_logState;
    static int s_leafState;

    void placeLog(Level* level, int x, int y, int z);
    void placeLeafAt(Level* level, int x, int y, int z);
    void placeLeavesLayer3(Level* level, int cx, int cy, int cz);
    void placeLeavesLayer1(Level* level, int cx, int cy, int cz);
};