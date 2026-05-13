#include "stdafx.h"
#include "ArmorStandArmorModel.h"
#include "ModelPart.h"
#include "../Minecraft.World/ArmorStand.h"

static const float DEG_TO_RAD = 0.017453292f;

ArmorStandArmorModel::ArmorStandArmorModel(float scale, int texWidth, int texHeight)
    : HumanoidModel(scale, 0.0f, texWidth, texHeight)
{
}

ArmorStandArmorModel::~ArmorStandArmorModel() {}

void ArmorStandArmorModel::setupAnim(float time, float r, float bob,
                                      float yRot, float xRot, float scale,
                                      shared_ptr<Entity> entity,
                                      unsigned int uiBitmaskOverrideAnim)
{
    if (!entity) return;

    
    if (!entity->instanceof(eTYPE_ARMORSTAND)) return;

    shared_ptr<ArmorStand> stand = dynamic_pointer_cast<ArmorStand>(entity);
    if (!stand) return;

    Rotations h  = stand->getHeadPose();
    Rotations b  = stand->getBodyPose();
    Rotations la = stand->getLeftArmPose();
    Rotations ra = stand->getRightArmPose();
    Rotations ll = stand->getLeftLegPose();
    Rotations rl = stand->getRightLegPose();

  
    head->xRot = DEG_TO_RAD * h.getX();
    head->yRot = DEG_TO_RAD * h.getY();
    head->zRot = DEG_TO_RAD * h.getZ();
    head->setPos(0.0f, 1.0f, 0.0f);

   
    body->xRot = DEG_TO_RAD * b.getX();
    body->yRot = DEG_TO_RAD * b.getY();
    body->zRot = DEG_TO_RAD * b.getZ();

    
    arm0->xRot = DEG_TO_RAD * la.getX();
    arm0->yRot = DEG_TO_RAD * la.getY();
    arm0->zRot = DEG_TO_RAD * la.getZ();

   
    arm1->xRot = DEG_TO_RAD * ra.getX();
    arm1->yRot = DEG_TO_RAD * ra.getY();
    arm1->zRot = DEG_TO_RAD * ra.getZ();

    
    leg1->xRot = DEG_TO_RAD * ll.getX();
    leg1->yRot = DEG_TO_RAD * ll.getY();
    leg1->zRot = DEG_TO_RAD * ll.getZ();
    leg1->setPos(1.9f, 11.0f, 0.0f);

   
    leg0->xRot = DEG_TO_RAD * rl.getX();
    leg0->yRot = DEG_TO_RAD * rl.getY();
    leg0->zRot = DEG_TO_RAD * rl.getZ();
    leg0->setPos(-1.9f, 11.0f, 0.0f);

    
    ModelPart::copyModelPart(head, hair);
}