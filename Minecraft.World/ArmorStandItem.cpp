#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.item.h"
#include "Mth.h"
#include "ArmorStandItem.h"
#include "../Minecraft.World/ArmorStand.h"
#include "../Minecraft.World/Entity.h"


ArmorStandItem::ArmorStandItem(int id) : Item(id)
{
    maxStackSize = 16;
}


void ArmorStandItem::randomizePose(shared_ptr<ArmorStand> stand, Random *rng)
{
    Rotations head = stand->getHeadPose();
    float newX = head.getX() + rng->nextFloat() * 5.0f;
    float newY = head.getY() + (rng->nextFloat() * 20.0f - 10.0f);
    stand->setBodyPose(Rotations(newX, newY, head.getZ()));

    Rotations body = stand->getBodyPose();
    float bodyY = body.getY() + (rng->nextFloat() * 10.0f - 5.0f);
    stand->setBodyPose(Rotations(body.getX(), bodyY, body.getZ()));
}


bool ArmorStandItem::useOn(shared_ptr<ItemInstance> itemInstance, shared_ptr<Player> player,
                           Level *level, int x, int y, int z, int face,
                           float clickX, float clickY, float clickZ,
                           bool bTestUseOnOnly)
{
    
    int px = x;
    int py = y;
    int pz = z;

    if (face == 0) py--;
    if (face == 1) py++;
    if (face == 2) pz--;
    if (face == 3) pz++;
    if (face == 4) px--;
    if (face == 5) px++;

    if (level->getTile(px, py,     pz) != 0) return false;
    if (level->getTile(px, py + 1, pz) != 0) return false;
    if (bTestUseOnOnly) return true;
    if (level->isClientSide) return true;

    //rotation
    float targetRot = player->yRot + 180.0f;
    float snapped = (float)(Mth::floor((targetRot + 22.5f) / 45.0f) * 45.0f);

    auto stand = std::make_shared<ArmorStand>(level);
    stand->moveTo(px + 0.5, py, pz + 0.5, snapped, 0.0f);
    stand->yRot = snapped;
    stand->yBodyRot = snapped;
    stand->yHeadRot = snapped;
    stand->yRotO = snapped;     
    stand->yBodyRotO = snapped; 
    stand->yHeadRotO = snapped;
    level->addEntity(stand);

    if (!player->abilities.instabuild)
        itemInstance->count--;

    return true;
}