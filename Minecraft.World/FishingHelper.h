#pragma once
using namespace std;
#include "../Minecraft.World/WeighedTreasure.h"
#include "../Minecraft.World/ItemInstance.h"
#include "net.minecraft.world.item.h"
#include <unordered_map>
#include <memory>


enum CatchType {
  FISH,
  TREASURE,
  JUNK
};

class CatchWeighedItem : public WeighedRandomItem {
	protected:
		int itemId;
		int count;
		int auxValue;

	public:
		CatchWeighedItem(int itemId, int count, int auxValue, int weight) : WeighedRandomItem(weight)
		{
			this->itemId = itemId;
			this->count = count;
			this->auxValue = auxValue;
		}
		int getItemId()
		{
			return this->itemId;
		}
		int getCount()
		{
			return this->count;
		}
		int getAuxValue()
		{
			return this->auxValue;
		}
};

class FishingHelper
{
	private:
		FishingHelper();

		WeighedRandomItemArray fishingFishArray;
		WeighedRandomItemArray fishingJunkArray;
		WeighedRandomItemArray fishingTreasuresArray;

		CatchWeighedItem* getRandCatch(CatchType catchType, Random* random);
		std::shared_ptr<ItemInstance> handleCatch(CatchWeighedItem* weighedCatch, CatchType catchType, Random* random);
		CatchType getRandCatchType(int luckLevel, int lureLevel, Random* random);
	public:
		// Setup singleton
		FishingHelper(const FishingHelper&) = delete;
		FishingHelper& operator=(const FishingHelper&) = delete;
		static FishingHelper* getInstance();

		std::shared_ptr<ItemInstance> getCatch(int luckLevel, int lureLevel, Random* random);
};