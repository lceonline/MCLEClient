#include "stdafx.h"

#include "Windows64_Launcher.h"
#include "Xbox/resource.h"
#include "../Minecraft.h"
#include "../MultiplayerLocalPlayer.h"
#include "Network/WinsockNetLayer.h"

#include <shlobj.h>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

#include "./ExtraLibs/discordsdk/discord.h"

#pragma comment(lib, "winhttp.lib")

float g_sleepPercentage = 100;
int g_autosaveInterval = 120;
bool g_doBoatBreak = true;

static const wchar_t* kAuthHost = L"auth.mclegacyedition.xyz";

static std::string g_username = "";
static std::string g_authToken = "";
std::string g_discordUserId = "";

static bool g_offlineMode = false;
static bool g_loadedDedicated = false;

discord::Core* g_discordCore = nullptr;
static bool g_startedCallbackThread = false;
static bool g_startedRPCThread = false;

HWND g_launcherHWND = NULL;
static bool g_shouldContinue = true;

static HWND hBottomBar;
static HWND hStatusText;
static HWND hBtnLaunch;
static HWND hBtnOffline;
static HWND hBtnLogin;
static HWND hBtnRegister;
static HWND hBtnLogout;
static HWND hBtnDiscord;
static HWND hBtnDiscordLogin;
static HWND hUsernameLabel;
static HWND hUsernameEdit;
static HWND hPasswordLabel;
static HWND hPasswordEdit;
static HWND hLoginUsernameLabel;

bool Windows64Launcher::IsInOfflineMode() { return g_offlineMode; }
const std::string& Windows64Launcher::GetAuthenticationToken() { return g_authToken; }
const std::string& Windows64Launcher::GetUsername() { return g_username; }

DWORD WINAPI DiscordCallbackThreadFunc(LPVOID lpParam)
{
    auto result = discord::Core::Create(1424889204110004316LL, DiscordCreateFlags_NoRequireDiscord, &g_discordCore);
    if (result != discord::Result::Ok || !g_discordCore)
        return 1;
    while (true)
    {
        g_discordCore->RunCallbacks();
        Sleep(100);
    }
    return 0;
}

DWORD WINAPI DiscordRPCThreadFunc(LPVOID lpParam)
{
    while (!g_discordCore)
        Sleep(100);

    discord::Activity activity{};
    activity.GetTimestamps().SetStart(time(0));
    static std::string detailsStr = "Playing as " + Windows64Launcher::GetUsername();
    activity.SetDetails(detailsStr.c_str());
    activity.GetAssets().SetLargeImage("MCLE");
    activity.GetAssets().SetLargeText("MCLE");
    g_discordCore->ActivityManager().UpdateActivity(activity, [](discord::Result r) {});

    while (true)
    {
        Minecraft* minecraft = Minecraft::GetInstance();
        if (minecraft && minecraft->player)
        {
            discord::Activity updatedActivity{};
            updatedActivity.SetDetails(detailsStr.c_str());
            switch (minecraft->player->dimension)
            {
            case -1:
                updatedActivity.GetAssets().SetSmallImage("nether");
                updatedActivity.GetAssets().SetSmallText("In The Nether");
                break;
            case 0:
                updatedActivity.GetAssets().SetSmallImage("overworld");
                updatedActivity.GetAssets().SetSmallText("In The Overworld");
                break;
            case 1:
                updatedActivity.GetAssets().SetSmallImage("end");
                updatedActivity.GetAssets().SetSmallText("In The End");
                break;
            }
            g_discordCore->ActivityManager().UpdateActivity(updatedActivity, [](discord::Result r) {});
        }
        Sleep(2000);
    }
    return 0;
}

static std::vector<std::string> SplitString(const std::string& str, char delim)
{
    std::vector<std::string> result;
    size_t start = 0, end = 0;
    while ((end = str.find(delim, start)) != std::string::npos)
    {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(str.substr(start));
    return result;
}

static bool IsValidUsername(const std::string& s)
{
    std::regex p("^[A-Za-z0-9_]+$");
    return std::regex_match(s, p);
}

static bool IsValidPassword(const std::string& s)
{
    std::regex p("^(?=.*[A-Za-z])(?!.*:).{8,}$");
    return std::regex_match(s, p);
}

static std::string GetLauncherFolder()
{
    char path[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path);
    std::string folder = path;
    folder += "\\LCENLauncher";
    CreateDirectoryA(folder.c_str(), NULL);
    return folder;
}

void Windows64Launcher::SaveAuthenticationData(const std::string& token, const std::string& username)
{
    if (g_loadedDedicated) return;
    std::string folder = GetLauncherFolder();
    std::string file = folder + "\\AccountToken_DoNotShare.dat";
    std::ofstream out(file, std::ios::trunc);
    out << token << "\n" << username;
    out.close();
}

bool Windows64Launcher::GetAuthenticationData(std::string& tokenOut, std::string& usernameOut, bool dedicated)
{
    std::ifstream in;

    if (dedicated)
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) *(lastSlash + 1) = L'\0';
        wchar_t filePath[MAX_PATH] = {};
        _snwprintf_s(filePath, MAX_PATH, _TRUNCATE, L"%sWindows64\\AccountToken_DoNotShare.dat", exePath);
        in.open(filePath);
        g_loadedDedicated = true;
    }

    if (!in.is_open())
    {
        std::string folder = GetLauncherFolder();
        in.open(folder + "\\AccountToken_DoNotShare.dat");
    }

    if (!in.is_open()) return false;

    std::getline(in, tokenOut);
    std::getline(in, usernameOut);
    in.close();

    if (tokenOut.empty()) return false;
    return true;
}

bool Windows64Launcher::GetAuthenticationDataAndLoad(bool dedicated)
{
    return Windows64Launcher::GetAuthenticationData(g_authToken, g_username, dedicated);
}

int Windows64Launcher::API_GetAccountInfo(const std::string token)
{
    std::vector<std::wstring> headers;
    std::wstring wtoken(token.begin(), token.end());
    headers.push_back(L"Authorization:" + wtoken);
    headers.push_back(L"Content-Type: text/plain");

    HttpResponse resp = WinsockNetLayer::DoWinHttpRequest(kAuthHost, L"/accountinfo", L"POST", "", headers);

    if (resp.status == 0) return -1;
    if (resp.status != 200) return 20000 + resp.status;
    if (resp.body.find('-') == std::string::npos) return std::stoi(resp.body);

    g_username = resp.body.substr(1);
    return 0;
}

int Windows64Launcher::API_AttemptAccountRegister(const std::string username, const std::string password, std::string& tokenOut)
{
    std::vector<std::wstring> headers;
    headers.push_back(L"Content-Type: text/plain");

    HttpResponse resp = WinsockNetLayer::DoWinHttpRequest(kAuthHost, L"/register", L"POST", username + ":" + password, headers);

    if (resp.status != 200) return 20000 + resp.status;
    if (resp.body.find('-') == std::string::npos) return std::stoi(resp.body);

    auto parts = SplitString(resp.body.substr(1), ':');
    if (parts.size() < 2) return -2;
    g_username = parts[0];
    g_authToken = parts[1];
    tokenOut = g_authToken;
    return 0;
}

int Windows64Launcher::API_AttemptAccountLogin(const std::string username, const std::string password, std::string& tokenOut)
{
    std::vector<std::wstring> headers;
    headers.push_back(L"Content-Type: text/plain");

    HttpResponse resp = WinsockNetLayer::DoWinHttpRequest(kAuthHost, L"/login", L"POST", username + ":" + password, headers);

    if (resp.status != 200) return 20000 + resp.status;
    if (resp.body.find('-') == std::string::npos) return std::stoi(resp.body);

    auto parts = SplitString(resp.body.substr(1), ':');
    if (parts.size() < 2) return -2;
    g_username = parts[0];
    g_authToken = parts[1];
    tokenOut = g_authToken;
    return 0;
}

int Windows64Launcher::API_AttemptDiscordLogin(const std::string& accessToken, std::string& tokenOut)
{
    std::vector<std::wstring> headers;
    headers.push_back(L"Content-Type: text/plain");

    HttpResponse resp = WinsockNetLayer::DoWinHttpRequest(kAuthHost, L"/discordlogin", L"POST", accessToken, headers);

    if (resp.status == 0) return -1;
    if (resp.status != 200) return 20000 + resp.status;
    if (resp.body.find('-') == std::string::npos) return std::stoi(resp.body);

    std::string body = resp.body.substr(1);
    size_t colon = body.find(':');
    if (colon == std::string::npos) return -2;

    g_username = body.substr(0, colon);
    g_authToken = body.substr(colon + 1);
    tokenOut = g_authToken;
    return 0;
}

static void ShowLoggedIn()
{
    SetWindowTextA(hLoginUsernameLabel, ("Welcome: " + g_username).c_str());
    ShowWindow(hBtnLaunch,          SW_SHOW);
    ShowWindow(hBtnOffline,         SW_SHOW);
    ShowWindow(hBtnLogout,          SW_SHOW);
    ShowWindow(hLoginUsernameLabel, SW_SHOW);
    ShowWindow(hUsernameLabel,      SW_HIDE);
    ShowWindow(hUsernameEdit,       SW_HIDE);
    ShowWindow(hPasswordLabel,      SW_HIDE);
    ShowWindow(hPasswordEdit,       SW_HIDE);
    ShowWindow(hBtnLogin,           SW_HIDE);
    ShowWindow(hBtnRegister,        SW_HIDE);
    ShowWindow(hBtnDiscordLogin,    SW_HIDE);

    if (!g_startedRPCThread)
    {
        HANDLE hThread = CreateThread(NULL, 0, DiscordRPCThreadFunc, NULL, 0, NULL);
        CloseHandle(hThread);
        g_startedRPCThread = true;
    }
}

static void ShowLoggedOut()
{
    ShowWindow(hBtnLaunch,          SW_HIDE);
    ShowWindow(hBtnOffline,         SW_HIDE);
    ShowWindow(hBtnLogout,          SW_HIDE);
    ShowWindow(hLoginUsernameLabel, SW_HIDE);
    ShowWindow(hUsernameLabel,      SW_SHOW);
    ShowWindow(hUsernameEdit,       SW_SHOW);
    ShowWindow(hPasswordLabel,      SW_SHOW);
    ShowWindow(hPasswordEdit,       SW_SHOW);
    ShowWindow(hBtnLogin,           SW_SHOW);
    ShowWindow(hBtnRegister,        SW_SHOW);
    ShowWindow(hBtnDiscordLogin,    SW_SHOW);
}

static void AttemptAutoLogin()
{
    std::string token, username;
    if (!Windows64Launcher::GetAuthenticationData(token, username))
    {
        ShowLoggedOut();
        return;
    }
    int result = Windows64Launcher::API_GetAccountInfo(token);
    if (result == 0)
    {
        g_authToken = token;
        ShowLoggedIn();
    }
    else
    {
        ShowLoggedOut();
        MessageBoxW(g_launcherHWND, L"Could not connect to saved account.", L"Login", MB_OK);
    }
}

static void DoRegister()
{
    char ubuf[32] = {}, pbuf[64] = {};
    GetWindowTextA(hUsernameEdit, ubuf, sizeof(ubuf));
    GetWindowTextA(hPasswordEdit, pbuf, sizeof(pbuf));

    if (!IsValidUsername(ubuf))
    { MessageBoxW(g_launcherHWND, L"Username: letters, numbers and underscores only.", L"Register", MB_OK); return; }
    if (!IsValidPassword(pbuf))
    { MessageBoxW(g_launcherHWND, L"Password: letters, numbers and %$#@&_ only.", L"Register", MB_OK); return; }

    std::string tokenOut;
    int r = Windows64Launcher::API_AttemptAccountRegister(ubuf, pbuf, tokenOut);
    if (r == 0)
    {
        Windows64Launcher::SaveAuthenticationData(g_authToken, g_username);
        AttemptAutoLogin();
    }
    else if (r == 3333) MessageBoxW(g_launcherHWND, L"Username already taken.",          L"Register", MB_OK);
    else if (r == 7777) MessageBoxW(g_launcherHWND, L"Username contains a banned word.", L"Register", MB_OK);
    else if (r == 8888) MessageBoxW(g_launcherHWND, L"That username is banned.",          L"Register", MB_OK);
    else if (r == 5555) MessageBoxW(g_launcherHWND, L"VPNs are not allowed.",            L"Register", MB_OK);
    else if (r == 6666) MessageBoxW(g_launcherHWND, L"Too many accounts from this IP.",  L"Register", MB_OK);
    else MessageBoxW(g_launcherHWND, (L"Registration failed: " + std::to_wstring(r)).c_str(), L"Register", MB_OK);
}

static void DoLogin()
{
    char ubuf[32] = {}, pbuf[64] = {};
    GetWindowTextA(hUsernameEdit, ubuf, sizeof(ubuf));
    GetWindowTextA(hPasswordEdit, pbuf, sizeof(pbuf));

    if (!IsValidUsername(ubuf))
    { MessageBoxW(g_launcherHWND, L"Username: letters, numbers and underscores only.", L"Login", MB_OK); return; }
    if (!IsValidPassword(pbuf))
    { MessageBoxW(g_launcherHWND, L"Password: letters, numbers and %$#@&_ only.", L"Login", MB_OK); return; }

    std::string tokenOut;
    int r = Windows64Launcher::API_AttemptAccountLogin(ubuf, pbuf, tokenOut);
    if (r == 0)
    {
        Windows64Launcher::SaveAuthenticationData(g_authToken, g_username);
        AttemptAutoLogin();
    }
    else if (r == 1111 || r == 2222) MessageBoxW(g_launcherHWND, L"Invalid username or password.", L"Login", MB_OK);
    else if (r == 2)                 MessageBoxW(g_launcherHWND, L"You are banned.",               L"Login", MB_OK);
    else MessageBoxW(g_launcherHWND, (L"Login failed: " + std::to_wstring(r)).c_str(), L"Login", MB_OK);
}

static void DoLogout()
{
    Windows64Launcher::SaveAuthenticationData("", g_username);
    g_authToken = "";
    ShowLoggedOut();
}

static void DoDiscordLogin()
{
    if (!g_discordCore)
    {
        MessageBoxW(g_launcherHWND, L"Discord is not running.", L"Discord Login", MB_OK);
        return;
    }
    g_discordCore->ApplicationManager().GetOAuth2Token([](discord::Result res, discord::OAuth2Token const& token)
    {
        if (res != discord::Result::Ok)
        {
            PostMessageA(g_launcherHWND, WM_APP + 2, 0, 0);
            return;
        }
        std::string t = token.GetAccessToken();
        PostMessageA(g_launcherHWND, WM_APP + 1, 0, (LPARAM)new std::string(t));
    });
}

static LRESULT CALLBACK LauncherWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        if (!g_startedCallbackThread)
        {
            HANDLE hThread = CreateThread(NULL, 0, DiscordCallbackThreadFunc, NULL, 0, NULL);
            CloseHandle(hThread);
            g_startedCallbackThread = true;
        }

        hBottomBar      = CreateWindowW(L"STATIC", nullptr,          WS_CHILD | WS_VISIBLE, 0,0,0,0, hWnd, nullptr, nullptr, nullptr);
        hStatusText     = CreateWindowW(L"STATIC", L"Launcher v" L"" LAUNCHER_VERSION, WS_CHILD | WS_VISIBLE, 10,10,300,20, hBottomBar, nullptr, nullptr, nullptr);

        hBtnLaunch      = CreateWindowW(L"BUTTON", L"Launch",        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0,0,80,25, hWnd, (HMENU)1, nullptr, nullptr);
        hBtnOffline     = CreateWindowW(L"BUTTON", L"Play Offline",  WS_CHILD | WS_VISIBLE, 0,0,80,25, hWnd, (HMENU)2, nullptr, nullptr);
        hBtnRegister    = CreateWindowW(L"BUTTON", L"Register",      WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0,0,80,25, hWnd, (HMENU)3, nullptr, nullptr);
        hBtnLogin       = CreateWindowW(L"BUTTON", L"Login",         WS_CHILD | WS_VISIBLE, 0,0,80,25, hWnd, (HMENU)4, nullptr, nullptr);
        hBtnLogout      = CreateWindowW(L"BUTTON", L"Logout",        WS_CHILD | WS_VISIBLE, 0,0,80,25, hWnd, (HMENU)5, nullptr, nullptr);
        hBtnDiscord     = CreateWindowW(L"BUTTON", L"Discord",       WS_CHILD | WS_VISIBLE, 0,0,80,25, hWnd, (HMENU)6, nullptr, nullptr);
        hBtnDiscordLogin = CreateWindowW(L"BUTTON", L"Discord Login", WS_CHILD | WS_VISIBLE, 0,0,95,25, hWnd, (HMENU)7, nullptr, nullptr);

        hUsernameLabel  = CreateWindowW(L"STATIC", L"Username:",     WS_CHILD | WS_VISIBLE, 0,0,80,20, hWnd, nullptr, nullptr, nullptr);
        hUsernameEdit   = CreateWindowW(L"EDIT",   L"",              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0,0,180,25, hWnd, nullptr, nullptr, nullptr);
        hPasswordLabel  = CreateWindowW(L"STATIC", L"Password:",     WS_CHILD | WS_VISIBLE, 0,0,80,20, hWnd, nullptr, nullptr, nullptr);
        hPasswordEdit   = CreateWindowW(L"EDIT",   L"",              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD, 0,0,180,25, hWnd, nullptr, nullptr, nullptr);
        hLoginUsernameLabel = CreateWindowW(L"STATIC", L"Welcome:",  WS_CHILD | WS_VISIBLE, 0,0,300,20, hWnd, nullptr, nullptr, nullptr);

        AttemptAutoLogin();
        return 0;
    }
    case WM_SIZE:
    {
        int W = LOWORD(lParam), H = HIWORD(lParam);
        int inputW = 180, labelW = 80, barH = 50;
        int cx = W / 2 - inputW / 2;
        int startY = 40;

        MoveWindow(hLoginUsernameLabel, 0, 0, 300, 20, TRUE);
        MoveWindow(hBtnLogout,  0, 30, labelW, 20, TRUE);

        MoveWindow(hBtnDiscord,      cx - 5,      125, 75, 20, TRUE);
        MoveWindow(hBtnDiscordLogin, cx + 75,     125, 95, 20, TRUE);

        MoveWindow(hUsernameLabel, cx - labelW + 50, startY - 5,  labelW, 20, TRUE);
        MoveWindow(hUsernameEdit,  cx + 50,           startY - 8,  inputW, 25, TRUE);
        MoveWindow(hPasswordLabel, cx - labelW + 50, startY + 40, labelW, 20, TRUE);
        MoveWindow(hPasswordEdit,  cx + 50,           startY + 37, inputW, 25, TRUE);

        MoveWindow(hBottomBar, 0, H - barH, W, barH, TRUE);
        int bY = H - barH + (barH - 25) / 2;
        MoveWindow(hBtnLaunch,   W - 10 - 80,     bY, 80, 25, TRUE);
        MoveWindow(hBtnLogin,    W - 10 - 80,     bY, 80, 25, TRUE);
        MoveWindow(hBtnRegister, W - 10*2 - 80*2, bY, 80, 25, TRUE);
        MoveWindow(hBtnOffline,  W - 10*2 - 80*2, bY, 80, 25, TRUE);
        MoveWindow(hStatusText,  10, (barH - 20) / 2, 300, 20, TRUE);
        return 0;
    }
    case WM_APP + 1:
    {
        std::string* token = reinterpret_cast<std::string*>(lParam);
        int r = Windows64Launcher::API_AttemptDiscordLogin(*token, g_authToken);
        delete token;

        if (r == 0)
        {
            Windows64Launcher::SaveAuthenticationData(g_authToken, g_username);
            AttemptAutoLogin();
        }
        else if (r == 1111) MessageBoxW(g_launcherHWND, L"Invalid username or password.",         L"Discord Login", MB_OK);
        else if (r == 2222) MessageBoxW(g_launcherHWND, L"You are banned.",                       L"Discord Login", MB_OK);
        else if (r == 3333) MessageBoxW(g_launcherHWND, L"Invalid OAuth token.",                  L"Discord Login", MB_OK);
        else if (r == 4444) MessageBoxW(g_launcherHWND, L"Discord isn't linked to any account.", L"Discord Login", MB_OK);
        else MessageBoxW(g_launcherHWND, (L"Unknown Error: " + std::to_wstring(r)).c_str(), L"Discord Login", MB_OK);
        return 0;
    }
    case WM_APP + 2:
        MessageBoxW(hWnd, L"Failed to get Discord token.\nMake sure Discord is open.", L"Discord Login", MB_OK);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: g_shouldContinue = true;  g_offlineMode = false; DestroyWindow(hWnd); break;
        case 2: g_shouldContinue = true;  g_offlineMode = true;  DestroyWindow(hWnd); break;
        case 3: DoRegister();    break;
        case 4: DoLogin();       break;
        case 5: DoLogout();      break;
        case 6: ShellExecuteA(0, 0, "https://discord.gg/mclegacyedition", 0, 0, SW_SHOW); break;
        case 7: DoDiscordLogin(); break;
        }
        return 0;
    case WM_CLOSE:
        g_shouldContinue = false;
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void Windows64Launcher::CreateLauncherWindow(HINSTANCE hInstance, std::function<void()> onLaunch)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = LauncherWndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIconW(hInstance, L"Minecraft");
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"LCENLauncherClass";
    wcex.hIconSm       = LoadIconW(hInstance, L"Minecraft");
    RegisterClassExW(&wcex);

    RECT wr = {0, 0, 320, 220};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    g_launcherHWND = CreateWindowW(L"LCENLauncherClass", L"LCEN Launcher",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_launcherHWND, SW_SHOW);
    UpdateWindow(g_launcherHWND);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_shouldContinue && onLaunch)
        onLaunch();
}