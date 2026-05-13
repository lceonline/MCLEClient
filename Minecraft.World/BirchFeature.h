#pragma once
#include "Feature.h" // O "AbstractTreeFeature.h" se ne hai uno

class BirchFeature : public Feature
{
private:
    bool useTallBirch; // Aggiunto dall'assembly (offset 8)

public:
    // Aggiunto il secondo parametro opzionale per evitare errori altrove
    BirchFeature(bool doUpdate, bool useTallBirch = false);

    virtual bool place(Level *level, Random *random, int x, int y, int z) override;
};