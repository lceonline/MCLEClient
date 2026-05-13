#pragma once
#include "HumanoidModel.h"

class Entity;

class ArmorStandArmorModel : public HumanoidModel {
public:
    
    ArmorStandArmorModel(float scale,
                         int texWidth  = 64,
                         int texHeight = 32);
    virtual ~ArmorStandArmorModel();

    virtual void setupAnim(float time, float r, float bob,
                           float yRot, float xRot, float scale,
                           shared_ptr<Entity> entity,
                           unsigned int uiBitmaskOverrideAnim = 0) override;
};