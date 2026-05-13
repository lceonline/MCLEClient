#pragma once

#ifdef _WINDOWS64

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <Windows.h>

namespace Win64Xuid
{

	inline PlayerUID DeriveXuidForPad(PlayerUID baseXuid, int iPad)
	{
		if (iPad == 0)
			return baseXuid;

		uint64_t a = (uint64_t)baseXuid + (uint64_t)iPad;
		a = (~a) + (a << 21);
		a = a ^ (a >> 24);
		a = (a + (a << 3)) + (a << 8);
		a = a ^ (a >> 14);
		a = (a + (a << 2)) + (a << 4);
		a = a ^ (a >> 28);
		a = a + (a << 31);

		a |= 0x8000000000000000ULL;
		if (a == (uint64_t)INVALID_XUID)
			a ^= 0x0100000000000001ULL;

		return (PlayerUID)a;
	}

	inline PlayerUID ResolvePersistentXuidFromName(const std::wstring& playerName)
	{
		const unsigned __int64 fnvOffset = 14695981039346656037ULL;
		const unsigned __int64 fnvPrime = 1099511628211ULL;
		unsigned __int64 hash = fnvOffset;

		for (size_t i = 0; i < playerName.length(); ++i)
		{
			unsigned short codeUnit = (unsigned short)playerName[i];
			hash ^= (unsigned __int64)(codeUnit & 0xFF);
			hash *= fnvPrime;
			hash ^= (unsigned __int64)((codeUnit >> 8) & 0xFF);
			hash *= fnvPrime;
		}

		// Namespace the hash away from legacy smallId-based values.
		hash ^= 0x9E3779B97F4A7C15ULL;
		hash |= 0x8000000000000000ULL;

		if (hash == (unsigned __int64)INVALID_XUID)
		{
			hash ^= 0x0100000000000001ULL;
		}

		return (PlayerUID)hash;
	}
}

#endif
