#include "stdafx.h"
#include "Rotations.h"
#include "ListTag.h"
#include "FloatTag.h"


Rotations::Rotations(ListTag<FloatTag>* list)
{
    x = list->get(0)->data;
    y = list->get(1)->data;
    z = list->get(2)->data;
}


ListTag<FloatTag>* Rotations::save() const
{
    ListTag<FloatTag>* tag = new ListTag<FloatTag>();
    tag->add(new FloatTag(L"", x));
    tag->add(new FloatTag(L"", y));
    tag->add(new FloatTag(L"", z));
    return tag;
}