#include "stdafx.h"
#include "net.minecraft.commands.h"
#include "../Minecraft.Client/MinecraftServer.h"
#include "../Minecraft.Client/ServerLevel.h"
#include "../Minecraft.Client/PlayerList.h"
#include "net.minecraft.network.packet.h"
#include "TimeCommand.h"

EGameCommand TimeCommand::getId()
{
    return eGameCommand_Time;
}

int TimeCommand::getPermissionLevel()
{
    return LEVEL_GAMEMASTERS;
}

static bool parseTimeValue(const std::wstring& input, int* outValue)
{
    if (input == L"day")   { *outValue = 0;     return true; }
    if (input == L"night") { *outValue = 12500; return true; }
    try   { *outValue = std::stoi(input); return true; }
    catch (...) { return false; }
}

void TimeCommand::execute(shared_ptr<CommandSender> source, byteArray commandData)
{
    ByteArrayInputStream bais(commandData);
    DataInputStream dis(&bais);
    std::wstring mode     = dis.readUTF();
    std::wstring valueStr = dis.readUTF();

    if (mode == L"set")
    {
        int value = 0;
        if (!parseTimeValue(valueStr, &value)) return;
        doSetTime(source, value);
    }
    else if (mode == L"add")
    {
        int value = 0;
        if (!parseTimeValue(valueStr, &value)) return;
        doAddTime(source, value);
    }
}

void TimeCommand::doSetTime(shared_ptr<CommandSender> source, int value)
{
    for (int i = 0; i < MinecraftServer::getInstance()->levels.length; i++)
        MinecraftServer::getInstance()->levels[i]->setDayTime(value);
}

void TimeCommand::doAddTime(shared_ptr<CommandSender> source, int value)
{
    for (int i = 0; i < MinecraftServer::getInstance()->levels.length; i++)
    {
        ServerLevel* level = MinecraftServer::getInstance()->levels[i];
        level->setDayTime(level->getDayTime() + value);
    }
}

shared_ptr<GameCommandPacket> TimeCommand::preparePacket(
    const std::wstring& mode, const std::wstring& value)
{
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);
    dos.writeUTF(mode);
    dos.writeUTF(value);
    return std::make_shared<GameCommandPacket>(eGameCommand_Time, baos.toByteArray());
}