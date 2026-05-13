#pragma once
#include <memory>
using namespace std;
class LivingEntity;

class RenderLayer {
public:
    virtual ~RenderLayer() {}
    virtual int  colorsOnDamage() = 0;
    virtual void render(shared_ptr<LivingEntity> mob,
                        float wp, float ws, float bob,
                        float headRot, float headRotX,
                        float scale, bool useCompiled) = 0;
};