#include "stdafx.h"
#include "HumanoidArmorLayer.h"
#include "HumanoidModel.h"
#include "ModelPart.h"

HumanoidArmorLayer::HumanoidArmorLayer(LivingEntityRenderer* renderer)
    : AbstractArmorLayer(renderer)
{
    
    armorModel1 = new HumanoidModel(0.5f);
    armorModel2 = new HumanoidModel(1.0f);
}

void HumanoidArmorLayer::createArmorModels() {
    delete armorModel1;
    delete armorModel2;
    armorModel1 = new HumanoidModel(0.5f);
    armorModel2 = new HumanoidModel(1.0f);
}

void HumanoidArmorLayer::setPartVisibility(HumanoidModel* m, unsigned int slot) {
    
    m->setAllVisible(false);

   
    switch (slot) {
        case 0: break;  
        case 1:         
            m->leg0->visible = true;
            m->leg1->visible = true;
            break;
        case 2:        
            m->body->visible = true;
            m->leg0->visible = true;
            m->leg1->visible = true;
            break;
        case 3:        
            m->arm1->visible = true;
            m->arm0->visible = true;
            break;
        case 4:         
            m->head->visible = true;
            if (m->hair) m->hair->visible = true;
            break;
        default: break;
    }
}