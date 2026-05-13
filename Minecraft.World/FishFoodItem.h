#pragma once
#pragma once

#include "FoodItem.h"

struct FishType {
	int id;
	std::wstring name;
	int uncookedHealAmount;
	float uncookedSaturationModifier;
	int cookedHealAmount = 0;
	float cookedSaturationModifier = 0.0f;
	bool cookable = false;
};

static const int FISH_COUNT = 4;

extern const FishType FISH_TYPES[FISH_COUNT];

class FishFoodItem : public FoodItem
{
public:
	static const unsigned int NAMES[FISH_COUNT];
	static const unsigned int COOKED_NAMES[FISH_COUNT];

	static const unsigned int DESCRIPTIONS[FISH_COUNT];
	static const unsigned int COOKED_DESCRIPTIONS[FISH_COUNT];

	int auxValue;
	bool cooked;
private:
	Icon* icons_raw[FISH_COUNT];
	Icon* icons_cooked[FISH_COUNT];
protected:
	virtual void addEatEffect(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player);
public:
	FishFoodItem(int id, bool cooked);

	virtual shared_ptr<ItemInstance> use(shared_ptr<ItemInstance> instance, Level* level, shared_ptr<Player> player);
	Icon* getIcon(int itemAuxValue);

	int getNutrition();
	float getSaturationModifier();

	virtual unsigned int getDescriptionId(int iData = -1);
	unsigned int getDescriptionId(shared_ptr<ItemInstance> instance);
	void registerIcons(IconRegister* iconRegister);
};