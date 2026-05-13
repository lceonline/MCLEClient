#pragma once
#include "Model.h"

class RabbitModel : public Model
{
private:
    ModelPart *head;
    ModelPart *nose;       
    ModelPart *leftEar;    
    ModelPart *rightEar;   

    ModelPart *body;
    ModelPart *tail;       
    ModelPart *leftFrontLeg;  
    ModelPart *rightFrontLeg; 

    ModelPart *leftHindThigh;
    ModelPart *leftHindFoot;
    ModelPart *rightHindThigh;
    ModelPart *rightHindFoot;

public:
    RabbitModel();

    virtual void render(shared_ptr<Entity> entity,
                        float time, float r, float bob,
                        float yRot, float xRot,
                        float scale, bool usecompiled) override;
};