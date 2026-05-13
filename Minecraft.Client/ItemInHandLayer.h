#pragma once
#include "RenderLayer.h"

class LivingEntityRenderer;
class LivingEntity;

class ItemInHandLayer : public RenderLayer {
public:
   
    LivingEntityRenderer* renderer;

    explicit ItemInHandLayer(LivingEntityRenderer* renderer);
    virtual ~ItemInHandLayer() {}

    virtual int  colorsOnDamage() override;
    virtual void render(shared_ptr<LivingEntity> mob,
                        float wp, float ws, float bob,
                        float headRot, float headRotX,
                        float scale, bool useCompiled) override;
};