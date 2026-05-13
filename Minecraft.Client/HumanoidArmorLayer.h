#pragma once
#include "AbstractArmorLayer.h"

class LivingEntityRenderer;
class LivingEntity;

class HumanoidArmorLayer : public AbstractArmorLayer {
public:
    explicit HumanoidArmorLayer(LivingEntityRenderer* renderer);
    virtual ~HumanoidArmorLayer() {}

    virtual void createArmorModels() override;
    virtual void setPartVisibility(HumanoidModel* model, unsigned int slot);
};