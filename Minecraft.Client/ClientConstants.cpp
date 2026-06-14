#include "stdafx.h"
#include "ClientConstants.h"
#include <windows.h>
#include <wininet.h>
#include <string>

#pragma comment(lib, "wininet.lib")

extern char g_LCENToken[512];

const wchar_t* ClientConstants::LCEN_HOST = L"https://network-server-7kuc.onrender.com";

const std::wstring ClientConstants::VERSION_STRING =
	L"Minecraft: Legacy Network Beta Build 2026.06.14-128 [DO NOT DISTRIBUTE]";

static std::wstring g_cachedStatus = L"LCEN: Establishing a connection";
static CRITICAL_SECTION g_statusLock;
static bool g_threadStarted = false;
static HANDLE g_threadHandle = nullptr;

DWORD WINAPI LCENStatusThread(LPVOID)
{
	while (true)
	{
		std::wstring newStatus = L"LCE Network offline";

		if (g_LCENToken[0] != '\0')
		{
			HINTERNET hNet = InternetOpenA("openLCE", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

			if (hNet)
			{
				char authHeader[600];
				_snprintf_s(authHeader, sizeof(authHeader), _TRUNCATE, "Authorization: Bearer %s", g_LCENToken);

				HINTERNET hReq = InternetOpenUrlA(
					hNet,
					"https://network-server-7kuc.onrender.com/auth/validate",
					authHeader,
					(DWORD)strlen(authHeader),
					INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
					0
				);

				if (hReq)
				{
					char buf[4096] = {};
					DWORD read = 0;
					InternetReadFile(hReq, buf, sizeof(buf) - 1, &read);

					if (read > 0)
					{
						if (strstr(buf, "\"Account banned\""))
						{
							newStatus = L"LCEN could not connect: Banned user profile";
						}
						else if (strstr(buf, "\"reason\":\""))
						{
							const char* r = strstr(buf, "\"reason\":\"") + 10;
							const char* end = strchr(r, '"');

							if (end)
							{
								char reason[128] = {};
								size_t len = min((size_t)(end - r), sizeof(reason) - 1);
								strncpy_s(reason, r, len);

								wchar_t wReason[128] = {};
								MultiByteToWideChar(CP_UTF8, 0, reason, -1, wReason, 128);

								newStatus = L"Banned dum dum: ";
								newStatus += wReason;
							}
						}
						else if (strstr(buf, "\"Invalid token\""))
						{
							newStatus = L"LCEN could not connect: Invalid Token";
						}
						else if (strstr(buf, "\"gamertag\":\""))
						{
							const char* g = strstr(buf, "\"gamertag\":\"") + 12;
							const char* end = strchr(g, '"');

							if (end)
							{
								char gamertag[64] = {};
								size_t len = min((size_t)(end - g), sizeof(gamertag) - 1);
								strncpy_s(gamertag, g, len);

								wchar_t wGamertag[64] = {};
								MultiByteToWideChar(CP_UTF8, 0, gamertag, -1, wGamertag, 64);

								newStatus = L"[Connected] ";
								newStatus += wGamertag;
							}
						}
						else
						{
							newStatus = L"LCE Network Authentication failed";
						}
					}

					InternetCloseHandle(hReq);
				}

				InternetCloseHandle(hNet);
			}
		}
		else
		{
			newStatus = L"Not connected with LCEN servers";
		}

		EnterCriticalSection(&g_statusLock);
		g_cachedStatus = newStatus;
		LeaveCriticalSection(&g_statusLock);

		Sleep(2000);
	}

	return 0;
}

static void InitLCENSystem()
{
	if (g_threadStarted)
		return;

	g_threadStarted = true;
	InitializeCriticalSection(&g_statusLock);

	g_threadHandle = CreateThread(nullptr, 0, LCENStatusThread, nullptr, 0, nullptr);
}

std::wstring ClientConstants::GetLCENString()
{
	InitLCENSystem();

	EnterCriticalSection(&g_statusLock);
	std::wstring copy = g_cachedStatus;
	LeaveCriticalSection(&g_statusLock);

	return copy;
}