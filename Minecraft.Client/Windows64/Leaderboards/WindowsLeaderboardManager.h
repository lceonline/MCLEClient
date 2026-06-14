#pragma once
#include "Common/Leaderboards/LeaderboardManager.h"

class WindowsLeaderboardManager : public LeaderboardManager
{
public:
    virtual void Tick() override {}
    virtual bool OpenSession()   override { return true; }
    virtual void CloseSession()  override {}
    virtual void DeleteSession() override {}

    virtual bool WriteStats(unsigned int viewCount, ViewIn views) override;

    virtual bool ReadStats_Friends(
        LeaderboardReadListener* callback,
        int difficulty,
        EStatsType type,
        PlayerUID myUID,
        unsigned int startIndex,
        unsigned int readCount) override;

    virtual bool ReadStats_MyScore(
        LeaderboardReadListener* callback,
        int difficulty,
        EStatsType type,
        PlayerUID myUID,
        unsigned int readCount) override;

    virtual bool ReadStats_TopRank(
        LeaderboardReadListener* callback,
        int difficulty,
        EStatsType type,
        unsigned int startIndex,
        unsigned int readCount) override;

    virtual void FlushStats()      override {}
    virtual void CancelOperation() override {}
    virtual bool isIdle() override { return true; }

private:
    bool ReadNetworkStats(
        LeaderboardReadListener* callback,
        int difficulty,
        EStatsType type,
        PlayerUID myUID,
        EFilterMode filterMode,
        unsigned int startIndex,
        unsigned int readCount);

    struct ValidatedIdentity
    {
        char         tokenSnapshot[512];
        std::string  xuidHex;
        std::wstring gamertag;
        bool         valid;
    };

    static ValidatedIdentity s_identity;

    static bool ResolveIdentity();
};