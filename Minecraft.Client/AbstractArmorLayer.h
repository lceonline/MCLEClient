#pragma once
#include <memory>
using namespace std;

class LivingEntityRenderer;
class HumanoidModel;
class LivingEntity;

class AbstractArmorLayer {
public:
    HumanoidModel*        armorModel1;
    HumanoidModel*        armorModel2;
    LivingEntityRenderer* renderer;
    float colorR;
    float colorG;
    float colorB;
    float colorA;
    bool  hasColor;

    explicit AbstractArmorLayer(LivingEntityRenderer* renderer);
    virtual ~AbstractArmorLayer() {}

    virtual HumanoidModel* getArmorModel(int slot);
    virtual void           createArmorModels();
    virtual int            colorsOnDamage();
    virtual void           resetColor();
};