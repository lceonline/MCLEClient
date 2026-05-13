#include "stdafx.h"
#include "RabbitRenderer.h"
#include "RabbitModel.h"
#include "MobRenderer.h"
#include "../Minecraft.World/Rabbit.h"


ResourceLocation RabbitRenderer::LOC_BROWN           = ResourceLocation(TN_MOB_RABBIT_BROWN);
ResourceLocation RabbitRenderer::LOC_WHITE           = ResourceLocation(TN_MOB_RABBIT_WHITE);
ResourceLocation RabbitRenderer::LOC_BLACK           = ResourceLocation(TN_MOB_RABBIT_BLACK);
ResourceLocation RabbitRenderer::LOC_GOLD            = ResourceLocation(TN_MOB_RABBIT_GOLD);
ResourceLocation RabbitRenderer::LOC_SALT            = ResourceLocation(TN_MOB_RABBIT_SALT);
ResourceLocation RabbitRenderer::LOC_WHITE_SPLOTCHED = ResourceLocation(TN_MOB_RABBIT_WHITE_SPLOTCHED);
ResourceLocation RabbitRenderer::LOC_TOAST           = ResourceLocation(TN_MOB_RABBIT_TOAST);
ResourceLocation RabbitRenderer::LOC_EVIL            = ResourceLocation(TN_MOB_RABBIT_CAERBANNOG);

RabbitRenderer::RabbitRenderer() : MobRenderer(new RabbitModel(), 0.3f)
{
    
}

ResourceLocation *RabbitRenderer::getTextureLocation(shared_ptr<Entity> entity)
{
    shared_ptr<Rabbit> mob = dynamic_pointer_cast<Rabbit>(entity);
    if (mob == nullptr) return &LOC_BROWN;

    // easter egg toast
    if (mob->hasCustomName() && mob->getCustomName() == L"Toast")
    {
        return &LOC_TOAST;
    }

    // variants
    switch (mob->getVariant())
    {
        case Rabbit::Variant::WHITE:           return &LOC_WHITE;
        case Rabbit::Variant::BLACK:           return &LOC_BLACK;
        case Rabbit::Variant::GOLD:            return &LOC_GOLD;
        case Rabbit::Variant::SALT:            return &LOC_SALT;
        case Rabbit::Variant::WHITE_SPLOTCHED: return &LOC_WHITE_SPLOTCHED;
        case Rabbit::Variant::EVIL:            return &LOC_EVIL;
        case Rabbit::Variant::BROWN:
        default:                               return &LOC_BROWN;
    }
}

void RabbitRenderer::scale(shared_ptr<LivingEntity> _mob, float a)
{
    // adult scaling
    float s = 0.65f;

    // baby scaling
    if (_mob->isBaby()) {
        s *= 0.8f;
    }

    glScalef(s, s, s);
}