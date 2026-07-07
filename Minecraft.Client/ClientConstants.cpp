#include "stdafx.h"
#include "ClientConstants.h"
#include <string>

const wchar_t* ClientConstants::LCEN_HOST = L"";

const std::wstring ClientConstants::VERSION_STRING =
    L"Minecraft: Legacy Network Beta Build 2026.06.14-128 [DO NOT DISTRIBUTE]";

std::wstring ClientConstants::GetLCENString()
{
    return L"";
}