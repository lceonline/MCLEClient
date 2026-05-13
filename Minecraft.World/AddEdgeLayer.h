#pragma once
#include "Layer.h"

class AddEdgeLayer : public Layer
{

    
private:
   
    int mode;
    intArray coolWarm(int xo, int yo, int w, int h);
    intArray heatIce(int xo, int yo, int w, int h);
    intArray introduceSpecial(int xo, int yo, int w, int h);


public:
    AddEdgeLayer(int64_t seed, std::shared_ptr<Layer> parent, int64_t seedMixup, int mode);
    virtual ~AddEdgeLayer() {}
    intArray getArea(int xo, int yo, int w, int h) override;
};