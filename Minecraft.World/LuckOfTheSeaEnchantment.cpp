#include "stdafx.h"
#include "LuckOfTheSeaEnchantment.h"
#include "EnchantmentCategory.h"


LuckOfTheSeaEnchantment::LuckOfTheSeaEnchantment(int id, int frequency) : Enchantment(id, frequency, EnchantmentCategory::fishing_rod)
{
    setDescriptionId(IDS_ENCHANTMENT_LUCK_OF_THE_SEA);
}

// Source: https://github.com/GRAnimated/MinecraftLCE

int LuckOfTheSeaEnchantment::getMinCost(int level)
{
    return 9 * level + 6;
}

int LuckOfTheSeaEnchantment::getMaxCost(int level)
{
    return Enchantment::getMinCost(level) + 50;;
}

int LuckOfTheSeaEnchantment::getMaxLevel()
{
    return 3;
}