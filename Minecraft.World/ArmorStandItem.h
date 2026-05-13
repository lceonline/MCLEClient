#pragma once
#include "Item.h"
#include "ArmorStand.h"

class ArmorStandItem : public Item
{
public:
    explicit ArmorStandItem(int id);
    virtual ~ArmorStandItem() {}

    virtual bool useOn(shared_ptr<ItemInstance> itemInstance,
                       shared_ptr<Player> player,
                       Level *level, int x, int y, int z, int face,
                       float clickX, float clickY, float clickZ,
                       bool bTestUseOnOnly = false) override;

    
    static void randomizePose(shared_ptr<ArmorStand> stand, Random *rng);
};