#include "stdafx.h"
#include "RabbitModel.h"
#include "ModelPart.h"
#include "../Minecraft.World/Mth.h"
#include "../Minecraft.World/Rabbit.h"

#define PI 3.14159265358979323846f

RabbitModel::RabbitModel() : Model()
{
    this->texWidth = 64;
    this->texHeight = 32;

    // head part
    head = new ModelPart(this, 32, 0);
    head->addBox(-2.5F, -4.0F, -5.0F, 5, 4, 5);
    head->setPos(0.0F, 16.0F, -1.0F);

    rightEar = new ModelPart(this, 52, 0);
    rightEar->addBox(-2.5F, -9.0F, -1.0F, 2, 5, 1);
    rightEar->setPos(0.0F, 0.0F, 0.0F);
    rightEar->yRot = -PI / 12.0f;

    leftEar = new ModelPart(this, 58, 0);
    leftEar->addBox(0.5F, -9.0F, -1.0F, 2, 5, 1);
    leftEar->setPos(0.0F, 0.0F, 0.0F);
    leftEar->yRot = PI / 12.0f;

    nose = new ModelPart(this, 32, 9);
    nose->addBox(-0.5F, -2.5F, -5.5F, 1, 1, 1);
    nose->setPos(0.0F, 0.0F, 0.0F);

    head->addChild(rightEar);
    head->addChild(leftEar);
    head->addChild(nose);

    // body part
    body = new ModelPart(this, 0, 0);
    body->addBox(-3.0F, -2.0F, -10.0F, 6, 5, 10);
    body->setPos(0.0F, 19.0F, 8.0F);
    body->xRot = -PI / 9.0f;

    tail = new ModelPart(this, 52, 6);
    tail->addBox(-1.5F, -1.5F, 0.0F, 3, 3, 2);
    tail->setPos(0.0F, 20.0F, 7.0F);
    tail->xRot = -0.3490659F;

    // front legs
    leftHindThigh = new ModelPart(this, 30, 15);
    leftHindThigh->addBox(-1.0F, 0.0F, 0.0F, 2, 4, 5);
    leftHindThigh->setPos(3.0F, 17.5F, 3.7F);
    leftHindThigh->xRot = -0.36651915F;

    leftHindFoot = new ModelPart(this, 26, 24);
    leftHindFoot->addBox(-1.0F, 5.5F, -3.7F, 2, 1, 7);
    leftHindFoot->setPos(0.0F, 0.0F, 0.0F); 
    leftHindFoot->xRot = 0.36651915F;
    leftHindThigh->addChild(leftHindFoot);

    rightHindThigh = new ModelPart(this, 16, 15);
    rightHindThigh->addBox(-1.0F, 0.0F, 0.0F, 2, 4, 5);
    rightHindThigh->setPos(-3.0F, 17.5F, 3.7F);
    rightHindThigh->xRot = -0.36651915F;

    rightHindFoot = new ModelPart(this, 8, 24);
    rightHindFoot->addBox(-1.0F, 5.5F, -3.7F, 2, 1, 7);
    rightHindFoot->setPos(0.0F, 0.0F, 0.0F);
    rightHindFoot->xRot = 0.36651915F;
    rightHindThigh->addChild(rightHindFoot);

    // back legs
    leftFrontLeg = new ModelPart(this, 8, 15);
    leftFrontLeg->addBox(-1.0F, 0.0F, -1.0F, 2, 7, 2);
    leftFrontLeg->setPos(3.0F, 17.0F, -1.0F);
    leftFrontLeg->xRot = -0.19198622F;

    rightFrontLeg = new ModelPart(this, 0, 15);
    rightFrontLeg->addBox(-1.0F, 0.0F, -1.0F, 2, 7, 2);
    rightFrontLeg->setPos(-3.0F, 17.0F, -1.0F);
    rightFrontLeg->xRot = -0.19198622F;
}

void RabbitModel::render(shared_ptr<Entity> entity, float time, float r, float bob, float yRot, float xRot, float scale, bool usecompiled)
{
    shared_ptr<Rabbit> rabbit = dynamic_pointer_cast<Rabbit>(entity);
    
    // head animation
    head->xRot = xRot * (PI / 180.0f);
    head->yRot = yRot * (PI / 180.0f);

    // jump animation
    float jumpCompletion = 0.0f;
    if (rabbit) jumpCompletion = rabbit->getJumpCompletion(bob);
    
    float jumpSin = Mth::sin(jumpCompletion * PI);

    leftHindThigh->xRot = -0.36651915F + (jumpSin * 50.0f * (PI / 180.0f));
    rightHindThigh->xRot = -0.36651915F + (jumpSin * 50.0f * (PI / 180.0f));
    leftFrontLeg->xRot = -0.19198622F + (jumpSin * -40.0f * (PI / 180.0f));
    rightFrontLeg->xRot = -0.19198622F + (jumpSin * -40.0f * (PI / 180.0f));

    
    head->render(scale, usecompiled);
    body->render(scale, usecompiled);
    tail->render(scale, usecompiled);
    leftHindThigh->render(scale, usecompiled);
    rightHindThigh->render(scale, usecompiled);
    leftFrontLeg->render(scale, usecompiled);
    rightFrontLeg->render(scale, usecompiled);
}