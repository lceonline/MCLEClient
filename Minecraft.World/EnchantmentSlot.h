#pragma once
// 4J Stu Added
// In EnchantmentMenu.java they create an anoymous class while creating some slot. I have moved the content
// of that anonymous class to here
#include "Item.h"
#include "Slot.h"

class Container;

class EnchantmentSlot : public Slot
{
public:
	int slotNum;
	//stack->getItem()->id == 351 && stack->getItem()->getMaterial() == 11
	EnchantmentSlot(shared_ptr<Container> container, int id, int x, int y) : Slot(container, id, x, y) { slotNum = id; }
	virtual bool mayPlace(shared_ptr<ItemInstance> item) {
		if (slotNum == 0 || (item->id == 351 && Item::items[item->id]->getMaterial() == 11)) {
			return true;
		}
		else {
			return false;
		}
	}
	virtual bool mayCombine(shared_ptr<ItemInstance> item) {return false;} // 4J Added
};