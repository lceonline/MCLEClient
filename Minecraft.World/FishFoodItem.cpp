#include "stdafx.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.entity.player.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.tile.entity.h"
#include "net.minecraft.world.effect.h"
#include "net.minecraft.h"
#include "net.minecraft.world.h"
#include "FishFoodItem.h"

const unsigned int FishFoodItem::NAMES[FISH_COUNT] = { IDS_ITEM_FISH_RAW, IDS_ITEM_SALMON_RAW, IDS_ITEM_CLOWNFISH, IDS_ITEM_PUFFERFISH };
const unsigned int FishFoodItem::COOKED_NAMES[FISH_COUNT] = { IDS_ITEM_FISH_COOKED, IDS_ITEM_SALMON_COOKED, NULL, NULL };

const unsigned int FishFoodItem::DESCRIPTIONS[FISH_COUNT] = { IDS_DESC_FISH_RAW, IDS_DESC_SALMON_RAW, IDS_DESC_CLOWNFISH, IDS_DESC_PUFFERFISH };
const unsigned int FishFoodItem::COOKED_DESCRIPTIONS[FISH_COUNT] = { IDS_DESC_FISH_COOKED, IDS_DESC_SALMON_COOKED, NULL, NULL };
// making cod temporarily fish
const FishType FISH_TYPES[] = {
	{ 0, L"fish",        2, 0.1f, 5, 0.6f, true  },
	{ 1, L"salmon",     2, 0.1f, 6, 0.8f, true  },
	{ 2, L"clownfish",  1, 0.1f, 0, 0.0f, false },
	{ 3, L"pufferfish", 1, 0.1f, 0, 0.0f, false },
};

FishFoodItem::FishFoodItem(int id, bool cooked) : FoodItem(id, 0, 0, true)
{
	this->cooked = cooked;
	auxValue = 0;
}

void FishFoodItem::addEatEffect(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player)
{
	if (!level->isClientSide)
	{
		if (FISH_TYPES[auxValue].name == L"pufferfish") {
			player->addEffect(new MobEffectInstance(MobEffect::poison->id, 60 * SharedConstants::TICKS_PER_SECOND, 3));
			player->addEffect(new MobEffectInstance(MobEffect::hunger->id, 15 * SharedConstants::TICKS_PER_SECOND, 2));
			player->addEffect(new MobEffectInstance(MobEffect::confusion->id, 15 * SharedConstants::TICKS_PER_SECOND, 1));
		}
	}
	FoodItem::addEatEffect(instance, level, player);
}

shared_ptr<ItemInstance> FishFoodItem::use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player)
{
	// really really REALLY janky way of doing this
	auxValue = instance->getAuxValue();
	return FoodItem::use(instance, level, player);
}

float FishFoodItem::getSaturationModifier()
{
	return this->cooked && FISH_TYPES[auxValue].cookable ? FISH_TYPES[auxValue].cookedSaturationModifier : FISH_TYPES[auxValue].uncookedSaturationModifier;
}

int FishFoodItem::getNutrition()
{
	return this->cooked && FISH_TYPES[auxValue].cookable ? FISH_TYPES[auxValue].cookedHealAmount : FISH_TYPES[auxValue].uncookedHealAmount;
}

Icon* FishFoodItem::getIcon(int itemAuxValue)
{
	if (itemAuxValue < 0 || itemAuxValue >= FISH_COUNT)
	{
		itemAuxValue = 0;
	}

	if (cooked && FISH_TYPES[itemAuxValue].cookable) {
		return icons_cooked[itemAuxValue];
	}
	return icons_raw[itemAuxValue];
}

unsigned int FishFoodItem::getDescriptionId(int iData)
{
	if (iData < 0 || iData >= FISH_COUNT)
	{
		iData = 0;
	}

	if (cooked)
		return COOKED_DESCRIPTIONS[iData];

	return DESCRIPTIONS[iData];
}

unsigned int FishFoodItem::getDescriptionId(shared_ptr<ItemInstance> instance)
{
	auxValue = instance->getAuxValue();
	if (auxValue < 0 || auxValue >= FISH_COUNT)
	{
		auxValue = 0;
	}

	if (cooked) {
		return COOKED_NAMES[auxValue];
	}

	return NAMES[auxValue];
}

void FishFoodItem::registerIcons(IconRegister* iconRegister)
{
	for (int i = 0; i < FISH_COUNT; i++)
	{
		if (FISH_TYPES[i].cookable) {
			icons_raw[i] = iconRegister->registerIcon(FISH_TYPES[i].name + L"Raw");
			icons_cooked[i] = iconRegister->registerIcon(FISH_TYPES[i].name + L"Cooked");
		}
		else {
			icons_raw[i] = iconRegister->registerIcon(FISH_TYPES[i].name);
		}
	}
}