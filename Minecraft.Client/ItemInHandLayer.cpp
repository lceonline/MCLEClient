#include "stdafx.h"
#include "ItemInHandLayer.h"
#include "LivingEntityRenderer.h"
#include "ModelPart.h"
#include "../Minecraft.World/ItemInstance.h"
#include "../Minecraft.World/Item.h"
#include "../Minecraft.World/LivingEntity.h"
#include "../Minecraft.World/Tile.h"

ItemInHandLayer::ItemInHandLayer(LivingEntityRenderer* renderer)
    : renderer(renderer)
{
}

int ItemInHandLayer::colorsOnDamage() {
    return 0;
}

void ItemInHandLayer::render(shared_ptr<LivingEntity> mob,
                              float wp, float ws, float bob,
                              float headRot, float headRotX,
                              float scale, bool useCompiled)
{
    
    ItemInstanceArray slots = mob->getEquipmentSlots();
    if (slots.length == 0) return;
    shared_ptr<ItemInstance> item = slots[0];
    if (!item) return;

    Item* heldItem = item->getItem();
    if (!heldItem) return;

    glPushMatrix();

   
    glTranslatef(-0.0625f, 0.4375f, 0.0625f);

    int itemId = heldItem->id;

    if (itemId > 0 && itemId < 0x100) {
        
        if (Tile::tiles[itemId]) {
            int shape = Tile::tiles[itemId]->getRenderShape();
            if (shape == Tile::SHAPE_BLOCK) {
                glTranslatef(0.0f, 0.1875f, -0.3125f);
                glRotatef(20.0f,  1.0f, 0.0f, 0.0f);
                glRotatef(45.0f,  0.0f, 1.0f, 0.0f);
                glScalef(-0.375f, -0.375f, 0.375f);
            }
        }
    } else {
        
        glTranslatef(0.25f, 0.1875f, -0.1875f);
        glScalef(0.375f, 0.375f, 0.375f);
        glRotatef(60.0f,  0.0f, 0.0f, 1.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(20.0f,  0.0f, 0.0f, 1.0f);
    }

    // TODO: renderItem

    glPopMatrix();
}