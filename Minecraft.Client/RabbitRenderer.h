#pragma once
#include "MobRenderer.h"

class Rabbit;
class LivingEntity;

class RabbitRenderer : public MobRenderer
{
private:
    
    static ResourceLocation LOC_BROWN;
    static ResourceLocation LOC_WHITE;
    static ResourceLocation LOC_BLACK;
    static ResourceLocation LOC_GOLD;
    static ResourceLocation LOC_SALT;
    static ResourceLocation LOC_WHITE_SPLOTCHED;
    static ResourceLocation LOC_TOAST;
    static ResourceLocation LOC_EVIL;

public:
    RabbitRenderer();

    virtual ResourceLocation *getTextureLocation(shared_ptr<Entity> entity) override;

protected:
    virtual void scale(shared_ptr<LivingEntity> mob, float a) override;
};