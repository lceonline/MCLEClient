#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.dimension.h"
#include "net.minecraft.world.item.enchantment.h"
#include "net.minecraft.world.food.h"
#include "net.minecraft.stats.h"
#include "PackedIceTile.h"

PackedIceTile::PackedIceTile(int id) : Tile(id, Material::packedIce)
{
	friction = 0.98f;
}
