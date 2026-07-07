#include "stdafx.h"
#include "net.minecraft.commands.common.h"
#include "net.minecraft.world.effect.h"
#include "../Minecraft.Client/MinecraftServer.h"
#include "../Minecraft.Client/ServerPlayer.h"
#include "../Minecraft.Client/PlayerList.h"
#include "SharedConstants.h"
#include "net.minecraft.network.packet.h"
#include "EffectCommand.h"

EGameCommand EffectCommand::getId()
{
    return eGameCommand_Effect;
}

int EffectCommand::getPermissionLevel()
{
    return LEVEL_GAMEMASTERS;
}

wstring EffectCommand::getUsage(CommandSender *source)
{
    return L"commands.effect.usage";
}

void EffectCommand::execute(shared_ptr<CommandSender> source, byteArray commandData)
{
    ByteArrayInputStream bais(commandData);
    DataInputStream dis(&bais);

    wstring targetName = dis.readUTF();
    int effectId       = dis.readInt();
    int duration       = dis.readInt();
    int amplifier      = dis.readInt();
    bool clear         = dis.readBoolean();

    shared_ptr<ServerPlayer> target =
        MinecraftServer::getInstance()->getPlayers()->getPlayer(targetName);
    if (target == nullptr)
        return;

    if (clear)
    {
        target->removeAllEffects();
        return;
    }

    if (effectId < 1 || effectId >= MobEffect::NUM_EFFECTS || MobEffect::effects[effectId] == nullptr)
        return;

    if (duration == 0)
    {
        if (target->hasEffect(effectId))
            target->removeEffect(effectId);
        return;
    }

    MobEffectInstance *instance = new MobEffectInstance(effectId, duration, amplifier);
    target->addEffect(instance);
}

shared_ptr<GameCommandPacket> EffectCommand::preparePacket(
    const wstring& targetName,
    int effectId,
    int duration,
    int amplifier,
    bool clear)
{
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);
    dos.writeUTF(targetName);
    dos.writeInt(effectId);
    dos.writeInt(duration);
    dos.writeInt(amplifier);
    dos.writeBoolean(clear);
    return std::make_shared<GameCommandPacket>(eGameCommand_Effect, baos.toByteArray());
}

wstring EffectCommand::getPlayerNames()
{
    return L"";
}

bool EffectCommand::isValidWildcardPlayerArgument(wstring args, int argumentIndex)
{
    return argumentIndex == 0;
}