
// formatter made it to be smaller, use a formatter to get back chunky code
#include "stdafx.h"

#include <dxgi1_4.h>

#include <assert.h>
#include <iostream>
#include <ShellScalingApi.h>
#include <shellapi.h>
#include "GameConfig/Minecraft.spa.h"
#include "../MinecraftServer.h"
#include "../LocalPlayer.h"
#include "../../Minecraft.World/ItemInstance.h"
#include "../../Minecraft.World/MapItem.h"
#include "../../Minecraft.World/Recipes.h"
#include "../../Minecraft.World/Recipy.h"
#include "../../Minecraft.World/Language.h"
#include "../../Minecraft.World/StringHelpers.h"
#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/Level.h"
#include "../../Minecraft.World/net.minecraft.world.level.tile.h"

#include "../ClientConnection.h"
#include "../Minecraft.h"
#include "../ChatScreen.h"
#include "KeyboardMouseInput.h"
#include "../User.h"
#include "../../Minecraft.World/Socket.h"
#include "../../Minecraft.World/ThreadName.h"
#include "../../Minecraft.Client/StatsCounter.h"
#include "../ConnectScreen.h"
#include "../../Minecraft.Client/Tesselator.h"
#include "../../Minecraft.Client/Options.h"
#include "../Gui.h"
#include "Sentient/SentientManager.h"
#include "../../Minecraft.World/IntCache.h"
#include "../Textures.h"
#include "../Settings.h"
#include "Resource.h"
#include "../../Minecraft.World/compression.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "Common/PostProcesser.h"
#include "../GameRenderer.h"
#include "Network/WinsockNetLayer.h"
#include "Windows64_Xuid.h"
#include "Common/UI/UI.h"
#include "stb_image_write.h"
#include "Windows64_Minecraft.h"
#ifndef MINECRAFT_SERVER_BUILD
#include "Windows64_Launcher.h"
#endif

class Renderer;
extern Renderer InternalRenderManager;

#include "Xbox/Resource.h"

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

#ifdef _MSC_VER
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

HINSTANCE hMyInst;
LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
char chGlobalText[256];
uint16_t ui16GlobalText[256];

#define THEME_NAME		"584111F70AAAAAAA"
#define THEME_FILESIZE	2797568
#define FIFTY_ONE_MB (1000000*51)

#define NUM_PROFILE_VALUES	5
#define NUM_PROFILE_SETTINGS 4
DWORD dwProfileSettingsA[NUM_PROFILE_VALUES]=
{
#ifdef _XBOX
	XPROFILE_OPTION_CONTROLLER_VIBRATION,
	XPROFILE_GAMER_YAXIS_INVERSION,
	XPROFILE_GAMER_CONTROL_SENSITIVITY,
	XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL,
	XPROFILE_TITLE_SPECIFIC1,
#else
	0,0,0,0,0
#endif
};

BOOL g_bWidescreen = TRUE;

int g_iScreenWidth = 1920;
int g_iScreenHeight = 1080;

int g_rScreenWidth = 1280;
int g_rScreenHeight = 720;
static bool f3ComboUsed = false;

float g_iAspectRatio = static_cast<float>(g_iScreenWidth) / g_iScreenHeight;
static bool g_bResizeReady = false;

char g_Win64Username[17] = { 0 };
char g_Win64Password[17] = { 0 };
char g_Win64SessionTicket[512] = { 0 };
wchar_t g_Win64UsernameW[17] = { 0 };

bool s_externalLauncher = false;
bool isOfflineMode = false;

bool Windows64Minecraft::IsOfflineMode() { return isOfflineMode; }
bool Windows64Minecraft::IsExternalLauncher() { return s_externalLauncher; }
std::string Windows64Minecraft::GetAuthenticationTicket() { return std::string(g_Win64SessionTicket); }

static bool FetchSessionInfo()
{
	std::vector<std::wstring> headers;
	std::string token = Windows64Minecraft::GetAuthenticationTicket();
	std::wstring wtoken(token.begin(), token.end());
	headers.push_back(L"Authorization:" + wtoken);
	headers.push_back(L"Content-Type: text/plain");

	HttpResponse response = WinsockNetLayer::DoWinHttpRequest(L"auth.mclegacyedition.xyz", L"/accountinfo", L"POST", "", headers);

	if (response.status == 0) return false;
	if (response.status != 200) return false;
	if (response.body.empty() || response.body[0] != '-') return false;

	std::string payload = response.body.substr(1);
	strncpy_s(g_Win64Username, sizeof(g_Win64Username), payload.c_str(), _TRUNCATE);
	MultiByteToWideChar(CP_UTF8, 0, g_Win64Username, -1, g_Win64UsernameW, sizeof(g_Win64UsernameW) / sizeof(wchar_t));
	return true;
}

static bool g_isFullscreen = false;
static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

using Win64LaunchOptions = Windows64Minecraft::Win64LaunchOptions;

static void CopyWideArgToAnsi(LPCWSTR source, char* dest, size_t destSize)
{
	if (destSize == 0) return;
	dest[0] = 0;
	if (source == nullptr) return;
	WideCharToMultiByte(CP_ACP, 0, source, -1, dest, static_cast<int>(destSize), nullptr, nullptr);
	dest[destSize - 1] = 0;
}

static void GetOptionsFilePath(char *out, size_t outSize)
{
	GetModuleFileNameA(nullptr, out, static_cast<DWORD>(outSize));
	char *p = strrchr(out, '\\');
	if (p) *(p + 1) = '\0';
	strncat_s(out, outSize, "options.txt", _TRUNCATE);
}

static void SaveFullscreenOption(bool fullscreen)
{
	char path[MAX_PATH];
	GetOptionsFilePath(path, sizeof(path));
	FILE *f = nullptr;
	if (fopen_s(&f, path, "w") == 0 && f)
	{
		fprintf(f, "fullscreen=%d\n", fullscreen ? 1 : 0);
		fclose(f);
	}
}

static bool LoadFullscreenOption()
{
	char path[MAX_PATH];
	GetOptionsFilePath(path, sizeof(path));
	FILE *f = nullptr;
	if (fopen_s(&f, path, "r") == 0 && f)
	{
		char line[256];
		while (fgets(line, sizeof(line), f))
		{
			int val = 0;
			if (sscanf_s(line, "fullscreen=%d", &val) == 1)
			{
				fclose(f);
				return val != 0;
			}
		}
		fclose(f);
	}
	return false;
}

static void ApplyScreenMode(int screenMode)
{
	switch (screenMode)
	{
	case 1: g_iScreenWidth = 1280; g_iScreenHeight = 720;  break;
	case 2: g_iScreenWidth = 640;  g_iScreenHeight = 480;  break;
	case 3: g_iScreenWidth = 720;  g_iScreenHeight = 408;  break;
	default: break;
	}
}

static Win64LaunchOptions ParseLaunchOptions()
{
	Win64LaunchOptions options = {};
	options.screenMode = 0;

	g_Win64MultiplayerJoin = false;
	g_Win64MultiplayerPort = WIN64_NET_DEFAULT_PORT;

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == nullptr) return options;

	if (argc > 1 && lstrlenW(argv[1]) == 1)
		if (argv[1][0] >= L'1' && argv[1][0] <= L'3')
			options.screenMode = argv[1][0] - L'0';

	for (int i = 1; i < argc; ++i)
	{
		if (_wcsicmp(argv[i], L"-fullscreen") == 0)
			options.fullscreen = true;
		else if (_wcsicmp(argv[i], L"-username") == 0 && (i + 1) < argc) {
			CopyWideArgToAnsi(argv[++i], g_Win64Username, sizeof(g_Win64Username));
			options.username = true;
		}
		else if (_wcsicmp(argv[i], L"-password") == 0 && (i + 1) < argc) {
			CopyWideArgToAnsi(argv[++i], g_Win64Password, sizeof(g_Win64Password));
			options.password = true;
		}
		else if (_wcsicmp(argv[i], L"-token") == 0 && (i + 1) < argc) {
			CopyWideArgToAnsi(argv[++i], g_Win64SessionTicket, sizeof(g_Win64SessionTicket));
			options.token = true;
		}
		else if (_wcsicmp(argv[i], L"--lcen-token") == 0 && (i + 1) < argc) {
			CopyWideArgToAnsi(argv[++i], g_Win64SessionTicket, sizeof(g_Win64SessionTicket));
			options.token = true;
		}
		else if (_wcsicmp(argv[i], L"-offline") == 0)
			isOfflineMode = true;
		else if (_wcsicmp(argv[i], L"-name") == 0 && (i + 1) < argc)
			CopyWideArgToAnsi(argv[++i], g_Win64Username, sizeof(g_Win64Username));
		else if (_wcsicmp(argv[i], L"-ip") == 0 && (i + 1) < argc) {
			char ipBuf[256];
			CopyWideArgToAnsi(argv[++i], ipBuf, sizeof(ipBuf));
			strncpy_s(g_Win64MultiplayerIP, sizeof(g_Win64MultiplayerIP), ipBuf, _TRUNCATE);
			g_Win64MultiplayerJoin = true;
		}
		else if (_wcsicmp(argv[i], L"-port") == 0 && (i + 1) < argc) {
			wchar_t* endPtr = nullptr;
			const long port = wcstol(argv[++i], &endPtr, 10);
			if (endPtr != argv[i] && *endPtr == 0 && port > 0 && port <= 65535)
				g_Win64MultiplayerPort = static_cast<int>(port);
		}
	}

	LocalFree(argv);
	return options;
}

void DefineActions(void)
{
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);

	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);

	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);
}

#if 0
HRESULT InitD3D( IDirect3DDevice9 **ppDevice, D3DPRESENT_PARAMETERS *pd3dPP ) { return S_OK; }
#endif

#ifdef MEMORY_TRACKING
void ResetMem();
void DumpMem();
void MemPixStuff();
#else
void MemSect(int sect) {}
#endif

HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
bool                    g_bVSync = false;
static bool             g_bPendingExclusiveFullscreen = false;
static bool             g_bPendingExclusiveFullscreenValue = false;

static bool TakeScreenshot(wstring& outFilename)
{
	if (!g_pSwapChain || !g_pd3dDevice || !g_pImmediateContext) return false;

	ID3D11Texture2D* pBackBuffer = nullptr;
	HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr)) return false;

	D3D11_TEXTURE2D_DESC desc;
	pBackBuffer->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	bool success = false;
	ID3D11Texture2D* pStaging = nullptr;
	hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &pStaging);
	if (SUCCEEDED(hr))
	{
		g_pImmediateContext->CopyResource(pStaging, pBackBuffer);

		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		wchar_t* lastSlash = wcsrchr(exePath, L'\\');
		if (lastSlash) *(lastSlash + 1) = L'\0';
		wstring screenshotDirPath = wstring(exePath) + L"screenshots";
		CreateDirectoryW(screenshotDirPath.c_str(), NULL);

		SYSTEMTIME st;
		GetLocalTime(&st);
		wchar_t filename[128];
		swprintf_s(filename, L"%04d-%02d-%02d_%02d.%02d.%02d.png", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		wstring screenshotPath = screenshotDirPath + L"\\" + filename;

		D3D11_MAPPED_SUBRESOURCE mapped;
		hr = g_pImmediateContext->Map(pStaging, 0, D3D11_MAP_READ, 0, &mapped);
		if (SUCCEEDED(hr))
		{
			unsigned char* rgba = new unsigned char[desc.Width * desc.Height * 4];
			for (UINT row = 0; row < desc.Height; row++)
			{
				unsigned char* src = (unsigned char*)mapped.pData + row * mapped.RowPitch;
				unsigned char* dst = rgba + row * desc.Width * 4;
				memcpy(dst, src, desc.Width * 4);
				for (UINT x = 0; x < desc.Width; x++) dst[x * 4 + 3] = 0xFF;
			}
			g_pImmediateContext->Unmap(pStaging, 0);
			string narrowPath(screenshotPath.begin(), screenshotPath.end());
			if (stbi_write_png(narrowPath.c_str(), desc.Width, desc.Height, 4, rgba, desc.Width * 4))
			{ outFilename = filename; success = true; }
			delete[] rgba;
		}
		pStaging->Release();
	}
	pBackBuffer->Release();
	return success;
}

ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11Texture2D*		g_pDepthStencilBuffer = nullptr;
static const float kClearColorWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const float kClearColorBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

static bool g_bDxgiExclusiveFullscreen = false;

static bool ResizeD3D(int newW, int newH);
static bool g_bInSizeMove = false;
static int  g_pendingResizeW = 0;
static int  g_pendingResizeH = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_EXIT: DestroyWindow(hWnd); break;
		default: return DefWindowProcW(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KILLFOCUS:
		g_KBMInput.ClearAllState();
		g_KBMInput.SetWindowFocused(false);
		if (g_KBMInput.IsMouseGrabbed()) g_KBMInput.SetMouseGrabbed(false);
		break;
	case WM_SETFOCUS:
		g_KBMInput.SetWindowFocused(true);
		break;
	case WM_CHAR:
		if (wParam >= 0x20 || wParam == 0x08 || wParam == 0x0D)
			g_KBMInput.OnChar(static_cast<wchar_t>(wParam));
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		int vk = static_cast<int>(wParam);
		if ((lParam & 0x40000000) && vk != VK_LEFT && vk != VK_RIGHT && vk != VK_BACK) break;
#ifdef _WINDOWS64
		const Minecraft* pm = Minecraft::GetInstance();
		ChatScreen* chat = pm && pm->screen ? dynamic_cast<ChatScreen*>(pm->screen) : nullptr;
		if (chat)
		{
			if (vk == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) { chat->handlePasteRequest(); break; }
			if ((vk == VK_UP || vk == VK_DOWN) && !(lParam & 0x40000000)) { if (vk == VK_UP) chat->handleHistoryUp(); else chat->handleHistoryDown(); break; }
			if (vk >= '1' && vk <= '9') break;
			if (vk == VK_SHIFT) break;
		}
#endif
		if (vk == VK_SHIFT) vk = (MapVirtualKey((lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
		else if (vk == VK_CONTROL) vk = (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
		else if (vk == VK_MENU) vk = (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
		g_KBMInput.OnKeyDown(vk);
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		int vk = static_cast<int>(wParam);
		if (vk == VK_SHIFT) vk = (MapVirtualKey((lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
		else if (vk == VK_CONTROL) vk = (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
		else if (vk == VK_MENU) vk = (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
		g_KBMInput.OnKeyUp(vk);
		break;
	}
	case WM_LBUTTONDOWN: g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_LEFT);   break;
	case WM_LBUTTONUP:   g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_LEFT);     break;
	case WM_RBUTTONDOWN: g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_RIGHT);  break;
	case WM_RBUTTONUP:   g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_RIGHT);    break;
	case WM_MBUTTONDOWN: g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_MIDDLE); break;
	case WM_MBUTTONUP:   g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_MIDDLE);   break;
	case WM_MOUSEMOVE:   g_KBMInput.OnMouseMove(LOWORD(lParam), HIWORD(lParam));         break;
	case WM_MOUSEWHEEL:  g_KBMInput.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));        break;
	case WM_INPUT:
	{
		UINT dwSize = 0;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
		if (dwSize > 0 && dwSize <= 256)
		{
			BYTE rawBuffer[256];
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
			{
				const RAWINPUT* raw = (RAWINPUT*)rawBuffer;
				if (raw->header.dwType == RIM_TYPEMOUSE)
					g_KBMInput.OnRawMouseDelta(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
			}
		}
		break;
	}
	case WM_ENTERSIZEMOVE:
		g_bInSizeMove = true;
		break;
	case WM_EXITSIZEMOVE:
		g_bInSizeMove = false;
		if (g_pendingResizeW > 0 && g_pendingResizeH > 0)
		{
			ResizeD3D(g_pendingResizeW, g_pendingResizeH);
			g_pendingResizeW = 0; g_pendingResizeH = 0;
		}
		break;
	case WM_SIZE:
	{
		int newW = LOWORD(lParam), newH = HIWORD(lParam);
		if (newW > 0 && newH > 0)
		{
			if (g_bInSizeMove) { g_pendingResizeW = newW; g_pendingResizeH = newH; }
			else ResizeD3D(newW, newH);
		}
		break;
	}
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;
	wcex.cbSize        = sizeof(WNDCLASSEXW);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = WndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = LoadIconW(hInstance, L"Minecraft");
	wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = L"Minecraft";
	wcex.lpszClassName = L"MinecraftClass";
	wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MINECRAFTWINDOWS));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance;
	RECT wr = {0, 0, g_rScreenWidth, g_rScreenHeight};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindowW(L"MinecraftClass", L"Minecraft: Legacy Console Edition", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, hInstance, nullptr);
	if (!g_hWnd) return FALSE;
	ShowWindow(g_hWnd, (nCmdShow != SW_HIDE) ? SW_SHOWMAXIMIZED : nCmdShow);
	UpdateWindow(g_hWnd);
	return TRUE;
}

void ClearGlobalText() { memset(chGlobalText,0,256); memset(ui16GlobalText,0,512); }

uint16_t *GetGlobalText()
{
	char* pchBuffer = (char*)ui16GlobalText;
	for(int i=0;i<256;i++) pchBuffer[i*2]=chGlobalText[i];
	return ui16GlobalText;
}

void SeedEditBox()
{
	DialogBox(hMyInst, MAKEINTRESOURCE(IDD_SEED), g_hWnd, reinterpret_cast<DLGPROC>(DlgProc));
}

LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG: return TRUE;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			GetDlgItemText(hWndDlg,IDC_EDIT,chGlobalText,256);
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

HRESULT InitDevice()
{
	HRESULT hr = S_OK;
	UINT width = g_rScreenWidth, height = g_rScreenHeight;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE };
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT i = 0; i < ARRAYSIZE(driverTypes); i++)
	{
		g_driverType = driverTypes[i];
		hr = D3D11CreateDeviceAndSwapChain(nullptr, g_driverType, nullptr, createDeviceFlags,
			featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &sd,
			&g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (HRESULT_SUCCEEDED(hr)) break;
	}
	if (FAILED(hr)) return hr;

	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) return hr;

	D3D11_TEXTURE2D_DESC descDepth; ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width; descDepth.Height = height; descDepth.MipLevels = 1; descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT; descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencilBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSView; ZeroMemory(&descDSView, sizeof(descDSView));
	descDSView.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &descDSView, &g_pDepthStencilView);
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width; vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f; vp.TopLeftX = 0; vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
	PostProcesser::GetInstance().Init();
	return S_OK;
}

void Render()
{
	const float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pSwapChain->Present(0, 0);
}

static bool ResizeD3D(int newW, int newH)
{
	if (newW <= 0 || newH <= 0 || !g_pSwapChain || !g_bResizeReady) return false;
	if (g_bDxgiExclusiveFullscreen) return false;

	int bbW = newW, bbH = newH;

	char* pRM = (char*)&InternalRenderManager;
	ID3D11RenderTargetView**   ppRM_RTV    = (ID3D11RenderTargetView**)(pRM + 0x28);
	ID3D11ShaderResourceView** ppRM_SRV    = (ID3D11ShaderResourceView**)(pRM + 0x50);
	ID3D11DepthStencilView**   ppRM_DSV    = (ID3D11DepthStencilView**)(pRM + 0x98);
	IDXGISwapChain**           ppRM_SC     = (IDXGISwapChain**)(pRM + 0x20);
	DWORD*                     pRM_BBWidth  = (DWORD*)(pRM + 0x5138);
	DWORD*                     pRM_BBHeight = (DWORD*)(pRM + 0x513C);
	ID3D11Device**             ppRM_Device  = (ID3D11Device**)(pRM + 0x10);

	if (*ppRM_Device != g_pd3dDevice || *ppRM_SC != g_pSwapChain)
	{
		app.DebugPrintf("[RESIZE] ERROR: RenderManager offset verification failed!\n");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC oldScDesc;
	g_pSwapChain->GetDesc(&oldScDesc);
	bool bbDimsValid = (*pRM_BBWidth == oldScDesc.BufferDesc.Width && *pRM_BBHeight == oldScDesc.BufferDesc.Height);

	RenderManager.Suspend();
	while (!RenderManager.Suspended()) { Sleep(1); }
	PostProcesser::GetInstance().Cleanup();
	g_pImmediateContext->ClearState();
	g_pImmediateContext->Flush();

	if (g_pRenderTargetView)  { g_pRenderTargetView->Release();  g_pRenderTargetView  = NULL; }
	if (g_pDepthStencilView)  { g_pDepthStencilView->Release();  g_pDepthStencilView  = NULL; }
	if (g_pDepthStencilBuffer){ g_pDepthStencilBuffer->Release(); g_pDepthStencilBuffer = NULL; }

	gdraw_D3D11_PreReset();

	IDXGISwapChain* pOldSwapChain = g_pSwapChain;
	bool success = false;
	HRESULT hr;

	IDXGIDevice* dxgiDevice = NULL; IDXGIAdapter* dxgiAdapter = NULL; IDXGIFactory* dxgiFactory = NULL;
	hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice); if (FAILED(hr)) goto postReset;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter); if (FAILED(hr)) { dxgiDevice->Release(); goto postReset; }
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory); dxgiAdapter->Release(); dxgiDevice->Release(); if (FAILED(hr)) goto postReset;

	{
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1; sd.BufferDesc.Width = bbW; sd.BufferDesc.Height = bbH;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 0; sd.BufferDesc.RefreshRate.Denominator = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		sd.OutputWindow = g_hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0; sd.Windowed = TRUE;

		IDXGISwapChain* pNewSwapChain = NULL;
		hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &pNewSwapChain);
		dxgiFactory->Release();
		if (FAILED(hr) || !pNewSwapChain) { app.DebugPrintf("[RESIZE] CreateSwapChain FAILED\n"); goto postReset; }
		pOldSwapChain->Release();
		g_pSwapChain = pNewSwapChain;
	}

	*ppRM_SC = g_pSwapChain;

	{
		ID3D11Texture2D* pBackBuffer = NULL;
		hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer); if (FAILED(hr)) goto postReset;
		hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView); if (FAILED(hr)) { pBackBuffer->Release(); goto postReset; }
		hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, ppRM_RTV); if (FAILED(hr)) { pBackBuffer->Release(); goto postReset; }
		D3D11_TEXTURE2D_DESC backDesc = {}; pBackBuffer->GetDesc(&backDesc); pBackBuffer->Release();
		D3D11_TEXTURE2D_DESC srvDesc = backDesc; srvDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		ID3D11Texture2D* srvTexture = NULL;
		hr = g_pd3dDevice->CreateTexture2D(&srvDesc, NULL, &srvTexture);
		if (SUCCEEDED(hr)) { hr = g_pd3dDevice->CreateShaderResourceView(srvTexture, NULL, ppRM_SRV); srvTexture->Release(); }
		if (FAILED(hr)) goto postReset;
	}

	{
		D3D11_TEXTURE2D_DESC descDepth = {};
		descDepth.Width = bbW; descDepth.Height = bbH; descDepth.MipLevels = 1; descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; descDepth.SampleDesc.Count = 1;
		descDepth.Usage = D3D11_USAGE_DEFAULT; descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencilBuffer); if (FAILED(hr)) goto postReset;
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSView = {};
		descDSView.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; descDSView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &descDSView, &g_pDepthStencilView); if (FAILED(hr)) goto postReset;
	}

	g_pDepthStencilView->AddRef();
	*ppRM_DSV = g_pDepthStencilView;
	if (bbDimsValid) { *pRM_BBWidth = (DWORD)bbW; *pRM_BBHeight = (DWORD)bbH; }

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	{ D3D11_VIEWPORT vp = {}; vp.Width=(FLOAT)bbW; vp.Height=(FLOAT)bbH; vp.MaxDepth=1.0f; g_pImmediateContext->RSSetViewports(1, &vp); }

	ui.updateRenderTargets(g_pRenderTargetView, g_pDepthStencilView);
	ui.updateScreenSize(bbW, bbH);
	g_rScreenWidth = bbW; g_rScreenHeight = bbH;
	success = true;

postReset:
	if (!success && g_pSwapChain)
	{
		DXGI_SWAP_CHAIN_DESC recoveryDesc; g_pSwapChain->GetDesc(&recoveryDesc);
		int recW = (int)recoveryDesc.BufferDesc.Width, recH = (int)recoveryDesc.BufferDesc.Height;
		if (!g_pRenderTargetView)
		{
			ID3D11Texture2D* pBB = NULL;
			if (SUCCEEDED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBB)))
			{ g_pd3dDevice->CreateRenderTargetView(pBB, NULL, &g_pRenderTargetView); pBB->Release(); }
		}
		if (!g_pDepthStencilView)
		{
			D3D11_TEXTURE2D_DESC dd = {}; dd.Width=recW; dd.Height=recH; dd.MipLevels=1; dd.ArraySize=1;
			dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; dd.SampleDesc.Count=1; dd.Usage=D3D11_USAGE_DEFAULT; dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
			if (!g_pDepthStencilBuffer) g_pd3dDevice->CreateTexture2D(&dd, NULL, &g_pDepthStencilBuffer);
			if (g_pDepthStencilBuffer) { D3D11_DEPTH_STENCIL_VIEW_DESC dsvd={}; dsvd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; dsvd.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D; g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvd, &g_pDepthStencilView); }
		}
		if (g_pRenderTargetView) g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
		ui.updateRenderTargets(g_pRenderTargetView, g_pDepthStencilView);
		if (g_pSwapChain != pOldSwapChain) { g_rScreenWidth=recW; g_rScreenHeight=recH; ui.updateScreenSize(recW, recH); }
		app.DebugPrintf("[RESIZE] FAILED but recovered at %dx%d\n", g_rScreenWidth, g_rScreenHeight);
	}

	gdraw_D3D11_PostReset();
	gdraw_D3D11_SetRendertargetSize(g_rScreenWidth, g_rScreenHeight);
	if (success) IggyFlushInstalledFonts();
	RenderManager.Resume();
	if (success) PostProcesser::GetInstance().Init();
	return success;
}

void ToggleFullscreen()
{
	const DWORD dwStyle = GetWindowLong(g_hWnd, GWL_STYLE);
	if (!g_isFullscreen)
	{
		MONITORINFO mi = { sizeof(mi) };
		if (GetWindowPlacement(g_hWnd, &g_wpPrev) && GetMonitorInfo(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLong(g_hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(g_hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(g_hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hWnd, &g_wpPrev);
		SetWindowPos(g_hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
	g_isFullscreen = !g_isFullscreen;
	SaveFullscreenOption(g_isFullscreen);
	if (g_KBMInput.IsWindowFocused()) g_KBMInput.SetWindowFocused(true);
}

void SetExclusiveFullscreen(bool enabled)
{
	if (enabled == g_isFullscreen) return;
	g_bPendingExclusiveFullscreen = true;
	g_bPendingExclusiveFullscreenValue = enabled;
}

static void ApplyExclusiveFullscreen(bool enabled)
{
	if (!g_pSwapChain) return;
	LONG styleBefore = GetWindowLong(g_hWnd, GWL_STYLE);

	if (enabled)
	{
		HMONITOR hMon = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(hMon, &mi))
		{
			int monW = mi.rcMonitor.right - mi.rcMonitor.left, monH = mi.rcMonitor.bottom - mi.rcMonitor.top;
			SetWindowLong(g_hWnd, GWL_STYLE, (styleBefore & ~WS_OVERLAPPEDWINDOW) | WS_VISIBLE);
			SetWindowPos(g_hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, monW, monH, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			DXGI_MODE_DESC tm = {}; tm.Width=monW; tm.Height=monH; tm.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
			tm.ScanlineOrdering=DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; tm.Scaling=DXGI_MODE_SCALING_UNSPECIFIED;
			g_pSwapChain->ResizeTarget(&tm);
		}
	}

	HRESULT hr = g_pSwapChain->SetFullscreenState(enabled ? TRUE : FALSE, nullptr);
	if (FAILED(hr)) return;
	g_bDxgiExclusiveFullscreen = enabled;

	if (enabled)
	{
		IDXGISwapChain3* pSC3 = nullptr;
		if (SUCCEEDED(g_pSwapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&pSC3)) && pSC3)
		{
			UINT support = 0;
			pSC3->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, &support);
			if (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
				pSC3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
			pSC3->Release();
		}
		HMONITOR hMon2 = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi2 = {}; mi2.cbSize = sizeof(mi2);
		if (GetMonitorInfo(hMon2, &mi2))
		{
			DXGI_MODE_DESC tm2 = {}; tm2.Width=mi2.rcMonitor.right-mi2.rcMonitor.left; tm2.Height=mi2.rcMonitor.bottom-mi2.rcMonitor.top;
			tm2.Format=DXGI_FORMAT_R8G8B8A8_UNORM; tm2.ScanlineOrdering=DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; tm2.Scaling=DXGI_MODE_SCALING_UNSPECIFIED;
			g_pSwapChain->ResizeTarget(&tm2);
		}
		g_isFullscreen = true;
	}
	else
	{
		SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		const int w=1280, h=720, sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
		SetWindowPos(g_hWnd, HWND_TOP, (sw-w)/2, (sh-h)/2, w, h, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		g_isFullscreen = false;
	}
}

void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pDepthStencilView)  g_pDepthStencilView->Release();
	if (g_pDepthStencilBuffer) g_pDepthStencilBuffer->Release();
	if (g_pRenderTargetView)  g_pRenderTargetView->Release();
	if (g_pSwapChain)         g_pSwapChain->Release();
	if (g_pImmediateContext)  g_pImmediateContext->Release();
	if (g_pd3dDevice)         g_pd3dDevice->Release();
}

static Minecraft* InitialiseMinecraftRuntime()
{
	app.loadMediaArchive();
	RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
	app.loadStringTable();
	ui.init(g_pd3dDevice, g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView, g_rScreenWidth, g_rScreenHeight);
	InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
	g_KBMInput.Init();
	DefineActions();
	InputManager.SetJoypadMapVal(0, 0);
	InputManager.SetKeyRepeatRate(0.3f, 0.2f);
	ProfileManager.Initialise(TITLEID_MINECRAFT, app.m_dwOfferID, PROFILE_VERSION_10,
		NUM_PROFILE_VALUES, NUM_PROFILE_SETTINGS, dwProfileSettingsA,
		app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT, &app.uiGameDefinedDataChangedBitmask);
	ProfileManager.SetDefaultOptionsCallback(&CConsoleMinecraftApp::DefaultOptionsCallback, (LPVOID)&app);
	g_NetworkManager.Initialise();
	for (int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; i++)
	{
		IQNet::m_player[i].m_smallId = static_cast<BYTE>(i);
		IQNet::m_player[i].m_isRemote = false;
		IQNet::m_player[i].m_isHostPlayer = (i == 0);
		swprintf_s(IQNet::m_player[i].m_gamertag, 32, L"Player%d", i);
	}
	wcscpy_s(IQNet::m_player[0].m_gamertag, 32, g_Win64UsernameW);
	WinsockNetLayer::Initialize();
	ProfileManager.SetDebugFullOverride(true);
	Tesselator::CreateNewThreadStorage(1024 * 1024);
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Compression::CreateNewThreadStorage();
	OldChunkStorage::CreateNewThreadStorage();
	Level::enableLightingCache();
	Tile::CreateNewThreadStorage();
	Minecraft::main();
	Minecraft* pMinecraft = Minecraft::GetInstance();
	if (!pMinecraft) return nullptr;
	app.InitGameSettings();
	app.InitialiseTips();
	return pMinecraft;
}

void StartGame(Win64LaunchOptions launchOptions, int nCmdShow);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	{
		char szExeDir[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, szExeDir, MAX_PATH);
		char* pSlash = strrchr(szExeDir, '\\');
		if (pSlash) { *(pSlash + 1) = '\0'; SetCurrentDirectoryA(szExeDir); }
	}

	SetProcessDPIAware();
	g_rScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
	g_rScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	Win64LaunchOptions launchOptions = ParseLaunchOptions();
	ApplyScreenMode(launchOptions.screenMode);

	hMyInst = hInstance;

	if (launchOptions.token)
	{
		s_externalLauncher = true;
		MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);
		FetchSessionInfo();
		Win64Xuid::ResolvePersistentXuid();
		if (g_Win64Username[0] == 0) strncpy_s(g_Win64Username, sizeof(g_Win64Username), "Player", _TRUNCATE);
		MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);
		StartGame(launchOptions, nCmdShow);
	}
	else if (launchOptions.username && launchOptions.password)
{
    std::string authToken;
#ifndef MINECRAFT_SERVER_BUILD
    int result = Windows64Launcher::API_AttemptAccountLogin(g_Win64Username, g_Win64Password, authToken);
    if (result != 0) return 0;
    const char* username = Windows64Launcher::GetUsername().c_str();
    strncpy_s(g_Win64Username, sizeof(g_Win64Username), username, _TRUNCATE);
    MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, ARRAYSIZE(g_Win64UsernameW));
#endif
    Win64Xuid::ResolvePersistentXuid();
    StartGame(launchOptions, nCmdShow);
}
	else if (isOfflineMode)
	{
		if (g_Win64Username[0] == 0) strncpy_s(g_Win64Username, sizeof(g_Win64Username), "Player", _TRUNCATE);
		MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);
		Win64Xuid::ResolvePersistentXuid();
		StartGame(launchOptions, nCmdShow);
	}
#ifdef MINECRAFT_SERVER_BUILD
    else
    {
        // Server build: no launcher UI, default to offline/headless
        if (g_Win64Username[0] == 0)
            strncpy_s(g_Win64Username, sizeof(g_Win64Username), "Player", _TRUNCATE);
        MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);
        isOfflineMode = true;
        Win64Xuid::ResolvePersistentXuid();
        StartGame(launchOptions, nCmdShow);
    }
#else
    else
    {
        Windows64Launcher::CreateLauncherWindow(hInstance, [launchOptions, nCmdShow]()
        {
            const char* username = Windows64Launcher::GetUsername().c_str();
            strncpy_s(g_Win64Username, sizeof(g_Win64Username), username, _TRUNCATE);
            MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);
            if (Windows64Launcher::IsInOfflineMode()) isOfflineMode = true;
            Win64Xuid::ResolvePersistentXuid();
            StartGame(launchOptions, nCmdShow);
        });
    }
#endif

	return 0;
}

void StartGame(Win64LaunchOptions launchOptions, int nCmdShow)
{
	if (g_Win64Username[0] == 0) return;

	MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);

	MyRegisterClass(hMyInst);
	if (!InitInstance(hMyInst, nCmdShow)) return;
	if (FAILED(InitDevice())) { CleanupDevice(); return; }

	if ((LoadFullscreenOption() && !g_isFullscreen) || launchOptions.fullscreen)
	{
		g_bPendingExclusiveFullscreen = true;
		g_bPendingExclusiveFullscreenValue = true;
	}

	static bool bTrialTimerDisplayed = true;

#ifdef MEMORY_TRACKING
	ResetMem();
	MEMORYSTATUS memStat; GlobalMemoryStatus(&memStat);
	printf("RESETMEM start: Avail. phys %d\n", memStat.dwAvailPhys/(1024*1024));
#endif

	Minecraft* pMinecraft = InitialiseMinecraftRuntime();
	if (!pMinecraft) { CleanupDevice(); return; }
	g_bResizeReady = true;

	MSG msg = {0};
	while (WM_QUIT != msg.message && !app.m_bShutdown)
	{
		g_KBMInput.Tick();

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg); DispatchMessage(&msg);
			if (msg.message == WM_QUIT) break;
		}
		if (msg.message == WM_QUIT) break;

		if (IsIconic(g_hWnd)) { Sleep(100); continue; }

		const float* clearColor = app.GetGameStarted() ? kClearColorBlack : kClearColorWhite;
		RenderManager.SetClearColour(clearColor);
		RenderManager.StartFrame();
		if (!app.GetGameStarted())
		{
			RenderManager.SetClearColour(kClearColorWhite);
			RenderManager.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		app.UpdateTime();
		PIXBeginNamedEvent(0, "Input manager tick");
		InputManager.Tick();

		if (InputManager.IsPadConnected(0))
		{
			const bool controllerUsed = InputManager.ButtonPressed(0) ||
				InputManager.GetJoypadStick_LX(0, false) != 0.0f || InputManager.GetJoypadStick_LY(0, false) != 0.0f ||
				InputManager.GetJoypadStick_RX(0, false) != 0.0f || InputManager.GetJoypadStick_RY(0, false) != 0.0f;
			if (controllerUsed) g_KBMInput.SetKBMActive(false);
			else if (g_KBMInput.HasAnyInput()) g_KBMInput.SetKBMActive(true);
		}
		else g_KBMInput.SetKBMActive(true);

		if (!g_KBMInput.IsMouseGrabbed())
		{
			if (!g_KBMInput.IsKBMActive()) g_KBMInput.SetCursorHiddenForUI(true);
			else if (!g_KBMInput.IsScreenCursorHidden()) g_KBMInput.SetCursorHiddenForUI(false);
		}

		PIXEndNamedEvent();
		PIXBeginNamedEvent(0, "Storage manager tick"); StorageManager.Tick(); PIXEndNamedEvent();
		PIXBeginNamedEvent(0, "Render manager tick");  RenderManager.Tick();  PIXEndNamedEvent();
		PIXBeginNamedEvent(0, "Network manager do work #1"); g_NetworkManager.DoWork(); PIXEndNamedEvent();

		if (app.GetGameStarted())
		{
			pMinecraft->applyFrameMouseLook();
			pMinecraft->run_middle();
			app.SetAppPaused(g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1 && ui.IsPauseMenuDisplayed(ProfileManager.GetPrimaryPad()));
		}
		else
		{
			MemSect(28);
			pMinecraft->soundEngine->tick(nullptr, 0.0f);
			MemSect(0);
			pMinecraft->textures->tick(true, false);
			IntCache::Reset();
			if (app.GetReallyChangingSessionType()) pMinecraft->tickAllConnections();
		}

		pMinecraft->soundEngine->playMusicTick();

#ifdef MEMORY_TRACKING
		static bool bResetMemTrack = false, bDumpMemTrack = false;
		MemPixStuff();
		if (bResetMemTrack) { ResetMem(); MEMORYSTATUS ms; GlobalMemoryStatus(&ms); printf("RESETMEM: %d\n",ms.dwAvailPhys/(1024*1024)); bResetMemTrack=false; }
		if (bDumpMemTrack)  { DumpMem();  MEMORYSTATUS ms; GlobalMemoryStatus(&ms); printf("DUMPMEM: %d\n",ms.dwAvailPhys/(1024*1024)); printf("Renderer used: %d\n",RenderManager.CBuffSize(-1)); bDumpMemTrack=false; }
#endif

		ui.tick();
		ui.render();
		pMinecraft->gameRenderer->ApplyGammaPostProcess();

		const bool forceVSyncForMenu = !app.GetGameStarted();
		if (!g_bVSync && !forceVSyncForMenu && g_pSwapChain)
		{
			HRESULT hrPresent = g_pSwapChain->Present(0, 0);
			if (FAILED(hrPresent)) RenderManager.Present();
		}
		else RenderManager.Present();

		ui.CheckMenuDisplayed();

		{
			static bool altToggleSuppressCapture = false;
			const bool shouldCapture = app.GetGameStarted() && !ui.GetMenuDisplayed(0) && pMinecraft->screen == nullptr;
			if (g_KBMInput.IsKeyPressed(VK_LMENU) || g_KBMInput.IsKeyPressed(VK_RMENU))
			{
				if (g_KBMInput.IsMouseGrabbed()) { g_KBMInput.SetMouseGrabbed(false); altToggleSuppressCapture = true; }
				else if (shouldCapture) { g_KBMInput.SetMouseGrabbed(true); altToggleSuppressCapture = false; }
			}
			else if (!shouldCapture) { if (g_KBMInput.IsMouseGrabbed()) g_KBMInput.SetMouseGrabbed(false); altToggleSuppressCapture = false; }
			else if (shouldCapture && !g_KBMInput.IsMouseGrabbed() && GetFocus() == g_hWnd && !altToggleSuppressCapture)
				g_KBMInput.SetMouseGrabbed(true);
		}

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_SCREENSHOT))
		{
			wstring filename;
			if (TakeScreenshot(filename) && pMinecraft->gui && pMinecraft->player)
				pMinecraft->gui->addMessage(L"Saved screenshot to " + filename, ProfileManager.GetPrimaryPad());
		}

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_TOGGLE_HUD))
		{
			const int pad = ProfileManager.GetPrimaryPad();
			const unsigned char v = app.GetGameSettings(pad, eGameSetting_DisplayHUD);
			app.SetGameSettings(pad, eGameSetting_DisplayHUD, v ? 0 : 1);
			app.SetGameSettings(pad, eGameSetting_DisplayHand, v ? 0 : 1);
		}

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_DEBUG_INFO)) f3ComboUsed = false;
		if (g_KBMInput.IsKeyDown(KeyboardMouseInput::KEY_DEBUG_INFO))
		{
			if (g_KBMInput.GetPressedKey() == 'H' && pMinecraft->options && app.GetGameStarted())
			{
				pMinecraft->options->advancedTooltips = !pMinecraft->options->advancedTooltips;
				pMinecraft->options->save();
				const wstring msg = wstring(L"Advanced tooltips: ") + (pMinecraft->options->advancedTooltips ? L"shown" : L"hidden");
				if (pMinecraft->gui) pMinecraft->gui->addMessage(msg, ProfileManager.GetPrimaryPad());
				f3ComboUsed = true;
			}
		}
		if (g_KBMInput.IsKeyReleased(KeyboardMouseInput::KEY_DEBUG_INFO) && !f3ComboUsed)
			if (pMinecraft->options) pMinecraft->options->renderDebug = !pMinecraft->options->renderDebug;

#ifdef _DEBUG_MENUS_ENABLED
		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_DEBUG_CONSOLE))
		{ static bool s_dc = false; s_dc = !s_dc; ui.ShowUIDebugConsole(s_dc); }
#endif

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_FULLSCREEN))
		{
			ApplyExclusiveFullscreen(!g_bDxgiExclusiveFullscreen);
			app.SetGameSettings(ProfileManager.GetPrimaryPad(), eGameSetting_ExclusiveFullscreen, g_bDxgiExclusiveFullscreen ? 1 : 0);
		}

		if (g_bPendingExclusiveFullscreen)
		{
			g_bPendingExclusiveFullscreen = false;
			ApplyExclusiveFullscreen(g_bPendingExclusiveFullscreenValue);
		}

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_HOST_SETTINGS) && !ui.GetMenuDisplayed(0))
			ui.NavigateToScene(0, eUIScene_InGameInfoMenu);

		if (g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_CHAT) && app.GetGameStarted() && !ui.GetMenuDisplayed(0) && pMinecraft->screen == NULL)
		{
			g_KBMInput.ClearCharBuffer();
			pMinecraft->setScreen(new ChatScreen());
			SetFocus(g_hWnd);
		}

		app.HandleXuiActions();

		if (!ProfileManager.IsFullVersion())
		{
			if (app.GetGameStarted())
			{
				if (app.IsAppPaused()) app.UpdateTrialPausedTimer();
				ui.UpdateTrialTimer(ProfileManager.GetPrimaryPad());
			}
		}
		else
		{
			if (bTrialTimerDisplayed) { ui.ShowTrialTimer(false); bTrialTimerDisplayed = false; }
		}

		Vec3::resetPool();
	}

	g_pd3dDevice->Release();
}

#ifdef MEMORY_TRACKING

int totalAllocGen = 0;
unordered_map<int,int> allocCounts;
bool trackEnable = false;
bool trackStarted = false;
volatile size_t sizeCheckMin = 1160;
volatile size_t sizeCheckMax = 1160;
volatile int sectCheck = 48;
CRITICAL_SECTION memCS;
DWORD tlsIdx;

LPVOID XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	if (!trackStarted) { void *p=XMemAllocDefault(dwSize,dwAllocAttributes); totalAllocGen+=XMemSizeDefault(p,dwAllocAttributes); return p; }
	EnterCriticalSection(&memCS);
	void *p=XMemAllocDefault(dwSize+16,dwAllocAttributes);
	size_t realSize=XMemSizeDefault(p,dwAllocAttributes)-16;
	if (trackEnable) {
		int sect=((int)TlsGetValue(tlsIdx))&0x3f;
		*(((unsigned char*)p)+realSize)=sect;
		if (p) { totalAllocGen+=realSize; trackEnable=false; int key=(sect<<26)|realSize; allocCounts[key]++; trackEnable=true; }
	}
	LeaveCriticalSection(&memCS);
	return p;
}
void* operator new(size_t size) { return (unsigned char*)XMemAlloc(size,MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP)); }
void operator delete(void *p) { XMemFree(p,MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP)); }
void WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	if (dwAllocAttributes==0) dwAllocAttributes=MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP);
	if (!trackStarted) { totalAllocGen-=XMemSizeDefault(pAddress,dwAllocAttributes); XMemFreeDefault(pAddress,dwAllocAttributes); return; }
	EnterCriticalSection(&memCS);
	if (pAddress) { size_t realSize=XMemSizeDefault(pAddress,dwAllocAttributes)-16; if (trackEnable) { int sect=*(((unsigned char*)pAddress)+realSize); totalAllocGen-=realSize; trackEnable=false; allocCounts[(sect<<26)|realSize]--; trackEnable=true; } XMemFreeDefault(pAddress,dwAllocAttributes); }
	LeaveCriticalSection(&memCS);
}
SIZE_T WINAPI XMemSize(PVOID pAddress, DWORD dwAllocAttributes) { return XMemSizeDefault(pAddress,dwAllocAttributes)-(trackStarted?16:0); }
void DumpMem() { int total=0; for (auto it=allocCounts.begin();it!=allocCounts.end();it++) if (it->second>0) { app.DebugPrintf("%d %d %d %d\n",(it->first>>26)&0x3f,it->first&0x03ffffff,it->second,(it->first&0x03ffffff)*it->second); total+=(it->first&0x03ffffff)*it->second; } app.DebugPrintf("Total %d\n",total); }
void ResetMem() { if (!trackStarted) { trackEnable=true; trackStarted=true; totalAllocGen=0; InitializeCriticalSection(&memCS); tlsIdx=TlsAlloc(); } EnterCriticalSection(&memCS); trackEnable=false; allocCounts.clear(); trackEnable=true; LeaveCriticalSection(&memCS); }
void MemSect(int section) { unsigned int value=(unsigned int)TlsGetValue(tlsIdx); if (section==0) value=(value>>6)&0x03ffffff; else value=(value<<6)|section; TlsSetValue(tlsIdx,(LPVOID)value); }
void MemPixStuff()
{
	const int MAX_SECT=46; int totals[MAX_SECT]={0};
	for (auto it=allocCounts.begin();it!=allocCounts.end();it++) if (it->second>0) { int sect=(it->first>>26)&0x3f; totals[sect]+=(it->first&0x03ffffff)*it->second; }
	unsigned int allTotal=0;
	for (int i=0;i<MAX_SECT;i++) { allTotal+=totals[i]; PIXAddNamedCounter(((float)totals[i])/1024.0f,"MemSect%d",i); }
	PIXAddNamedCounter(((float)allTotal)/4096.0f,"MemSect total pages");
}

#endif
