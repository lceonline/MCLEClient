#include "stdafx.h"
#include "Class.h"
#include "BasicTypeContainers.h"
#include "InputOutputStream.h"
#include "net.minecraft.h"
#include "net.minecraft.network.packet.h"
#include "net.minecraft.world.item.h"
#include "SynchedEntityData.h"

SynchedEntityData::SynchedEntityData()
{
    m_isDirty = false;
    m_isEmpty = true;
}



void SynchedEntityData::define(int id, int value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_INT, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, byte value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_BYTE, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, short value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_SHORT, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, float value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_FLOAT, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, const wstring& value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_STRING, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, const Rotations& value)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_ROTATIONS, id, value);
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::defineNULL(int id, void* pVal)
{
    MemSect(17);
    checkId(id);
    itemsById[id] = std::make_shared<DataItem>(TYPE_ITEMINSTANCE, id, shared_ptr<ItemInstance>());
    MemSect(0);
    m_isEmpty = false;
}

void SynchedEntityData::checkId(int id)
{
    // validation disabled in shipping build
}



byte SynchedEntityData::getByte(int id)             { return itemsById[id]->getValue_byte(); }
short SynchedEntityData::getShort(int id)           { return itemsById[id]->getValue_short(); }
int SynchedEntityData::getInteger(int id)           { return itemsById[id]->getValue_int(); }
float SynchedEntityData::getFloat(int id)           { return itemsById[id]->getValue_float(); }
wstring SynchedEntityData::getString(int id)        { return itemsById[id]->getValue_wstring(); }
shared_ptr<ItemInstance> SynchedEntityData::getItemInstance(int id) { return itemsById[id]->getValue_itemInstance(); }

Pos* SynchedEntityData::getPos(int id)
{
    assert(false);
    return nullptr;
}

Rotations SynchedEntityData::getRotations(int id)
{
    return itemsById[id]->getValue_rotations();
}



void SynchedEntityData::set(int id, int value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_int()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, byte value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_byte()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, short value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_short()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, float value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_float()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, const wstring& value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_wstring()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, shared_ptr<ItemInstance> value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_itemInstance()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}

void SynchedEntityData::set(int id, const Rotations& value)
{
    shared_ptr<DataItem> item = itemsById[id];
    if (value != item->getValue_rotations()) { item->setValue(value); item->setDirty(true); m_isDirty = true; }
}



void SynchedEntityData::markDirty(int id)   { itemsById[id]->dirty = true; m_isDirty = true; }
bool SynchedEntityData::isDirty()           { return m_isDirty; }
bool SynchedEntityData::isEmpty()           { return m_isEmpty; }
void SynchedEntityData::clearDirty()        { m_isDirty = false; }

void SynchedEntityData::pack(vector<shared_ptr<DataItem>>* items, DataOutputStream* output)
{
    if (items)
        for (auto& item : *items)
            writeDataItem(output, item);
    output->writeByte(EOF_MARKER);
}

vector<shared_ptr<SynchedEntityData::DataItem>>* SynchedEntityData::packDirty()
{
    vector<shared_ptr<DataItem>>* result = nullptr;
    if (m_isDirty)
    {
        for (int i = 0; i <= MAX_ID_VALUE; i++)
        {
            shared_ptr<DataItem> item = itemsById[i];
            if (item && item->isDirty())
            {
                item->setDirty(false);
                if (!result) result = new vector<shared_ptr<DataItem>>();
                result->push_back(item);
            }
        }
    }
    m_isDirty = false;
    return result;
}

void SynchedEntityData::packAll(DataOutputStream* output)
{
    for (int i = 0; i <= MAX_ID_VALUE; i++)
        if (itemsById[i]) writeDataItem(output, itemsById[i]);
    output->writeByte(EOF_MARKER);
}

vector<shared_ptr<SynchedEntityData::DataItem>>* SynchedEntityData::getAll()
{
    vector<shared_ptr<DataItem>>* result = nullptr;
    for (int i = 0; i <= MAX_ID_VALUE; i++)
    {
        if (itemsById[i])
        {
            if (!result) result = new vector<shared_ptr<DataItem>>();
            result->push_back(itemsById[i]);
        }
    }
    return result;
}

void SynchedEntityData::writeDataItem(DataOutputStream* output, shared_ptr<DataItem> item)
{
    int header = ((item->getType() << TYPE_SHIFT) | (item->getId() & MAX_ID_VALUE)) & 0xff;
    output->writeByte(header);
    switch (item->getType())
    {
    case TYPE_BYTE:         output->writeByte(item->getValue_byte());   break;
    case TYPE_INT:          output->writeInt(item->getValue_int());     break;
    case TYPE_SHORT:        output->writeShort(item->getValue_short()); break;
    case TYPE_FLOAT:        output->writeFloat(item->getValue_float()); break;
    case TYPE_STRING:       Packet::writeUtf(item->getValue_wstring(), output); break;
    case TYPE_ITEMINSTANCE: Packet::writeItem(item->getValue_itemInstance(), output); break;
    case TYPE_ROTATIONS:
        output->writeFloat(item->getValue_rotations().getX());
        output->writeFloat(item->getValue_rotations().getY());
        output->writeFloat(item->getValue_rotations().getZ());
        break;
    default: assert(false); break;
    }
}

vector<shared_ptr<SynchedEntityData::DataItem>>* SynchedEntityData::unpack(DataInputStream* input)
{
    vector<shared_ptr<DataItem>>* result = nullptr;
    int currentHeader = input->readByte();
    int itemCount = 0;
    const int MAX_ENTITY_DATA_ITEMS = 256;

    while (currentHeader != EOF_MARKER && itemCount < MAX_ENTITY_DATA_ITEMS)
    {
        if (!result) result = new vector<shared_ptr<DataItem>>();
        int itemType = (currentHeader & TYPE_MASK) >> TYPE_SHIFT;
        int itemId   = (currentHeader & MAX_ID_VALUE);
        shared_ptr<DataItem> item;

        switch (itemType)
        {
        case TYPE_BYTE:         item = std::make_shared<DataItem>(itemType, itemId, (byte)input->readByte()); break;
        case TYPE_SHORT:        item = std::make_shared<DataItem>(itemType, itemId, (short)input->readShort()); break;
        case TYPE_INT:          item = std::make_shared<DataItem>(itemType, itemId, (int)input->readInt()); break;
        case TYPE_FLOAT:        item = std::make_shared<DataItem>(itemType, itemId, (float)input->readFloat()); break;
        case TYPE_STRING:       item = std::make_shared<DataItem>(itemType, itemId, Packet::readUtf(input, MAX_STRING_DATA_LENGTH)); break;
        case TYPE_ITEMINSTANCE: item = std::make_shared<DataItem>(itemType, itemId, Packet::readItem(input)); break;
        case TYPE_ROTATIONS:
        {
            float rx = input->readFloat();
            float ry = input->readFloat();
            float rz = input->readFloat();
            item = std::make_shared<DataItem>(itemType, itemId, Rotations(rx, ry, rz));
            break;
        }
        default:
            app.DebugPrintf(" ------ garbage data, or early end of stream\n");
            delete result;
            return nullptr;
        }
        result->push_back(item);
        itemCount++;
        currentHeader = input->readByte();
    }
    return result;
}

void SynchedEntityData::assignValues(vector<shared_ptr<DataItem>>* items)
{
    for (auto& item : *items)
    {
        shared_ptr<DataItem> dest = itemsById[item->getId()];
        if (!dest) continue;
        switch (item->getType())
        {
        case TYPE_BYTE:         dest->setValue(item->getValue_byte());          break;
        case TYPE_SHORT:        dest->setValue(item->getValue_short());         break;
        case TYPE_INT:          dest->setValue(item->getValue_int());           break;
        case TYPE_FLOAT:        dest->setValue(item->getValue_float());         break;
        case TYPE_STRING:       dest->setValue(item->getValue_wstring());       break;
        case TYPE_ITEMINSTANCE: dest->setValue(item->getValue_itemInstance());  break;
        case TYPE_ROTATIONS:    dest->setValue(item->getValue_rotations());     break;
        default: assert(false); break;
        }
    }
    m_isDirty = true;
}

int SynchedEntityData::getSizeInBytes()
{
    int size = 1;
    for (int i = 0; i <= MAX_ID_VALUE; i++)
    {
        shared_ptr<DataItem> item = itemsById[i];
        if (!item) continue;
        size += 1;
        switch (item->getType())
        {
        case TYPE_BYTE:         size += 1; break;
        case TYPE_SHORT:        size += 2; break;
        case TYPE_INT:          size += 4; break;
        case TYPE_FLOAT:        size += 4; break;
        case TYPE_STRING:       size += (int)item->getValue_wstring().length() + 2; break;
        case TYPE_ITEMINSTANCE: size += 5; break;
        case TYPE_ROTATIONS:    size += 12; break;
        default: break;
        }
    }
    return size;
}



SynchedEntityData::DataItem::DataItem(int type, int id, int value)      : type(type), id(id) { value_int   = value; dirty = true; }
SynchedEntityData::DataItem::DataItem(int type, int id, byte value)     : type(type), id(id) { value_byte  = value; dirty = true; }
SynchedEntityData::DataItem::DataItem(int type, int id, short value)    : type(type), id(id) { value_short = value; dirty = true; }
SynchedEntityData::DataItem::DataItem(int type, int id, float value)    : type(type), id(id) { value_float = value; dirty = true; }
SynchedEntityData::DataItem::DataItem(int type, int id, const wstring& value) : type(type), id(id) { value_wstring = value; dirty = true; }
SynchedEntityData::DataItem::DataItem(int type, int id, shared_ptr<ItemInstance> value) : type(type), id(id) { value_itemInstance = value; dirty = true; }


SynchedEntityData::DataItem::DataItem(int type, int id, const Rotations& value)
    : type(type), id(id), value_rotations(value)
{
    value_int = 0; 
    dirty = true;
}

void SynchedEntityData::DataItem::setValue(const Rotations& value) { value_rotations = value; }
Rotations SynchedEntityData::DataItem::getValue_rotations()        { return value_rotations; }

int    SynchedEntityData::DataItem::getId()                        { return id; }
int    SynchedEntityData::DataItem::getType()                      { return type; }
bool   SynchedEntityData::DataItem::isDirty()                      { return dirty; }
void   SynchedEntityData::DataItem::setDirty(bool d)               { dirty = d; }

void   SynchedEntityData::DataItem::setValue(int v)                { value_int   = v; }
void   SynchedEntityData::DataItem::setValue(byte v)               { value_byte  = v; }
void   SynchedEntityData::DataItem::setValue(short v)              { value_short = v; }
void   SynchedEntityData::DataItem::setValue(float v)              { value_float = v; }
void   SynchedEntityData::DataItem::setValue(const wstring& v)     { value_wstring = v; }
void   SynchedEntityData::DataItem::setValue(shared_ptr<ItemInstance> v) { value_itemInstance = v; }

int    SynchedEntityData::DataItem::getValue_int()                 { return value_int; }
short  SynchedEntityData::DataItem::getValue_short()               { return value_short; }
float  SynchedEntityData::DataItem::getValue_float()               { return value_float; }
byte   SynchedEntityData::DataItem::getValue_byte()                { return value_byte; }
wstring SynchedEntityData::DataItem::getValue_wstring()            { return value_wstring; }
shared_ptr<ItemInstance> SynchedEntityData::DataItem::getValue_itemInstance() { return value_itemInstance; }