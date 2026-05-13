#include "stdafx.h"
#include "HumanoidMobRenderer.h"
#include "ArmorStandRenderer.h"
#include "ArmorStandArmorModel.h"
#include "HumanoidModel.h"
#include "ArmorStandModel.h"
#include "CustomHeadLayer.h"
#include "Textures.h"
#include "../Minecraft.World/ArmorStand.h"
#include "../Minecraft.World/ArmorItem.h"
#include "../Minecraft.World/Tile.h"
#include "../Minecraft.World/Facing.h"
#include <cmath>
#include "EntityRenderDispatcher.h"
#include "SkullTileRenderer.h"
#include "PlayerRenderer.h"
#include "../Minecraft.World/SkullItem.h"
#include "../Minecraft.World/SkullTileEntity.h"

static const float DEG_TO_RAD = 3.14159265f / 180.0f;

ResourceLocation ArmorStandRenderer::LOC_ARMOR_STAND = ResourceLocation(TN_MOB_ARMORSTAND);

ArmorStandRenderer::ArmorStandArmorLayer::ArmorStandArmorLayer(LivingEntityRenderer* renderer)
    : HumanoidArmorLayer(renderer)
{
    delete armorModel1;
    delete armorModel2;
    armorModel1 = new ArmorStandArmorModel(0.5f);
    armorModel2 = new ArmorStandArmorModel(1.0f);
}

void ArmorStandRenderer::ArmorStandArmorLayer::createArmorModels()
{
    delete armorModel1;
    delete armorModel2;
    armorModel1 = new ArmorStandArmorModel(0.5f);
    armorModel2 = new ArmorStandArmorModel(1.0f);
}

ArmorStandRenderer::ArmorStandRenderer()
    : LivingEntityRenderer(new ArmorStandModel(0.0f), 0.0f)
{
    armorLayer = new ArmorStandArmorLayer(this);

    ArmorStandModel* m = static_cast<ArmorStandModel*>(getModel());
    
    
    headLayer = m ? new CustomHeadLayer(m->head, this) : nullptr;
}

ArmorStandRenderer::~ArmorStandRenderer() {}

ResourceLocation* ArmorStandRenderer::getTextureLocation(shared_ptr<Entity> entity)
{
    return &LOC_ARMOR_STAND;
}

bool ArmorStandRenderer::shouldShowName(shared_ptr<LivingEntity> entity)
{
    if (!entity) return false;
    return entity->isCustomNameVisible();
}

void ArmorStandRenderer::setupRotations(shared_ptr<LivingEntity> mob,
                                        float bob, float bodyRot, float a)
{
    shared_ptr<ArmorStand> stand = dynamic_pointer_cast<ArmorStand>(mob);
    glRotatef(180.0f - bodyRot, 0.0f, 1.0f, 0.0f);

    if (stand)
    {
        long long ticksSinceHit = (long long)stand->tickCount - stand->lastHit;
        if (ticksSinceHit >= 0 && ticksSinceHit < 5)
        {
            float wobble = (float)(ticksSinceHit + a) / 5.0f;
            float angle  = (float)stand->hurtDir * sinf(wobble * 3.14159265f) * 3.0f;
            glRotatef(angle, 0.0f, 0.0f, 1.0f);
        }
    }
}

void ArmorStandRenderer::render(shared_ptr<Entity> entity,
                                double x, double y, double z,
                                float rot, float a)
{
    LivingEntityRenderer::render(entity, x, y, z, rot, a);
}

void ArmorStandRenderer::renderModel(shared_ptr<LivingEntity> mob,
                                     float wp, float ws, float bob,
                                     float headRotMinusBodyRot,
                                     float headRotx, float scale)
{
    shared_ptr<ArmorStand> stand = dynamic_pointer_cast<ArmorStand>(mob);
    if (!stand) return;

    ArmorStandModel* m = static_cast<ArmorStandModel*>(getModel());
    if (!m) return;

    Rotations h  = stand->getHeadPose();
    Rotations b  = stand->getBodyPose();
    Rotations la = stand->getLeftArmPose();
    Rotations ra = stand->getRightArmPose();
    Rotations ll = stand->getLeftLegPose();
    Rotations rl = stand->getRightLegPose();

    m->setupPose(
        h.x  * DEG_TO_RAD, h.y  * DEG_TO_RAD, h.z  * DEG_TO_RAD,
        b.x  * DEG_TO_RAD, b.y  * DEG_TO_RAD, b.z  * DEG_TO_RAD,
        la.x * DEG_TO_RAD, la.y * DEG_TO_RAD, la.z * DEG_TO_RAD,
        ra.x * DEG_TO_RAD, ra.y * DEG_TO_RAD, ra.z * DEG_TO_RAD,
        ll.x * DEG_TO_RAD, ll.y * DEG_TO_RAD, ll.z * DEG_TO_RAD,
        rl.x * DEG_TO_RAD, rl.y * DEG_TO_RAD, rl.z * DEG_TO_RAD
    );
    
    if (armorLayer)
    {
        auto applyPose = [&](HumanoidModel* am)
        {
            if (!am) return;
            am->head->xRot = h.x  * DEG_TO_RAD;
            am->head->yRot = h.y  * DEG_TO_RAD;
            am->head->zRot = h.z  * DEG_TO_RAD;
            if (am->hair)
            {
                am->hair->xRot = h.x * DEG_TO_RAD;
                am->hair->yRot = h.y * DEG_TO_RAD;
                am->hair->zRot = h.z * DEG_TO_RAD;
            }
            am->body->xRot = b.x  * DEG_TO_RAD;
            am->body->yRot = b.y  * DEG_TO_RAD;
            am->body->zRot = b.z  * DEG_TO_RAD;
            am->arm1->xRot = la.x * DEG_TO_RAD;
            am->arm1->yRot = la.y * DEG_TO_RAD;
            am->arm1->zRot = la.z * DEG_TO_RAD;
            am->arm0->xRot = ra.x * DEG_TO_RAD;
            am->arm0->yRot = ra.y * DEG_TO_RAD;
            am->arm0->zRot = ra.z * DEG_TO_RAD;
            am->leg0->xRot = rl.x * DEG_TO_RAD;
            am->leg0->yRot = rl.y * DEG_TO_RAD;
            am->leg0->zRot = rl.z * DEG_TO_RAD;
            am->leg1->xRot = ll.x * DEG_TO_RAD;
            am->leg1->yRot = ll.y * DEG_TO_RAD;
            am->leg1->zRot = ll.z * DEG_TO_RAD;
        };

        applyPose(static_cast<HumanoidModel*>(armorLayer->armorModel1));
        applyPose(static_cast<HumanoidModel*>(armorLayer->armorModel2));
    }

    LivingEntityRenderer::renderModel(mob, wp, ws, bob, headRotMinusBodyRot, headRotx, scale);

    
    if (headLayer)
    {
        float fScale = 1.0f / 16.0f;
        float bodyRot = mob->yBodyRotO + (mob->yBodyRot - mob->yBodyRotO) * 0.0f;
        float headRot = mob->yHeadRotO + (mob->yHeadRot - mob->yHeadRotO) * 0.0f;
        float headRotX = mob->xRotO    + (mob->xRot    - mob->xRotO)    * 0.0f;
        headLayer->render(mob, wp, ws, bob, headRot - bodyRot, headRotX, fScale, true);
    }
}


void ArmorStandRenderer::additionalRendering(shared_ptr<LivingEntity> mob, float a)
{
    
}


int ArmorStandRenderer::prepareArmor(shared_ptr<LivingEntity> mob, int layer, float a)
{
    if (!armorLayer) return -1;

    shared_ptr<ItemInstance> itemInstance = mob->getArmor(3 - layer);
    if (!itemInstance) return -1;

    Item* item = itemInstance->getItem();
    if (!item) return -1;

    ArmorItem* armorItem = dynamic_cast<ArmorItem*>(item);
    if (!armorItem) return -1; 

    bindTexture(HumanoidMobRenderer::getArmorLocation(armorItem, layer));

    HumanoidModel* am = armorLayer->getArmorModel(layer);
    if (!am) return -1;

    am->head->visible = (layer == 0);
    if (am->hair) am->hair->visible = (layer == 0);
    am->body->visible = (layer == 1 || layer == 2);
    am->arm0->visible = (layer == 1);
    am->arm1->visible = (layer == 1);
    am->leg0->visible = (layer == 2 || layer == 3);
    am->leg1->visible = (layer == 2 || layer == 3);

    setArmor(am);
    am->attackTime = model->attackTime;
    am->riding     = model->riding;
    am->young      = mob->isBaby();

    if (armorItem->getMaterial() == ArmorItem::ArmorMaterial::CLOTH)
    {
        int   color = armorItem->getColor(itemInstance);
        float red   = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
        float green = static_cast<float>((color >>  8) & 0xFF) / 255.0f;
        float blue  = static_cast<float>( color        & 0xFF) / 255.0f;
        glColor3f(red, green, blue);
        return itemInstance->isEnchanted() ? 0x1f : 0x10;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    return itemInstance->isEnchanted() ? 15 : 1;
}