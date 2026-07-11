#include "stdafx.h"
#include "../../../Minecraft.World/Socket.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "PlatformNetworkManagerStub.h"
#include "../../Xbox/Network/NetworkPlayerXbox.h"
#ifdef _WINDOWS64
#include "../../Windows64/Network/WinsockNetLayer.h"
#include "../../Windows64/Windows64_Xuid.h"
#include "../../Minecraft.h"
#include "../../User.h"
#include "../../MinecraftServer.h"
#include "../../PlayerList.h"
#include <iostream>
#endif

extern int  g_Win64RelayServerPort;
extern bool g_Win64LCENServer;

CPlatformNetworkManagerStub* g_pPlatformNetworkManager;

void CPlatformNetworkManagerStub::NotifyPlayerJoined(IQNetPlayer* pQNetPlayer)
{
	const char* pszDescription;

	bool createFakeSocket = false;
	bool localPlayer = false;

	NetworkPlayerXbox* networkPlayer = static_cast<NetworkPlayerXbox*>(addNetworkPlayer(pQNetPlayer));

	if (pQNetPlayer->IsLocal())
	{
		localPlayer = true;
		if (pQNetPlayer->IsHost())
		{
			pszDescription = "local host";
			m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
		}
		else
		{
			pszDescription = "local";
			createFakeSocket = true;
		}
	}
	else
	{
		if (pQNetPlayer->IsHost())
		{
			pszDescription = "remote host";
		}
		else
		{
			pszDescription = "remote";
			if (m_pIQNet->IsHost())
				createFakeSocket = true;
		}

		if (m_pIQNet->IsHost() && !m_bHostChanged)
		{
			bool systemHasPrimaryPlayer = false;
			for (auto& pQNetPrimaryPlayer : m_machineQNetPrimaryPlayers)
			{
				if (pQNetPlayer->IsSameSystem(pQNetPrimaryPlayer))
				{
					systemHasPrimaryPlayer = true;
					break;
				}
			}
			if (!systemHasPrimaryPlayer)
				m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
		}
	}

	g_NetworkManager.PlayerJoining(networkPlayer);

	if (createFakeSocket && !m_bHostChanged)
		g_NetworkManager.CreateSocket(networkPlayer, localPlayer);

	app.DebugPrintf("Player 0x%p \"%ls\" joined; %s; voice %i; camera %i.\n",
		pQNetPlayer,
		pQNetPlayer->GetGamertag(),
		pszDescription,
		pQNetPlayer->HasVoice(),
		pQNetPlayer->HasCamera());

	if (m_pIQNet->IsHost())
		SystemFlagAddPlayer(networkPlayer);

	for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
	{
		if (playerChangedCallback[idx] != nullptr)
			playerChangedCallback[idx](playerChangedCallbackParam[idx], networkPlayer, false);
	}

	if (m_pIQNet->GetState() == QNET_STATE_GAME_PLAY)
	{
		int localPlayerCount = 0;
		for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
		{
			if (m_pIQNet->GetLocalPlayerByUserIndex(idx) != nullptr) ++localPlayerCount;
		}
		m_lastPlayerEventTimeStart = app.getAppTime();
	}
}

void CPlatformNetworkManagerStub::NotifyPlayerLeaving(IQNetPlayer* pQNetPlayer)
{
	app.DebugPrintf("Player 0x%p \"%ls\" leaving.\n", pQNetPlayer, pQNetPlayer->GetGamertag());

	INetworkPlayer* networkPlayer = getNetworkPlayer(pQNetPlayer);
	if (networkPlayer == nullptr)
		return;

	Socket* socket = networkPlayer->GetSocket();
	if (socket != nullptr)
	{
		if (m_pIQNet->IsHost())
		{
			g_NetworkManager.CloseConnection(networkPlayer);
			socket->close(true);
		}
	}

	if (m_pIQNet->IsHost())
		SystemFlagRemovePlayer(networkPlayer);

	g_NetworkManager.PlayerLeaving(networkPlayer);

	for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
	{
		if (playerChangedCallback[idx] != nullptr)
			playerChangedCallback[idx](playerChangedCallbackParam[idx], networkPlayer, true);
	}

	removeNetworkPlayer(pQNetPlayer);
}

bool CPlatformNetworkManagerStub::Initialise(CGameNetworkManager* pGameNetworkManager, int flagIndexSize)
{
	m_pGameNetworkManager = pGameNetworkManager;
	m_flagIndexSize       = flagIndexSize;
	m_pIQNet              = new IQNet();
	g_pPlatformNetworkManager = this;

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
		playerChangedCallback[i] = nullptr;

	m_bLeavingGame    = false;
	m_bLeaveGameOnTick = false;
	m_bHostChanged    = false;

	m_bSearchResultsReady = false;
	m_bSearchPending      = false;
	m_bIsOfflineGame      = false;

#ifdef _WINDOWS64
	m_bJoinPending = false;
#endif

	m_pSearchParam            = nullptr;
	m_SessionsUpdatedCallback = nullptr;

	for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
	{
		m_searchResultsCount[i]       = 0;
		m_lastSearchStartTime[i]      = 0;
		m_pSearchResults[i]           = nullptr;
		m_pQoSResult[i]               = nullptr;
		m_pCurrentSearchResults[i]    = nullptr;
		m_pCurrentQoSResult[i]        = nullptr;
		m_currentSearchResultsCount[i] = 0;
	}

	return true;
}

void CPlatformNetworkManagerStub::Terminate() {}

int CPlatformNetworkManagerStub::GetJoiningReadyPercentage() { return 100; }
int CPlatformNetworkManagerStub::CorrectErrorIDS(int IDS)    { return IDS; }

bool CPlatformNetworkManagerStub::isSystemPrimaryPlayer(IQNetPlayer* pQNetPlayer) { return true; }

void CPlatformNetworkManagerStub::DoWork()
{
#ifdef _WINDOWS64
	extern QNET_STATE _iQNetStubState;

	if (_iQNetStubState == QNET_STATE_SESSION_STARTING && app.GetGameStarted())
	{
		_iQNetStubState = QNET_STATE_GAME_PLAY;
		if (m_pIQNet->IsHost())
			WinsockNetLayer::UpdateAdvertiseJoinable(true);
	}

	if (_iQNetStubState == QNET_STATE_IDLE)
		TickSearch();

	if (_iQNetStubState == QNET_STATE_GAME_PLAY && m_pIQNet->IsHost())
	{
		BYTE disconnectedSmallId;
		while (WinsockNetLayer::PopDisconnectedSmallId(&disconnectedSmallId))
		{
			IQNetPlayer* qnetPlayer = m_pIQNet->GetPlayerBySmallId(disconnectedSmallId);
			if (qnetPlayer != nullptr && qnetPlayer->m_smallId == disconnectedSmallId)
			{
				NotifyPlayerLeaving(qnetPlayer);
				qnetPlayer->m_smallId        = 0;
				qnetPlayer->m_isRemote       = false;
				qnetPlayer->m_isHostPlayer   = false;
				qnetPlayer->m_resolvedXuid   = INVALID_XUID;
				qnetPlayer->m_gamertag[0]    = 0;
				qnetPlayer->SetCustomDataValue(0);

				// Recalculate s_playerCount: scan from the top, drop empty trailing slots.
				while (IQNet::s_playerCount > 1 &&
					   IQNet::m_player[IQNet::s_playerCount - 1].GetCustomDataValue() == 0)
					IQNet::s_playerCount--;
			}
			SystemFlagRemoveBySmallId(disconnectedSmallId);
		}
	}

	// Client-side host disconnect detection.
	if (_iQNetStubState == QNET_STATE_GAME_PLAY && !m_pIQNet->IsHost() && !m_bLeavingGame)
	{
		if (!WinsockNetLayer::IsConnected())
		{
			if (!m_bLeaveGameOnTick)
			{
				m_bLeaveGameOnTick = true;
				g_NetworkManager.HandleDisconnect(false);
			}
		}
		else
		{
			m_bLeaveGameOnTick = false;
		}
	}

	// Async join finalization.
if (m_bJoinPending)
	{
		WinsockNetLayer::eJoinState state = WinsockNetLayer::GetJoinState();

		if (state == WinsockNetLayer::eJoinState_Success)
		{
			if (!WinsockNetLayer::FinalizeJoin())
			{
				app.DebugPrintf("DoWork: FinalizeJoin failed\n");
				m_bJoinPending = false;
				return;
			}

			BYTE localSmallId = WinsockNetLayer::GetLocalSmallId();
			if (localSmallId == 0 || localSmallId >= MINECRAFT_NET_MAX_PLAYERS)
			{
				app.DebugPrintf("DoWork: bad localSmallId=%d from FinalizeJoin\n", localSmallId);
				m_bJoinPending = false;
				return;
			}

			// Clear stale slots between host (0) and our slot so
			// GetPlayerBySmallId can't resolve them to ghost players.
			for (BYTE i = 1; i < localSmallId; i++)
			{
				IQNet::m_player[i].m_smallId      = 0xFF;
				IQNet::m_player[i].m_isRemote     = false;
				IQNet::m_player[i].m_isHostPlayer = false;
				IQNet::m_player[i].m_resolvedXuid = INVALID_XUID;
				IQNet::m_player[i].m_gamertag[0]  = 0;
				IQNet::m_player[i].SetCustomDataValue(0);
			}

			IQNet::m_player[localSmallId].m_smallId      = localSmallId;
			IQNet::m_player[localSmallId].m_isRemote     = false;
			IQNet::m_player[localSmallId].m_isHostPlayer = false;
			IQNet::m_player[localSmallId].m_resolvedXuid = Win64Xuid::ResolvePersistentXuid();

			Minecraft* pMinecraft = Minecraft::GetInstance();
			wcscpy_s(IQNet::m_player[localSmallId].m_gamertag, 32, pMinecraft->user->name.c_str());
			IQNet::s_playerCount = localSmallId + 1;

			// Register host slot then our local slot so the socket and player
			// objects exist before the recv thread dispatches any inbound packets.
			NotifyPlayerJoined(&IQNet::m_player[0]);
			NotifyPlayerJoined(&IQNet::m_player[localSmallId]);

			// MUST be called after NotifyPlayerJoined: recv thread needs the
			// INetworkPlayer sockets created above to route HandleDataReceived.
			WinsockNetLayer::StartClientRecvThread();

			m_pGameNetworkManager->StateChange_AnyToStarting();
			m_bJoinPending = false;
		}
		else if (state == WinsockNetLayer::eJoinState_Failed ||
		         state == WinsockNetLayer::eJoinState_Rejected ||
		         state == WinsockNetLayer::eJoinState_Cancelled)
		{
			if (state == WinsockNetLayer::eJoinState_Rejected)
			{
				DisconnectPacket::eDisconnectReason reason = WinsockNetLayer::GetJoinRejectReason();
				Minecraft::GetInstance()->connectionDisconnected(
					ProfileManager.GetPrimaryPad(), reason);
			}
			m_bJoinPending = false;
		}
	}
#endif
}

bool CPlatformNetworkManagerStub::CanAcceptMoreConnections()
{
#ifdef _WINDOWS64
	MinecraftServer* server = MinecraftServer::getInstance();
	if (server == NULL) return true;
	PlayerList* list = server->getPlayerList();
	if (list == NULL) return true;
	return static_cast<unsigned int>(list->players.size()) <
	       static_cast<unsigned int>(list->getMaxPlayers());
#else
	return true;
#endif
}

int  CPlatformNetworkManagerStub::GetPlayerCount()           { return m_pIQNet->GetPlayerCount(); }
bool CPlatformNetworkManagerStub::ShouldMessageForFullSession() { return false; }
int  CPlatformNetworkManagerStub::GetOnlinePlayerCount()     { return 1; }
int  CPlatformNetworkManagerStub::GetLocalPlayerMask(int playerIndex) { return 1 << playerIndex; }

bool CPlatformNetworkManagerStub::AddLocalPlayerByUserIndex(int userIndex)
{
	if (m_pIQNet->AddLocalPlayerByUserIndex(userIndex) != S_OK)
		return false;
	NotifyPlayerJoined(&IQNet::m_player[userIndex]);
	return true;
}

bool CPlatformNetworkManagerStub::RemoveLocalPlayerByUserIndex(int userIndex)
{
#ifdef _WINDOWS64
	if (userIndex > 0 && userIndex < XUSER_MAX_COUNT && !m_pIQNet->IsHost())
	{
		IQNetPlayer* qp = &IQNet::m_player[userIndex];
		if (qp->GetCustomDataValue() != 0)
			NotifyPlayerLeaving(qp);
		WinsockNetLayer::CloseSplitScreenConnection(userIndex);
		qp->m_smallId      = 0;
		qp->m_isRemote     = false;
		qp->m_isHostPlayer = false;
		qp->m_resolvedXuid = INVALID_XUID;
		qp->m_gamertag[0]  = 0;
		qp->SetCustomDataValue(0);
	}
#endif
	return true;
}

bool CPlatformNetworkManagerStub::IsInStatsEnabledSession() { return true; }
bool CPlatformNetworkManagerStub::SessionHasSpace(unsigned int spaceRequired) { return true; }
void CPlatformNetworkManagerStub::SendInviteGUI(int quadrant) {}
bool CPlatformNetworkManagerStub::IsAddingPlayer() { return false; }

bool CPlatformNetworkManagerStub::LeaveGame(bool bMigrateHost)
{
	if (m_bLeavingGame) return true;

	m_bLeavingGame     = true;
	m_bLeaveGameOnTick = false;

#ifdef _WINDOWS64
	WinsockNetLayer::StopAdvertising();
#endif

	if (m_pIQNet->IsHost() && g_NetworkManager.ServerStoppedValid())
	{
		m_pIQNet->EndGame();
		g_NetworkManager.ServerStoppedWait();
		g_NetworkManager.ServerStoppedDestroy();
	}
	else
	{
		m_pIQNet->EndGame();
	}

	for (auto& it : currentNetworkPlayers)
		delete it;
	currentNetworkPlayers.clear();
	m_machineQNetPrimaryPlayers.clear();
	SystemFlagReset();

#ifdef _WINDOWS64
	WinsockNetLayer::Shutdown();
	WinsockNetLayer::Initialize();
#endif

	return true;
}

bool CPlatformNetworkManagerStub::_LeaveGame(bool bMigrateHost, bool bLeaveRoom) { return true; }

void CPlatformNetworkManagerStub::HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
	unsigned char publicSlots, unsigned char privateSlots)
{
	SetLocalGame(!bOnlineGame);
	SetPrivateGame(bIsPrivate);
	SystemFlagReset();

	localUsersMask |= GetLocalPlayerMask(g_NetworkManager.GetPrimaryPad());

	m_bLeavingGame     = false;
	m_bLeaveGameOnTick = false;

	m_pIQNet->HostGame();

#ifdef _WINDOWS64
	IQNet::m_player[0].m_smallId      = 0;
	IQNet::m_player[0].m_isRemote     = false;
	IQNet::m_player[0].m_isHostPlayer = true;
	IQNet::m_player[0].m_resolvedXuid = Win64Xuid::GetLegacyEmbeddedHostXuid();
	IQNet::s_playerCount = 1;
#endif

	_HostGame(localUsersMask, publicSlots, privateSlots);

#ifdef _WINDOWS64
	int port = WIN64_NET_DEFAULT_PORT;
	const char* bindIp = nullptr;
	if (g_Win64DedicatedServer)
	{
		if (g_Win64DedicatedServerPort > 0)
			port = g_Win64DedicatedServerPort;
		if (g_Win64DedicatedServerBindIP[0] != 0)
			bindIp = g_Win64DedicatedServerBindIP;
	}

	if (!WinsockNetLayer::IsActive())
		WinsockNetLayer::HostGame(port, bindIp);

	if (WinsockNetLayer::IsActive())
	{
		bool enableLanAdvertising = !g_Win64DedicatedServer || g_Win64DedicatedServerLanAdvertise;

		if (enableLanAdvertising)
		{
			const wchar_t* hostName = IQNet::m_player[0].m_gamertag;
			unsigned int settings = app.GetGameHostOption(eGameHostOption_All);
			WinsockNetLayer::StartAdvertising(port, hostName, settings, 0, 0, MINECRAFT_NET_VERSION);
		}
		else
		{
			WinsockNetLayer::StopAdvertising();
		}
	}
#endif
}

void CPlatformNetworkManagerStub::_HostGame(int usersMask, unsigned char publicSlots, unsigned char privateSlots) {}
bool CPlatformNetworkManagerStub::_StartGame() { return true; }

int CPlatformNetworkManagerStub::JoinGame(FriendSessionInfo* searchResult, int localUsersMask, int primaryUserIndex)
{
#ifdef _WINDOWS64
	if (searchResult == nullptr)
		return CGameNetworkManager::JOINGAME_FAIL_GENERAL;

	const char* hostIP   = searchResult->data.hostIP;
	int         hostPort = searchResult->data.hostPort;

	if (hostPort <= 0 || hostIP[0] == 0)
		return CGameNetworkManager::JOINGAME_FAIL_GENERAL;

	m_bLeavingGame     = false;
	m_bLeaveGameOnTick = false;
	IQNet::s_isHosting = false;
	m_pIQNet->ClientJoinGame();

	IQNet::m_player[0].m_smallId      = 0;
	IQNet::m_player[0].m_isRemote     = true;
	IQNet::m_player[0].m_isHostPlayer = true;
	IQNet::m_player[0].m_resolvedXuid = Win64Xuid::GetLegacyEmbeddedHostXuid();
	wcsncpy_s(IQNet::m_player[0].m_gamertag, 32, searchResult->data.hostName, _TRUNCATE);

	WinsockNetLayer::StopDiscovery();

	// Detect whether this session comes from the LCEN relay or is a direct TCP connection.
	// Relay sessions have hostPort == g_Win64RelayServerPort (set in DiscoveryThreadProc).
	// Direct servers (servers.db entries, LAN) use their actual port.
	g_Win64LCENServer = (hostPort == g_Win64RelayServerPort);

	if (!WinsockNetLayer::BeginJoinGame(hostIP, hostPort))
	{
		app.DebugPrintf("Win64: Failed to start async join to %s:%d\n", hostIP, hostPort);
		return CGameNetworkManager::JOINGAME_FAIL_GENERAL;
	}

	m_bJoinPending = true;
	return CGameNetworkManager::JOINGAME_PENDING;
#else
	return CGameNetworkManager::JOINGAME_SUCCESS;
#endif
}

bool CPlatformNetworkManagerStub::SetLocalGame(bool isLocal)
{
	m_bIsOfflineGame = isLocal;
	return true;
}

void CPlatformNetworkManagerStub::SetPrivateGame(bool isPrivate)
{
	app.DebugPrintf("Setting as private game: %s\n", isPrivate ? "yes" : "no");
	m_bIsPrivateGame = isPrivate;
}

void CPlatformNetworkManagerStub::RegisterPlayerChangedCallback(int iPad,
	void (*callback)(void*, INetworkPlayer*, bool), void* callbackParam)
{
	playerChangedCallback[iPad]      = callback;
	playerChangedCallbackParam[iPad] = callbackParam;
}

void CPlatformNetworkManagerStub::UnRegisterPlayerChangedCallback(int iPad,
	void (*callback)(void*, INetworkPlayer*, bool), void* callbackParam)
{
	if (playerChangedCallbackParam[iPad] == callbackParam)
	{
		playerChangedCallback[iPad]      = nullptr;
		playerChangedCallbackParam[iPad] = nullptr;
	}
}

void CPlatformNetworkManagerStub::HandleSignInChange() {}

bool CPlatformNetworkManagerStub::_RunNetworkGame()
{
#ifdef _WINDOWS64
	extern QNET_STATE _iQNetStubState;
	_iQNetStubState = QNET_STATE_GAME_PLAY;

	for (DWORD i = 0; i < IQNet::s_playerCount; i++)
	{
		if (IQNet::m_player[i].m_isRemote)
		{
			INetworkPlayer* pNetworkPlayer = getNetworkPlayer(&IQNet::m_player[i]);
			if (pNetworkPlayer != nullptr && pNetworkPlayer->GetSocket() != nullptr)
				Socket::addIncomingSocket(pNetworkPlayer->GetSocket());
		}
	}
#endif
	return true;
}

void CPlatformNetworkManagerStub::UpdateAndSetGameSessionData(INetworkPlayer* pNetworkPlayerLeaving) {}

int CPlatformNetworkManagerStub::RemovePlayerOnSocketClosedThreadProc(void* lpParam)
{
	INetworkPlayer* pNetworkPlayer = static_cast<INetworkPlayer*>(lpParam);
	Socket* socket = pNetworkPlayer->GetSocket();
	if (socket != nullptr)
	{
		socket->m_socketClosedEvent->WaitForSignal(INFINITE);
		pNetworkPlayer->SetSocket(nullptr);
		delete socket;
	}
	return g_pPlatformNetworkManager->RemoveLocalPlayer(pNetworkPlayer);
}

bool CPlatformNetworkManagerStub::RemoveLocalPlayer(INetworkPlayer* pNetworkPlayer) { return true; }

CPlatformNetworkManagerStub::PlayerFlags::PlayerFlags(INetworkPlayer* pNetworkPlayer, unsigned int count)
{
	count = (count + 8 - 1) & ~(8 - 1);
	this->m_pNetworkPlayer = pNetworkPlayer;
	this->flags            = new unsigned char[count / 8];
	memset(this->flags, 0, count / 8);
	this->count   = count;
	this->m_smallId = (pNetworkPlayer && pNetworkPlayer->IsLocal()) ? 256 :
	                  (pNetworkPlayer ? static_cast<int>(pNetworkPlayer->GetSmallId()) : -1);
}

CPlatformNetworkManagerStub::PlayerFlags::~PlayerFlags()
{
	delete[] flags;
}

void CPlatformNetworkManagerStub::SystemFlagAddPlayer(INetworkPlayer* pNetworkPlayer)
{
	PlayerFlags* newPlayerFlags = new PlayerFlags(pNetworkPlayer, m_flagIndexSize);
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
	{
		if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer))
		{
			memcpy(newPlayerFlags->flags, m_playerFlags[i]->flags, m_playerFlags[i]->count / 8);
			break;
		}
	}
	m_playerFlags.push_back(newPlayerFlags);
}

void CPlatformNetworkManagerStub::SystemFlagRemovePlayer(INetworkPlayer* pNetworkPlayer)
{
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
	{
		if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer)
		{
			delete m_playerFlags[i];
			m_playerFlags[i] = m_playerFlags.back();
			m_playerFlags.pop_back();
			return;
		}
	}
}

void CPlatformNetworkManagerStub::SystemFlagRemoveBySmallId(int smallId)
{
	if (smallId < 0) return;
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
	{
		if (m_playerFlags[i]->m_smallId == smallId)
		{
			delete m_playerFlags[i];
			m_playerFlags[i] = m_playerFlags.back();
			m_playerFlags.pop_back();
			return;
		}
	}
}

void CPlatformNetworkManagerStub::SystemFlagReset()
{
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
		delete m_playerFlags[i];
	m_playerFlags.clear();
}

void CPlatformNetworkManagerStub::SystemFlagSet(INetworkPlayer* pNetworkPlayer, int index)
{
	if (index < 0 || index >= m_flagIndexSize || pNetworkPlayer == nullptr) return;
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
	{
		if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer))
			m_playerFlags[i]->flags[index / 8] |= (128 >> (index % 8));
	}
}

bool CPlatformNetworkManagerStub::SystemFlagGet(INetworkPlayer* pNetworkPlayer, int index)
{
	if (index < 0 || index >= m_flagIndexSize || pNetworkPlayer == nullptr) return false;
	for (unsigned int i = 0; i < m_playerFlags.size(); i++)
	{
		if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer)
			return ((m_playerFlags[i]->flags[index / 8] & (128 >> (index % 8))) != 0);
	}
	return false;
}

wstring CPlatformNetworkManagerStub::GatherStats() { return L""; }

wstring CPlatformNetworkManagerStub::GatherRTTStats()
{
	wstring stats(L"Rtt: ");
	wchar_t stat[32];
	for (unsigned int i = 0; i < GetPlayerCount(); ++i)
	{
		IQNetPlayer* pQNetPlayer = static_cast<NetworkPlayerXbox*>(GetPlayerByIndex(i))->GetQNetPlayer();
		if (!pQNetPlayer->IsLocal())
		{
			ZeroMemory(stat, 32 * sizeof(WCHAR));
			swprintf(stat, 32, L"%d: %d/", i, pQNetPlayer->GetCurrentRtt());
			stats.append(stat);
		}
	}
	return stats;
}

void CPlatformNetworkManagerStub::TickSearch()
{
#ifdef _WINDOWS64
	if (m_SessionsUpdatedCallback == nullptr) return;

	static DWORD lastSearchTime = 0;
	DWORD now = GetTickCount();
	if (now - lastSearchTime < 2000) return;
	lastSearchTime = now;

	SearchForGames();
#endif
}

void CPlatformNetworkManagerStub::SearchForGames()
{
#ifdef _WINDOWS64
	std::vector<Win64LANSession> lanSessions = WinsockNetLayer::GetDiscoveredSessions();

	for (size_t i = 0; i < friendsSessions[0].size(); i++)
		delete friendsSessions[0][i];
	friendsSessions[0].clear();

	// Add sessions discovered via LAN broadcast or relay.
	for (size_t i = 0; i < lanSessions.size(); i++)
	{
		FriendSessionInfo* info = new FriendSessionInfo();
		size_t nameLen = wcslen(lanSessions[i].hostName);
		info->displayLabel = new wchar_t[nameLen + 1];
		wcscpy_s(info->displayLabel, nameLen + 1, lanSessions[i].hostName);
		info->displayLabelLength             = static_cast<unsigned char>(nameLen);
		info->displayLabelViewableStartIndex = 0;

		info->data.netVersion           = lanSessions[i].netVersion;
		info->data.m_uiGameHostSettings = lanSessions[i].gameHostSettings;
		info->data.texturePackParentId  = lanSessions[i].texturePackParentId;
		info->data.subTexturePackId     = lanSessions[i].subTexturePackId;
		info->data.isReadyToJoin        = lanSessions[i].isJoinable;
		info->data.isJoinable           = lanSessions[i].isJoinable;
		strncpy_s(info->data.hostIP, sizeof(info->data.hostIP), lanSessions[i].hostIP, _TRUNCATE);
		info->data.hostPort             = lanSessions[i].hostPort;
		wcsncpy_s(info->data.hostName, XUSER_NAME_SIZE, lanSessions[i].hostName, _TRUNCATE);
		info->data.playerCount          = lanSessions[i].playerCount;
		info->data.maxPlayers           = lanSessions[i].maxPlayers;

		// sessionId: for relay sessions hostIP is a non-numeric string so inet_addr
		// returns INADDR_NONE — that's fine, it just gives a unique-ish key.
		info->sessionId = static_cast<uint64_t>(inet_addr(lanSessions[i].hostIP)) |
		                  (static_cast<uint64_t>(lanSessions[i].hostPort) << 32);

		friendsSessions[0].push_back(info);
	}

	// Add servers from servers.db (manual / direct TCP entries).
	if (std::FILE* file = std::fopen("servers.db", "rb"))
	{
		char magic[4] = {};
		if (std::fread(magic, 1, 4, file) == 4 && memcmp(magic, "MCSV", 4) == 0)
		{
			uint32_t version = 0, count = 0;
			std::fread(&version, sizeof(uint32_t), 1, file);
			std::fread(&count,   sizeof(uint32_t), 1, file);

			if (version == 1)
			{
				for (uint32_t s = 0; s < count; s++)
				{
					uint16_t ipLen = 0, port = 0, nameLen = 0;
					if (std::fread(&ipLen,  sizeof(uint16_t), 1, file) != 1) break;
					if (ipLen == 0 || ipLen > 256) break;

					char ipBuf[257] = {};
					if (std::fread(ipBuf, 1, ipLen, file) != ipLen) break;
					if (std::fread(&port, sizeof(uint16_t), 1, file) != 1) break;

					if (std::fread(&nameLen, sizeof(uint16_t), 1, file) != 1) break;
					if (nameLen > 256) break;

					char nameBuf[257] = {};
					if (nameLen > 0)
					{
						if (std::fread(nameBuf, 1, nameLen, file) != nameLen) break;
					}

					wstring wName = convStringToWstring(nameBuf);

					FriendSessionInfo* info = new FriendSessionInfo();
					size_t nLen = wName.length();
					info->displayLabel = new wchar_t[nLen + 1];
					wcscpy_s(info->displayLabel, nLen + 1, wName.c_str());
					info->displayLabelLength             = static_cast<unsigned char>(nLen);
					info->displayLabelViewableStartIndex = 0;
					info->data.isReadyToJoin = true;
					info->data.isJoinable    = true;
					strncpy_s(info->data.hostIP, sizeof(info->data.hostIP), ipBuf, _TRUNCATE);
					info->data.hostPort = port;
					// Direct servers: sessionId uses real IP + port.
					info->sessionId = static_cast<uint64_t>(inet_addr(ipBuf)) |
					                  (static_cast<uint64_t>(port) << 32);
					friendsSessions[0].push_back(info);
				}
			}
		}
		std::fclose(file);
	}

	m_searchResultsCount[0] = static_cast<int>(friendsSessions[0].size());

	if (m_SessionsUpdatedCallback != nullptr)
		m_SessionsUpdatedCallback(m_pSearchParam);
#endif
}

int CPlatformNetworkManagerStub::SearchForGamesThreadProc(void* lpParameter) { return 0; }

void CPlatformNetworkManagerStub::SetSearchResultsReady(int resultCount)
{
	m_bSearchResultsReady = true;
	m_searchResultsCount[m_lastSearchPad] = resultCount;
}

vector<FriendSessionInfo*>* CPlatformNetworkManagerStub::GetSessionList(int iPad, int localPlayers, bool partyOnly)
{
	vector<FriendSessionInfo*>* filteredList = new vector<FriendSessionInfo*>();
	for (size_t i = 0; i < friendsSessions[0].size(); i++)
		filteredList->push_back(new FriendSessionInfo(*friendsSessions[0][i]));
	return filteredList;
}

bool CPlatformNetworkManagerStub::GetGameSessionInfo(int iPad, SessionID sessionId, FriendSessionInfo* foundSessionInfo)
{
	return false;
}

void CPlatformNetworkManagerStub::SetSessionsUpdatedCallback(
	void (*SessionsUpdatedCallback)(LPVOID pParam), LPVOID pSearchParam)
{
	m_SessionsUpdatedCallback = SessionsUpdatedCallback;
	m_pSearchParam            = pSearchParam;
}

void CPlatformNetworkManagerStub::GetFullFriendSessionInfo(
	FriendSessionInfo* foundSession,
	void (*FriendSessionUpdatedFn)(bool success, void* pParam),
	void* pParam)
{
	FriendSessionUpdatedFn(true, pParam);
}

void CPlatformNetworkManagerStub::ForceFriendsSessionRefresh()
{
	app.DebugPrintf("Resetting friends session search data\n");
	for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
	{
		m_searchResultsCount[i]  = 0;
		m_lastSearchStartTime[i] = 0;
		delete m_pSearchResults[i];
		m_pSearchResults[i] = nullptr;
	}
#ifdef _WINDOWS64
	SearchForGames();
#endif
}

INetworkPlayer* CPlatformNetworkManagerStub::addNetworkPlayer(IQNetPlayer* pQNetPlayer)
{
	NetworkPlayerXbox* pNetworkPlayer = new NetworkPlayerXbox(pQNetPlayer);
	pQNetPlayer->SetCustomDataValue((ULONG_PTR)pNetworkPlayer);
	currentNetworkPlayers.push_back(pNetworkPlayer);
	return pNetworkPlayer;
}

void CPlatformNetworkManagerStub::removeNetworkPlayer(IQNetPlayer* pQNetPlayer)
{
	INetworkPlayer* pNetworkPlayer = getNetworkPlayer(pQNetPlayer);
	for (auto it = currentNetworkPlayers.begin(); it != currentNetworkPlayers.end(); it++)
	{
		if (*it == pNetworkPlayer)
		{
			currentNetworkPlayers.erase(it);
			return;
		}
	}
}

INetworkPlayer* CPlatformNetworkManagerStub::getNetworkPlayer(IQNetPlayer* pQNetPlayer)
{
	return pQNetPlayer ? (INetworkPlayer*)(pQNetPlayer->GetCustomDataValue()) : nullptr;
}

INetworkPlayer* CPlatformNetworkManagerStub::GetLocalPlayerByUserIndex(int userIndex)
{
	return getNetworkPlayer(m_pIQNet->GetLocalPlayerByUserIndex(userIndex));
}

INetworkPlayer* CPlatformNetworkManagerStub::GetPlayerByIndex(int playerIndex)
{
	return getNetworkPlayer(m_pIQNet->GetPlayerByIndex(playerIndex));
}

INetworkPlayer* CPlatformNetworkManagerStub::GetPlayerByXuid(PlayerUID xuid)
{
	return getNetworkPlayer(m_pIQNet->GetPlayerByXuid(xuid));
}

INetworkPlayer* CPlatformNetworkManagerStub::GetPlayerBySmallId(unsigned char smallId)
{
	return getNetworkPlayer(m_pIQNet->GetPlayerBySmallId(smallId));
}

INetworkPlayer* CPlatformNetworkManagerStub::GetHostPlayer()
{
	return getNetworkPlayer(m_pIQNet->GetHostPlayer());
}

bool CPlatformNetworkManagerStub::IsHost()
{
	return m_pIQNet->IsHost() && !m_bHostChanged;
}

bool CPlatformNetworkManagerStub::JoinGameFromInviteInfo(int userIndex, int userMask, const INVITE_INFO* pInviteInfo)
{
	return (m_pIQNet->JoinGameFromInviteInfo(userIndex, userMask, pInviteInfo) == S_OK);
}

void CPlatformNetworkManagerStub::SetSessionTexturePackParentId(int id) { m_hostGameSessionData.texturePackParentId = id; }
void CPlatformNetworkManagerStub::SetSessionSubTexturePackId(int id)    { m_hostGameSessionData.subTexturePackId    = id; }
void CPlatformNetworkManagerStub::Notify(int ID, ULONG_PTR Param)       {}

bool CPlatformNetworkManagerStub::IsInSession()
{
	return m_pIQNet->GetState() != QNET_STATE_IDLE;
}

bool CPlatformNetworkManagerStub::IsInGameplay()
{
	return m_pIQNet->GetState() == QNET_STATE_GAME_PLAY;
}

bool CPlatformNetworkManagerStub::IsReadyToPlayOrIdle() { return true; }