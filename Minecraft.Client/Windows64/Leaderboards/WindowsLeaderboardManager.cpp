#include "stdafx.h"
#include "WindowsLeaderboardManager.h"
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

#include "../json.hpp"
using json = nlohmann::json;

#include "../../Minecraft.h"
#include "../../StatsCounter.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/Stats.h"
#include "../../../Minecraft.World/net.minecraft.world.item.h"
#include "../../../Minecraft.World/net.minecraft.world.level.tile.h"

extern char g_LCENToken[512];

namespace
{
    static const char* kBaseUrl = "https://network-server-7kuc.onrender.com";

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

    static std::string UidToHex(PlayerUID uid)
    {
        unsigned long long raw = static_cast<unsigned long long>(uid);
        char buf[17] = {};
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%016llX", raw);
        return std::string(buf);
    }

    static PlayerUID HexToUid(const std::string& hex)
    {
        if (hex.empty()) return INVALID_XUID;
        unsigned long long raw = 0;
        sscanf_s(hex.c_str(), "%llX", &raw);
        return static_cast<PlayerUID>(raw);
    }

    static unsigned int CombinePigmanKills(StatsCounter* stats, unsigned int difficulty)
    {
        if (!stats) return 0;
        return stats->getValue(Stats::killsZombiePigman, difficulty)
             + stats->getValue(Stats::killsNetherZombiePigman, difficulty);
    }

    static std::string WstrToUtf8(const std::wstring& w)
    {
        if (w.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1) return {};
        std::string s(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], len, nullptr, nullptr);
        return s;
    }

    static std::string HttpRequest(const std::string& method,
                                   const std::string& url,
                                   const std::string& body = "")
    {
        std::string result;

        HINTERNET hNet = InternetOpenA("openLCE", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hNet) return result;

        std::string headers;
        if (g_LCENToken[0] != '\0')
        {
            headers  = "Authorization: Bearer ";
            headers += g_LCENToken;
            headers += "\r\n";
        }

        DWORD flags = INTERNET_FLAG_RELOAD
                    | INTERNET_FLAG_NO_CACHE_WRITE
                    | INTERNET_FLAG_SECURE
                    | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
                    | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

        HINTERNET hReq = NULL;

        if (method == "POST")
        {
            headers += "Content-Type: application/json\r\n";

            URL_COMPONENTSA uc = {};
            uc.dwStructSize       = sizeof(uc);
            char host[256]  = {};
            char path[1024] = {};
            uc.lpszHostName     = host;
            uc.dwHostNameLength = sizeof(host);
            uc.lpszUrlPath      = path;
            uc.dwUrlPathLength  = sizeof(path);

            if (InternetCrackUrlA(url.c_str(), 0, 0, &uc))
            {
                HINTERNET hConn = InternetConnectA(
                    hNet, host,
                    uc.nPort ? uc.nPort : INTERNET_DEFAULT_HTTPS_PORT,
                    NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

                if (hConn)
                {
                    DWORD reqFlags = INTERNET_FLAG_RELOAD
                                   | INTERNET_FLAG_NO_CACHE_WRITE
                                   | INTERNET_FLAG_SECURE
                                   | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
                                   | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

                    hReq = HttpOpenRequestA(hConn, "POST", path, NULL, NULL, NULL, reqFlags, 0);
                    if (hReq)
                    {
                        HttpSendRequestA(hReq,
                            headers.c_str(), static_cast<DWORD>(headers.size()),
                            const_cast<char*>(body.c_str()), static_cast<DWORD>(body.size()));
                    }
                    InternetCloseHandle(hConn);
                }
            }
        }
        else
        {
            hReq = InternetOpenUrlA(hNet, url.c_str(),
                headers.empty() ? NULL : headers.c_str(),
                headers.empty() ? 0    : static_cast<DWORD>(headers.size()),
                flags, 0);
        }

        if (hReq)
        {
            char buf[4096] = {};
            DWORD read = 0;
            while (InternetReadFile(hReq, buf, sizeof(buf) - 1, &read) && read > 0)
            {
                buf[read] = '\0';
                result.append(buf, read);
                read = 0;
            }
            InternetCloseHandle(hReq);
        }

        InternetCloseHandle(hNet);
        return result;
    }

    static std::string HttpGet(const std::string& url)
    {
        return HttpRequest("GET", url);
    }

    static std::string HttpPost(const std::string& url, const std::string& jsonBody)
    {
        return HttpRequest("POST", url, jsonBody);
    }

    static void ParseServerEntries(const std::string& jsonText,
                                   std::vector<LeaderboardManager::ReadScore>& outScores)
    {
        if (jsonText.empty()) return;
        try
        {
            json arr = json::parse(jsonText);
            if (!arr.is_array()) return;
            for (auto& entry : arr)
            {
                LeaderboardManager::ReadScore score = {};
                score.m_rank            = 1;
                score.m_idsErrorMessage = 0;

                if (entry.contains("uid") && entry["uid"].is_string())
                    score.m_uid = HexToUid(entry["uid"].get<std::string>());

                if (entry.contains("gamertag") && entry["gamertag"].is_string())
                    score.m_name = convStringToWstring(entry["gamertag"].get<std::string>());

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
    }

    static void AssignRanks(std::vector<LeaderboardManager::ReadScore>& scores)
    {
        for (size_t i = 0; i < scores.size(); ++i)
            scores[i].m_rank = static_cast<unsigned long>(i + 1);
    }

    static int FindPlayerIndex(const std::vector<LeaderboardManager::ReadScore>& scores,
                               PlayerUID myUID, const std::wstring& myName)
    {
        for (int i = 0; i < static_cast<int>(scores.size()); ++i)
        {
            const auto& s = scores[i];
            if (myUID != INVALID_XUID && s.m_uid == myUID) return i;
            if (!myName.empty() && !s.m_name.empty() &&
                _wcsicmp(s.m_name.c_str(), myName.c_str()) == 0) return i;
        }
        return -1;
    }

    static bool BuildLocalScore(LeaderboardManager::ReadScore& out,
                                StatsCounter* stats,
                                LeaderboardManager::EStatsType type,
                                unsigned int diff,
                                PlayerUID uid,
                                const std::wstring& name)
    {
        ZeroMemory(&out, sizeof(out));
        out.m_uid             = uid;
        out.m_rank            = 1;
        out.m_idsErrorMessage = 0;
        out.m_name            = name;

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

WindowsLeaderboardManager::ValidatedIdentity WindowsLeaderboardManager::s_identity = {};

bool WindowsLeaderboardManager::ResolveIdentity()
{
    if (g_LCENToken[0] == '\0')
    {
        s_identity = {};
        return false;
    }

    if (s_identity.valid &&
        strncmp(g_LCENToken, s_identity.tokenSnapshot, sizeof(s_identity.tokenSnapshot)) == 0)
    {
        return true;
    }

    s_identity = {};

    std::string resp = HttpGet(std::string(kBaseUrl) + "/auth/validate");
    if (resp.empty()) return false;

    try
    {
        json j = json::parse(resp);

        if (j.contains("banned") && j["banned"].get<bool>()) return false;
        if (j.contains("error"))                             return false;
        if (!j.contains("xuid") || !j.contains("gamertag")) return false;

        strncpy_s(s_identity.tokenSnapshot, g_LCENToken, _TRUNCATE);
        s_identity.xuidHex  = j["xuid"].get<std::string>();
        s_identity.gamertag = convStringToWstring(j["gamertag"].get<std::string>());
        s_identity.valid    = true;
        return true;
    }
    catch (...) { return false; }
}

LeaderboardManager* LeaderboardManager::m_instance = new WindowsLeaderboardManager();

bool WindowsLeaderboardManager::WriteStats(unsigned int viewCount, ViewIn views)
{
    if (views == nullptr || viewCount == 0)
        return true;

    if (!ResolveIdentity())
    {
        delete[] views;
        return true;
    }

    std::string gamertag = WstrToUtf8(s_identity.gamertag);
    if (gamertag.empty()) gamertag = "Player";

    for (unsigned int i = 0; i < viewCount; ++i)
    {
        const RegisterScore& src = views[i];

        int diff = ClampDifficulty(src.m_difficulty);
        if (src.m_commentData.m_statsType == eStatsType_Kills && diff == 0)
            diff = 1;

        int type = static_cast<int>(src.m_commentData.m_statsType);
        if (type < 0 || type >= static_cast<int>(eStatsType_MAX)) continue;

        json statsArr = json::array();
        unsigned long long total = 0;

        switch (src.m_commentData.m_statsType)
        {
        case eStatsType_Travelling:
            statsArr = { src.m_commentData.m_travelling.m_walked,
                         src.m_commentData.m_travelling.m_fallen,
                         src.m_commentData.m_travelling.m_minecart,
                         src.m_commentData.m_travelling.m_boat };
            break;
        case eStatsType_Mining:
            statsArr = { src.m_commentData.m_mining.m_dirt,
                         src.m_commentData.m_mining.m_cobblestone,
                         src.m_commentData.m_mining.m_sand,
                         src.m_commentData.m_mining.m_stone,
                         src.m_commentData.m_mining.m_gravel,
                         src.m_commentData.m_mining.m_clay,
                         src.m_commentData.m_mining.m_obsidian };
            break;
        case eStatsType_Farming:
            statsArr = { src.m_commentData.m_farming.m_eggs,
                         src.m_commentData.m_farming.m_wheat,
                         src.m_commentData.m_farming.m_mushroom,
                         src.m_commentData.m_farming.m_sugarcane,
                         src.m_commentData.m_farming.m_milk,
                         src.m_commentData.m_farming.m_pumpkin };
            break;
        case eStatsType_Kills:
            statsArr = { src.m_commentData.m_kills.m_zombie,
                         src.m_commentData.m_kills.m_skeleton,
                         src.m_commentData.m_kills.m_creeper,
                         src.m_commentData.m_kills.m_spider,
                         src.m_commentData.m_kills.m_spiderJockey,
                         src.m_commentData.m_kills.m_zombiePigman,
                         src.m_commentData.m_kills.m_slime };
            break;
        default:
            continue;
        }

        for (auto& v : statsArr) total += v.get<unsigned int>();

        unsigned long long serverTotal = total;
        if (src.m_score > 0 && static_cast<unsigned long long>(src.m_score) > serverTotal)
            serverTotal = static_cast<unsigned long long>(src.m_score);

        json body;
        body["uid"]        = s_identity.xuidHex;
        body["gamertag"]   = gamertag;
        body["type"]       = type;
        body["difficulty"] = diff;
        body["stats"]      = statsArr;
        body["totalScore"] = serverTotal;

        HttpPost(std::string(kBaseUrl) + "/leaderboard/submit", body.dump());
    }

    delete[] views;
    return true;
}

bool WindowsLeaderboardManager::ReadStats_Friends(LeaderboardReadListener* callback,
    int difficulty, EStatsType type, PlayerUID myUID,
    unsigned int startIndex, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_Friends(callback, difficulty, type, myUID, startIndex, readCount))
        return false;
    return ReadNetworkStats(callback, difficulty, type, myUID, eFM_Friends, startIndex, readCount);
}

bool WindowsLeaderboardManager::ReadStats_MyScore(LeaderboardReadListener* callback,
    int difficulty, EStatsType type, PlayerUID myUID, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_MyScore(callback, difficulty, type, myUID, readCount))
        return false;
    return ReadNetworkStats(callback, difficulty, type, myUID, eFM_MyScore, 1, readCount);
}

bool WindowsLeaderboardManager::ReadStats_TopRank(LeaderboardReadListener* callback,
    int difficulty, EStatsType type,
    unsigned int startIndex, unsigned int readCount)
{
    if (!LeaderboardManager::ReadStats_TopRank(callback, difficulty, type, startIndex, readCount))
        return false;

    PlayerUID uid = INVALID_XUID;
    if (s_identity.valid)
        uid = HexToUid(s_identity.xuidHex);

    return ReadNetworkStats(callback, difficulty, type, uid, eFM_TopRank, startIndex, readCount);
}

bool WindowsLeaderboardManager::ReadNetworkStats(LeaderboardReadListener* callback,
    int difficulty, EStatsType type, PlayerUID myUID,
    EFilterMode filterMode,
    unsigned int startIndex, unsigned int readCount)
{
    if (!callback) return false;

    bool authenticated = ResolveIdentity();

    PlayerUID    resolvedUID  = INVALID_XUID;
    std::wstring resolvedName;
    std::string  resolvedHex;

    if (authenticated)
    {
        resolvedUID  = HexToUid(s_identity.xuidHex);
        resolvedName = s_identity.gamertag;
        resolvedHex  = s_identity.xuidHex;
    }
    else
    {
        // Unauthenticated: accept passed-in UID for display only; no POSTs.
        resolvedUID = myUID;
        const int pad = ProfileManager.GetPrimaryPad();
        if (pad >= 0 && pad < XUSER_MAX_COUNT)
        {
            char* gt = ProfileManager.GetGamertag(pad);
            if (gt && gt[0]) resolvedName = convStringToWstring(gt);
            if (resolvedUID == INVALID_XUID)
                ProfileManager.GetXUID(pad, &resolvedUID, true);
        }
        if (resolvedUID != INVALID_XUID) resolvedHex = UidToHex(resolvedUID);
    }

    unsigned int diff = static_cast<unsigned int>(ClampDifficulty(difficulty));
    if (type == eStatsType_Kills && diff == 0) diff = 1;

    // Submit live score only when authenticated.
    if (authenticated && resolvedUID != INVALID_XUID)
    {
        Minecraft* mc = Minecraft::GetInstance();
        const int pad = ProfileManager.GetPrimaryPad();
        if (mc && pad >= 0 && pad < XUSER_MAX_COUNT)
        {
            StatsCounter* stats = mc->stats[pad];
            if (stats)
            {
                LeaderboardManager::ReadScore localScore = {};
                if (BuildLocalScore(localScore, stats, type, diff, resolvedUID, resolvedName))
                {
                    json statsArr = json::array();
                    for (unsigned int i = 0; i < localScore.m_statsSize; ++i)
                        statsArr.push_back(localScore.m_statsData[i]);

                    std::string gt = WstrToUtf8(resolvedName);
                    if (gt.empty()) gt = "Player";

                    json submitBody;
                    submitBody["uid"]        = s_identity.xuidHex;
                    submitBody["gamertag"]   = gt;
                    submitBody["type"]       = static_cast<int>(type);
                    submitBody["difficulty"] = static_cast<int>(diff);
                    submitBody["stats"]      = statsArr;
                    submitBody["totalScore"] = static_cast<unsigned long long>(localScore.m_totalScore);

                    HttpPost(std::string(kBaseUrl) + "/leaderboard/submit", submitBody.dump());
                }
            }
        }
    }

    std::vector<LeaderboardManager::ReadScore> allRows;

    if (filterMode == eFM_Friends)
    {
        std::string xuidList = resolvedHex;

        if (authenticated && g_LCENToken[0] != '\0')
        {
            std::string friendsJson = HttpGet(std::string(kBaseUrl) + "/friends");
            if (!friendsJson.empty())
            {
                try
                {
                    json arr = json::parse(friendsJson);
                    if (arr.is_array())
                        for (auto& f : arr)
                            if (f.contains("xuid") && f["xuid"].is_string())
                            {
                                if (!xuidList.empty()) xuidList += ',';
                                xuidList += f["xuid"].get<std::string>();
                            }
                }
                catch (...) {}
            }
        }

        if (!xuidList.empty())
        {
            std::string url = std::string(kBaseUrl)
                + "/leaderboard/friends?type=" + std::to_string(static_cast<int>(type))
                + "&difficulty=" + std::to_string(diff)
                + "&xuids=" + xuidList;
            ParseServerEntries(HttpGet(url), allRows);
        }
    }
    else
    {
        unsigned int limit  = (readCount > 0) ? (std::min)(readCount, 100u) : 64u;
        unsigned int offset = (startIndex > 1) ? (startIndex - 1) : 0;

        if (filterMode == eFM_MyScore) { limit = 100; offset = 0; }

        std::string url = std::string(kBaseUrl)
            + "/leaderboard?type=" + std::to_string(static_cast<int>(type))
            + "&difficulty=" + std::to_string(diff)
            + "&limit=" + std::to_string(limit)
            + "&offset=" + std::to_string(offset);

        ParseServerEntries(HttpGet(url), allRows);
    }

    if (FindPlayerIndex(allRows, resolvedUID, resolvedName) < 0 &&
        resolvedUID != INVALID_XUID && !resolvedHex.empty())
    {
        std::string rankUrl = std::string(kBaseUrl)
            + "/leaderboard/rank?uid=" + resolvedHex
            + "&type=" + std::to_string(static_cast<int>(type))
            + "&difficulty=" + std::to_string(diff);

        std::string rankResp = HttpGet(rankUrl);
        if (!rankResp.empty())
        {
            try
            {
                json rj = json::parse(rankResp);
                if (rj.contains("totalScore") && rj["totalScore"].is_number())
                {
                    LeaderboardManager::ReadScore ps = {};
                    ps.m_uid        = resolvedUID;
                    ps.m_name       = resolvedName;
                    ps.m_totalScore = ClampToULong(
                        static_cast<unsigned long long>(rj["totalScore"].get<double>()));
                    ps.m_rank = static_cast<unsigned long>(
                        rj.contains("rank") ? rj["rank"].get<int>() : 0);

                    Minecraft* mc2 = Minecraft::GetInstance();
                    const int pad2 = ProfileManager.GetPrimaryPad();
                    if (mc2 && pad2 >= 0 && pad2 < XUSER_MAX_COUNT && mc2->stats[pad2])
                        BuildLocalScore(ps, mc2->stats[pad2], type, diff, resolvedUID, resolvedName);

                    allRows.push_back(ps);
                }
            }
            catch (...) {}
        }
        else if (!authenticated)
        {
            Minecraft* mc2 = Minecraft::GetInstance();
            const int pad2 = ProfileManager.GetPrimaryPad();
            if (mc2 && pad2 >= 0 && pad2 < XUSER_MAX_COUNT)
            {
                StatsCounter* stats = mc2->stats[pad2];
                if (stats)
                {
                    LeaderboardManager::ReadScore ps = {};
                    if (BuildLocalScore(ps, stats, type, diff, resolvedUID, resolvedName))
                        allRows.push_back(ps);
                }
            }
        }
    }

    std::sort(allRows.begin(), allRows.end(),
        [](const LeaderboardManager::ReadScore& a, const LeaderboardManager::ReadScore& b)
        {
            if (a.m_totalScore != b.m_totalScore) return a.m_totalScore > b.m_totalScore;
            const wchar_t* na = a.m_name.empty() ? L"" : a.m_name.c_str();
            const wchar_t* nb = b.m_name.empty() ? L"" : b.m_name.c_str();
            int cmp = _wcsicmp(na, nb);
            if (cmp != 0) return cmp < 0;
            return a.m_uid < b.m_uid;
        });

    AssignRanks(allRows);

    if (allRows.empty())
    {
        ReadView empty = {};
        callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, empty);
        return true;
    }

    int    playerIdx = FindPlayerIndex(allRows, resolvedUID, resolvedName);
    size_t pageStart = 0;
    size_t pageCount = allRows.size();

    if (filterMode == eFM_MyScore)
    {
        pageStart = (playerIdx >= 0) ? static_cast<size_t>(playerIdx) : 0;
        pageCount = 1;
    }
    else if (filterMode == eFM_TopRank)
    {
        if (startIndex > 1)
            pageStart = (std::min<size_t>)(static_cast<size_t>(startIndex - 1), allRows.size());
        pageCount = allRows.size() - pageStart;
        if (readCount > 0)
            pageCount = (std::min<size_t>)(pageCount, static_cast<size_t>(readCount));
    }

    if (pageStart >= allRows.size())
    {
        ReadView empty = {};
        callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, empty);
        return true;
    }

    pageCount = (std::min)(pageCount, allRows.size() - pageStart);

    std::vector<LeaderboardManager::ReadScore> page(
        allRows.begin() + static_cast<std::ptrdiff_t>(pageStart),
        allRows.begin() + static_cast<std::ptrdiff_t>(pageStart + pageCount));

    if (page.empty())
    {
        ReadView empty = {};
        callback->OnStatsReadComplete(eStatsReturn_NoResults, 0, empty);
        return true;
    }

    ReadView view = {};
    view.m_numQueries = static_cast<unsigned int>(page.size());
    view.m_queries    = page.data();

    int totalResults = (filterMode == eFM_MyScore)
        ? static_cast<int>(page.size())
        : static_cast<int>(allRows.size());

    callback->OnStatsReadComplete(eStatsReturn_Success, totalResults, view);
    return true;
}