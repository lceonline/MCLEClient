#include "stdafx.h"
#include "LureEnchantment.h"
#include "EnchantmentCategory.h"


LureEnchantment::LureEnchantment(int id, int frequency) : Enchantment(id, frequency, EnchantmentCategory::fishing_rod)
{
    setDescriptionId(IDS_ENCHANTMENT_LURE);
}

// Source: https://github.com/GRAnimated/MinecraftLCE

int LureEnchantment::getMinCost(int level)
{
    return 9 * level + 6;
}

int LureEnchantment::getMaxCost(int level)
{
    return Enchantment::getMinCost(level) + 50;;
}

int LureEnchantment::getMaxLevel()
{
    return 3;
}