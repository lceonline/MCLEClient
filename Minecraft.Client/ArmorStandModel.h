
#pragma once
#include "HumanoidModel.h"
#include "ModelPart.h"

class ArmorStandModel : public HumanoidModel
{
public:
    ModelPart* rightBodyStick;
    ModelPart* leftBodyStick;
    ModelPart* shoulderStick;
    ModelPart* basePlate;

    ArmorStandModel(float scale = 0.0f);
    virtual ~ArmorStandModel() {}

    virtual void setupAnim(float time, float r, float bob, float yRot, float xRot,
                           float scale, shared_ptr<Entity> entity,
                           unsigned int uiBitmaskOverrideAnim = 0) override;

    void setupPose(float hX, float hY, float hZ,
                   float bX, float bY, float bZ,
                   float lAX, float lAY, float lAZ,
                   float rAX, float rAY, float rAZ,
                   float lLX, float lLY, float lLZ,
                   float rLX, float rLY, float rLZ);

    virtual void render(shared_ptr<Entity> entity,
                        float time, float r, float bob,
                        float yRot, float xRot,
                        float scale, bool usecompiled) override;
};