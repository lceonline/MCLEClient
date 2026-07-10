// formatted to be smaller, just use a formatter to get back the big chunky code
#include "stdafx.h"

#ifdef _WINDOWS64

#include "WinsockNetLayer.h"
#include "../../Common/Network/PlatformNetworkManagerStub.h"
#include "../../../Minecraft.World/Socket.h"
#if defined(MINECRAFT_SERVER_BUILD)
#include "../../../Minecraft.Server/Access/Access.h"
#include "../../../Minecraft.Server/ServerLogManager.h"
#include "../../../Minecraft.Server/ServerLogger.h"
#include "../../../Minecraft.Server/Security/SecurityConfig.h"
#include "../../../Minecraft.Server/Security/RateLimiter.h"
#include "../../../Minecraft.Server/Security/ConnectionCipher.h"
#else
#include "../Windows64_Launcher.h"
#include "../Windows64_Minecraft.h"
#endif
#include "../../../Minecraft.World/DisconnectPacket.h"
#include "../../Minecraft.h"
#include <windns.h>
#pragma comment(lib, "dnsapi.lib")
#include "../4JLibs/inc/4J_Profile.h"

#include "json.hpp"
#include <string>

static bool RecvExact(SOCKET sock, BYTE* buf, int len);
#if defined(MINECRAFT_SERVER_BUILD)
static bool TryGetNumericRemoteIp(const sockaddr_in& remoteAddress, std::string* outIp);
#endif

static const BYTE kCipherAckPattern[] = {
    0xFA,
    0x00, 0x07,
    0x00,0x4D,0x00,0x43,0x00,0x7C,0x00,0x43,0x00,0x41,0x00,0x63,0x00,0x6B,
    0x00, 0x00
};
static const int kCipherAckPatternSize = sizeof(kCipherAckPattern);

static const BYTE kCipherOnPattern[] = {
    0xFA,
    0x00, 0x06,
    0x00,0x4D,0x00,0x43,0x00,0x7C,0x00,0x43,0x00,0x4F,0x00,0x6E,
    0x00, 0x00
};
static const int kCipherOnPatternSize = sizeof(kCipherOnPattern);

bool g_Win64LCENServer = false;

SOCKET WinsockNetLayer::s_listenSocket         = INVALID_SOCKET;
SOCKET WinsockNetLayer::s_hostConnectionSocket = INVALID_SOCKET;
HANDLE WinsockNetLayer::s_acceptThread         = nullptr;
HANDLE WinsockNetLayer::s_clientRecvThread     = nullptr;

bool WinsockNetLayer::s_isHost      = false;
bool WinsockNetLayer::s_connected   = false;
bool WinsockNetLayer::s_active      = false;
bool WinsockNetLayer::s_initialized = false;

BYTE         WinsockNetLayer::s_localSmallId = 0;
BYTE         WinsockNetLayer::s_hostSmallId  = 0;
unsigned int WinsockNetLayer::s_nextSmallId  = XUSER_MAX_COUNT;

CRITICAL_SECTION WinsockNetLayer::s_sendLock;
CRITICAL_SECTION WinsockNetLayer::s_connectionsLock;
std::vector<Win64RemoteConnection> WinsockNetLayer::s_connections;

SOCKET           WinsockNetLayer::s_advertiseSock   = INVALID_SOCKET;
HANDLE           WinsockNetLayer::s_advertiseThread = nullptr;
volatile bool    WinsockNetLayer::s_advertising     = false;
Win64LANBroadcast WinsockNetLayer::s_advertiseData  = {};
CRITICAL_SECTION WinsockNetLayer::s_advertiseLock;
int              WinsockNetLayer::s_hostGamePort     = WIN64_NET_DEFAULT_PORT;

SOCKET           WinsockNetLayer::s_discoverySock   = INVALID_SOCKET;
HANDLE           WinsockNetLayer::s_discoveryThread = nullptr;
volatile bool    WinsockNetLayer::s_discovering     = false;
CRITICAL_SECTION WinsockNetLayer::s_discoveryLock;
std::vector<Win64LANSession> WinsockNetLayer::s_discoveredSessions;

CRITICAL_SECTION     WinsockNetLayer::s_disconnectLock;
std::vector<BYTE>    WinsockNetLayer::s_disconnectedSmallIds;

CRITICAL_SECTION     WinsockNetLayer::s_freeSmallIdLock;
std::vector<BYTE>    WinsockNetLayer::s_freeSmallIds;
SOCKET               WinsockNetLayer::s_smallIdToSocket[256];
CRITICAL_SECTION     WinsockNetLayer::s_smallIdToSocketLock;

SOCKET WinsockNetLayer::s_splitScreenSocket[XUSER_MAX_COUNT]      = { INVALID_SOCKET,INVALID_SOCKET,INVALID_SOCKET,INVALID_SOCKET };
BYTE   WinsockNetLayer::s_splitScreenSmallId[XUSER_MAX_COUNT]     = { 0xFF,0xFF,0xFF,0xFF };
HANDLE WinsockNetLayer::s_splitScreenRecvThread[XUSER_MAX_COUNT]  = { nullptr,nullptr,nullptr,nullptr };

HANDLE                              WinsockNetLayer::s_joinThread          = nullptr;
volatile WinsockNetLayer::eJoinState WinsockNetLayer::s_joinState          = WinsockNetLayer::eJoinState_Idle;
volatile int                        WinsockNetLayer::s_joinAttempt         = 0;
volatile bool                       WinsockNetLayer::s_joinCancel          = false;
char                                WinsockNetLayer::s_joinIP[256]         = {};
int                                 WinsockNetLayer::s_joinPort            = 0;
BYTE                                WinsockNetLayer::s_joinAssignedSmallId = 0;
DisconnectPacket::eDisconnectReason WinsockNetLayer::s_joinRejectReason    = DisconnectPacket::eDisconnect_Quitting;

ServerRuntime::Security::StreamCipher WinsockNetLayer::s_clientSendCipher;
ServerRuntime::Security::StreamCipher WinsockNetLayer::s_clientRecvCipher;
CRITICAL_SECTION WinsockNetLayer::s_clientCipherLock;
uint8_t  WinsockNetLayer::s_clientPendingKey[ServerRuntime::Security::StreamCipher::KEY_SIZE] = {};
bool     WinsockNetLayer::s_clientKeyStored = false;

bool g_Win64MultiplayerHost             = false;
bool g_Win64MultiplayerJoin             = false;
int  g_Win64MultiplayerPort             = WIN64_NET_DEFAULT_PORT;
char g_Win64MultiplayerIP[256]          = "127.0.0.1";
bool g_Win64DedicatedServer             = false;
int  g_Win64DedicatedServerPort         = WIN64_NET_DEFAULT_PORT;
char g_Win64DedicatedServerBindIP[256]  = "";
bool g_Win64DedicatedServerLanAdvertise = true;

char    g_Win64RelayServerIP[256]      = "relay.mclegacyedition.xyz";
wchar_t g_Win64RelayServerIP_Wide[256] = L"relay.mclegacyedition.xyz";
int     g_Win64RelayServerPort         = 2052;
char    g_GameVersion[]                = "TU19";

static std::string GetAuthToken()
{
#if defined(MINECRAFT_SERVER_BUILD)
    return "";
#else
    if (Windows64Minecraft::IsExternalLauncher())
        return Windows64Minecraft::GetAuthenticationTicket();
    return Windows64Launcher::GetAuthenticationToken();
#endif
}

static SOCKET ConnectToRelay()
{
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[16];
    sprintf_s(portStr, "%d", g_Win64RelayServerPort);

    if (getaddrinfo(g_Win64RelayServerIP, portStr, &hints, &result) != 0)
        return INVALID_SOCKET;

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) { freeaddrinfo(result); return INVALID_SOCKET; }

    int noDelay = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
    { closesocket(sock); freeaddrinfo(result); return INVALID_SOCKET; }

    freeaddrinfo(result);
    return sock;
}

static bool ResolveSRV(const char* hostname, char* outHost, size_t outHostSize, int* outPort)
{
    if (hostname[0] >= '0' && hostname[0] <= '9')
    {
        bool hasLetter = false;
        for (const char* p = hostname; *p; ++p)
            if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) { hasLetter = true; break; }
        if (!hasLetter) return false;
    }
    char srvName[300];
    sprintf_s(srvName, "_minecraft._tcp.%s", hostname);

    DNS_RECORD* records = nullptr;
    if (DnsQuery_A(srvName, DNS_TYPE_SRV, DNS_QUERY_STANDARD, nullptr, &records, nullptr) != 0 || !records)
        return false;

    for (DNS_RECORD* r = records; r; r = r->pNext)
    {
        if (r->wType == DNS_TYPE_SRV)
        {
            strncpy_s(outHost, outHostSize, r->Data.SRV.pNameTarget, _TRUNCATE);
            *outPort = r->Data.SRV.wPort;
            DnsRecordListFree(records, DnsFreeRecordList);
            return true;
        }
    }
    DnsRecordListFree(records, DnsFreeRecordList);
    return false;
}

static bool RecvExact(SOCKET sock, BYTE* buf, int len)
{
    int total = 0;
    while (total < len)
    { int r = recv(sock, (char*)buf + total, len - total, 0); if (r <= 0) return false; total += r; }
    return true;
}

static void SendRejectWithReason(SOCKET sock, DisconnectPacket::eDisconnectReason reason)
{
    BYTE buf[6];
    buf[0] = WIN64_SMALLID_REJECT; buf[1] = 255;
    int r = (int)reason;
    buf[2]=(BYTE)((r>>24)&0xff); buf[3]=(BYTE)((r>>16)&0xff);
    buf[4]=(BYTE)((r>> 8)&0xff); buf[5]=(BYTE)( r      &0xff);
    send(sock, (const char*)buf, sizeof(buf), 0);
}

#if defined(MINECRAFT_SERVER_BUILD)
static bool TryGetNumericRemoteIp(const sockaddr_in& addr, std::string* outIp)
{
    if (!outIp) return false;
    char buf[64] = {};
    const char* ip = inet_ntop(AF_INET, (void*)&addr.sin_addr, buf, sizeof(buf));
    if (!ip || !ip[0]) return false;
    *outIp = ip; return true;
}

enum EProxyParseResult { eProxyParse_Success, eProxyParse_Unknown, eProxyParse_Malformed, eProxyParse_Timeout, eProxyParse_SocketError };

static EProxyParseResult TryReadProxyProtocolHeader(SOCKET sock, std::string* outSrcIp)
{
    if (outSrcIp) outSrcIp->clear();
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    auto restore = [sock](){ DWORD t=0; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&t,sizeof(t)); };

    char peek[6];
    int pr = recv(sock, peek, 6, MSG_PEEK);
    if (pr == 0)          { restore(); return eProxyParse_SocketError; }
    if (pr == SOCKET_ERROR){ restore(); return WSAGetLastError()==WSAETIMEDOUT ? eProxyParse_Timeout : eProxyParse_SocketError; }
    if (pr < 6 || memcmp(peek,"PROXY ",6)!=0) { restore(); return eProxyParse_Malformed; }

    char line[108]={}; int len=0; bool found=false;
    while (len<107)
    {
        char ch; int r=recv(sock,&ch,1,0);
        if (r!=1){ restore(); int e=WSAGetLastError(); return (r==SOCKET_ERROR&&e==WSAETIMEDOUT)?eProxyParse_Timeout:eProxyParse_SocketError; }
        line[len++]=ch;
        if (len>=2 && line[len-2]=='\r' && line[len-1]=='\n'){ line[len-2]='\0'; found=true; break; }
    }
    restore();
    if (!found) return eProxyParse_Malformed;

    char* tok[6]={}; int tc=0; char* ctx=nullptr;
    char* t=strtok_s(line," ",&ctx);
    while(t&&tc<6){ tok[tc++]=t; t=strtok_s(nullptr," ",&ctx); }
    if (tc<2||strcmp(tok[0],"PROXY")!=0) return eProxyParse_Malformed;
    if (strcmp(tok[1],"UNKNOWN")==0)     return eProxyParse_Unknown;
    if (strcmp(tok[1],"TCP4")!=0||tc<6)  return eProxyParse_Malformed;
    struct in_addr a; if (inet_pton(AF_INET,tok[2],&a)!=1) return eProxyParse_Malformed;
    if (outSrcIp) *outSrcIp=tok[2];
    return eProxyParse_Success;
}
#endif

bool WinsockNetLayer::Initialize()
{
    if (s_initialized) return true;
    WSADATA wd;
    if (WSAStartup(MAKEWORD(2,2),&wd)!=0) return false;

    InitializeCriticalSection(&s_sendLock);
    InitializeCriticalSection(&s_connectionsLock);
    InitializeCriticalSection(&s_advertiseLock);
    InitializeCriticalSection(&s_discoveryLock);
    InitializeCriticalSection(&s_disconnectLock);
    InitializeCriticalSection(&s_freeSmallIdLock);
    InitializeCriticalSection(&s_smallIdToSocketLock);
    InitializeCriticalSection(&s_clientCipherLock);
    for (int i=0;i<256;i++) s_smallIdToSocket[i]=INVALID_SOCKET;

    s_initialized = true;

    if (!g_Win64DedicatedServer)
        StartDiscovery();

    return true;
}

void WinsockNetLayer::Shutdown()
{
    StopAdvertising();
    StopDiscovery();

    g_Win64LCENServer = false;

    s_joinCancel = true;
    if (s_joinThread) { WaitForSingleObject(s_joinThread,5000); CloseHandle(s_joinThread); s_joinThread=nullptr; }
    s_joinState = eJoinState_Idle;

    s_active = false; s_connected = false;

    if (s_listenSocket!=INVALID_SOCKET)        { closesocket(s_listenSocket);        s_listenSocket=INVALID_SOCKET; }
    if (s_hostConnectionSocket!=INVALID_SOCKET) { closesocket(s_hostConnectionSocket); s_hostConnectionSocket=INVALID_SOCKET; }

    if (s_acceptThread) { WaitForSingleObject(s_acceptThread,2000); CloseHandle(s_acceptThread); s_acceptThread=nullptr; }

    std::vector<HANDLE> rv;
    EnterCriticalSection(&s_connectionsLock);
    for (size_t i=0;i<s_connections.size();i++)
    {
        s_connections[i].active=false;
        if (s_connections[i].tcpSocket!=INVALID_SOCKET){ closesocket(s_connections[i].tcpSocket); s_connections[i].tcpSocket=INVALID_SOCKET; }
        if (s_connections[i].recvThread){ rv.push_back(s_connections[i].recvThread); s_connections[i].recvThread=nullptr; }
    }
    LeaveCriticalSection(&s_connectionsLock);
    for (size_t i=0;i<rv.size();i++){ WaitForSingleObject(rv[i],2000); CloseHandle(rv[i]); }
    EnterCriticalSection(&s_connectionsLock); s_connections.clear(); LeaveCriticalSection(&s_connectionsLock);

    if (s_clientRecvThread){ WaitForSingleObject(s_clientRecvThread,2000); CloseHandle(s_clientRecvThread); s_clientRecvThread=nullptr; }

    for (int i=0;i<XUSER_MAX_COUNT;i++)
    {
        if (s_splitScreenSocket[i]!=INVALID_SOCKET){ closesocket(s_splitScreenSocket[i]); s_splitScreenSocket[i]=INVALID_SOCKET; }
        if (s_splitScreenRecvThread[i]){ WaitForSingleObject(s_splitScreenRecvThread[i],2000); CloseHandle(s_splitScreenRecvThread[i]); s_splitScreenRecvThread[i]=nullptr; }
        s_splitScreenSmallId[i]=0xFF;
    }

    if (s_initialized)
    {
        EnterCriticalSection(&s_disconnectLock); s_disconnectedSmallIds.clear(); LeaveCriticalSection(&s_disconnectLock);
        EnterCriticalSection(&s_freeSmallIdLock); s_freeSmallIds.clear(); LeaveCriticalSection(&s_freeSmallIdLock);
        ResetClientCipher();
        DeleteCriticalSection(&s_clientCipherLock);
        DeleteCriticalSection(&s_sendLock);
        DeleteCriticalSection(&s_connectionsLock);
        DeleteCriticalSection(&s_advertiseLock);
        DeleteCriticalSection(&s_discoveryLock);
        DeleteCriticalSection(&s_disconnectLock);
        DeleteCriticalSection(&s_freeSmallIdLock);
        DeleteCriticalSection(&s_smallIdToSocketLock);
        WSACleanup();
        s_initialized = false;
    }
}

void WinsockNetLayer::StoreClientCipherKey(const uint8_t key[ServerRuntime::Security::StreamCipher::KEY_SIZE])
{
    EnterCriticalSection(&s_clientCipherLock);
    memcpy(s_clientPendingKey, key, ServerRuntime::Security::StreamCipher::KEY_SIZE);
    s_clientKeyStored = true;
    LeaveCriticalSection(&s_clientCipherLock);
}

bool WinsockNetLayer::SendAckAndActivateClientSendCipher()
{
    if (s_hostConnectionSocket==INVALID_SOCKET) return false;
    EnterCriticalSection(&s_sendLock);

    BYTE hdr[4];
    hdr[0]=(BYTE)((kCipherAckPatternSize>>24)&0xFF); hdr[1]=(BYTE)((kCipherAckPatternSize>>16)&0xFF);
    hdr[2]=(BYTE)((kCipherAckPatternSize>> 8)&0xFF); hdr[3]=(BYTE)( kCipherAckPatternSize     &0xFF);

    bool ok=true; int t=0;
    while(ok&&t<4){ int s=send(s_hostConnectionSocket,(const char*)hdr+t,4-t,0); if(s<=0){ok=false;break;} t+=s; }
    t=0;
    while(ok&&t<kCipherAckPatternSize){ int s=send(s_hostConnectionSocket,(const char*)kCipherAckPattern+t,kCipherAckPatternSize-t,0); if(s<=0){ok=false;break;} t+=s; }

    if (ok)
    {
        EnterCriticalSection(&s_clientCipherLock);
        s_clientSendCipher.Initialize(s_clientPendingKey, ServerRuntime::Security::StreamCipher::Client);
        LeaveCriticalSection(&s_clientCipherLock);
        app.DebugPrintf("Client: Send cipher activated\n");
    }
    else
    { app.DebugPrintf("Client: MC|CAck send failed\n"); closesocket(s_hostConnectionSocket); s_hostConnectionSocket=INVALID_SOCKET; }

    LeaveCriticalSection(&s_sendLock);
    return ok;
}

void WinsockNetLayer::ActivateClientRecvCipher()
{
    EnterCriticalSection(&s_clientCipherLock);
    s_clientRecvCipher.Initialize(s_clientPendingKey, ServerRuntime::Security::StreamCipher::Client);
    SecureZeroMemory(s_clientPendingKey, sizeof(s_clientPendingKey));
    s_clientKeyStored = false;
    LeaveCriticalSection(&s_clientCipherLock);
}

void WinsockNetLayer::ResetClientCipher()
{
    EnterCriticalSection(&s_clientCipherLock);
    s_clientSendCipher.Reset(); s_clientRecvCipher.Reset();
    SecureZeroMemory(s_clientPendingKey, sizeof(s_clientPendingKey));
    s_clientKeyStored = false;
    LeaveCriticalSection(&s_clientCipherLock);
}

bool WinsockNetLayer::TryEncryptClientOutgoing(uint8_t* data, int length)
{
    if (!data||length<=0) return false;
    EnterCriticalSection(&s_clientCipherLock);
    bool active=s_clientSendCipher.IsActive();
    if (active) s_clientSendCipher.Encrypt(data,length);
    LeaveCriticalSection(&s_clientCipherLock);
    return active;
}

#if defined(MINECRAFT_SERVER_BUILD)
bool WinsockNetLayer::SendCOnAndCommitServerCipher(BYTE smallId)
{
    auto& reg=ServerRuntime::Security::GetCipherRegistry();
    SOCKET sock=GetSocketForSmallId(smallId);
    if (sock==INVALID_SOCKET) return false;
    if (!reg.HasPendingKey(smallId)){ app.DebugPrintf("Server: no pending key for smallId=%d\n",smallId); return false; }

    EnterCriticalSection(&s_sendLock);
    BYTE hdr[4];
    hdr[0]=(BYTE)((kCipherOnPatternSize>>24)&0xFF); hdr[1]=(BYTE)((kCipherOnPatternSize>>16)&0xFF);
    hdr[2]=(BYTE)((kCipherOnPatternSize>> 8)&0xFF); hdr[3]=(BYTE)( kCipherOnPatternSize     &0xFF);
    bool ok=true; int t=0;
    while(ok&&t<4){ int s=send(sock,(const char*)hdr+t,4-t,0); if(s<=0){ok=false;break;} t+=s; }
    t=0;
    while(ok&&t<kCipherOnPatternSize){ int s=send(sock,(const char*)kCipherOnPattern+t,kCipherOnPatternSize-t,0); if(s<=0){ok=false;break;} t+=s; }
    if (ok){ reg.CommitCipher(smallId); app.DebugPrintf("Server: Cipher committed smallId=%d\n",smallId); }
    else   { app.DebugPrintf("Server: MC|COn send failed smallId=%d\n",smallId); reg.CancelPending(smallId); closesocket(sock); ClearSocketForSmallId(smallId); }
    LeaveCriticalSection(&s_sendLock);
    return ok;
}
#endif

bool WinsockNetLayer::HostGame(int port, const char* bindIp)
{
    if (!s_initialized && !Initialize()) return false;

    s_isHost       = true;
    s_localSmallId = 0;
    s_hostSmallId  = 0;
    s_nextSmallId  = XUSER_MAX_COUNT;
    s_hostGamePort = port;

    EnterCriticalSection(&s_freeSmallIdLock); s_freeSmallIds.clear(); LeaveCriticalSection(&s_freeSmallIdLock);
    EnterCriticalSection(&s_smallIdToSocketLock);
    for (int i=0;i<256;i++) s_smallIdToSocket[i]=INVALID_SOCKET;
    LeaveCriticalSection(&s_smallIdToSocketLock);

    s_listenSocket = ConnectToRelay();
    if (s_listenSocket == INVALID_SOCKET)
    { app.DebugPrintf("Win64: Failed to connect to relay for HOST\n"); return false; }

    std::string isDed = g_Win64DedicatedServer ? "1" : "0";
    std::string req   = "HOST " + GetAuthToken() + " " + isDed + "\n";
    send(s_listenSocket, req.c_str(), (int)req.size(), 0);
    app.DebugPrintf("Win64: Hosting via relay %s:%d\n", g_Win64RelayServerIP, g_Win64RelayServerPort);

    s_active    = true;
    s_connected = true;
    s_acceptThread = CreateThread(nullptr, 0, AcceptThreadProc, nullptr, 0, nullptr);
    return true;
}

bool WinsockNetLayer::JoinGame(const char* ip, int port)
{
    if (!s_initialized && !Initialize()) return false;

    s_isHost=false; s_hostSmallId=0; s_connected=false; s_active=false;

    if (s_hostConnectionSocket!=INVALID_SOCKET){ closesocket(s_hostConnectionSocket); s_hostConnectionSocket=INVALID_SOCKET; }
    if (s_clientRecvThread){ WaitForSingleObject(s_clientRecvThread,5000); CloseHandle(s_clientRecvThread); s_clientRecvThread=nullptr; }

    SOCKET sock=INVALID_SOCKET;
    BYTE assignedSmallId=0;
    bool connected=false;

    if (g_Win64LCENServer)
    {
        sock=ConnectToRelay();
        if (sock==INVALID_SOCKET){ app.DebugPrintf("Win64: relay connect failed for JOIN\n"); return false; }
        std::string req="JOIN "+GetAuthToken()+" 0 "+std::string(ip)+" 0\n";
        send(sock, req.c_str(), (int)req.size(), 0);

        DWORD to=10000; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&to,sizeof(to));
        BYTE ab[1];
        if (recv(sock,(char*)ab,1,0)!=1){ closesocket(sock); return false; }
        if (ab[0]==WIN64_SMALLID_REJECT)
        {
            BYTE rb[5]; RecvExact(sock,rb,5);
            int r=((rb[1]&0xff)<<24)|((rb[2]&0xff)<<16)|((rb[3]&0xff)<<8)|(rb[4]&0xff);
            Minecraft::GetInstance()->connectionDisconnected(ProfileManager.GetPrimaryPad(),(DisconnectPacket::eDisconnectReason)r);
            closesocket(sock); return false;
        }
        assignedSmallId=ab[0]; connected=true;
        DWORD noTo=0; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&noTo,sizeof(noTo));
    }
    else
    {
        char rHost[256]; int rPort=port;
        strncpy_s(rHost,sizeof(rHost),ip,_TRUNCATE);
        if (ResolveSRV(ip,rHost,sizeof(rHost),&rPort)) app.DebugPrintf("Win64: SRV %s->%s:%d\n",ip,rHost,rPort);

        struct addrinfo hints={},*result=nullptr;
        hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM; hints.ai_protocol=IPPROTO_TCP;
        char ps[16]; sprintf_s(ps,"%d",rPort);
        if (getaddrinfo(rHost,ps,&hints,&result)!=0){ app.DebugPrintf("Win64: getaddrinfo failed %s:%d\n",ip,port); return false; }

        for (int attempt=0;attempt<3&&!connected;attempt++)
        {
            sock=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
            if (sock==INVALID_SOCKET) break;
            int nd=1; setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nd,sizeof(nd));

            u_long nb=1; ioctlsocket(sock,FIONBIO,&nb);
            int ir=connect(sock,result->ai_addr,(int)result->ai_addrlen);
            if (ir==SOCKET_ERROR && WSAGetLastError()==WSAEWOULDBLOCK)
            {
                fd_set ws,es; FD_ZERO(&ws); FD_SET(sock,&ws); FD_ZERO(&es); FD_SET(sock,&es);
                struct timeval tv={5,0};
                if (select(0,nullptr,&ws,&es,&tv)<=0||FD_ISSET(sock,&es))
                { app.DebugPrintf("Win64: connect timed out (attempt %d)\n",attempt+1); closesocket(sock); sock=INVALID_SOCKET; continue; }
            }
            else if (ir==SOCKET_ERROR)
            { app.DebugPrintf("Win64: connect failed (attempt %d): %d\n",attempt+1,WSAGetLastError()); closesocket(sock); sock=INVALID_SOCKET; Sleep(200); continue; }

            u_long bl=0; ioctlsocket(sock,FIONBIO,&bl);
            DWORD to=5000; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&to,sizeof(to));

            BYTE ab[1];
            if (recv(sock,(char*)ab,1,0)!=1){ app.DebugPrintf("Win64: no smallId (attempt %d)\n",attempt+1); closesocket(sock); sock=INVALID_SOCKET; continue; }
            if (ab[0]==WIN64_SMALLID_REJECT)
            {
                BYTE rb[5]; RecvExact(sock,rb,5);
                int r=((rb[1]&0xff)<<24)|((rb[2]&0xff)<<16)|((rb[3]&0xff)<<8)|(rb[4]&0xff);
                Minecraft::GetInstance()->connectionDisconnected(ProfileManager.GetPrimaryPad(),(DisconnectPacket::eDisconnectReason)r);
                closesocket(sock); freeaddrinfo(result); return false;
            }
            assignedSmallId=ab[0]; connected=true;
        }
        freeaddrinfo(result);
        if (!connected) return false;
        DWORD noTo=0; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&noTo,sizeof(noTo));
    }

    s_localSmallId=assignedSmallId;
    strncpy_s(g_Win64MultiplayerIP,sizeof(g_Win64MultiplayerIP),ip,_TRUNCATE);
    g_Win64MultiplayerPort=port;
    s_hostConnectionSocket=sock;
    s_active=true; s_connected=true;
    app.DebugPrintf("Win64: Connected to %s:%d smallId=%d\n",ip,port,s_localSmallId);
    s_clientRecvThread=CreateThread(nullptr,0,ClientRecvThreadProc,nullptr,0,nullptr);
    return true;
}

bool WinsockNetLayer::BeginJoinGame(const char* ip, int port)
{
    if (!s_initialized && !Initialize()) return false;

    CancelJoinGame();
    if (s_joinThread){ WaitForSingleObject(s_joinThread,5000); CloseHandle(s_joinThread); s_joinThread=nullptr; }

    s_isHost=false; s_hostSmallId=0; s_connected=false; s_active=false;
    if (s_hostConnectionSocket!=INVALID_SOCKET){ closesocket(s_hostConnectionSocket); s_hostConnectionSocket=INVALID_SOCKET; }
    if (s_clientRecvThread){ WaitForSingleObject(s_clientRecvThread,5000); CloseHandle(s_clientRecvThread); s_clientRecvThread=nullptr; }

    strncpy_s(s_joinIP,sizeof(s_joinIP),ip,_TRUNCATE);
    s_joinPort=port; s_joinAttempt=0; s_joinCancel=false;
    s_joinAssignedSmallId=0; s_joinRejectReason=DisconnectPacket::eDisconnect_Quitting;
    s_joinState=eJoinState_Connecting;

    s_joinThread=CreateThread(nullptr,0,JoinThreadProc,nullptr,0,nullptr);
    if (!s_joinThread){ s_joinState=eJoinState_Failed; return false; }
    return true;
}

DWORD WINAPI WinsockNetLayer::JoinThreadProc(LPVOID)
{
    const bool isRelay = g_Win64LCENServer;

    SOCKET sock=INVALID_SOCKET;
    BYTE assignedSmallId=0;
    bool connected=false;

    if (isRelay)
    {
        for (int attempt=0;attempt<JOIN_MAX_ATTEMPTS;attempt++)
        {
            if (s_joinCancel){ s_joinState=eJoinState_Cancelled; return 0; }
            s_joinAttempt=attempt+1;

            sock=ConnectToRelay();
            if (sock==INVALID_SOCKET)
            { app.DebugPrintf("Win64: relay connect failed (attempt %d/%d)\n",attempt+1,JOIN_MAX_ATTEMPTS); for(int w=0;w<4&&!s_joinCancel;w++) Sleep(50); continue; }

            std::string req="JOIN "+GetAuthToken()+" 0 "+std::string(s_joinIP)+" 0\n";
            send(sock,req.c_str(),(int)req.size(),0);

            DWORD to=10000; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&to,sizeof(to));
            BYTE ab[1];
            if (recv(sock,(char*)ab,1,0)!=1)
            { app.DebugPrintf("Win64: relay no smallId (attempt %d)\n",attempt+1); closesocket(sock); sock=INVALID_SOCKET; continue; }

            if (ab[0]==WIN64_SMALLID_REJECT)
            {
                BYTE rb[5]; if(!RecvExact(sock,rb,5)){closesocket(sock);sock=INVALID_SOCKET;continue;}
                int r=((rb[1]&0xff)<<24)|((rb[2]&0xff)<<16)|((rb[3]&0xff)<<8)|(rb[4]&0xff);
                s_joinRejectReason=(DisconnectPacket::eDisconnectReason)r;
                closesocket(sock); s_joinState=eJoinState_Rejected; return 0;
            }
            assignedSmallId=ab[0]; connected=true; break;
        }
    }
    else
    {
        char rHost[256]; int rPort=s_joinPort;
        strncpy_s(rHost,sizeof(rHost),s_joinIP,_TRUNCATE);
        if (ResolveSRV(s_joinIP,rHost,sizeof(rHost),&rPort)) app.DebugPrintf("Win64: SRV %s->%s:%d\n",s_joinIP,rHost,rPort);

        struct addrinfo hints={},*result=nullptr;
        hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM; hints.ai_protocol=IPPROTO_TCP;
        char ps[16]; sprintf_s(ps,"%d",rPort);
        if (getaddrinfo(rHost,ps,&hints,&result)!=0){ s_joinState=eJoinState_Failed; return 0; }

        for (int attempt=0;attempt<JOIN_MAX_ATTEMPTS;attempt++)
        {
            if (s_joinCancel){ freeaddrinfo(result); s_joinState=eJoinState_Cancelled; return 0; }
            s_joinAttempt=attempt+1;

            sock=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
            if (sock==INVALID_SOCKET) break;
            int nd=1; setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nd,sizeof(nd));

            u_long nb=1; ioctlsocket(sock,FIONBIO,&nb);
            int ir=connect(sock,result->ai_addr,(int)result->ai_addrlen);
            if (ir==SOCKET_ERROR && WSAGetLastError()==WSAEWOULDBLOCK)
            {
                fd_set ws,es; FD_ZERO(&ws); FD_SET(sock,&ws); FD_ZERO(&es); FD_SET(sock,&es);
                struct timeval tv={5,0};
                if (select(0,nullptr,&ws,&es,&tv)<=0||FD_ISSET(sock,&es))
                { app.DebugPrintf("Win64: connect timed out (attempt %d/%d)\n",attempt+1,JOIN_MAX_ATTEMPTS); closesocket(sock); sock=INVALID_SOCKET; continue; }
            }
            else if (ir==SOCKET_ERROR)
            { app.DebugPrintf("Win64: connect failed (attempt %d/%d): %d\n",attempt+1,JOIN_MAX_ATTEMPTS,WSAGetLastError()); closesocket(sock); sock=INVALID_SOCKET; for(int w=0;w<4&&!s_joinCancel;w++) Sleep(50); continue; }

            u_long bl=0; ioctlsocket(sock,FIONBIO,&bl);
            DWORD to=5000; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&to,sizeof(to));

            BYTE ab[1];
            if (recv(sock,(char*)ab,1,0)!=1)
            { app.DebugPrintf("Win64: no smallId from host (attempt %d/%d)\n",attempt+1,JOIN_MAX_ATTEMPTS); closesocket(sock); sock=INVALID_SOCKET; continue; }
            if (ab[0]==WIN64_SMALLID_REJECT)
            {
                BYTE rb[5]; if(!RecvExact(sock,rb,5)){closesocket(sock);sock=INVALID_SOCKET;continue;}
                int r=((rb[1]&0xff)<<24)|((rb[2]&0xff)<<16)|((rb[3]&0xff)<<8)|(rb[4]&0xff);
                s_joinRejectReason=(DisconnectPacket::eDisconnectReason)r;
                closesocket(sock); freeaddrinfo(result); s_joinState=eJoinState_Rejected; return 0;
            }
            assignedSmallId=ab[0]; connected=true; break;
        }
        freeaddrinfo(result);

        if (connected)
        { DWORD noTo=0; setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&noTo,sizeof(noTo)); }
    }

    if (s_joinCancel){ if(sock!=INVALID_SOCKET) closesocket(sock); s_joinState=eJoinState_Cancelled; return 0; }
    if (!connected)  { if(sock!=INVALID_SOCKET) closesocket(sock); s_joinState=eJoinState_Failed;    return 0; }

    s_hostConnectionSocket=sock;
    s_joinAssignedSmallId=assignedSmallId;
    s_joinState=eJoinState_Success;
    return 0;
}

void WinsockNetLayer::CancelJoinGame()
{
    s_joinCancel=true;
    SOCKET sock=s_hostConnectionSocket;
    if (sock!=INVALID_SOCKET){ s_hostConnectionSocket=INVALID_SOCKET; closesocket(sock); }
    if (s_joinState==eJoinState_Success||s_joinState==eJoinState_Connecting)
        s_joinState=eJoinState_Cancelled;
}

bool WinsockNetLayer::FinalizeJoin()
{
    if (s_joinState != eJoinState_Success)
        return false;

    s_localSmallId = s_joinAssignedSmallId;

    strncpy_s(g_Win64MultiplayerIP, sizeof(g_Win64MultiplayerIP), s_joinIP, _TRUNCATE);
    g_Win64MultiplayerPort = s_joinPort;

    app.DebugPrintf("Win64: FinalizeJoin %s:%d smallId=%d\n",
        s_joinIP, s_joinPort, s_localSmallId);

    s_active    = true;
    s_connected = true;

    if (s_joinThread)
    {
        WaitForSingleObject(s_joinThread, 2000);
        CloseHandle(s_joinThread);
        s_joinThread = nullptr;
    }
    s_joinState = eJoinState_Idle;
    return true;
}
void WinsockNetLayer::StartClientRecvThread()
{
    if (s_clientRecvThread)
    {
        WaitForSingleObject(s_clientRecvThread, 2000);
        CloseHandle(s_clientRecvThread);
        s_clientRecvThread = nullptr;
    }
    s_clientRecvThread = CreateThread(nullptr, 0, ClientRecvThreadProc, nullptr, 0, nullptr);
}

WinsockNetLayer::eJoinState WinsockNetLayer::GetJoinState()         { return s_joinState; }
int WinsockNetLayer::GetJoinAttempt()                               { return s_joinAttempt; }
int WinsockNetLayer::GetJoinMaxAttempts()                           { return JOIN_MAX_ATTEMPTS; }
DisconnectPacket::eDisconnectReason WinsockNetLayer::GetJoinRejectReason() { return s_joinRejectReason; }

bool WinsockNetLayer::SendOnSocket(SOCKET sock, const void* data, int dataSize)
{
    if (sock==INVALID_SOCKET||dataSize<=0||dataSize>WIN64_NET_MAX_PACKET_SIZE) return false;
    EnterCriticalSection(&s_sendLock);

    BYTE hdr[4];
    hdr[0]=(BYTE)((dataSize>>24)&0xFF); hdr[1]=(BYTE)((dataSize>>16)&0xFF);
    hdr[2]=(BYTE)((dataSize>> 8)&0xFF); hdr[3]=(BYTE)( dataSize     &0xFF);

    int t=0;
    while(t<4){ int s=send(sock,(const char*)hdr+t,4-t,0); if(s<=0){LeaveCriticalSection(&s_sendLock);return false;} t+=s; }
    t=0;
    while(t<dataSize){ int s=send(sock,(const char*)data+t,dataSize-t,0); if(s<=0){LeaveCriticalSection(&s_sendLock);return false;} t+=s; }

    LeaveCriticalSection(&s_sendLock);
    return true;
}

bool WinsockNetLayer::SendToSmallId(BYTE targetSmallId, const void* data, int dataSize)
{
    if (!s_active) return false;

    if (s_isHost)
    {
        SOCKET sock = GetSocketForSmallId(targetSmallId);
        if (sock == INVALID_SOCKET) return false;
#if defined(MINECRAFT_SERVER_BUILD)
        if (g_Win64DedicatedServer && dataSize > 0)
        {
            std::vector<BYTE> buf(static_cast<const BYTE*>(data),
                static_cast<const BYTE*>(data) + dataSize);
            if (ServerRuntime::Security::GetCipherRegistry().TryEncryptOutgoing(
                    targetSmallId, buf.data(), dataSize))
                return SendOnSocket(sock, buf.data(), dataSize);
        }
#endif
        return SendOnSocket(sock, data, dataSize);
    }
    else
    {
        EnterCriticalSection(&s_clientCipherLock);
        if (s_clientSendCipher.IsActive() && dataSize > 0)
        {
            std::vector<BYTE> buf(static_cast<const BYTE*>(data),
                static_cast<const BYTE*>(data) + dataSize);
            s_clientSendCipher.Encrypt(buf.data(), dataSize);
            LeaveCriticalSection(&s_clientCipherLock);
            return SendOnSocket(s_hostConnectionSocket, buf.data(), dataSize);
        }
        LeaveCriticalSection(&s_clientCipherLock);
        return SendOnSocket(s_hostConnectionSocket, data, dataSize);
    }
}

SOCKET WinsockNetLayer::GetSocketForSmallId(BYTE smallId)
{
    EnterCriticalSection(&s_smallIdToSocketLock);
    SOCKET sock=s_smallIdToSocket[smallId];
    LeaveCriticalSection(&s_smallIdToSocketLock);
    return sock;
}

void WinsockNetLayer::ClearSocketForSmallId(BYTE smallId)
{
    EnterCriticalSection(&s_smallIdToSocketLock);
    s_smallIdToSocket[smallId]=INVALID_SOCKET;
    LeaveCriticalSection(&s_smallIdToSocketLock);
}

void WinsockNetLayer::HandleDataReceived(BYTE fromSmallId, BYTE toSmallId, unsigned char* data, unsigned int dataSize)
{
    INetworkPlayer* pFrom=g_NetworkManager.GetPlayerBySmallId(fromSmallId);
    INetworkPlayer* pTo  =g_NetworkManager.GetPlayerBySmallId(toSmallId);

    if (!pFrom||!pTo)
    {
        app.DebugPrintf("NET RECV: DROPPED %u bytes from=%d to=%d (player NULL)\n",dataSize,fromSmallId,toSmallId);
        return;
    }

    if (s_isHost)
    {
        ::Socket* pSocket=pFrom->GetSocket();
        if (pSocket) pSocket->pushDataToQueue(data,dataSize,false);
        else app.DebugPrintf("NET RECV: DROPPED host pSocket NULL from=%d\n",fromSmallId);
    }
    else
    {
        ::Socket* pSocket=pTo->GetSocket();
        if (pSocket) pSocket->pushDataToQueue(data,dataSize,true);
        else app.DebugPrintf("NET RECV: DROPPED client pSocket NULL to=%d\n",toSmallId);
    }
}

DWORD WINAPI WinsockNetLayer::AcceptThreadProc(LPVOID)
{
    char buf[512];
    std::string pending;
    while (s_active)
    {
        int ret=recv(s_listenSocket,buf,sizeof(buf)-1,0);
        if (ret<=0){ if(s_active) app.DebugPrintf("Win64: Relay control socket disconnected\n"); break; }
        buf[ret]=0; pending+=buf;

        size_t pos;
        while ((pos=pending.find('\n'))!=std::string::npos)
        {
            std::string line=pending.substr(0,pos);
            pending.erase(0,pos+1);
            if (line.size()<8||line.substr(0,7)!="CLIENT ") continue;
            std::string clientId=line.substr(7);

            SOCKET clientSocket=ConnectToRelay();
            if (clientSocket==INVALID_SOCKET){ app.DebugPrintf("Win64: Failed to open relay data socket for %s\n",clientId.c_str()); continue; }

            int nd=1; setsockopt(clientSocket,IPPROTO_TCP,TCP_NODELAY,(const char*)&nd,sizeof(nd));

            std::string isDed=g_Win64DedicatedServer?"1":"0";
            std::string acc="ACCEPT "+GetAuthToken()+" "+isDed+" "+clientId+"\n";
            send(clientSocket,acc.c_str(),(int)acc.size(),0);

            extern QNET_STATE _iQNetStubState;
            if (_iQNetStubState!=QNET_STATE_GAME_PLAY)
            { app.DebugPrintf("Win64: Rejecting relay client, game not ready\n"); closesocket(clientSocket); continue; }

            extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
            if (g_pPlatformNetworkManager&&!g_pPlatformNetworkManager->CanAcceptMoreConnections())
            { SendRejectWithReason(clientSocket,DisconnectPacket::eDisconnect_ServerFull); closesocket(clientSocket); continue; }

            BYTE assignedSmallId;
            EnterCriticalSection(&s_freeSmallIdLock);
            if (!s_freeSmallIds.empty()){ assignedSmallId=s_freeSmallIds.back(); s_freeSmallIds.pop_back(); }
            else if (s_nextSmallId<(unsigned int)MINECRAFT_NET_MAX_PLAYERS){ assignedSmallId=(BYTE)s_nextSmallId++; }
            else{ LeaveCriticalSection(&s_freeSmallIdLock); SendRejectWithReason(clientSocket,DisconnectPacket::eDisconnect_ServerFull); closesocket(clientSocket); continue; }
            LeaveCriticalSection(&s_freeSmallIdLock);

            BYTE ab[1]={assignedSmallId};
            if (send(clientSocket,(const char*)ab,1,0)!=1)
            { app.DebugPrintf("Win64: Failed to send smallId to relay client\n"); closesocket(clientSocket); PushFreeSmallId(assignedSmallId); continue; }

            app.DebugPrintf("Win64: Relay client accepted smallId=%d\n",assignedSmallId);

            Win64RemoteConnection conn;
            conn.tcpSocket=clientSocket; conn.smallId=assignedSmallId; conn.active=true; conn.recvThread=nullptr;

            EnterCriticalSection(&s_connectionsLock);
            s_connections.push_back(conn);
            int connIdx=(int)s_connections.size()-1;
            LeaveCriticalSection(&s_connectionsLock);

            EnterCriticalSection(&s_smallIdToSocketLock);
            s_smallIdToSocket[assignedSmallId]=clientSocket;
            LeaveCriticalSection(&s_smallIdToSocketLock);

            IQNetPlayer* qp=&IQNet::m_player[assignedSmallId];
            extern void Win64_SetupRemoteQNetPlayer(IQNetPlayer*,BYTE,bool,bool);
            Win64_SetupRemoteQNetPlayer(qp,assignedSmallId,false,false);

            extern CPlatformNetworkManagerStub* g_pPlatformNetworkManager;
            g_pPlatformNetworkManager->NotifyPlayerJoined(qp);

            DWORD* param=new DWORD; *param=connIdx;
            HANDLE hThread=CreateThread(nullptr,0,RecvThreadProc,param,0,nullptr);
            EnterCriticalSection(&s_connectionsLock);
            if (connIdx<(int)s_connections.size()) s_connections[connIdx].recvThread=hThread;
            LeaveCriticalSection(&s_connectionsLock);
        }
    }
    return 0;
}

DWORD WINAPI WinsockNetLayer::RecvThreadProc(LPVOID param)
{
    DWORD connIdx=*static_cast<DWORD*>(param); delete static_cast<DWORD*>(param);

    EnterCriticalSection(&s_connectionsLock);
    if (connIdx>=(DWORD)s_connections.size()){ LeaveCriticalSection(&s_connectionsLock); return 0; }
    SOCKET sock=s_connections[connIdx].tcpSocket;
    BYTE clientSmallId=s_connections[connIdx].smallId;
    LeaveCriticalSection(&s_connectionsLock);

    std::vector<BYTE> buf(WIN64_NET_RECV_BUFFER_SIZE);
    while (s_active)
    {
        BYTE hdr[4];
        if (!RecvExact(sock,hdr,4)){ app.DebugPrintf("Win64: smallId=%d disconnected (hdr)\n",clientSmallId); break; }
        int sz=((uint32_t)hdr[0]<<24)|((uint32_t)hdr[1]<<16)|((uint32_t)hdr[2]<<8)|(uint32_t)hdr[3];
        if (sz<=0||sz>WIN64_NET_MAX_PACKET_SIZE){ app.DebugPrintf("Win64: bad packet size %d from smallId=%d\n",sz,clientSmallId); break; }
        if ((int)buf.size()<sz) buf.resize(sz);
        if (!RecvExact(sock,buf.data(),sz)){ app.DebugPrintf("Win64: smallId=%d disconnected (body)\n",clientSmallId); break; }
#if defined(MINECRAFT_SERVER_BUILD)
        if (g_Win64DedicatedServer&&sz==kCipherAckPatternSize&&memcmp(buf.data(),kCipherAckPattern,kCipherAckPatternSize)==0)
        { SendCOnAndCommitServerCipher(clientSmallId); continue; }
        if (g_Win64DedicatedServer) ServerRuntime::Security::GetCipherRegistry().DecryptIncoming(clientSmallId,buf.data(),sz);
#endif
        HandleDataReceived(clientSmallId,s_hostSmallId,buf.data(),sz);
    }

    EnterCriticalSection(&s_connectionsLock);
    for (size_t i=0;i<s_connections.size();i++)
    {
        if (s_connections[i].smallId==clientSmallId)
        { s_connections[i].active=false; if(s_connections[i].tcpSocket!=INVALID_SOCKET){closesocket(s_connections[i].tcpSocket);s_connections[i].tcpSocket=INVALID_SOCKET;} break; }
    }
    LeaveCriticalSection(&s_connectionsLock);

    EnterCriticalSection(&s_disconnectLock);
    s_disconnectedSmallIds.push_back(clientSmallId);
    LeaveCriticalSection(&s_disconnectLock);
    return 0;
}

DWORD WINAPI WinsockNetLayer::ClientRecvThreadProc(LPVOID)
{
    std::vector<BYTE> buf(WIN64_NET_RECV_BUFFER_SIZE);

    while (s_active && s_hostConnectionSocket != INVALID_SOCKET)
    {
        BYTE hdr[4];
        if (!RecvExact(s_hostConnectionSocket, hdr, 4))
        {
            app.DebugPrintf("Win64: disconnected from host (hdr)\n");
            break;
        }

        int sz = ((int)hdr[0] << 24) | ((int)hdr[1] << 16) | ((int)hdr[2] << 8) | (int)hdr[3];
        if (sz <= 0 || sz > WIN64_NET_MAX_PACKET_SIZE)
        {
            app.DebugPrintf("Win64: invalid packet size %d from host\n", sz);
            break;
        }

        if ((int)buf.size() < sz)
            buf.resize(sz);

        if (!RecvExact(s_hostConnectionSocket, buf.data(), sz))
        {
            app.DebugPrintf("Win64: disconnected from host (body)\n");
            break;
        }

        if (sz == kCipherOnPatternSize &&
            memcmp(buf.data(), kCipherOnPattern, kCipherOnPatternSize) == 0)
        {
            ActivateClientRecvCipher();
            app.DebugPrintf("Client: Recv cipher activated (MC|COn received)\n");
            continue;
        }

        EnterCriticalSection(&s_clientCipherLock);
        if (s_clientRecvCipher.IsActive())
            s_clientRecvCipher.Decrypt(buf.data(), sz);
        LeaveCriticalSection(&s_clientCipherLock);

        HandleDataReceived(s_hostSmallId, s_localSmallId, buf.data(), sz);
    }

    s_connected = false;
    ResetClientCipher();
    return 0;
}

bool WinsockNetLayer::PopDisconnectedSmallId(BYTE* out)
{
    bool found=false;
    EnterCriticalSection(&s_disconnectLock);
    if (!s_disconnectedSmallIds.empty()){ *out=s_disconnectedSmallIds.back(); s_disconnectedSmallIds.pop_back(); found=true; }
    LeaveCriticalSection(&s_disconnectLock);
    return found;
}

void WinsockNetLayer::PushFreeSmallId(BYTE smallId)
{
#if defined(MINECRAFT_SERVER_BUILD)
    if (g_Win64DedicatedServer) ServerRuntime::Security::GetCipherRegistry().DeactivateCipher(smallId);
#endif
    if (smallId<(BYTE)XUSER_MAX_COUNT) return;
    EnterCriticalSection(&s_freeSmallIdLock);
    bool already=false;
    for (size_t i=0;i<s_freeSmallIds.size();i++) if(s_freeSmallIds[i]==smallId){already=true;break;}
    if (!already) s_freeSmallIds.push_back(smallId);
    LeaveCriticalSection(&s_freeSmallIdLock);
}

void WinsockNetLayer::CloseConnectionBySmallId(BYTE smallId)
{
    EnterCriticalSection(&s_connectionsLock);
    for (size_t i=0;i<s_connections.size();i++)
    {
        if (s_connections[i].smallId==smallId&&s_connections[i].active&&s_connections[i].tcpSocket!=INVALID_SOCKET)
        { closesocket(s_connections[i].tcpSocket); s_connections[i].tcpSocket=INVALID_SOCKET; break; }
    }
    LeaveCriticalSection(&s_connectionsLock);
}

BYTE WinsockNetLayer::GetSplitScreenSmallId(int padIndex)
{
    if (padIndex<=0||padIndex>=XUSER_MAX_COUNT) return 0xFF;
    return s_splitScreenSmallId[padIndex];
}

SOCKET WinsockNetLayer::GetLocalSocket(BYTE senderSmallId)
{
    if (senderSmallId==s_localSmallId) return s_hostConnectionSocket;
    for (int i=1;i<XUSER_MAX_COUNT;i++)
        if (s_splitScreenSmallId[i]==senderSmallId&&s_splitScreenSocket[i]!=INVALID_SOCKET)
            return s_splitScreenSocket[i];
    return INVALID_SOCKET;
}

bool WinsockNetLayer::JoinSplitScreen(int padIndex, BYTE* outSmallId)
{
    if (!s_active||s_isHost||padIndex<=0||padIndex>=XUSER_MAX_COUNT) return false;
    if (s_splitScreenSocket[padIndex]!=INVALID_SOCKET) return false;

    SOCKET sock=INVALID_SOCKET;

    if (g_Win64LCENServer)
    {
        sock=ConnectToRelay();
        if (sock==INVALID_SOCKET){ app.DebugPrintf("Win64: split-screen relay connect failed\n"); return false; }
        int nd=1; setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nd,sizeof(nd));
        std::string req="JOIN "+GetAuthToken()+" 0 "+std::string(s_joinIP)+" 1\n";
        send(sock,req.c_str(),(int)req.size(),0);
    }
    else
    {
        struct addrinfo hints={},*result=nullptr;
        hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM; hints.ai_protocol=IPPROTO_TCP;
        char ps[16]; sprintf_s(ps,"%d",g_Win64MultiplayerPort);
        if (getaddrinfo(g_Win64MultiplayerIP,ps,&hints,&result)!=0||!result)
        { app.DebugPrintf("Win64: split-screen getaddrinfo failed\n"); return false; }
        sock=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
        if (sock==INVALID_SOCKET){ freeaddrinfo(result); return false; }
        int nd=1; setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nd,sizeof(nd));
        if (connect(sock,result->ai_addr,(int)result->ai_addrlen)==SOCKET_ERROR)
        { app.DebugPrintf("Win64: split-screen connect failed: %d\n",WSAGetLastError()); closesocket(sock); freeaddrinfo(result); return false; }
        freeaddrinfo(result);
    }

    BYTE ab[1];
    if (!RecvExact(sock,ab,1)){ app.DebugPrintf("Win64: split-screen no smallId\n"); closesocket(sock); return false; }
    if (ab[0]==WIN64_SMALLID_REJECT){ BYTE rb[5]; RecvExact(sock,rb,5); app.DebugPrintf("Win64: split-screen rejected\n"); closesocket(sock); return false; }

    s_splitScreenSocket[padIndex]=sock;
    s_splitScreenSmallId[padIndex]=ab[0];
    *outSmallId=ab[0];
    app.DebugPrintf("Win64: split-screen pad %d smallId=%d\n",padIndex,ab[0]);

    int* p=new int; *p=padIndex;
    s_splitScreenRecvThread[padIndex]=CreateThread(nullptr,0,SplitScreenRecvThreadProc,p,0,nullptr);
    if (!s_splitScreenRecvThread[padIndex]){ delete p; closesocket(sock); s_splitScreenSocket[padIndex]=INVALID_SOCKET; s_splitScreenSmallId[padIndex]=0xFF; return false; }
    return true;
}

void WinsockNetLayer::CloseSplitScreenConnection(int padIndex)
{
    if (padIndex<=0||padIndex>=XUSER_MAX_COUNT) return;
    if (s_splitScreenSocket[padIndex]!=INVALID_SOCKET){ closesocket(s_splitScreenSocket[padIndex]); s_splitScreenSocket[padIndex]=INVALID_SOCKET; }
    s_splitScreenSmallId[padIndex]=0xFF;
    if (s_splitScreenRecvThread[padIndex]){ WaitForSingleObject(s_splitScreenRecvThread[padIndex],2000); CloseHandle(s_splitScreenRecvThread[padIndex]); s_splitScreenRecvThread[padIndex]=nullptr; }
}

DWORD WINAPI WinsockNetLayer::SplitScreenRecvThreadProc(LPVOID param)
{
    int padIndex=*(int*)param; delete (int*)param;
    SOCKET sock=s_splitScreenSocket[padIndex];
    BYTE lsid=s_splitScreenSmallId[padIndex];
    std::vector<BYTE> buf(WIN64_NET_RECV_BUFFER_SIZE);

    while (s_active&&s_splitScreenSocket[padIndex]!=INVALID_SOCKET)
    {
        BYTE hdr[4];
        if (!RecvExact(sock,hdr,4)){ app.DebugPrintf("Win64: split-screen pad %d disconnected\n",padIndex); break; }
        int sz=((uint32_t)hdr[0]<<24)|((uint32_t)hdr[1]<<16)|((uint32_t)hdr[2]<<8)|(uint32_t)hdr[3];
        if (sz<=0||sz>WIN64_NET_MAX_PACKET_SIZE) break;
        if ((int)buf.size()<sz) buf.resize(sz);
        if (!RecvExact(sock,buf.data(),sz)) break;
        HandleDataReceived(s_hostSmallId,lsid,buf.data(),sz);
    }

    EnterCriticalSection(&s_disconnectLock);
    s_disconnectedSmallIds.push_back(lsid);
    LeaveCriticalSection(&s_disconnectLock);
    return 0;
}

bool WinsockNetLayer::StartAdvertising(int gamePort, const wchar_t* hostName, unsigned int gameSettings,
    unsigned int texPackId, unsigned char subTexId, unsigned short netVer)
{
    if (s_advertising) return true;
    if (!s_initialized) return false;

    EnterCriticalSection(&s_advertiseLock);
    memset(&s_advertiseData,0,sizeof(s_advertiseData));
    s_advertiseData.magic=WIN64_LAN_BROADCAST_MAGIC;
    s_advertiseData.netVersion=netVer;
    s_advertiseData.gamePort=(WORD)gamePort;
    wcsncpy_s(s_advertiseData.hostName,32,hostName,_TRUNCATE);
    s_advertiseData.playerCount=1;
    s_advertiseData.maxPlayers=MINECRAFT_NET_MAX_PLAYERS;
    s_advertiseData.gameHostSettings=gameSettings;
    s_advertiseData.texturePackParentId=texPackId;
    s_advertiseData.subTexturePackId=subTexId;
    s_advertiseData.isJoinable=0;
    s_hostGamePort=gamePort;
    LeaveCriticalSection(&s_advertiseLock);

    s_advertising=true;
    s_advertiseThread=CreateThread(nullptr,0,AdvertiseThreadProc,nullptr,0,nullptr);
    app.DebugPrintf("Win64: Started advertising via relay\n");
    return true;
}

void WinsockNetLayer::StopAdvertising()
{
    s_advertising=false;
    if (s_advertiseSock!=INVALID_SOCKET){ closesocket(s_advertiseSock); s_advertiseSock=INVALID_SOCKET; }
    if (s_advertiseThread){ WaitForSingleObject(s_advertiseThread,2000); CloseHandle(s_advertiseThread); s_advertiseThread=nullptr; }
}

void WinsockNetLayer::UpdateAdvertisePlayerCount(BYTE c) { EnterCriticalSection(&s_advertiseLock); s_advertiseData.playerCount=c;    LeaveCriticalSection(&s_advertiseLock); }
void WinsockNetLayer::UpdateAdvertiseMaxPlayers(BYTE m)  { EnterCriticalSection(&s_advertiseLock); s_advertiseData.maxPlayers=m;     LeaveCriticalSection(&s_advertiseLock); }
void WinsockNetLayer::UpdateAdvertiseJoinable(bool j)    { EnterCriticalSection(&s_advertiseLock); s_advertiseData.isJoinable=j?1:0; LeaveCriticalSection(&s_advertiseLock); }

DWORD WINAPI WinsockNetLayer::AdvertiseThreadProc(LPVOID)
{
    while (s_advertising)
    {
        EnterCriticalSection(&s_advertiseLock);
        Win64LANBroadcast data=s_advertiseData;
        LeaveCriticalSection(&s_advertiseLock);

        SOCKET sock=ConnectToRelay();
        if (sock!=INVALID_SOCKET)
        {
            std::string req="ADVERTISE "+GetAuthToken()+" "+(g_Win64DedicatedServer?"1":"0");
            req+=" "+std::to_string(data.playerCount);
            req+=" "+std::to_string(data.maxPlayers);
            req+=" "+std::to_string(data.gameHostSettings);
            req+=" "+std::to_string(data.texturePackParentId);
            req+=" "+std::to_string(data.subTexturePackId);
            req+=" "+std::to_string(data.isJoinable);
            req+=" "+std::string(g_GameVersion)+"\n";
            send(sock,req.c_str(),(int)req.size(),0);
            closesocket(sock);
        }
        Sleep(1500);
    }
    return 0;
}

bool WinsockNetLayer::StartDiscovery()
{
    if (s_discovering) return true;
    if (!s_initialized) return false;
    s_discovering=true;
    s_discoveryThread=CreateThread(nullptr,0,DiscoveryThreadProc,nullptr,0,nullptr);
    app.DebugPrintf("Win64: Started session discovery via relay\n");
    return true;
}

void WinsockNetLayer::StopDiscovery()
{
    s_discovering=false;
    if (s_discoverySock!=INVALID_SOCKET){ closesocket(s_discoverySock); s_discoverySock=INVALID_SOCKET; }
    if (s_discoveryThread){ WaitForSingleObject(s_discoveryThread,2000); CloseHandle(s_discoveryThread); s_discoveryThread=nullptr; }
    EnterCriticalSection(&s_discoveryLock); s_discoveredSessions.clear(); LeaveCriticalSection(&s_discoveryLock);
}

std::vector<Win64LANSession> WinsockNetLayer::GetDiscoveredSessions()
{
    std::vector<Win64LANSession> r;
    EnterCriticalSection(&s_discoveryLock); r=s_discoveredSessions; LeaveCriticalSection(&s_discoveryLock);
    return r;
}

DWORD WINAPI WinsockNetLayer::DiscoveryThreadProc(LPVOID)
{
    while (s_discovering)
    {
        SOCKET sock=ConnectToRelay();
        if (sock==INVALID_SOCKET){ Sleep(8000); continue; }

        std::string req="LIST "+GetAuthToken()+" 0 dedicated "+std::string(g_GameVersion)+"\n";
        send(sock,req.c_str(),(int)req.size(),0);

        char rb[8192]; std::string json;
        while (true)
        {
            int r=recv(sock,rb,sizeof(rb)-1,0);
            if (r<=0) break;
            rb[r]=0; json+=rb;
            if (json.find('\n')!=std::string::npos) break;
        }
        closesocket(sock);

        while (!json.empty()&&(json.back()=='\n'||json.back()=='\r')) json.pop_back();

        if (!json.empty())
        {
            try
            {
                auto arr=nlohmann::json::parse(json);
                DWORD now=GetTickCount();
                EnterCriticalSection(&s_discoveryLock);
                s_discoveredSessions.clear();

                for (auto& el : arr)
                {
                    if (!el.contains("sessionid")||!el.contains("username")||
                        !el.contains("playerCount")||
                        !el.contains("maxPlayers")||!el.contains("gameHostSettings")||
                        !el.contains("texturePackParentId")||!el.contains("subTexturePackId")||
                        !el.contains("isJoinable")) continue;

                    std::string sid     = el["sessionid"];
                    std::string uname   = el["username"];
                    int    players      = el["playerCount"];
                    int    maxPlayers   = el["maxPlayers"];
                    unsigned int hs     = el["gameHostSettings"];
                    unsigned int tpid   = el["texturePackParentId"];
                    unsigned int stid   = el["subTexturePackId"];
                    bool   joinable     = el["isJoinable"];

                    Win64LANSession s={};
                    strncpy_s(s.hostIP,sizeof(s.hostIP),uname.c_str(),_TRUNCATE);
                    s.hostPort            =g_Win64RelayServerPort;
                    MultiByteToWideChar(CP_UTF8,0,uname.c_str(),-1,s.hostName,32);
                    s.playerCount         =(BYTE)players;
                    s.maxPlayers          =(BYTE)maxPlayers;
                    s.isJoinable          =joinable;
                    s.lastSeenTick        =now;
                    s.netVersion          =MINECRAFT_NET_VERSION;
                    s.gameHostSettings    =hs;
                    s.texturePackParentId =tpid;
                    s.subTexturePackId    =(BYTE)stid;
                    s_discoveredSessions.push_back(s);
                }
                LeaveCriticalSection(&s_discoveryLock);
            }
            catch (...){ app.DebugPrintf("Win64: Failed to parse discovery JSON\n"); }
        }
        Sleep(8000);
    }
    return 0;
}

HttpResponse WinsockNetLayer::DoWinHttpRequest(const std::wstring& host, const std::wstring& path,
    const wchar_t* method, const std::string& body, const std::vector<std::wstring>& headers)
{
    HttpResponse resp; resp.status=0;
    HINTERNET hSess=WinHttpOpen(L"LCEN Client",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if (!hSess) return resp;
    HINTERNET hConn=WinHttpConnect(hSess,host.c_str(),443,0);
    if (!hConn){ WinHttpCloseHandle(hSess); return resp; }
    HINTERNET hReq=WinHttpOpenRequest(hConn,method,path.c_str(),NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);
    if (!hReq){ WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return resp; }
    for (const auto& h:headers) WinHttpAddRequestHeaders(hReq,h.c_str(),(DWORD)-1,WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE);
    BOOL ok=WinHttpSendRequest(hReq,WINHTTP_NO_ADDITIONAL_HEADERS,0,(LPVOID)body.c_str(),(DWORD)body.size(),(DWORD)body.size(),0);
    if (ok) ok=WinHttpReceiveResponse(hReq,NULL);
    if (ok)
    {
        DWORD st=0,sz=sizeof(st);
        WinHttpQueryHeaders(hReq,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&st,&sz,WINHTTP_NO_HEADER_INDEX);
        resp.status=(int)st;
        DWORD dw=0;
        do
        {
            if (!WinHttpQueryDataAvailable(hReq,&dw)||dw==0) break;
            char* b=new char[dw+1]; DWORD rd=0;
            if (WinHttpReadData(hReq,b,dw,&rd)) resp.body.append(b,rd);
            delete[] b;
        } while (dw>0);
    }
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess);
    return resp;
}

#endif