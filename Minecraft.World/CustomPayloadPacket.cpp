#include "stdafx.h"
#include "InputOutputStream.h"
#include "PacketListener.h"
#include "BasicTypeContainers.h"
#include "CustomPayloadPacket.h"

// Mojang-defined custom packets
const wstring CustomPayloadPacket::CUSTOM_BOOK_PACKET = CreateVanillaPayloadKey(L"BEdit");
const wstring CustomPayloadPacket::CUSTOM_BOOK_SIGN_PACKET = CreateVanillaPayloadKey(L"BSign");
const wstring CustomPayloadPacket::TEXTURE_PACK_PACKET = CreateVanillaPayloadKey(L"TPack");
const wstring CustomPayloadPacket::TRADER_LIST_PACKET = CreateVanillaPayloadKey(L"TrList");
const wstring CustomPayloadPacket::TRADER_SELECTION_PACKET = CreateVanillaPayloadKey(L"TrSel");

// neoLegacy-defined custom packets
const wstring CustomPayloadPacket::UPDATE_RECIPE_REGISTRY = CreatePayloadKey(L"neo", L"UpdRReg");
const wstring CustomPayloadPacket::UPDATE_CREATIVE_REGISTRY = CreatePayloadKey(L"neo", L"UpdCReg");

//todo: figure out if we should replace the packets in the comment section with a custom payload identifier
//comment section start
const wstring CustomPayloadPacket::SET_ADVENTURE_COMMAND_PACKET = CreateVanillaPayloadKey(L"AdvCdm");
const wstring CustomPayloadPacket::SET_BEACON_PACKET = CreateVanillaPayloadKey(L"Beacon");
const wstring CustomPayloadPacket::SET_ITEM_NAME_PACKET = CreateVanillaPayloadKey(L"ItemName");

const wstring CustomPayloadPacket::CIPHER_KEY_CHANNEL = CreateVanillaPayloadKey(L"CKey");
const wstring CustomPayloadPacket::CIPHER_ACK_CHANNEL = CreateVanillaPayloadKey(L"CAck");
const wstring CustomPayloadPacket::CIPHER_ON_CHANNEL = CreateVanillaPayloadKey(L"COn");

const wstring CustomPayloadPacket::IDENTITY_TOKEN_ISSUE = CreateVanillaPayloadKey(L"CTIssue");
const wstring CustomPayloadPacket::IDENTITY_TOKEN_CHALLENGE = CreateVanillaPayloadKey(L"CTChallenge");
const wstring CustomPayloadPacket::IDENTITY_TOKEN_RESPONSE = CreateVanillaPayloadKey(L"CTResponse");

const wstring CustomPayloadPacket::FORK_HELLO_CHANNEL = CreateVanillaPayloadKey(L"ForkHello");
const wstring CustomPayloadPacket::FORK_PLAYER_LEAVE_CHANNEL = CreateVanillaPayloadKey(L"ForkPLeave");

const wstring CustomPayloadPacket::ENCHANTMENT_LIST_PACKET = CreateVanillaPayloadKey(L"EnchList");
//comment section end

//removed cause its now handled on the server side
// const wstring CustomPayloadPacket::QUICK_EQUIP_PACKET = CreateVanillaPayloadKey(L"QEquip");
// const wstring CustomPayloadPacket::QUICK_EQUIP_SERVER_PACKET = CreateVanillaPayloadKey(L"QEquipServer");

CustomPayloadPacket::CustomPayloadPacket()
	: length(0)
{
}

CustomPayloadPacket::CustomPayloadPacket(const wstring &identifier, byteArray data)
{
	this->identifier = identifier;
	this->data = data;
	this->length = 0;

	if (data.data != nullptr)
	{
		length = data.length;

		if (length > Short::MAX_VALUE)
		{
			app.DebugPrintf("Payload may not be larger than 32K\n");
#ifndef _CONTENT_PACKAGE
			DEBUG_BREAK();
#endif
			//throw new IllegalArgumentException("Payload may not be larger than 32k");
		}
	}
}

void CustomPayloadPacket::read(DataInputStream *dis)
{
	identifier = readUtf(dis, 20);
	length = dis->readShort();

	if (length > 0 && length <= Short::MAX_VALUE)
	{
		if(data.data != nullptr)
		{
			delete [] data.data;
		}
		data = byteArray(length);
		dis->readFully(data);
	}
}

void CustomPayloadPacket::write(DataOutputStream *dos)
{
	writeUtf(identifier, dos);
	dos->writeShort(static_cast<short>(length));
	if (data.data != nullptr)
	{
		dos->write(data);
	}
}

void CustomPayloadPacket::handle(PacketListener *listener)
{
	listener->handleCustomPayload( shared_from_this() );
}

int CustomPayloadPacket::getEstimatedSize()
{
	return 2 + identifier.length() * 2 + 2 + length;
}