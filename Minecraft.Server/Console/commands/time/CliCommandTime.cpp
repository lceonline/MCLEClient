#include "stdafx.h"

#include "CliCommandTime.h"

#include "../../ServerCliEngine.h"
#include "../../ServerCliParser.h"
#include "../../../Common/StringUtils.h"
#include "../../../../Minecraft.World/GameCommandPacket.h"
#include "../../../../Minecraft.World/TimeCommand.h"

namespace ServerRuntime
{
	namespace
	{
		constexpr const char *kTimeUsage = "time <set|add> <day|night|value>";

		static void SuggestLiteral(const char *candidate,
			const ServerCliCompletionContext &context,
			std::vector<std::string> *out)
		{
			if (candidate == nullptr || out == nullptr)
			{
				return;
			}

			const std::string text(candidate);
			if (StringUtils::StartsWithIgnoreCase(text, context.prefix))
			{
				out->push_back(context.linePrefix + text);
			}
		}
	}

	const char *CliCommandTime::Name() const
	{
		return "time";
	}

	const char *CliCommandTime::Usage() const
	{
		return kTimeUsage;
	}

	const char *CliCommandTime::Description() const
	{
		return "Set or modify world time.";
	}

	bool CliCommandTime::Execute(const ServerCliParsedLine &line, ServerCliEngine *engine)
	{
		if (line.tokens.size() < 2)
		{
			engine->LogWarn(std::string("Usage: ") + kTimeUsage);
			return false;
		}

		std::string modeStr = StringUtils::ToLowerAscii(line.tokens[0]);

		if (modeStr != "set" && modeStr != "add")
		{
			engine->LogWarn(std::string("Usage: ") + kTimeUsage);
			return false;
		}

		std::string valueStr = StringUtils::ToLowerAscii(line.tokens[1]);

		std::wstring mode(modeStr.begin(), modeStr.end());
		std::wstring value(valueStr.begin(), valueStr.end());

		std::shared_ptr<GameCommandPacket> packet =
			TimeCommand::preparePacket(mode, value);

		if (packet == nullptr)
		{
			engine->LogError("Failed to build time command packet.");
			return false;
		}

		return engine->DispatchWorldCommand(packet->command, packet->data);
	}

	void CliCommandTime::Complete(
		const ServerCliCompletionContext &context,
		const ServerCliEngine *engine,
		std::vector<std::string> *out) const
	{
		(void)engine;

		if (context.currentTokenIndex == 1)
		{
			SuggestLiteral("set", context, out);
			SuggestLiteral("add", context, out);
		}
		else if (context.currentTokenIndex == 2)
		{
			SuggestLiteral("day", context, out);
			SuggestLiteral("night", context, out);
		}
	}
}