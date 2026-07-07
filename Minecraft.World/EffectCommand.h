#pragma once
#include "Command.h"

class EffectCommand : public Command
{
public:
    virtual EGameCommand getId();
    virtual int getPermissionLevel();
    virtual wstring getUsage(CommandSender *source);
    virtual void execute(shared_ptr<CommandSender> source, byteArray commandData);
    static shared_ptr<GameCommandPacket> preparePacket(
        const wstring& targetName,
        int effectId,
        int duration,
        int amplifier,
        bool clear = false);
protected:
    wstring getPlayerNames();
    bool isValidWildcardPlayerArgument(wstring args, int argumentIndex);
};