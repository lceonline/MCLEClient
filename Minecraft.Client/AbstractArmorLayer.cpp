#include "stdafx.h"
#include "AbstractArmorLayer.h"
#include "LivingEntityRenderer.h"
#include "HumanoidModel.h"

AbstractArmorLayer::AbstractArmorLayer(LivingEntityRenderer* renderer)
    : armorModel1(nullptr),
      armorModel2(nullptr),
      renderer(renderer),
      colorR(1.0f),
      colorG(1.0f),
      colorB(1.0f),
      colorA(1.0f),
      hasColor(false)
{
}

HumanoidModel* AbstractArmorLayer::getArmorModel(int slot) {
    if (slot == 2)
        return armorModel1;
    return armorModel2;
}

int AbstractArmorLayer::colorsOnDamage() {
    return 0;
}

void AbstractArmorLayer::resetColor() {
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    hasColor = false;
}
void AbstractArmorLayer::createArmorModels() {
    // default: no-op
}

