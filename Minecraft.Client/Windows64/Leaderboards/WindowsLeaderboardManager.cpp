#include "stdafx.h"

#include "WindowsLeaderboardManager.h"

#include <algorithm>
#include <vector>
#include <thread>

#include "../Network/json.hpp"
using json = nlohmann::json;

#include "../../Minecraft.h"
#include "../../StatsCounter.h"
#ifndef MINECRAFT_SERVER_BUILD
#include "../Windows64_Launcher.h"
#endif
#include "../Windows64_Minecraft.h"
#include "../Network/WinsockNetLayer.h"

#include "../../../Minecraft.World/Stats.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/net.minecraft.world.item.h"
#include "../../../Minecraft.World/net.minecraft.world.level.tile.h"

wchar_t g_Win64LeaderboardURL[256] = L"leaderboard.mclegacyedition.xyz";

namespace
{
    static int ClampDifficulty(int d)
    {
        if (d < 0) return 0;
        if (d > 3) return 3;
        return d;
    }

    static unsigned long ClampToULong(unsigned long long v)
    {
        if (v > static_cast<unsigned long long>(ULONG_MAX)) return ULONG_MAX;
        return static_cast<unsigned long>(v);
    }

    static unsigned int CombinePigmanKills(StatsCounter* stats, unsigned int difficulty)
    {
        if (!stats) return 0;
        return stats->getValue(Stats::killsZombiePigman, difficulty) + stats->getValue(Stats::killsNetherZombiePigman, difficulty);
    }

    static PlayerUID HexToUid(const std::string& hex)
    {
        if (hex.empty()) return INVALID_XUID;
        unsigned long long raw = 0;
        sscanf_s(hex.c_str(), "%llX", &raw);
        return static_cast<PlayerUID>(raw);
    }

    static int ParseServerEntries(const std::string& jsonText,
        std::vector<LeaderboardManager::ReadScore>& outScores)
    {
        if (jsonText.empty()) return 0;
        int serverTotal = 0;
        try
        {
            json root = json::parse(jsonText);

            if (root.contains("total") && root["total"].is_number())
                serverTotal = root["total"].get<int>();

            json arr = root.contains("entries") ? root["entries"] : root;
            if (!arr.is_array()) return 0;

            for (auto& entry : arr)
            {
                LeaderboardManager::ReadScore score = {};
                score.m_rank = 1;
                score.m_idsErrorMessage = 0;

                if (entry.contains("uuid") && entry["uuid"].is_string())
                    score.m_uid = HexToUid(entry["uuid"].get<std::string>());

                if (entry.contains("username") && entry["username"].is_string())
                    score.m_name = convStringToWstring(entry["username"].get<std::string>());

                if (entry.contains("totalScore") && entry["totalScore"].is_number())
                    score.m_totalScore = ClampToULong(
                        static_cast<unsigned long long>(entry["totalScore"].get<double>()));

                if (entry.contains("stats") && entry["stats"].is_array())
                {
                    auto& sa = entry["stats"];
                    unsigned int count = static_cast<unsigned int>(
                        (std::min<size_t>)(sa.size(), LeaderboardManager::ReadScore::STATSDATA_MAX));
                    score.m_statsSize = static_cast<unsigned short>(count);
                    for (unsigned int i = 0; i < count; ++i)
                        if (sa[i].is_number())
                            score.m_statsData[i] = static_cast<unsigned long>(sa[i].get<double>());
                }

                outScores.push_back(score);
            }
        }
        catch (...) {}
        return serverTotal;
    }

    static void AssignRanks(std::vector<LeaderboardManager::ReadScore>& scores, unsigned int offset)
    {
        for (size_t i = 0; i < scores.size(); ++i)
            scores[i].m_rank = static_cast<unsigned long>(offset + i + 1);
    }

    static bool BuildLocalScore(LeaderboardManager::ReadScore& out,
                                StatsCounter* stats,
                                LeaderboardManager::EStatsType type,
                                unsigned int diff)
    {
        ZeroMemory(&out, sizeof(out));
        out.m_rank            = 1;
        out.m_idsErrorMessage = 0;

        unsigned long long total = 0;
        auto col = [&](unsigned int idx, unsigned int val)
        {
            if (idx >= LeaderboardManager::ReadScore::STATSDATA_MAX) return;
            out.m_statsData[idx] = val;
            total += val;
        };

        switch (type)
        {
        case LeaderboardManager::eStatsType_Travelling:
            out.m_statsSize = 4;
            col(0, stats->getValue(Stats::walkOneM,     diff));
            col(1, stats->getValue(Stats::fallOneM,     diff));
            col(2, stats->getValue(Stats::minecartOneM, diff));
            col(3, stats->getValue(Stats::boatOneM,     diff));
            break;
        case LeaderboardManager::eStatsType_Mining:
            out.m_statsSize = 7;
            col(0, stats->getValue(Stats::blocksMined[Tile::dirt_Id],        diff));
            col(1, stats->getValue(Stats::blocksMined[Tile::cobblestone_Id], diff));
            col(2, stats->getValue(Stats::blocksMined[Tile::sand_Id],        diff));
            col(3, stats->getValue(Stats::blocksMined[Tile::stone_Id],       diff));
            col(4, stats->getValue(Stats::blocksMined[Tile::gravel_Id],      diff));
            col(5, stats->getValue(Stats::blocksMined[Tile::clay_Id],        diff));
            col(6, stats->getValue(Stats::blocksMined[Tile::obsidian_Id],    diff));
            break;
        case LeaderboardManager::eStatsType_Farming:
            out.m_statsSize = 6;
            col(0, stats->getValue(Stats::itemsCollected[Item::egg_Id],        diff));
            col(1, stats->getValue(Stats::blocksMined[Tile::wheat_Id],          diff));
            col(2, stats->getValue(Stats::blocksMined[Tile::mushroom_brown_Id], diff));
            col(3, stats->getValue(Stats::blocksMined[Tile::reeds_Id],          diff));
            col(4, stats->getValue(Stats::cowsMilked,                           diff));
            col(5, stats->getValue(Stats::itemsCollected[Tile::pumpkin_Id],     diff));
            break;
        case LeaderboardManager::eStatsType_Kills:
            out.m_statsSize = 7;
            col(0, stats->getValue(Stats::killsZombie,       diff));
            col(1, stats->getValue(Stats::killsSkeleton,     diff));
            col(2, stats->getValue(Stats::killsCreeper,      diff));
            col(3, stats->getValue(Stats::killsSpider,       diff));
            col(4, stats->getValue(Stats::killsSpiderJockey, diff));
            col(5, CombinePigmanKills(stats,                 diff));
            col(6, stats->getValue(Stats::killsSlime,        diff));
            break;
        default:
            return false;
        }

        out.m_totalScore = ClampToULong(total);
        return true;
    }
}

LeaderboardManager* LeaderboardManager::m_instance = new WindowsLeaderboardManager();

WindowsLeaderboardManager::WindowsLeaderboardManager()
    : m_eStatsState(eStatsState_Idle)
    , m_tickCount(0)
{
}

bool WindowsLeaderboardManager::isIdle()
{
    return m_eStatsState == eStatsState_Idle;
}

void WindowsLeaderboardManager::CancelOperation()
{
    m_eStatsState = eStatsState_Idle;
}

bool WindowsLeaderboardManager::WriteStats(unsigned int viewCount, ViewIn views)
{
    return false;
}

void WindowsLeaderboardManager::Tick()
{
    if (++m_tickCount < 1000) return;
    m_tickCount = 0;

    std::thread([this]()
    {
        static const EStatsType types[] = {
            eStatsType_Travelling,
            eStatsType_Mining,
            eStatsType_Farming,
            eStatsType_Kills,
        };
        for (EStatsType t : types)
            for (int diff = 0; diff <= 3; ++diff)
                SendStats(t, diff);
    }).detach();
}


bool WindowsLeaderboardManager::SendStats(EStatsType type, int diff)
{
    #ifndef MINECRAFT_SERVER_BUILD
    if (!Windows64Launcher::IsInOfflineMode() || Windows64Minecraft::IsOfflineMode())
    {
        Minecraft* mc = Minecraft::GetInstance();
        const int pad = ProfileManager.GetPrimaryPad();
        if (mc && pad >= 0 && pad < XUSER_MAX_COUNT)
        {
            StatsCounter* stats = mc->stats[pad];
            if (stats)
            {
                LeaderboardManager::ReadScore localScore = {};
                if (BuildLocalScore(localScore, stats, type, diff))
                {
                    json statsArr = json::array();
                    for (unsigned int i = 0; i < localScore.m_statsSize; ++i)
                        statsArr.push_back(localScore.m_statsData[i]);

                    json submitBody;
                    submitBody["type"]       = static_cast<int>(type);
                    submitBody["difficulty"] = static_cast<int>(diff);
                    submitBody["stats"]      = statsArr;
                    submitBody["totalScore"] = static_cast<unsigned long long>(localScore.m_totalScore);

                    sendScores(submitBody.dump());
                }
            }
        }
    }
    #endif
    return false;
}

bool WindowsLeaderboardManager::ReadStats_Friends(LeaderboardReadListener* callback,
    int difficulty, EStatsType type, PlayerUID myUID,
    unsigned int startIndex, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_Friends(callback, difficulty, type, startIndex, myUID, readCount))
        return false;
    return ReadNetworkStats(callback, difficulty, type, eFM_Friends, startIndex, readCount);
}

bool WindowsLeaderboardManager::ReadStats_MyScore(LeaderboardReadListener* callback,
    int difficulty, EStatsType type, PlayerUID myUID, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_MyScore(callback, difficulty, type, myUID, readCount))
        return false;
    return ReadNetworkStats(callback, difficulty, type, eFM_MyScore, 1, readCount);
}

bool WindowsLeaderboardManager::ReadStats_TopRank(LeaderboardReadListener* callback,
    int difficulty, EStatsType type,
    unsigned int startIndex, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_TopRank(callback, difficulty, type, startIndex, readCount))
        return false;
    return ReadNetworkStats(callback, difficulty, type, eFM_TopRank, startIndex, readCount);
}

bool WindowsLeaderboardManager::ReadNetworkStats(LeaderboardReadListener* callback,
    int difficulty, EStatsType type,
    EFilterMode filterMode,
    unsigned int startIndex, unsigned int readCount)
{
    if (!callback) return false;
    if (m_eStatsState != eStatsState_Idle) return false;

    unsigned int diff = static_cast<unsigned int>(ClampDifficulty(difficulty));
    if (type == eStatsType_Kills && diff == 0) diff = 1;

    m_eStatsState = eStatsState_Getting;

    unsigned int offset = (startIndex > 1) ? (startIndex - 1) : 0;
    unsigned int limit  = (readCount > 0) ? readCount : 15u;

    std::thread([this, callback, type, diff, filterMode, offset, limit]()
    {
        #ifndef MINECRAFT_SERVER_BUILD
        if (!Windows64Launcher::IsInOfflineMode() || Windows64Minecraft::IsOfflineMode())
            SendStats(static_cast<EStatsType>(type), static_cast<int>(diff));
        #endif

        std::vector<LeaderboardManager::ReadScore> rows;
        int serverTotal = 0;

        if (filterMode == eFM_Friends)
            serverTotal = ParseServerEntries(fetchFriends(type, diff, offset, limit), rows);
        else if (filterMode == eFM_MyScore)
            serverTotal = ParseServerEntries(fetchMyscore(type, diff, offset, limit), rows);
        else
            serverTotal = ParseServerEntries(fetchOverall(type, diff, offset, limit), rows);

        if (m_eStatsState == eStatsState_Canceled)
            return;

        AssignRanks(rows, offset);

        int totalResults;
        if (filterMode == eFM_MyScore)
            totalResults = static_cast<int>(rows.size());
        else if (serverTotal > 0)
            totalResults = serverTotal;
        else
            totalResults = static_cast<int>(rows.size());

        if (rows.empty())
        {
            m_eStatsState = eStatsState_Idle;
            ReadView empty = {};
            callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, empty);
            return;
        }

        ReadView view = {};
        view.m_numQueries = static_cast<unsigned int>(rows.size());
        view.m_queries    = rows.data();

        m_eStatsState = eStatsState_Idle;
        callback->OnStatsReadComplete(eStatsReturn_Success, totalResults, view);
    }).detach();

    return true;
}
int WindowsLeaderboardManager::sendScores(const std::string& _data)
{
    std::vector<std::wstring> headers;
    std::string authToken;
    #ifndef MINECRAFT_SERVER_BUILD
    if (Windows64Minecraft::IsExternalLauncher())
        authToken = Windows64Minecraft::GetAuthenticationTicket();
    else
        authToken = Windows64Launcher::GetAuthenticationToken();
    #endif
    headers.push_back(L"Content-Type: application/json");
    headers.push_back(L"Authorization: " + std::wstring(authToken.begin(), authToken.end()));

    HttpResponse response = WinsockNetLayer::DoWinHttpRequest(g_Win64LeaderboardURL, L"/sendscores", L"POST", _data, headers);
    return response.status;
}

std::string WindowsLeaderboardManager::fetchOverall(int type, int difficulty, int offset, int limit)
{
    std::vector<std::wstring> headers;
    std::wstring path = L"/overall?type=" + std::to_wstring(type)
                      + L"&difficulty=" + std::to_wstring(difficulty)
                      + L"&offset=" + std::to_wstring(offset)
                      + L"&limit=" + std::to_wstring(limit);
    HttpResponse response = WinsockNetLayer::DoWinHttpRequest(g_Win64LeaderboardURL, path, L"GET", "", headers);
    return response.body;
}

std::string WindowsLeaderboardManager::fetchFriends(int type, int difficulty, int offset, int limit)
{
    std::vector<std::wstring> headers;
    std::string authToken;
    #ifndef MINECRAFT_SERVER_BUILD
    if (Windows64Minecraft::IsExternalLauncher())
        authToken = Windows64Minecraft::GetAuthenticationTicket();
    else
        authToken = Windows64Launcher::GetAuthenticationToken();
    #endif
    headers.push_back(L"Authorization: " + std::wstring(authToken.begin(), authToken.end()));
    std::wstring path = L"/friends?type=" + std::to_wstring(type)
                      + L"&difficulty=" + std::to_wstring(difficulty)
                      + L"&offset=" + std::to_wstring(offset)
                      + L"&limit=" + std::to_wstring(limit);
    HttpResponse response = WinsockNetLayer::DoWinHttpRequest(g_Win64LeaderboardURL, path, L"GET", "", headers);
    return response.body;
}

std::string WindowsLeaderboardManager::fetchMyscore(int type, int difficulty, int offset, int limit)
{
    std::vector<std::wstring> headers;
    std::string authToken;
    #ifndef MINECRAFT_SERVER_BUILD
    if (Windows64Minecraft::IsExternalLauncher())
        authToken = Windows64Minecraft::GetAuthenticationTicket();
    else
        authToken = Windows64Launcher::GetAuthenticationToken();
    #endif
    headers.push_back(L"Authorization: " + std::wstring(authToken.begin(), authToken.end()));
    std::wstring path = L"/myscore?type=" + std::to_wstring(type)
                      + L"&difficulty=" + std::to_wstring(difficulty)
                      + L"&offset=" + std::to_wstring(offset)
                      + L"&limit=" + std::to_wstring(limit);
    HttpResponse response = WinsockNetLayer::DoWinHttpRequest(g_Win64LeaderboardURL, path, L"GET", "", headers);
    return response.body;
}