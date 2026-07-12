#pragma once
#include "Common/Leaderboards/LeaderboardManager.h"
#include <atomic>

class WindowsLeaderboardManager : public LeaderboardManager
{
public:
    WindowsLeaderboardManager();
    virtual ~WindowsLeaderboardManager() {}

    virtual void Tick() override;
    virtual bool OpenSession()   override { return true; }
    virtual void CloseSession()  override {}
    virtual void DeleteSession() override {}
    virtual bool WriteStats(unsigned int viewCount, ViewIn views) override;
    virtual bool SendStats(EStatsType type, int diff);
    virtual bool ReadStats_Friends(LeaderboardReadListener* callback, int difficulty, EStatsType type, PlayerUID myUID, unsigned int startIndex, unsigned int readCount) override;
    virtual bool ReadStats_MyScore(LeaderboardReadListener* callback, int difficulty, EStatsType type, PlayerUID myUID, unsigned int readCount) override;
    virtual bool ReadStats_TopRank(LeaderboardReadListener* callback, int difficulty, EStatsType type, unsigned int startIndex, unsigned int readCount) override;
    virtual void FlushStats()      override {}
    virtual void CancelOperation() override;
    virtual bool isIdle() override;

private:
    enum EStatsState { eStatsState_Idle, eStatsState_Getting, eStatsState_Canceled };
    std::atomic<EStatsState> m_eStatsState;
    int m_tickCount;

    bool ReadNetworkStats(LeaderboardReadListener* callback, int difficulty, EStatsType type, EFilterMode filterMode, unsigned int startIndex, unsigned int readCount);
    int sendScores(const std::string& data);
    std::string fetchOverall(int type, int difficulty, int offset, int limit);
    std::string fetchFriends(int type, int difficulty, int offset, int limit);
    std::string fetchMyscore(int type, int difficulty, int offset, int limit);
};