#pragma once

#include "Enchantment.h"

class LuckOfTheSeaEnchantment : public Enchantment
{
public:
    LuckOfTheSeaEnchantment(int id, int frequency);

    virtual int getMinCost(int level);
    virtual int getMaxCost(int level);
    virtual int getMaxLevel();
};