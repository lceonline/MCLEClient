#include "NetworkHelpers.h"
#include <windows.h>
#include <wininet.h>
#include <string>
#include <vector>

#pragma comment(lib, "wininet.lib")

static void Debug(const std::string& msg)
{
    OutputDebugStringA((msg + "\n").c_str());
}

bool HttpPost(const char* url, const Json& payload)
{
    URL_COMPONENTSA uc = {};
    uc.dwStructSize = sizeof(uc);
    char host[256] = {}, path[1024] = {};
    uc.lpszHostName    = host; uc.dwHostNameLength = sizeof(host);
    uc.lpszUrlPath     = path; uc.dwUrlPathLength  = sizeof(path);

    if (!InternetCrackUrlA(url, 0, 0, &uc))
    {
        Debug("InternetCrackUrlA FAILED");
        return false;
    }

    HINTERNET hNet = InternetOpenA("MinecraftWinINet/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hNet) { Debug("InternetOpenA FAILED"); return false; }

    INTERNET_PORT port = uc.nPort ? uc.nPort : INTERNET_DEFAULT_HTTPS_PORT;
    HINTERNET hConn = InternetConnectA(hNet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn)
    {
        Debug("InternetConnectA FAILED");
        InternetCloseHandle(hNet);
        return false;
    }

    DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    HINTERNET hReq = HttpOpenRequestA(hConn, "POST", path, NULL, NULL, NULL, flags, 0);
    if (!hReq)
    {
        Debug("HttpOpenRequestA FAILED");
        InternetCloseHandle(hConn);
        InternetCloseHandle(hNet);
        return false;
    }

    std::string body    = payload.dump();
    const char* headers = "Content-Type: application/json\r\n";
    BOOL ok = HttpSendRequestA(hReq, headers, (DWORD)strlen(headers),
                               (LPVOID)body.c_str(), (DWORD)body.size());
    if (!ok)
    {
        Debug("HttpSendRequestA FAILED");
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hNet);
        return false;
    }

    char buffer[4096]; DWORD bytesRead = 0; std::string response;
    while (InternetReadFile(hReq, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        response.append(buffer, bytesRead);

    Debug("POST Response: " + response);
    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hNet);
    return true;
}

bool HttpGet(const char* url,
             std::function<void(const Json&)> onSuccess,
             std::function<void()> onError)
{
    HINTERNET hInternet = InternetOpenA(
        "MinecraftWinINet/1.0",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL, NULL, 0);

    if (!hInternet)
    {
        if (onError) onError();
        return false;
    }

    HINTERNET hConnect = InternetOpenUrlA(
        hInternet,
        url,
        NULL,
        0,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0);

    if (!hConnect)
    {
        if (onError) onError();
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    std::string response;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        response.append(buffer, bytesRead);

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (response.empty())
    {
        if (onError) onError();
        return false;
    }

    try
    {
        Json parsed = Json::parse(response);
        onSuccess(parsed);
        return true;
    }
    catch (...)
    {
        if (onError) onError();
        return false;
    }
}

