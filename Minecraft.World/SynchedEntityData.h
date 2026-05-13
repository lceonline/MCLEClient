#pragma once
#include "Rotations.h"
using namespace std;

class Pos;

class SynchedEntityData
{
public:
    class DataItem
    {
        friend class SynchedEntityData;
    private:
        const int type;
        const int id;
        union {
            byte    value_byte;
            int     value_int;
            short   value_short;
            float   value_float;
        };
        wstring value_wstring;
        shared_ptr<ItemInstance> value_itemInstance;
        bool dirty;

    public:
        Rotations value_rotations;

        DataItem(int type, int id, const Rotations& value);
        void setValue(const Rotations& value);
        Rotations getValue_rotations();

        DataItem(int type, int id, byte value);
        DataItem(int type, int id, int value);
        DataItem(int type, int id, const wstring& value);
        DataItem(int type, int id, shared_ptr<ItemInstance> itemInstance);
        DataItem(int type, int id, short value);
        DataItem(int type, int id, float value);

        int getId();
        void setValue(byte value);
        void setValue(int value);
        void setValue(short value);
        void setValue(float value);
        void setValue(const wstring& value);
        void setValue(shared_ptr<ItemInstance> value);
        byte getValue_byte();
        int getValue_int();
        short getValue_short();
        float getValue_float();
        wstring getValue_wstring();
        shared_ptr<ItemInstance> getValue_itemInstance();
        int getType();
        bool isDirty();
        void setDirty(bool dirty);
    };

public:
    static const int MAX_STRING_DATA_LENGTH = 64;
    static const int EOF_MARKER = 0x7f;

    static const int TYPE_BYTE          = 0;
    static const int TYPE_SHORT         = 1;
    static const int TYPE_INT           = 2;
    static const int TYPE_FLOAT         = 3;
    static const int TYPE_STRING        = 4;
    static const int TYPE_ITEMINSTANCE  = 5;
    static const int TYPE_POS           = 6;
    static const int TYPE_ROTATIONS     = 7;

private:
    bool m_isEmpty;

    static const int TYPE_MASK  = 0xe0;
    static const int TYPE_SHIFT = 5;
    static const int MAX_ID_VALUE = ~TYPE_MASK & 0xff;

    shared_ptr<DataItem> itemsById[MAX_ID_VALUE + 1];
    bool m_isDirty;

public:
    SynchedEntityData();

    void define(int id, byte value);
    void define(int id, const wstring& value);
    void define(int id, int value);
    void define(int id, short value);
    void define(int id, float value);
    void define(int id, const Rotations& value);
    void defineNULL(int id, void* pVal);

    void checkId(int id);
    byte getByte(int id);
    short getShort(int id);
    int getInteger(int id);
    float getFloat(int id);
    wstring getString(int id);
    shared_ptr<ItemInstance> getItemInstance(int id);
    Pos* getPos(int id);

    void set(int id, byte value);
    void set(int id, int value);
    void set(int id, short value);
    void set(int id, float value);
    void set(int id, const wstring& value);
    void set(int id, shared_ptr<ItemInstance>);
    void set(int id, const Rotations& value);

    Rotations getRotations(int id);

    void markDirty(int id);
    bool isDirty();
    static void pack(vector<shared_ptr<DataItem>>* items, DataOutputStream* output);
    vector<shared_ptr<DataItem>>* packDirty();
    void packAll(DataOutputStream* output);
    vector<shared_ptr<DataItem>>* getAll();

private:
    static void writeDataItem(DataOutputStream* output, shared_ptr<DataItem> dataItem);

public:
    static vector<shared_ptr<DataItem>>* unpack(DataInputStream* input);
    void assignValues(vector<shared_ptr<DataItem>>* items);
    bool isEmpty();
    void clearDirty();

    // 4J Added
    int getSizeInBytes();
};